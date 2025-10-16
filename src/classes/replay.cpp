#include "replay.h"
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QtZlib/zlib.h>
#include <QTextStream>
#include <QDir>
#include <QDateTime>

const QByteArray Replay::MAGIC = QByteArray::fromHex("e5ac0010");
Replay::~Replay()
{
}

typedef struct Header{
    QByteArray magic;
    int version;
    QString rawLevel; //128
    QString rawLevelSettings; //260
    QString rawBattleType; //128
    QString rawEnvironment; //128
    QString rawVisibility; //32
    int resultBlkOffset;
    Constants::Difficulty difficulty;
    //Uknown 35 bytes
    int sessionType;
    //Uknown 4 bytes (or 7)?
    quint64 sessionId;
    //byte replayPartNumber ? (1 byte) always 0 for client replays
    //Uknonw 3 bytes
    int mSetSize;
    qint16 settingsBLKSize;
    //Uknown 30 bytes
    QString rawLocName; //128
    int startTime;
    int timeLimit;
    int scoreLimit;
    //Uknown 48 bytes
    QString rawBattleClass; //128
    QString rawBattleKillStreak; //128
    //Uknown 2 bytes
};


typedef struct WRPL{
    Header WRPLHeader;

};

enum class PacketType : uint8_t {
    EndMarker = 0,
    StartMarker = 1,
    AircraftSmall = 2,
    Chat = 3,
    MPI = 4,
    NextSegment = 5,
    ECS = 6,
    Snapshot = 7,
    ReplayHeaderInfo = 8
};

struct ParsedPacket {
    QString name;
    QVariant data; // use QVariant to store arbitrary parse results
};

struct WRPLRawPacket {
    quint32 currentTime = 0;
    uint8_t packetType = 0;
    QByteArray packetPayload;
    QSharedPointer<ParsedPacket> parsed; // nullptr if not parsed
    QString parseError; // non-empty if parse failed
};

// Reads the exact same format as the Go `readVariableLengthSize` you provided.
// Returns true on success, sets `out` to the parsed size.
// On EOF (no bytes available when reading the first byte) sets eof = true and returns false.
// On other errors sets `err` and returns false.
static bool readVariableLengthSize(QIODevice *dev, uint32_t &out, QString &err, bool &eof)
{
    err.clear();
    eof = false;
    out = 0;

    char firstBuf;
    qint64 n = dev->read(&firstBuf, 1);
    if (n == 0) {
        eof = true;
        return false;
    }
    if (n < 0) {
        err = QStringLiteral("read error while reading first size byte");
        return false;
    }

    unsigned char first = static_cast<unsigned char>(firstBuf);
    int64_t payload = 0;

    if ((first & 0x80) != 0) {
        // High bit SET (1xxxxxxx)
        if ((first & 0x40) == 0) {
            // 10xxxxxx -> 1 byte total
            payload = int64_t(first & 0x7F);
        } else {
            err = QString::fromLatin1("invalid first size prefix byte encountered: 0x%1")
            .arg(QString::number(first, 16).rightJustified(2, '0'));
            return false;
        }
    } else {
        // High bit CLEAR (0xxxxxxx)
        if ((first & 0x40) != 0) {
            // 01xxxxxx -> 2 bytes total
            char b1;
            if (dev->read(&b1, 1) != 1) {
                err = QStringLiteral("failed to read 2nd byte of 2-byte size prefix");
                return false;
            }
            payload = ( (int64_t(first) << 8) | int64_t(static_cast<unsigned char>(b1)) ) ^ 0x4000LL;
        } else if ((first & 0x20) != 0) {
            // 001xxxxx -> 3 bytes total
            char b12[2];
            if (dev->read(b12, 2) != 2) {
                err = QStringLiteral("failed to read bytes 2-3 of 3-byte size prefix");
                return false;
            }
            payload = ( (int64_t(first) << 16)
                       | (int64_t(static_cast<unsigned char>(b12[0])) << 8)
                       | int64_t(static_cast<unsigned char>(b12[1])) ) ^ 0x200000LL;
        } else if ((first & 0x10) != 0) {
            // 0001xxxx -> 4 bytes total
            char b123[3];
            if (dev->read(b123, 3) != 3) {
                err = QStringLiteral("failed to read bytes 2-4 of 4-byte size prefix");
                return false;
            }
            payload = ( (int64_t(first) << 24)
                       | (int64_t(static_cast<unsigned char>(b123[0])) << 16)
                       | (int64_t(static_cast<unsigned char>(b123[1])) << 8)
                       | int64_t(static_cast<unsigned char>(b123[2])) ) ^ 0x10000000LL;
        } else {
            // 0000xxxx -> 5 bytes total (little-endian u32)
            char b1234[4];
            if (dev->read(b1234, 4) != 4) {
                err = QStringLiteral("failed to read bytes 2-5 of 5-byte size prefix");
                return false;
            }
            // little-endian uint32
            uint32_t v =  static_cast<uint32_t>(static_cast<unsigned char>(b1234[0]))
                         | (static_cast<uint32_t>(static_cast<unsigned char>(b1234[1])) << 8)
                         | (static_cast<uint32_t>(static_cast<unsigned char>(b1234[2])) << 16)
                         | (static_cast<uint32_t>(static_cast<unsigned char>(b1234[3])) << 24);
            payload = int64_t(v);
        }
    }

    // keep parity with Go checks
    if (payload < 0) {
        // Go logged a warning but continued; here we treat it as an error to be safe.
        // If you prefer to mimic Go exactly (ignore), remove the error and continue.
        err = QStringLiteral("payload computed negative (corrupted size prefix)");
        return false;
    }
    if (payload > int64_t(std::numeric_limits<uint32_t>::max())) {
        err = QString::fromLatin1("payload size %1 cannot fit into uint32 (prefix starts with 0x%2)")
        .arg(QString::number(payload)).arg(QString::number(first, 16).rightJustified(2, '0'));
        return false;
    }

    out = static_cast<uint32_t>(payload);
    return true;
}
// Helper: read exactly n bytes; throws std::runtime_error on short read
static QByteArray readExact(QIODevice *dev, qint64 n)
{
    QByteArray out;
    out.reserve(n);
    while (out.size() < n) {
        QByteArray piece = dev->read(n - out.size());
        if (piece.isEmpty()) {
            throw std::runtime_error("unexpected EOF while reading packet payload");
        }
        out.append(piece);
    }
    return out;
}


typedef struct ParsedPacketChat {
    QString sender;
    QString content;
    uint8_t channelType;
    uint8_t isEnemy;
};
// Forward declare packet-specific parsers (implement these with real logic)
static QSharedPointer<ParsedPacket> parsePacketChat(const QSharedPointer<WRPLRawPacket> &pk, QString &err)
{
    err.clear();

    const QByteArray &buf = pk->packetPayload;
    int pos = 0;
    auto need = [&](int n) -> bool {
        if (pos + n <= buf.size()) return true;
        err = QStringLiteral("unexpected EOF while parsing chat packet");
        return false;
    };

    auto readLenString = [&](QString &out) -> bool {
        if (!need(1)) return false;
        const unsigned char len = static_cast<unsigned char>(buf[pos]);
        pos += 1;
        if (!need(len)) return false;
        if (len == 0) {
            out.clear();
            return true;
        }
        QByteArray s = buf.mid(pos, len);
        pos += len;
        // assume UTF-8 encoded strings; if it's another encoding adjust here
        out = QString::fromUtf8(s);
        return true;
    };

    QString sender;
    if (!readLenString(sender)) return nullptr;

    QString content;
    if (!readLenString(content)) return nullptr;

    if (!need(1)) return nullptr;
    quint8 channelType = static_cast<quint8>(buf[pos]);
    pos += 1;

    if (!need(1)) return nullptr;
    quint8 isEnemy = static_cast<quint8>(buf[pos]);
    pos += 1;

    // Build result similar to Go's ParsedPacket{ Name: "chat", Data: parsed }
    QSharedPointer<ParsedPacket> out = QSharedPointer<ParsedPacket>::create();
    out->name = QStringLiteral("chat");

    qDebug() << content;

    // Represent Data as a QVariantMap (you can replace with a struct if you prefer)
    QVariantMap data;
    data.insert(QStringLiteral("Sender"), sender);
    data.insert(QStringLiteral("Content"), content);
    data.insert(QStringLiteral("ChannelType"), QVariant::fromValue(channelType));
    data.insert(QStringLiteral("IsEnemy"), QVariant::fromValue(isEnemy));

    out->data = data;
    return out;
}

// Forward-declare types used earlier in your code
struct ParsedPacket;
struct WRPLRawPacket;

static double readLEDouble(const char *p) {
    // assemble little-endian uint64_t
    uint64_t v = (uint64_t)(static_cast<unsigned char>(p[0])) |
                 (uint64_t)(static_cast<unsigned char>(p[1])) << 8 |
                 (uint64_t)(static_cast<unsigned char>(p[2])) << 16 |
                 (uint64_t)(static_cast<unsigned char>(p[3])) << 24 |
                 (uint64_t)(static_cast<unsigned char>(p[4])) << 32 |
                 (uint64_t)(static_cast<unsigned char>(p[5])) << 40 |
                 (uint64_t)(static_cast<unsigned char>(p[6])) << 48 |
                 (uint64_t)(static_cast<unsigned char>(p[7])) << 56;
    double d;
    std::memcpy(&d, &v, sizeof(d));
    return d;
}

// Stubs for the detailed MPI parsers — implement these.
QSharedPointer<ParsedPacket> parsePacketMPI_CompressedBlobs(const QSharedPointer<WRPLRawPacket> &pk, QBuffer *r, QString &err);
QSharedPointer<ParsedPacket> parsePacketMPI_Kill(const QSharedPointer<WRPLRawPacket> &pk, QBuffer *r, QString &err);

typedef struct ParsedPacketAward {
    uint8_t awardType;
    QString Always0x003E;
    QString Always0x000000;
    uint8_t player;
    QString awardName;
    QString rem;
};

static QString readToHexStr(QIODevice *dev, int length)
{
    QByteArray bytes = dev->read(length);
    return bytes.toHex(' ');
}

static QString readLenString(QIODevice *dev)
{
    QByteArray lenByte = dev->read(1);
    if (lenByte.isEmpty())
        return QString();

    quint8 len = static_cast<quint8>(lenByte[0]);
    QByteArray data = dev->read(len);
    return QString::fromUtf8(data);
}

static QString readToHexStrFull(QIODevice *dev)
{
    QByteArray data = dev->readAll();
    return data.toHex(' ');
}

QSharedPointer<ParsedPacket> parsePacketMPI_Award(const QSharedPointer<WRPLRawPacket> &pk, QBuffer *r, QString &err)
{
    err.clear();
    const QByteArray &payload = pk->packetPayload;
    QBuffer buf;
    buf.setData(payload);
    buf.open(QIODevice::ReadOnly);

    QByteArray b;

    // 1. AwardType (1 byte)
    b = buf.read(1);
    if (b.size() < 1) { err = "unexpected EOF reading awardType"; return nullptr; }
    uint8_t awardType = static_cast<quint8>(b[0]);

    // 2. Always0x003E (2 bytes, shown as hex)
    QString always003E = buf.read(2).toHex(' ');

    // 3. Player (1 byte)
    b = buf.read(1);
    if (b.size() < 1) { err = "unexpected EOF reading player"; return nullptr; }
    uint8_t player = static_cast<quint8>(b[0]);

    // 4. Always0x000000 (3 bytes, shown as hex)
    QString always000000 = buf.read(3).toHex(' ');

    // 5. AwardName (length-prefixed string) //TODO: FIX VARIABLE LENGHT READING
    //b = buf.read(1);
    //if (b.size() < 1) { err = "unexpected EOF reading award name length"; return nullptr; }
    //quint8 len = static_cast<quint8>(b[0]);
    //QByteArray nameData = buf.read(len);
    QString awardName = readLenString(&buf);

    // 6. Remaining data (if any)
    QString rem = buf.readAll().toHex(' ');

     QVariantMap data;
     data.insert(QStringLiteral("awardType"), QVariant::fromValue(static_cast<quint32>(awardType)));
     data.insert(QStringLiteral("Player"), QVariant::fromValue(static_cast<quint32>(player)));
     data.insert(QStringLiteral("awardName"), QVariant::fromValue(awardName));
     data.insert(QStringLiteral("rem"), QVariant::fromValue(rem));

     QSharedPointer<ParsedPacket> out = QSharedPointer<ParsedPacket>::create();
     out->name = QStringLiteral("award");
     out->data = data;

    return out;
}

QSharedPointer<ParsedPacket> parsePacketMPI_SlotMessage(const QSharedPointer<WRPLRawPacket> &pk, QBuffer *r, QString &err);


static bool appendMovementToCsv(const QString &csvPath,
                                quint32 time,
                                quint32 eid,
                                double x, double y, double z,
                                QString &err)
{
    err.clear();
    QFile file(csvPath);
    bool isNew = !file.exists();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        err = QStringLiteral("Failed to open CSV file '%1' for append: %2").arg(csvPath, file.errorString());
        return false;
    }

    QTextStream ts(&file);

    if (isNew) {
        ts << "Time,Eid,X,Y,Z\n";
    }

    // Use ISO date/time and high-precision float formatting
    ts << time<< ','
       << eid << ','
       << QString::number(x, 'g', 17) << ','
       << QString::number(y, 'g', 17) << ','
       << QString::number(z, 'g', 17) << '\n';

    file.close();
    return true;
}

static QSharedPointer<ParsedPacket> parsePacketMPI_Movement(const QSharedPointer<WRPLRawPacket> &pk, QBuffer * /*r*/, const QByteArray &signature, QString &err)
{
    err.clear();

    const QByteArray &payload = pk->packetPayload;

    // same initial guard as Go
    if (payload.size() < 40) {
        return nullptr;
    }

    const unsigned char *p = reinterpret_cast<const unsigned char*>(payload.constData());

    if (p[0] != 0xff ||
        p[1] != 0x0f ||
        p[5] != 0xa3 ||
        p[6] != 0xf0 ||
        p[10] != 0x00 ||
        p[11] != 0x00 ||
        p[13] != 0x14) {
        return nullptr;
    }

    // signature must be at least 4 bytes
    if (signature.size() < 4) {
        return nullptr;
    }

    // Eid = uint16(signature[2]) | uint16(signature[3])<<8
    const unsigned char *s = reinterpret_cast<const unsigned char*>(signature.constData());
    quint16 eid = static_cast<quint16>(s[2]) | (static_cast<quint16>(s[3]) << 8);

    // read X,Y,Z (float64 little-endian) at offsets 14, 22, 30
    double x = readLEDouble(payload.constData() + 14);
    double y = readLEDouble(payload.constData() + 22);
    double z = readLEDouble(payload.constData() + 30);

    // build result similar to Go's ParsedPacketMovement/EntityPosition
    QVariantMap data;
    data.insert(QStringLiteral("Eid"), QVariant::fromValue(static_cast<quint32>(eid)));
    data.insert(QStringLiteral("Time"), QVariant::fromValue(pk->currentTime));
    data.insert(QStringLiteral("X"), QVariant::fromValue(x));
    data.insert(QStringLiteral("Y"), QVariant::fromValue(y));
    data.insert(QStringLiteral("Z"), QVariant::fromValue(z));

    qDebug() << pk->currentTime << " X:" << x << " Y:" << y << " Z:" << z;

    QSharedPointer<ParsedPacket> out = QSharedPointer<ParsedPacket>::create();
    out->name = QStringLiteral("movement");
    out->data = data;

    // --- append to CSV ---
    QString csvErr;
    // path: current working dir. change to absolute path if you prefer
    const QString csvPath = QDir::current().filePath(QStringLiteral("movement.csv"));

    // pk->currentTime is assumed to be QDateTime; adjust if it's another type
    if (!appendMovementToCsv(csvPath,
                             pk->currentTime,                // QDateTime
                             static_cast<quint32>(eid),     // Eid
                             x, y, z,
                             csvErr)) {
        qWarning() << "Failed to write movement to CSV:" << csvErr;
    }

    return out;
}
static QSharedPointer<ParsedPacket> parsePacketMPI(const QSharedPointer<WRPLRawPacket> &pk, QString &err)
{
    err.clear();

    // Wrap packet payload in a QBuffer so we can read sequentially (like bytes.NewReader in Go)
    QBuffer buf(const_cast<QByteArray*>(&pk->packetPayload));
    if (!buf.open(QIODevice::ReadOnly)) {
        err = QStringLiteral("failed to open buffer for MPI packet");
        return nullptr;
    }

    QByteArray signature = buf.read(4);
    if (signature.size() < 4) {
        err = QStringLiteral("MPI packet too short to contain signature");
        return nullptr;
    }


    // Compare signature bytes (exact same checks as the Go switch)
    const unsigned char *s = reinterpret_cast<const unsigned char*>(signature.constData());


    // Helper lambda for memcmp compare literal bytes
    auto equal4 = [&](const unsigned char a, const unsigned char b, const unsigned char c, const unsigned char d) {
        unsigned char lit[4] = { a, b, c, d };
        return std::memcmp(s, lit, 4) == 0;
    };

    // dispatch
    if (equal4(0x00, 0x58, 0x22, 0xf0)) {
        // ^00 58 22 f0  zstd blobs
        qDebug() << "zstd blobs packets";

        return nullptr;//parsePacketMPI_CompressedBlobs(pk, &buf, err);
    }

    if (equal4(0x02, 0x58, 0x58, 0xf0)) {
        // ^02 58 58 f0  kill screen?
        qDebug() << "kill packets";

        return nullptr; //parsePacketMPI_Kill(pk, &buf, err);
    }

    if (equal4(0x02, 0x58, 0x78, 0xf0)) {
        // ^02 58 78 f0  awards
        qDebug() << "award packets";

        return parsePacketMPI_Award(pk, &buf, err);
    }

    if (equal4(0x02, 0x58, 0xaa, 0xff) || equal4(0x02, 0x58, 0x2d, 0xf0)) {
        // ^02 58 aa ff  OR  ^02 58 2d f0  (fallthrough in Go)
        return nullptr; //parsePacketMPI_SlotMessage(pk, &buf, err);
    }

    // signature[0] == 0xff && signature[1] == 0x0f  -> movement
    if (static_cast<unsigned char>(signature[0]) == 0xff && static_cast<unsigned char>(signature[1]) == 0x0f) {
        return parsePacketMPI_Movement(pk, &buf, signature, err);
    }

    // default: unknown signature -> like Go: return nil, nil
    return nullptr;
}

// ParsePacket - dispatch to packet-specific parser
static QSharedPointer<ParsedPacket> ParsePacket(/*const WRPL* rpl,*/ const QSharedPointer<WRPLRawPacket> &pk, QString &err)
{
    switch (static_cast<PacketType>(pk->packetType)) {
    case PacketType::Chat:
        return parsePacketChat(pk, err);
    case PacketType::MPI:
        //qDebug() << "Parsing MPI packet...\n";
        return parsePacketMPI(pk, err);
    default:
        err = QStringLiteral("unknown packet");
        return nullptr;
    }
}

// The main conversion of ParsePacketStream
// - `dev` must be positioned at the start of the compressed/decompressed packet stream.
// - Throws std::runtime_error on read errors (EOF is used to end the loop).
static QVector<QSharedPointer<WRPLRawPacket>> ParsePacketStreamFromDevice(QIODevice *dev /*, optional: WRPL* rpl */)
{
    QVector<QSharedPointer<WRPLRawPacket>> ret;
    quint32 currentTime = 0;
    QString error;
    quint32 packetSize;

    while (true) {
        bool eof = false;
        readVariableLengthSize(dev, packetSize, error, eof);
        if (eof) {
            // same behavior as Go: EOF ends loop gracefully
            break;
        }

        if (packetSize == 0) {
            // Go code just continues when packet size == 0
            continue;
        }

        // read packet payload
        QByteArray packetBytes = readExact(dev, packetSize);

        if (packetBytes.size() == 0) {
            // defensive
            break;
        }

        const unsigned char firstByte = static_cast<unsigned char>(packetBytes[0]);
        uint8_t packetType = 0;
        QByteArray packetPayload;

        if ((firstByte & 0b00010000) != 0) {
            // "short" form: packetType = firstByte ^ 0x10, payload starts at index 2
            packetType = static_cast<uint8_t>(firstByte ^ 0b00010000);
            if (packetBytes.size() >= 2)
                packetPayload = packetBytes.mid(2);
            else
                packetPayload = QByteArray();
        } else {
            // "long" form: first byte = type, then a timestamp (uint32 LE) is read from bytes[1:]
            packetType = static_cast<uint8_t>(firstByte);
            // Ensure we have at least 5 bytes to read timestamp (Go used binary.Read on packetBytes[1:])
            if (packetBytes.size() >= 5) {
                // read 4 bytes little-endian from offset 1
                quint32 t = 0;
                const unsigned char *p = reinterpret_cast<const unsigned char*>(packetBytes.constData() + 1);
                t = static_cast<quint32>(p[0])
                    | (static_cast<quint32>(p[1]) << 8)
                    | (static_cast<quint32>(p[2]) << 16)
                    | (static_cast<quint32>(p[3]) << 24);
                currentTime = t;
            } else {
                throw std::runtime_error("packet too short to contain timestamp");
            }
            // Go used packetBytes[6:] — keep the same offset to match original behavior
            if (packetBytes.size() > 6)
                packetPayload = packetBytes.mid(6);
            else
                packetPayload = QByteArray();
        }

        if (packetType == 0) {
            // End marker -> stop parsing
            break;
        }

        auto pk = QSharedPointer<WRPLRawPacket>::create();
        pk->currentTime = currentTime;
        pk->packetType = packetType;
        pk->packetPayload = packetPayload;

        // Try to parse packet; keep parse result or error string
        QString perr;
        pk->parsed = ParsePacket(pk, perr); // note: first arg in Go was rpl; provide if needed
        pk->parseError = perr;
        ret.append(pk);
    }

    return ret;
}


Replay::Replay(const QByteArray& buffer) {
    // wrap the whole QByteArray in a QBuffer (QIODevice)
    QBuffer device(const_cast<QByteArray*>(&buffer)); // QBuffer doesn't copy by default if you pass pointer
    if (!device.open(QIODevice::ReadOnly))
        throw std::runtime_error("failed to open buffer");

    QDataStream stream(&device);
    stream.setByteOrder(QDataStream::LittleEndian);

	QByteArray magic(4, 0);
	stream.readRawData(magic.data(), 4);
	if (magic != MAGIC) {
		throw std::runtime_error("Invalid magic number, maybe not a replay file?");
	}

	stream >> m_version;
	m_level = readString(stream, 128).replace("levels/", "").replace(".bin", "");
	m_levelSettings = readString(stream, 260);
	m_battleType = readString(stream, 128);
	m_environment = readString(stream, 128);
	m_visibility = readString(stream, 32);
	stream >> m_rezOffset;
	quint8 difficultyTemp;
	stream >> difficultyTemp;
	difficultyTemp &= 0x0F;
	m_difficulty = static_cast<Constants::Difficulty>(difficultyTemp);
	stream.skipRawData(35); // Skip 35 bytes
	stream >> m_sessionType;
    stream.skipRawData(7); // Skip 7 bytes

	quint64 sessionIdInt;
	stream >> sessionIdInt;
	m_sessionId = QString::number(sessionIdInt, 16);

	stream.skipRawData(4); // Skip 4 bytes
	stream >> m_mSetSize;
    quint16 settingsBLKSize;
    stream >> settingsBLKSize;
    stream.skipRawData(30); // Skip 32 bytes
	m_locName = readString(stream, 128);
	stream >> m_startTime >> m_timeLimit >> m_scoreLimit;
	stream.skipRawData(48); // Skip 48 bytes
	m_battleClass = readString(stream, 128);
    m_battleKillStreak = readString(stream, 128);
    stream.skipRawData(2); // Skip 2 final bytes

    qint64 afterHeaderPos = device.pos();
    qint64 compressedStart = afterHeaderPos + settingsBLKSize;

    // sanity check
    if (compressedStart < 0 || compressedStart > device.size())
        throw std::runtime_error("invalid settings block size / offset");

    // seek to the start of the compressed packets
    if (!device.seek(compressedStart))
        throw std::runtime_error("seek to compressed start failed");

    QByteArray compressed = device.readAll();
    qDebug() << "compressed head:" << compressed.left(4).toHex();

    QByteArray decompressedData;
    decompressedData.resize(compressed.size() * 4); // initial guess

    z_stream streamZ;
    memset(&streamZ, 0, sizeof(streamZ));
    streamZ.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.constData()));
    streamZ.avail_in = static_cast<uInt>(compressed.size());



    int ret = inflateInit2(&streamZ, 15);
    if (ret != Z_OK) {
        // inflateInit2 returns zlib error code, convert to exception
        throw std::runtime_error(QString("inflateInit2 failed: %1").arg(ret).toStdString());
    }

    int retCode;
    do {
        if (streamZ.total_out >= static_cast<uLong>(decompressedData.size())) {
            // double capacity
            decompressedData.resize(decompressedData.size() * 2);
        }

        streamZ.next_out = reinterpret_cast<Bytef*>(decompressedData.data() + streamZ.total_out);
        streamZ.avail_out = static_cast<uInt>(decompressedData.size() - streamZ.total_out);

        retCode = ::inflate(&streamZ, Z_NO_FLUSH);
    } while (retCode == Z_OK);

    inflateEnd(&streamZ);

    if (retCode != Z_STREAM_END)
        throw std::runtime_error("zlib inflate failed");

    decompressedData.resize(streamZ.total_out);

    ///////////////////////// BEGINNING OF PACKETS PARSING
    QBuffer buffer2(&decompressedData);
    if (!buffer2.open(QIODevice::ReadOnly)) throw std::runtime_error("buffer2 open failed");
    auto packets = ParsePacketStreamFromDevice(&buffer2);

    if (!packets.isEmpty()) {
        qDebug() << "first packet type" << packets[0]->packetType
                 << "time" << packets[0]->currentTime
                 << "parseError" << packets[0]->parseError;
    }




	try {
		auto results = unpackResults(m_rezOffset, buffer);
		parseResults(results);
	}
	catch (const std::exception& e) {
		qWarning() << "Error unpacking results:" << e.what();
	}
}

Replay::Replay()
{
}

Replay Replay::fromFile(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error("Failed to open file");
	}

	QByteArray fileContent = file.readAll();
	return Replay(fileContent);
}

QString Replay::readString(QDataStream& stream, int length) {
	QByteArray bytes(length, 0);
	stream.readRawData(bytes.data(), length);

	int nullIndex = bytes.indexOf('\0');
	if (nullIndex != -1) {
		bytes.truncate(nullIndex);
	}

	return QString::fromUtf8(bytes);
}

QJsonObject Replay::unpackResults(int offset, const QByteArray& buffer) {
	QByteArray dataAfterRez = buffer.mid(offset);

	QProcess process;
	QString exe = QCoreApplication::applicationDirPath() + "/wt_ext_cli";
#ifdef Q_OS_WIN
	exe += ".exe";
#endif
	QStringList args{ "--unpack_raw_blk", "--stdout", "--stdin", "--format", "Json" };

	process.start(exe, args);
	if (!process.waitForStarted(3000)) {
		qCritical() << "Failed to start process:" << process.error() << process.errorString();
		throw std::runtime_error("Failed to start wt_ext_cli");
	}

	process.write(dataAfterRez);
	process.closeWriteChannel();

	if (!process.waitForFinished()) {
		throw std::runtime_error("Process did not finish");
	}

	QByteArray output = process.readAllStandardOutput();
	QJsonDocument jsonDoc = QJsonDocument::fromJson(output);
	if (jsonDoc.isNull() || !jsonDoc.isObject()) {
		throw std::runtime_error("Invalid JSON output");
	}

	return jsonDoc.object();
}

void Replay::parseResults(const QJsonObject& results) {
	this->m_status = results.value("status").toString("left");
	this->m_timePlayed = results.value("timePlayed").toDouble();
	this->m_authorUserId = results.value("authorUserId").toString();
	this->m_author = results.value("author").toString();

	if (this->m_authorUserId == "" || this->m_author == "") {
		this->m_authorUserId = "-1";
		this->m_author = "server";
	}

	QJsonArray playersArray = results.value("player").toArray();
	QJsonObject uiScriptsData = results.value("uiScriptsData").toObject();
	QJsonObject playersInfoObject = uiScriptsData.value("playersInfo").toObject();

	for (const auto& playerElement : playersArray) {
		QJsonObject playerObject = playerElement.toObject();
		for (auto it = playersInfoObject.begin(); it != playersInfoObject.end(); ++it) {
			QJsonObject playerInfoObject = it.value().toObject();
			if (playerInfoObject.value("id").toInteger() == playerObject.value("userId").toString().toULongLong()) {
				Player p = Player::fromJson(playerInfoObject);
				PlayerReplayData prd = PlayerReplayData::fromJson(playerObject);

				prd.setWaitTime(playerInfoObject.value("wait_time").toDouble());
				QJsonObject crafts = playerInfoObject.value("crafts").toObject();
				QList<QString> lineup;
				for (auto it1 = crafts.constBegin(); it1 != crafts.constEnd(); ++it1) {
					lineup.append(it1.value().toString());
				}
				prd.setLineup(lineup);
				QPair<Player, PlayerReplayData> pPair(p, prd);
				this->m_players.append(pPair);
				break;
			}
		}
	}
}

QString Replay::getSessionId() const { return m_sessionId; }
QString Replay::getLevel() const { return m_level; }
QString Replay::getBattleType() const { return m_battleType; }
Constants::Difficulty Replay::getDifficulty() const { return m_difficulty; }
int Replay::getStartTime() const { return m_startTime; }
QString Replay::getStatus() const { return m_status; }
double Replay::getTimePlayed() const { return m_timePlayed; }
QString Replay::getAuthorUserId() const { return m_authorUserId; }
QList<QPair<Player, PlayerReplayData>> Replay::getPlayers() const { return m_players; }

void Replay::setSessionId(QString sessionId)
{
	this->m_sessionId = sessionId;
}

void Replay::setAuthorUserId(QString authorUserId)
{
	this->m_authorUserId = authorUserId;
}

void Replay::setStartTime(int startTime)
{
	this->m_startTime = startTime;
}

void Replay::setLevel(QString level)
{
	this->m_level = level;
}

void Replay::setBattleType(QString battleType)
{
	this->m_battleType = battleType;
}

void Replay::setDifficulty(Constants::Difficulty difficulty)
{
	this->m_difficulty = difficulty;
}

void Replay::setStatus(QString status)
{
	this->m_status = status;
}

void Replay::setTimePlayed(double timePlayed)
{
	this->m_timePlayed = timePlayed;
}

void Replay::setPlayers(QList<QPair<Player, PlayerReplayData>> players)
{
	this->m_players = players;
}
