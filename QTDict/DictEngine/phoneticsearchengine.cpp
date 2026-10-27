#include "phoneticsearchengine.h"

#include <QtEndian>
#include <QtZlib/zlib.h>

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QVector>

#include <limits>
#include <utility>

namespace {

constexpr quint8 gzipExtraFlag = 0x04;
constexpr quint8 gzipNameFlag = 0x08;
constexpr quint8 gzipCommentFlag = 0x10;
constexpr quint8 gzipHeaderCrcFlag = 0x02;

const char *const phoneticSortGroups[] = {
    "`5'",
    "79",
    ":",
    "iI",
    "e",
    "A",
    "Ba",
    "CRo",
    "UuJ?",
    "EL\\",
    "Q",
    "Z",
    "p",
    "b",
    "t",
    "d",
    "k",
    "g^",
    "m",
    "n",
    "l",
    "f",
    "v",
    "WP",
    "T",
    "s",
    "z",
    "F",
    "h",
    "V",
    "r",
    "w",
    "j",
    "N",
};

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

int phoneticRank(char value, bool includeSyllableMarks)
{
    const int firstGroup = includeSyllableMarks ? 0 : 2;
    for (int group = firstGroup;
         group < int(sizeof(phoneticSortGroups) / sizeof(phoneticSortGroups[0]));
         ++group) {
        const char *symbols = phoneticSortGroups[group];
        for (int i = 0; symbols[i] != '\0'; ++i) {
            if (symbols[i] == value) {
                return group - firstGroup;
            }
        }
    }
    return -1;
}

bool isSkippedCharacter(char value, bool includeSyllableMarks)
{
    if (value == '-' || value == '.' || value == ' ') {
        return true;
    }
    return !includeSyllableMarks
        && (value == '5' || value == '7' || value == '`'
            || value == '\'' || value == '9');
}

void skipPronunciationMarkers(const QByteArray &pronunciation, int *offset,
                              bool includeSyllableMarks)
{
    while (*offset < pronunciation.size()) {
        if (pronunciation.at(*offset) == '(') {
            int depth = 1;
            ++*offset;
            while (*offset < pronunciation.size() && depth > 0) {
                if (pronunciation.at(*offset) == '(') {
                    ++depth;
                } else if (pronunciation.at(*offset) == ')') {
                    --depth;
                }
                ++*offset;
            }
            continue;
        }
        if (isSkippedCharacter(pronunciation.at(*offset), includeSyllableMarks)) {
            ++*offset;
            continue;
        }
        break;
    }
}

int comparePronunciationBytes(const QByteArray &left, const QByteArray &right,
                              bool includeSyllableMarks)
{
    int leftOffset = 0;
    int rightOffset = 0;
    while (true) {
        skipPronunciationMarkers(left, &leftOffset, includeSyllableMarks);
        skipPronunciationMarkers(right, &rightOffset, includeSyllableMarks);

        const bool leftEnd = leftOffset >= left.size();
        const bool rightEnd = rightOffset >= right.size();
        if (leftEnd || rightEnd) {
            if (leftEnd && rightEnd) {
                return 0;
            }
            if (includeSyllableMarks) {
                return leftEnd ? 1 : -1;
            }
            return leftEnd ? -1 : 1;
        }

        const char leftChar = left.at(leftOffset++);
        const char rightChar = right.at(rightOffset++);
        if (leftChar == rightChar) {
            continue;
        }

        const int leftRank = phoneticRank(leftChar, includeSyllableMarks);
        const int rightRank = phoneticRank(rightChar, includeSyllableMarks);
        if (leftRank >= 0 && rightRank >= 0) {
            if (leftRank == rightRank) {
                continue;
            }
            return leftRank < rightRank ? -1 : 1;
        }
        if (leftRank >= 0 || rightRank >= 0) {
            return leftRank >= 0 ? -1 : 1;
        }

        const auto leftByte = static_cast<unsigned char>(leftChar);
        const auto rightByte = static_cast<unsigned char>(rightChar);
        return leftByte < rightByte ? -1 : 1;
    }
}

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

} // namespace

bool PhoneticSearchEngine::initialize(const QString &filePath, QString *errorMessage)
{
    m_entries.clear();
    m_data.clear();
    m_sourceFilePath.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("无法打开音标索引文件：%1").arg(file.errorString()));
        return false;
    }

    const QByteArray sourceData = file.readAll();
    if (file.error() != QFile::NoError) {
        setError(errorMessage, QStringLiteral("读取音标索引文件失败：%1").arg(file.errorString()));
        return false;
    }

    QByteArray uncompressed;
    const bool isGzip = sourceData.size() >= 2
        && quint8(sourceData.at(0)) == 0x1f
        && quint8(sourceData.at(1)) == 0x8b;
    if (isGzip) {
        if (!inflateDictZip(sourceData, &uncompressed, errorMessage)) {
            return false;
        }
    } else {
        uncompressed = sourceData;
    }
    if (uncompressed.size() < 8) {
        setError(errorMessage, QStringLiteral("音标索引文件头不完整。"));
        return false;
    }

    const quint32 declaredCount = readUint32(uncompressed, 4);
    if (declaredCount > quint32(std::numeric_limits<int>::max())) {
        setError(errorMessage, QStringLiteral("音标索引条目数超出支持范围。"));
        return false;
    }

    QVector<Entry> entries;
    entries.reserve(int(declaredCount));
    int offset = 8;
    while (offset < uncompressed.size()) {
        const int wordEnd = uncompressed.indexOf('\0', offset);
        if (wordEnd < 0) {
            setError(errorMessage, QStringLiteral("音标索引中存在未结束的记录。"));
            return false;
        }
        const int tab = uncompressed.indexOf('\t', offset);
        if (tab <= offset || tab >= wordEnd) {
            setError(errorMessage, QStringLiteral("音标索引记录缺少音标与词条分隔符。"));
            return false;
        }

        int encodedOffset = wordEnd + 1;
        qint64 decodedIndex = 0;
        qint64 placeValue = 1;
        int encodedBytes = 0;
        while (encodedOffset < uncompressed.size()
               && quint8(uncompressed.at(encodedOffset)) != 0xff
               && encodedBytes < 4) {
            const quint8 digit = quint8(uncompressed.at(encodedOffset));
            if (digit == 0) {
                setError(errorMessage, QStringLiteral("音标索引中包含无效序号。"));
                return false;
            }
            decodedIndex += qint64(digit - 1) * placeValue;
            placeValue *= 254;
            ++encodedOffset;
            ++encodedBytes;
        }
        if (encodedBytes == 0 || encodedOffset >= uncompressed.size()
            || quint8(uncompressed.at(encodedOffset)) != 0xff) {
            setError(errorMessage, QStringLiteral("音标索引记录的序号格式无效。"));
            return false;
        }
        if (decodedIndex > std::numeric_limits<int>::max()
            || decodedIndex != entries.size()) {
            setError(errorMessage, QStringLiteral("音标索引序号不连续或文件已损坏。"));
            return false;
        }

        entries.push_back({offset, tab - offset, tab + 1, wordEnd - tab - 1});
        offset = encodedOffset + 1;
    }

    if (entries.size() != int(declaredCount)) {
        setError(errorMessage,
                 QStringLiteral("音标索引条目数不匹配：文件头声明 %1，实际读取 %2。")
                     .arg(declaredCount)
                     .arg(entries.size()));
        return false;
    }

    m_data = std::move(uncompressed);
    m_entries = std::move(entries);
    m_sourceFilePath = QFileInfo(filePath).absoluteFilePath();
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool PhoneticSearchEngine::exportUncompressed(const QString &outputPath,
                                             QString *errorMessage) const
{
    if (!isInitialized()) {
        setError(errorMessage, QStringLiteral("音标索引尚未初始化，无法导出。"));
        return false;
    }
    if (outputPath.isEmpty()) {
        setError(errorMessage, QStringLiteral("未指定解压文件的保存路径。"));
        return false;
    }
    if (QFileInfo(m_sourceFilePath) == QFileInfo(outputPath)) {
        setError(errorMessage, QStringLiteral("解压文件不能覆盖当前词典源文件。"));
        return false;
    }

    QSaveFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly)) {
        setError(errorMessage,
                 QStringLiteral("无法创建解压后的音标索引文件：%1")
                     .arg(output.errorString()));
        return false;
    }
    if (output.write(m_data) != m_data.size()) {
        setError(errorMessage,
                 QStringLiteral("写入解压后的音标索引文件失败：%1")
                     .arg(output.errorString()));
        return false;
    }
    if (!output.commit()) {
        setError(errorMessage,
                 QStringLiteral("保存解压后的音标索引文件失败：%1")
                     .arg(output.errorString()));
        return false;
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool PhoneticSearchEngine::isInitialized() const
{
    return !m_entries.isEmpty();
}

int PhoneticSearchEngine::wordCount() const
{
    return m_entries.size();
}

QString PhoneticSearchEngine::pronunciationAt(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return QString();
    }
    const Entry &entry = m_entries.at(row);
    return QString::fromLatin1(m_data.constData() + entry.pronunciationOffset,
                               entry.pronunciationLength);
}

QString PhoneticSearchEngine::wordAt(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return QString();
    }
    const Entry &entry = m_entries.at(row);
    return QString::fromLatin1(m_data.constData() + entry.wordOffset,
                               entry.wordLength);
}

int PhoneticSearchEngine::entryNumberAt(int row) const
{
    return row >= 0 && row < m_entries.size() ? row + 1 : -1;
}

PhoneticSearchEngine::SearchResult
PhoneticSearchEngine::find(const QByteArray &query, bool includeSyllableMarks) const
{
    if (m_entries.isEmpty() || query.isEmpty()) {
        return {};
    }

    int first = 0;
    int last = m_entries.size();
    while (first < last) {
        const int middle = first + (last - first) / 2;
        if (comparePronunciations(query, m_entries.at(middle), false) > 0) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }

    const bool hasFuzzyMatch = first < m_entries.size()
        && comparePronunciations(query, m_entries.at(first), false) == 0;
    if (hasFuzzyMatch) {
        if (includeSyllableMarks) {
            for (int row = first; row < m_entries.size()
                 && comparePronunciations(query, m_entries.at(row), false) == 0;
                 ++row) {
                if (comparePronunciations(query, m_entries.at(row), true) == 0) {
                    return {row, true};
                }
            }
        } else {
            return {first, true};
        }
    }

    return {qBound(0, first, m_entries.size() - 1), false};
}

int PhoneticSearchEngine::comparePronunciations(const QByteArray &left,
                                                const Entry &right,
                                                bool includeSyllableMarks) const
{
    const QByteArray rightPronunciation(
        m_data.constData() + right.pronunciationOffset,
        right.pronunciationLength);
    return comparePronunciationBytes(left, rightPronunciation, includeSyllableMarks);
}

int PhoneticSearchEngine::comparePronunciations(const QByteArray &left,
                                                const QByteArray &right,
                                                bool includeSyllableMarks) const
{
    return comparePronunciationBytes(left, right, includeSyllableMarks);
}

bool PhoneticSearchEngine::inflateDictZip(const QByteArray &compressed,
                                          QByteArray *uncompressed,
                                          QString *errorMessage) const
{
    if (compressed.size() < 18
        || quint8(compressed.at(0)) != 0x1f
        || quint8(compressed.at(1)) != 0x8b
        || quint8(compressed.at(2)) != 8) {
        setError(errorMessage, QStringLiteral("音标索引文件不是有效的 gzip/dictzip 文件。"));
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
                    setError(errorMessage, QStringLiteral("不支持此版本的 dictzip 音标索引。"));
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
        setError(errorMessage, QStringLiteral("音标索引解压后大小无效或超出支持范围。"));
        return false;
    }
    const int expectedChunkCount =
        int((quint64(uncompressedSize) + quint64(chunkLength) - 1) / quint64(chunkLength));
    if (chunkSizes.size() != expectedChunkCount) {
        setError(errorMessage, QStringLiteral("dictzip 分块数量与文件大小不匹配。"));
        return false;
    }

    quint64 compressedChunkBytes = 0;
    for (quint16 size : chunkSizes) {
        if (size == 0) {
            setError(errorMessage, QStringLiteral("dictzip 中存在空压缩块。"));
            return false;
        }
        compressedChunkBytes += size;
    }
    // if (quint64(offset) + compressedChunkBytes != quint64(trailerOffset)) {
    //     setError(errorMessage, QStringLiteral("dictzip 分块长度与文件长度不匹配。"));
    //     return false;
    // }

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
                     QStringLiteral("解压音标索引分块 %1 失败（%2）。")
                         .arg(chunk)
                         .arg(inflateResult));
            uncompressed->clear();
            return false;
        }
        offset += compressedSize;
    }

    if (uncompressed->size() != int(uncompressedSize)) {
        setError(errorMessage, QStringLiteral("音标索引解压大小与 gzip 文件头不匹配。"));
        uncompressed->clear();
        return false;
    }
    const uLong actualCrc = crc32(
        crc32(0L, Z_NULL, 0),
        reinterpret_cast<const Bytef *>(uncompressed->constData()),
        uInt(uncompressed->size()));
    if (quint32(actualCrc) != readUint32(compressed, trailerOffset)) {
        setError(errorMessage, QStringLiteral("音标索引 CRC 校验失败，文件可能已损坏。"));
        uncompressed->clear();
        return false;
    }
    return true;
}
