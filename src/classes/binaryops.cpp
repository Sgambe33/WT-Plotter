#include "binaryops.h"

bool readVariableLengthSize(QIODevice *dev, uint32_t &out, QString &err, bool &eof) {
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
            payload = ((int64_t(first) << 8) | int64_t(static_cast<unsigned char>(b1))) ^ 0x4000LL;
        } else if ((first & 0x20) != 0) {
            // 001xxxxx -> 3 bytes total
            char b12[2];
            if (dev->read(b12, 2) != 2) {
                err = QStringLiteral("failed to read bytes 2-3 of 3-byte size prefix");
                return false;
            }
            payload = ((int64_t(first) << 16)
                       | (int64_t(static_cast<unsigned char>(b12[0])) << 8)
                       | int64_t(static_cast<unsigned char>(b12[1]))) ^ 0x200000LL;
        } else if ((first & 0x10) != 0) {
            // 0001xxxx -> 4 bytes total
            char b123[3];
            if (dev->read(b123, 3) != 3) {
                err = QStringLiteral("failed to read bytes 2-4 of 4-byte size prefix");
                return false;
            }
            payload = ((int64_t(first) << 24)
                       | (int64_t(static_cast<unsigned char>(b123[0])) << 16)
                       | (int64_t(static_cast<unsigned char>(b123[1])) << 8)
                       | int64_t(static_cast<unsigned char>(b123[2]))) ^ 0x10000000LL;
        } else {
            // 0000xxxx -> 5 bytes total (little-endian u32)
            char b1234[4];
            if (dev->read(b1234, 4) != 4) {
                err = QStringLiteral("failed to read bytes 2-5 of 5-byte size prefix");
                return false;
            }
            // little-endian uint32
            uint32_t v = static_cast<uint32_t>(static_cast<unsigned char>(b1234[0]))
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


QByteArray readExact(QIODevice *dev, qint64 n) {
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

double readLEDouble(const char *p) {
    // assemble little-endian uint64_t
    uint64_t v = (uint64_t) (static_cast<unsigned char>(p[0])) |
                 (uint64_t) (static_cast<unsigned char>(p[1])) << 8 |
                 (uint64_t) (static_cast<unsigned char>(p[2])) << 16 |
                 (uint64_t) (static_cast<unsigned char>(p[3])) << 24 |
                 (uint64_t) (static_cast<unsigned char>(p[4])) << 32 |
                 (uint64_t) (static_cast<unsigned char>(p[5])) << 40 |
                 (uint64_t) (static_cast<unsigned char>(p[6])) << 48 |
                 (uint64_t) (static_cast<unsigned char>(p[7])) << 56;
    double d;
    std::memcpy(&d, &v, sizeof(d));
    return d;
}

QString readToHexStr(QIODevice *dev, int length) {
    QByteArray bytes = dev->read(length);
    return bytes.toHex(' ');
}

QString readLenString(QIODevice *dev) {
    QByteArray lenByte = dev->read(1);
    if (lenByte.isEmpty())
        return QString();

    quint8 len = static_cast<quint8>(lenByte[0]);
    QByteArray data = dev->read(len);
    return QString::fromUtf8(data);
}

QString readToHexStrFull(QIODevice *dev) {
    QByteArray data = dev->readAll();
    return data.toHex(' ');
}
