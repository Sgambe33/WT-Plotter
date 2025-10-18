#include "wrpl.h"

#include <QVector>
#include <QDebug>
#include <QtEndian>
#include <cstring>
#include "libs/zstd/include/zstd.h"

using QVariantAny = QVariant;
using QVariantMapAny = QVariantMap;
using QVariantListAny = QVariantList;

namespace wrpl {

    // --- helpers ---

    static bool readULEB128(const QByteArray& b, int& pos, quint64& outVal, QString& err) {
        outVal = 0;
        unsigned shift = 0;
        int start = pos;
        while (pos < b.size()) {
            unsigned char cur = static_cast<unsigned char>(b[pos++]);
            outVal |= quint64(cur & 0x7F) << shift;
            if ((cur & 0x80) == 0) {
                return true;
            }
            shift += 7;
            if (shift > 63) {
                err = QStringLiteral("uleb128: overflow");
                return false;
            }
        }
        pos = start;
        err = QStringLiteral("uleb128: buffer too small");
        return false;
    }

    static bool uleb128From(const QByteArray& b, int& at, quint64& outVal, QString& err) {
        return readULEB128(b, at, outVal, err);
    }

    static QVector<QString> parseNullSeparatedStrings(const QByteArray& b) {
        QVector<QString> out;
        int start = 0;
        for (int i = 0; i < b.size(); ++i) {
            if (static_cast<unsigned char>(b[i]) == 0) {
                out.append(QString::fromUtf8(reinterpret_cast<const char*>(b.constData() + start), i - start));
                start = i + 1;
            }
        }
        // trailing partial without null is ignored (same behavior as Go)
        return out;
    }

    // ZSTD streaming decompression (works when frame content size unknown)
    static bool decompressZstd(const QByteArray& src, QByteArray& dst, QString& err) {
        ZSTD_DStream* dctx = ZSTD_createDStream();
        if (!dctx) {
            err = QStringLiteral("ZSTD_createDStream failed");
            return false;
        }
        size_t ret = ZSTD_initDStream(dctx);
        if (ZSTD_isError(ret)) {
            err = QString::fromUtf8(ZSTD_getErrorName(ret));
            ZSTD_freeDStream(dctx);
            return false;
        }

        // input buffer
        ZSTD_inBuffer in;
        in.src = src.constData();
        in.size = (size_t)src.size();
        in.pos = 0;

        QByteArray outBuf;
        const size_t chunk = ZSTD_DStreamOutSize(); // recommended output chunk
        outBuf.reserve(in.size > 0 ? qMin<qint64>(in.size * 4 + 1024, 16 * 1024 * 1024) : 65536);

        while (in.pos < in.size) {
            QByteArray tmp;
            tmp.resize((int)chunk);
            ZSTD_outBuffer out;
            out.dst = tmp.data();
            out.size = chunk;
            out.pos = 0;

            size_t dret = ZSTD_decompressStream(dctx, &out, &in);
            if (ZSTD_isError(dret)) {
                err = QString::fromUtf8(ZSTD_getErrorName(dret));
                ZSTD_freeDStream(dctx);
                return false;
            }
            // append produced bytes
            outBuf.append(tmp.constData(), (int)out.pos);
        }

        // flush any remaining
        while (true) {
            QByteArray tmp;
            tmp.resize((int)chunk);
            ZSTD_outBuffer out;
            out.dst = tmp.data();
            out.size = chunk;
            out.pos = 0;
            size_t dret = ZSTD_decompressStream(dctx, &out, &in);
            if (ZSTD_isError(dret)) {
                err = QString::fromUtf8(ZSTD_getErrorName(dret));
                ZSTD_freeDStream(dctx);
                return false;
            }
            outBuf.append(tmp.constData(), (int)out.pos);
            if (dret == 0) break; // finished
        }

        ZSTD_freeDStream(dctx);
        dst = outBuf;
        return true;
    }

    static void putKV(QVariantMap& m, const QString& k, const QVariant& v) {
        if (m.contains(k)) {
            QVariant existing = m.value(k);
            if (existing.type() == QVariant::List) {
                QVariantList list = existing.toList();
                list.append(v);
                m.insert(k, list);
            }
            else {
                QVariantList list;
                list.append(existing);
                list.append(v);
                m.insert(k, list);
            }
        }
        else {
            m.insert(k, v);
        }
    }

    // --- main parseFatBlk implementation ---
    static QVariantMap parseFatBlk(const QByteArray& buf, QString& err) {
        int p = 0;
        auto readULEB = [&](quint64& out) -> bool {
            return readULEB128(buf, p, out, err);
            };

        // names_count (ignored value)
        quint64 tmp;
        if (!readULEB(tmp)) { err = QStringLiteral("names_count: ") + err; return {}; }

        // names_size
        quint64 namesSize64;
        if (!readULEB(namesSize64)) { err = QStringLiteral("names_size: ") + err; return {}; }
        int namesSize = (int)namesSize64;
        if (p + namesSize > buf.size()) { err = QStringLiteral("names buffer truncated"); return {}; }
        QByteArray namesRaw = buf.mid(p, namesSize);
        p += namesSize;
        QVector<QString> names = parseNullSeparatedStrings(namesRaw);

        // total blocks
        quint64 totalBlocks64;
        if (!readULEB(totalBlocks64)) { err = QStringLiteral("total blocks: ") + err; return {}; }
        int totalBlocks = (int)totalBlocks64;

        // params
        quint64 paramsCount64;
        if (!readULEB(paramsCount64)) { err = QStringLiteral("params_count: ") + err; return {}; }
        int paramsCount = (int)paramsCount64;
        quint64 paramsDataSize64;
        if (!readULEB(paramsDataSize64)) { err = QStringLiteral("params_data_size: ") + err; return {}; }
        int paramsDataSize = (int)paramsDataSize64;
        if (p + paramsDataSize > buf.size()) { err = QStringLiteral("params data truncated"); return {}; }
        QByteArray paramsData = buf.mid(p, paramsDataSize);
        p += paramsDataSize;

        if (p + paramsCount * 8 > buf.size()) { err = QStringLiteral("params info truncated"); return {}; }
        QByteArray paramsInfo = buf.mid(p, paramsCount * 8);
        p += paramsCount * 8;

        QByteArray blockInfo = buf.mid(p);
        int bp = 0;

        struct BlockDesc {
            quint64 nameID;
            int fieldCount;
            int childCount;
            int firstChildID;
        };
        QVector<BlockDesc> descs;
        descs.reserve(totalBlocks);
        for (int i = 0; i < totalBlocks; ++i) {
            quint64 nameID;
            if (!uleb128From(blockInfo, bp, nameID, err)) { err = QStringLiteral("block[") + QString::number(i) + QStringLiteral("] name_id: ") + err; return {}; }
            quint64 fieldCount64;
            if (!uleb128From(blockInfo, bp, fieldCount64, err)) { err = QStringLiteral("block[") + QString::number(i) + QStringLiteral("] field_count: ") + err; return {}; }
            quint64 childCount64;
            if (!uleb128From(blockInfo, bp, childCount64, err)) { err = QStringLiteral("block[") + QString::number(i) + QStringLiteral("] child_count: ") + err; return {}; }
            int firstChild = 0;
            if (childCount64 > 0) {
                quint64 fc;
                if (!uleb128From(blockInfo, bp, fc, err)) { err = QStringLiteral("block[") + QString::number(i) + QStringLiteral("] first_child: ") + err; return {}; }
                firstChild = (int)fc;
            }
            descs.append({ nameID, (int)fieldCount64, (int)childCount64, firstChild });
        }

        auto readAt = [&](int off, int n, QByteArray& out) -> bool {
            if (off < 0 || off + n > paramsData.size()) { return false; }
            out = paramsData.mid(off, n);
            return true;
            };

        auto parseFloat32 = [](const QByteArray& b, int offset = 0) -> double {
            uint32_t bits = qFromLittleEndian<uint32_t>(reinterpret_cast<const uchar*>(b.constData() + offset));
            float f;
            std::memcpy(&f, &bits, sizeof(float));
            return double(f);
            };

        auto parseInt32 = [](const QByteArray& b, int offset = 0) -> qint64 {
            uint32_t u = qFromLittleEndian<uint32_t>(reinterpret_cast<const uchar*>(b.constData() + offset));
            int32_t s = static_cast<int32_t>(u);
            return (qint64)s;
            };

        // Helper: getNthParam
        auto getNthParam = [&](int index, QString& errOut) -> QPair<QString, QVariant> {
            int start = index * 8;
            if (start + 8 > paramsInfo.size()) { errOut = QStringLiteral("param[%1]: info out of bounds").arg(index); return {}; }
            QByteArray chunk = paramsInfo.mid(start, 8);
            quint32 nameID = (quint32(static_cast<unsigned char>(chunk[0])) |
                (quint32(static_cast<unsigned char>(chunk[1])) << 8) |
                (quint32(static_cast<unsigned char>(chunk[2])) << 16));
            unsigned char typeID = static_cast<unsigned char>(chunk[3]);
            QByteArray data = chunk.mid(4, 4);

            if ((int)nameID >= names.size()) { errOut = QStringLiteral("param[%1]: name id %2 out of range %3").arg(index).arg(nameID).arg(names.size()); return {}; }
            QString name = names[nameID];

            QVariant value;

            auto parseInt64At = [&](int off, bool& ok) -> qint64 {
                ok = false;
                if (off < 0 || off + 8 > paramsData.size()) { return 0; }
                quint64 v = qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(paramsData.constData() + off));
                ok = true;
                return static_cast<qint64>(v);
                };

            switch (typeID) {
            case 0x01: { // STRING
                quint32 raw = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                bool inNM = ((raw >> 31) == 1);
                int off = raw & 0x7fffffff;
                QString s;
                if (inNM) {
                    if (off < 0 || off >= names.size()) { errOut = QStringLiteral("param[%1]: string nm offset %2 OOB").arg(index).arg(off); return {}; }
                    s = names[off];
                }
                else {
                    if (off < 0 || off >= paramsData.size()) { errOut = QStringLiteral("param[%1]: string offset %2 OOB").arg(index).arg(off); return {}; }
                    QByteArray rest = paramsData.mid(off);
                    int end = rest.indexOf('\0');
                    if (end < 0) { errOut = QStringLiteral("param[%1]: unterminated string").arg(index); return {}; }
                    s = QString::fromUtf8(rest.constData(), end);
                }
                value = s;
                break;
            }
            case 0x02: { // INT
                value = QVariant(parseInt32(data, 0));
                break;
            }
            case 0x03: { // FLOAT
                double f = parseFloat32(data, 0);
                value = QVariant(f);
                break;
            }
            case 0x04: { // FLOAT2
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 8, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList l;
                l << parseFloat32(bs, 0) << parseFloat32(bs, 4);
                value = l;
                break;
            }
            case 0x05: { // FLOAT3
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 12, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList l;
                l << parseFloat32(bs, 0) << parseFloat32(bs, 4) << parseFloat32(bs, 8);
                value = l;
                break;
            }
            case 0x06: { // FLOAT4
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 16, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList l;
                l << parseFloat32(bs, 0) << parseFloat32(bs, 4) << parseFloat32(bs, 8) << parseFloat32(bs, 12);
                value = l;
                break;
            }
            case 0x07: { // INT2
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 8, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList l;
                l << (qint64)(int32_t)qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bs.constData() + 0));
                l << (qint64)(int32_t)qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bs.constData() + 4));
                value = l;
                break;
            }
            case 0x08: { // INT3
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 12, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList l;
                l << (qint64)(int32_t)qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bs.constData() + 0));
                l << (qint64)(int32_t)qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bs.constData() + 4));
                l << (qint64)(int32_t)qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bs.constData() + 8));
                value = l;
                break;
            }
            case 0x09: { // BOOL
                quint32 u = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                value = QVariant(u != 0);
                break;
            }
            case 0x0A: { // COLOR r,g,b,a each 1 byte
                QVariantList l;
                l << (qint64) static_cast<unsigned char>(data[0]);
                l << (qint64) static_cast<unsigned char>(data[1]);
                l << (qint64) static_cast<unsigned char>(data[2]);
                l << (qint64) static_cast<unsigned char>(data[3]);
                value = l;
                break;
            }
            case 0x0B: { // FLOAT12 (3x4 matrix) => 48 bytes at offset
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 48, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList rows;
                for (int r = 0; r < 4; ++r) {
                    int base = r * 12;
                    QVariantList row;
                    row << parseFloat32(bs, base) << parseFloat32(bs, base + 4) << parseFloat32(bs, base + 8);
                    rows << row;
                }
                value = rows;
                break;
            }
            case 0x0C: { // LONG (8-byte)
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                bool ok;
                qint64 v = parseInt64At(off, ok);
                if (!ok) { errOut = QStringLiteral("param[%1]: long offset OOB").arg(index); return {}; }
                value = QVariant::fromValue(v);
                break;
            }
            case 0x0D: { // INT4
                quint32 off = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData()));
                QByteArray bs;
                if (!readAt(off, 16, bs)) { errOut = QStringLiteral("param[%1]: offset OOB").arg(index); return {}; }
                QVariantList l;
                for (int i = 0; i < 4; ++i) {
                    l << (qint64)(int32_t)qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bs.constData() + i * 4));
                }
                value = l;
                break;
            }
            default:
                errOut = QStringLiteral("param[%1]: unknown type id 0x%2").arg(index).arg(QString::number(typeID, 16));
                return {};
            }

            return qMakePair(name, value);
            };

        // Build flat blocks
        int paramPtr = 0;
        struct FlatBlock {
            QString name;
            QVector<QPair<QString, QVariant>> fields;
            int childCount;
            int firstChild;
        };
        QVector<FlatBlock> flat;
        flat.reserve(descs.size());
        for (int i = 0; i < descs.size(); ++i) {
            auto& d = descs[i];
            QString name = QStringLiteral("root");
            if (d.nameID != 0) {
                int id = int(d.nameID - 1);
                if (id < 0 || id >= names.size()) { err = QStringLiteral("block[%1]: name index %2 out of range").arg(i).arg(id); return {}; }
                name = names[id];
            }
            QVector<QPair<QString, QVariant>> fields;
            fields.reserve(d.fieldCount);
            for (int j = 0; j < d.fieldCount; ++j) {
                QString paramErr;
                auto pr = getNthParam(paramPtr + j, paramErr);
                if (!paramErr.isEmpty()) { err = paramErr; return {}; }
                fields.append(pr);
            }
            paramPtr += d.fieldCount;
            int firstChild = 0;
            if (d.childCount > 0) firstChild = d.firstChildID;
            flat.append({ name, fields, d.childCount, firstChild });
        }

        // Build nested map recursively
        std::function<QVariantMap(int)> build = [&](int idx) -> QVariantMap {
            const FlatBlock& fb = flat[idx];
            QVariantMap m;
            // fields
            for (const auto& f : fb.fields) {
                putKV(m, f.first, f.second);
            }
            // children
            if (fb.childCount > 0) {
                for (int c = fb.firstChild; c < fb.firstChild + fb.childCount; ++c) {
                    QVariantMap childMap = build(c);
                    QString childName = flat[c].name;
                    putKV(m, childName, childMap);
                }
            }
            return m;
            };

        if (flat.isEmpty()) {
            return QVariantMap{};
        }
        QVariantMap root = build(0);
        return root;
    }

    // --- top-level ParseBlk ---
    QVariantMap ParseBlk(const QByteArray& input, QString& outErr) {
        outErr.clear();
        if (input.isEmpty()) {
            outErr = QStringLiteral("empty BLK buffer");
            return {};
        }
        unsigned char hdr = static_cast<unsigned char>(input[0]);
        switch (hdr) {
        case 0x01: { // FAT
            QByteArray payload = input.mid(1);
            return parseFatBlk(payload, outErr);
        }
        case 0x02: { // FAT_ZSTD
            if (input.size() < 4) { outErr = QStringLiteral("FAT_ZSTD: truncated header"); return {}; }
            quint32 l = (quint32(static_cast<unsigned char>(input[1])) << 16) |
                (quint32(static_cast<unsigned char>(input[2])) << 8) |
                (quint32(static_cast<unsigned char>(input[3])));
            if ((int)input.size() < 4 + (int)l) {
                outErr = QStringLiteral("FAT_ZSTD: compressed payload truncated");
                return {};
            }
            QByteArray comp = input.mid(4, (int)l);
            QByteArray out;
            if (!decompressZstd(comp, out, outErr)) {
                outErr = QStringLiteral("FAT_ZSTD: decode: ") + outErr;
                return {};
            }
            if (out.isEmpty() || static_cast<unsigned char>(out[0]) != 0x01) {
                outErr = QStringLiteral("FAT_ZSTD: decoded payload missing FAT header");
                return {};
            }
            QByteArray payload = out.mid(1);
            return parseFatBlk(payload, outErr);
        }
        case 0x03: {
            outErr = QStringLiteral("SLIM BLK is not yet supported (and won't lol)");
            return {};
        }
        case 0x04: { // SLIM_ZSTD
            QByteArray comp = input.mid(1);
            QByteArray out;
            if (!decompressZstd(comp, out, outErr)) {
                outErr = QStringLiteral("SLIM_ZSTD: decode: ") + outErr;
                return {};
            }
            Q_UNUSED(out);
            outErr = QStringLiteral("SLIM_ZSTD BLK is not supported without an external name map");
            return {};
        }
        case 0x05: {
            outErr = QStringLiteral("SLIM_ZSTD_DICT BLK not supported (requires dictionary and external name map)");
            return {};
        }
        case 0x00: {
            outErr = QStringLiteral("BBF BLK not supported");
            return {};
        }
        default: {
            outErr = QStringLiteral("unknown header 0x%1").arg(QString::number(hdr, 16));
            return {};
        }
        }
    }

} // namespace wrpl
