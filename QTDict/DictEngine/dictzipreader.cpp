#include "dictzipreader.h"

#include <QtEndian>
#include <QtZlib/zlib.h>

#include <QVector>

#include <limits>

namespace {

constexpr quint8 gzipExtraFlag = 0x04;
constexpr quint8 gzipNameFlag = 0x08;
constexpr quint8 gzipCommentFlag = 0x10;
constexpr quint8 gzipHeaderCrcFlag = 0x02;

quint16 readUint16(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(data.constData() + offset));
}

quint32 readUint32(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(data.constData() + offset));
}

bool readCString(const QByteArray &data, int *offset, int limit)
{
    while (*offset < limit && data.at(*offset) != '\0') {
        ++*offset;
    }
    if (*offset >= limit) {
        return false;
    }
    ++*offset;
    return true;
}

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

} // namespace

bool DictZipReader::decompress(const QByteArray &compressed,
                               QByteArray *uncompressed,
                               QString *errorMessage)
{
    if (compressed.size() < 18
        || quint8(compressed.at(0)) != 0x1f
        || quint8(compressed.at(1)) != 0x8b
        || quint8(compressed.at(2)) != 8) {
        setError(errorMessage, QStringLiteral("索引文件不是有效的 gzip/dictzip 文件。"));
        return false;
    }

    const quint8 flags = quint8(compressed.at(3));
    const int trailerOffset = compressed.size() - 8;
    int offset = 10;
    int chunkLength = 0;
    QVector<quint16> chunkSizes;

    if (flags & gzipExtraFlag) {
        if (offset + 2 > trailerOffset) {
            setError(errorMessage, QStringLiteral("dictzip 扩展头不完整。"));
            return false;
        }
        const quint16 extraLength = readUint16(compressed, offset);
        offset += 2;
        const int extraEnd = offset + extraLength;
        if (extraEnd > trailerOffset) {
            setError(errorMessage, QStringLiteral("dictzip 扩展头长度无效。"));
            return false;
        }

        while (offset + 4 <= extraEnd) {
            const QByteArray identifier = compressed.mid(offset, 2);
            const quint16 fieldLength = readUint16(compressed, offset + 2);
            offset += 4;
            if (offset + fieldLength > extraEnd) {
                setError(errorMessage, QStringLiteral("dictzip 扩展字段长度无效。"));
                return false;
            }
            if (identifier == QByteArrayLiteral("RA")) {
                if (fieldLength < 6 || readUint16(compressed, offset) != 1) {
                    setError(errorMessage, QStringLiteral("不支持此版本的 dictzip 索引。"));
                    return false;
                }
                chunkLength = readUint16(compressed, offset + 2);
                const quint16 chunkCount = readUint16(compressed, offset + 4);
                if (chunkLength == 0 || chunkCount == 0
                    || fieldLength < 6 + int(chunkCount) * 2) {
                    setError(errorMessage, QStringLiteral("dictzip 分块表无效。"));
                    return false;
                }
                chunkSizes.reserve(chunkCount);
                for (int i = 0; i < chunkCount; ++i) {
                    chunkSizes.push_back(readUint16(compressed, offset + 6 + i * 2));
                }
            }
            offset += fieldLength;
        }
        if (offset != extraEnd) {
            setError(errorMessage, QStringLiteral("dictzip 扩展字段无法解析。"));
            return false;
        }
    }

    if (flags & gzipNameFlag) {
        if (!readCString(compressed, &offset, trailerOffset)) {
            setError(errorMessage, QStringLiteral("gzip 文件名字段不完整。"));
            return false;
        }
    }
    if (flags & gzipCommentFlag) {
        if (!readCString(compressed, &offset, trailerOffset)) {
            setError(errorMessage, QStringLiteral("gzip 注释字段不完整。"));
            return false;
        }
    }
    if (flags & gzipHeaderCrcFlag) {
        offset += 2;
    }
    if (offset > trailerOffset || chunkSizes.isEmpty()) {
        setError(errorMessage, QStringLiteral("dictzip 文件缺少有效的随机访问分块表。"));
        return false;
    }

    const quint32 uncompressedSize = readUint32(compressed, trailerOffset + 4);
    if (uncompressedSize == 0
        || uncompressedSize > quint32(std::numeric_limits<int>::max())) {
        setError(errorMessage, QStringLiteral("索引解压后大小无效或超出支持范围。"));
        return false;
    }
    const int expectedChunkCount =
        int((quint64(uncompressedSize) + quint64(chunkLength) - 1) / quint64(chunkLength));
    if (chunkSizes.size() != expectedChunkCount) {
        setError(errorMessage, QStringLiteral("dictzip 分块数量与文件大小不匹配。"));
        return false;
    }

    for (quint16 size : chunkSizes) {
        if (size == 0) {
            setError(errorMessage, QStringLiteral("dictzip 中存在空压缩块。"));
            return false;
        }
    }

    uncompressed->clear();
    uncompressed->reserve(int(uncompressedSize));
    for (int chunk = 0; chunk < chunkSizes.size(); ++chunk) {
        const int compressedSize = chunkSizes.at(chunk);
        const int outputSize = qMin(chunkLength,
                                    int(uncompressedSize) - uncompressed->size());
        const int previousSize = uncompressed->size();
        uncompressed->resize(previousSize + outputSize);

        z_stream stream{};
        const int initResult = inflateInit2(&stream, -MAX_WBITS);
        if (initResult != Z_OK) {
            setError(errorMessage, QStringLiteral("初始化 zlib 解压器失败（%1）。")
                                      .arg(initResult));
            uncompressed->clear();
            return false;
        }
        stream.next_in = reinterpret_cast<Bytef *>(
            const_cast<char *>(compressed.constData() + offset));
        stream.avail_in = uInt(compressedSize);
        stream.next_out = reinterpret_cast<Bytef *>(uncompressed->data() + previousSize);
        stream.avail_out = uInt(outputSize);

        const int inflateResult = inflate(&stream, Z_SYNC_FLUSH);
        const bool chunkValid = (inflateResult == Z_OK || inflateResult == Z_STREAM_END)
            && stream.avail_in == 0
            && stream.total_out == uLong(outputSize);
        inflateEnd(&stream);
        if (!chunkValid) {
            setError(errorMessage,
                     QStringLiteral("解压索引分块 %1 失败（%2）。")
                         .arg(chunk)
                         .arg(inflateResult));
            uncompressed->clear();
            return false;
        }
        offset += compressedSize;
    }

    if (uncompressed->size() != int(uncompressedSize)) {
        setError(errorMessage, QStringLiteral("索引解压大小与 gzip 文件头不匹配。"));
        uncompressed->clear();
        return false;
    }
    const uLong actualCrc = crc32(
        crc32(0L, Z_NULL, 0),
        reinterpret_cast<const Bytef *>(uncompressed->constData()),
        uInt(uncompressed->size()));
    if (quint32(actualCrc) != readUint32(compressed, trailerOffset)) {
        setError(errorMessage, QStringLiteral("索引 CRC 校验失败，文件可能已损坏。"));
        uncompressed->clear();
        return false;
    }
    return true;
}
