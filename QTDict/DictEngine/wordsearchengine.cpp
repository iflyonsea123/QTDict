#include "wordsearchengine.h"

#include "dictzipreader.h"

#include <QtEndian>

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <limits>
#include <utility>

namespace {

constexpr int wordIndexHeaderSize = 8;
constexpr int alphabetIndexEntrySize = 8;
constexpr int alphabetIndexCount = 28;

quint32 readUint32(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(data.constData() + offset));
}

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

int compareWords(const char *left, int leftLength,
                 const char *right, int rightLength)
{
    int offset = 0;
    while (offset < leftLength && offset < rightLength) {
        const quint8 leftChar = quint8(left[offset]);
        const quint8 rightChar = quint8(right[offset]);
        const quint8 lowerLeft = leftChar >= 'A' && leftChar <= 'Z'
            ? quint8(leftChar + ('a' - 'A')) : leftChar;
        const quint8 lowerRight = rightChar >= 'A' && rightChar <= 'Z'
            ? quint8(rightChar + ('a' - 'A')) : rightChar;
        if (lowerLeft == lowerRight) {
            ++offset;
            continue;
        }
        if (lowerLeft == ' ' && (lowerRight == '-' || lowerRight == '\'')) {
            return 1;
        }
        if (lowerRight == ' ' && (lowerLeft == '-' || lowerLeft == '\'')) {
            return -1;
        }
        return lowerLeft < lowerRight ? -1 : 1;
    }

    const quint8 leftChar = offset < leftLength ? quint8(left[offset]) : 0;
    const quint8 rightChar = offset < rightLength ? quint8(right[offset]) : 0;
    const quint8 lowerLeft = leftChar >= 'A' && leftChar <= 'Z'
        ? quint8(leftChar + ('a' - 'A')) : leftChar;
    const quint8 lowerRight = rightChar >= 'A' && rightChar <= 'Z'
        ? quint8(rightChar + ('a' - 'A')) : rightChar;
    if (lowerLeft == lowerRight) {
        return 0;
    }
    if (lowerLeft == ' ' && (lowerRight == '-' || lowerRight == '\'')) {
        return 1;
    }
    if (lowerRight == ' ' && (lowerLeft == '-' || lowerLeft == '\'')) {
        return -1;
    }
    return lowerLeft < lowerRight ? -1 : 1;
}

bool readEncodedInteger(const QByteArray &data, int *offset, int limit,
                        quint8 terminator, int *value)
{
    quint64 decodedValue = 0;
    quint64 placeValue = 1;
    for (int digitCount = 0; digitCount < 4 && *offset < limit; ++digitCount) {
        const quint8 digit = quint8(data.at(*offset));
        if (digit == terminator) {
            if (decodedValue > quint64(std::numeric_limits<int>::max())) {
                return false;
            }
            *value = int(decodedValue);
            if (terminator == 0xff) {
                ++*offset;
            }
            return true;
        }
        if (digit == 0 || digit == 0xff) {
            return false;
        }
        decodedValue += quint64(digit - 1) * placeValue;
        placeValue *= 254;
        ++*offset;
    }
    return false;
}

} // namespace

bool WordSearchEngine::initialize(const QString &filePath, QString *errorMessage)
{
    m_data.clear();
    m_wordOffsets.clear();
    m_definitionOffsets.clear();
    m_sourceFilePath.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage,
                 QStringLiteral("无法打开单词索引文件：%1").arg(file.errorString()));
        return false;
    }

    const QByteArray sourceData = file.readAll();
    if (file.error() != QFile::NoError) {
        setError(errorMessage,
                 QStringLiteral("读取单词索引文件失败：%1").arg(file.errorString()));
        return false;
    }

    QByteArray data;
    const bool isGzip = sourceData.size() >= 2
        && quint8(sourceData.at(0)) == 0x1f
        && quint8(sourceData.at(1)) == 0x8b;
    if (isGzip) {
        if (!DictZipReader::decompress(sourceData, &data, errorMessage)) {
            return false;
        }
    } else {
        data = sourceData;
    }

    const int firstWordOffset =
        wordIndexHeaderSize + alphabetIndexEntrySize * alphabetIndexCount;
    if (data.size() < firstWordOffset) {
        setError(errorMessage, QStringLiteral("单词索引文件头不完整。"));
        return false;
    }

    const quint32 declaredCount = readUint32(data, 4);
    if (declaredCount > quint32(std::numeric_limits<int>::max())) {
        setError(errorMessage, QStringLiteral("单词索引条目数超出支持范围。"));
        return false;
    }

    QVector<int> offsets;
    QVector<DefinitionOffsets> definitionOffsets;
    offsets.reserve(int(declaredCount));
    definitionOffsets.reserve(int(declaredCount));
    int recordOffset = firstWordOffset;
    const QByteArray recordEndMarker(2, '\0');
    while (recordOffset < data.size()) {
        const int wordEnd = data.indexOf('\0', recordOffset);
        if (wordEnd <= recordOffset) {
            setError(errorMessage,
                     QStringLiteral("单词索引中存在空词头或未结束的记录。"));
            return false;
        }
        const int recordEnd = data.indexOf(recordEndMarker, wordEnd + 1);
        if (recordEnd < 0) {
            setError(errorMessage, QStringLiteral("单词索引记录缺少结束标记。"));
            return false;
        }
        if (recordEnd + 1 >= data.size()) {
            setError(errorMessage, QStringLiteral("单词索引记录结束标记不完整。"));
            return false;
        }

        for (int i = recordOffset; i < wordEnd; ++i) {
            if (quint8(data.at(i)) > 0x7f) {
                setError(errorMessage,
                         QStringLiteral("单词索引中包含不支持的非 ASCII 词头。"));
                return false;
            }
        }
        if (!offsets.isEmpty()) {
            const int previousOffset = offsets.constLast();
            const int previousEnd = data.indexOf('\0', previousOffset);
            if (previousEnd < 0
                || compareWords(data.constData() + previousOffset,
                                previousEnd - previousOffset,
                                data.constData() + recordOffset,
                                wordEnd - recordOffset) > 0) {
                setError(errorMessage,
                         QStringLiteral("单词索引记录未按词头顺序排列。"));
                return false;
            }
        }

        int metadataOffset = wordEnd + 1;
        const quint8 flags = quint8(data.at(metadataOffset));
        ++metadataOffset;
        int subIndex = 0;
        if (!readEncodedInteger(data, &metadataOffset, recordEnd,
                                0xff, &subIndex)) {
            setError(errorMessage,
                     QStringLiteral("单词索引记录中的词条序号格式无效。"));
            return false;
        }
        Q_UNUSED(subIndex);

        DefinitionOffsets entryDefinitions;
        const bool hasSimple = (flags & 0x01) != 0;
        const bool hasDetail = (flags & 0x02) != 0;
        int definitionCount = int(hasSimple) + int(hasDetail);
        for (int definitionIndex = 0; definitionIndex < definitionCount;
             ++definitionIndex) {
            const bool isLastDefinition = definitionIndex + 1 == definitionCount;
            const quint8 terminator = isLastDefinition ? 0x00 : 0xff;
            int definitionOffset = 0;
            if (!readEncodedInteger(data, &metadataOffset, recordEnd + 1,
                                    terminator, &definitionOffset)) {
                setError(errorMessage,
                         QStringLiteral("单词索引记录中的词义偏移格式无效。"));
                return false;
            }
            if (hasSimple && (definitionIndex == 0 || !hasDetail)) {
                entryDefinitions.simple = definitionOffset;
            } else {
                entryDefinitions.detail = definitionOffset;
            }
        }
        if (metadataOffset != recordEnd) {
            setError(errorMessage,
                     QStringLiteral("单词索引记录的词义偏移与结束标记不匹配。"));
            return false;
        }

        offsets.push_back(recordOffset);
        definitionOffsets.push_back(entryDefinitions);
        recordOffset = recordEnd + 2;
    }

    if (offsets.size() != int(declaredCount)) {
        setError(errorMessage,
                 QStringLiteral("单词索引条目数不匹配：文件头声明 %1，实际读取 %2。")
                     .arg(declaredCount)
                     .arg(offsets.size()));
        return false;
    }

    m_data = std::move(data);
    m_wordOffsets = std::move(offsets);
    m_definitionOffsets = std::move(definitionOffsets);
    m_sourceFilePath = QFileInfo(filePath).absoluteFilePath();
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool WordSearchEngine::exportUncompressed(const QString &outputPath,
                                          QString *errorMessage) const
{
    if (!isInitialized()) {
        setError(errorMessage, QStringLiteral("单词索引尚未初始化，无法导出。"));
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
                 QStringLiteral("无法创建解压后的单词索引文件：%1")
                     .arg(output.errorString()));
        return false;
    }
    if (output.write(m_data) != m_data.size()) {
        setError(errorMessage,
                 QStringLiteral("写入解压后的单词索引文件失败：%1")
                     .arg(output.errorString()));
        return false;
    }
    if (!output.commit()) {
        setError(errorMessage,
                 QStringLiteral("保存解压后的单词索引文件失败：%1")
                     .arg(output.errorString()));
        return false;
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool WordSearchEngine::isInitialized() const
{
    return !m_wordOffsets.isEmpty();
}

int WordSearchEngine::wordCount() const
{
    return m_wordOffsets.size();
}

QString WordSearchEngine::wordAt(int row) const
{
    if (row < 0 || row >= m_wordOffsets.size()) {
        return QString();
    }
    const int start = m_wordOffsets.at(row);
    const int end = m_data.indexOf('\0', start);
    if (end < 0) {
        return QString();
    }
    return QString::fromLatin1(m_data.constData() + start, end - start);
}

WordSearchEngine::DefinitionOffsets
WordSearchEngine::definitionOffsetsAt(int row) const
{
    if (row < 0 || row >= m_definitionOffsets.size()) {
        return {};
    }
    return m_definitionOffsets.at(row);
}

WordSearchEngine::SearchResult WordSearchEngine::find(const QByteArray &query) const
{
    if (!isInitialized() || query.isEmpty()) {
        return {};
    }
    for (char character : query) {
        if (quint8(character) > 0x7f) {
            return {};
        }
    }

    int first = 0;
    int last = m_wordOffsets.size();
    while (first < last) {
        const int middle = first + (last - first) / 2;
        if (compareWordAt(middle, query) < 0) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }

    const int row = qMin(first, m_wordOffsets.size() - 1);
    return {row, first < m_wordOffsets.size() && compareWordAt(first, query) == 0};
}

int WordSearchEngine::compareWordAt(int row, const QByteArray &query) const
{
    const int offset = m_wordOffsets.at(row);
    const int end = m_data.indexOf('\0', offset);
    return compareWords(m_data.constData() + offset, end - offset,
                        query.constData(), query.size());
}
