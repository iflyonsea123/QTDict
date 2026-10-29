#include "worddefinitionengine.h"

#include "dictzipreader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <limits>
#include <utility>

namespace {

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

bool readDictionaryFile(const QString &filePath, QByteArray *data,
                        QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage,
                 QStringLiteral("无法打开词义文件 %1：%2")
                     .arg(filePath, file.errorString()));
        return false;
    }

    const QByteArray sourceData = file.readAll();
    if (file.error() != QFile::NoError) {
        setError(errorMessage,
                 QStringLiteral("读取词义文件 %1 失败：%2")
                     .arg(filePath, file.errorString()));
        return false;
    }

    const bool isGzip = sourceData.size() >= 2
        && quint8(sourceData.at(0)) == 0x1f
        && quint8(sourceData.at(1)) == 0x8b;
    if (isGzip) {
        if (!DictZipReader::decompress(sourceData, data, errorMessage)) {
            return false;
        }
    } else {
        *data = sourceData;
    }
    return true;
}

bool decodeLength(const QByteArray &data, int offset, int *length,
                  int *contentOffset)
{
    if (offset < 0 || offset >= data.size()) {
        return false;
    }

    quint64 decodedLength = 0;
    quint64 placeValue = 1;
    for (int digitCount = 0; digitCount < 4 && offset < data.size(); ++digitCount) {
        const quint8 digit = quint8(data.at(offset));
        if (digit == 0) {
            return false;
        }
        if (digit == 0xff) {
            if (decodedLength > quint64(std::numeric_limits<int>::max())) {
                return false;
            }
            *length = int(decodedLength);
            *contentOffset = offset + 1;
            return true;
        }

        decodedLength += quint64(digit - 1) * placeValue;
        placeValue *= 254;
        ++offset;
    }
    return false;
}

} // namespace

bool WordDefinitionEngine::initialize(const QString &simpleFilePath,
                                      const QString &detailFilePath,
                                      QString *errorMessage)
{
    m_simpleData.clear();
    m_detailData.clear();
    m_simpleSourceFilePath.clear();
    m_detailSourceFilePath.clear();

    QByteArray simpleData;
    if (!readDictionaryFile(simpleFilePath, &simpleData, errorMessage)) {
        return false;
    }

    QByteArray detailData;
    if (!readDictionaryFile(detailFilePath, &detailData, errorMessage)) {
        return false;
    }

    m_simpleData = std::move(simpleData);
    m_detailData = std::move(detailData);
    m_simpleSourceFilePath = QFileInfo(simpleFilePath).absoluteFilePath();
    m_detailSourceFilePath = QFileInfo(detailFilePath).absoluteFilePath();
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool WordDefinitionEngine::isInitialized() const
{
    return !m_simpleData.isEmpty() && !m_detailData.isEmpty();
}

bool WordDefinitionEngine::readSimpleMeaning(int offset, QByteArray *meaning,
                                             QString *errorMessage) const
{
    return readMeaning(m_simpleData, offset, meaning, errorMessage);
}

bool WordDefinitionEngine::readDetailMeaning(int offset, QByteArray *meaning,
                                            QString *errorMessage) const
{
    return readMeaning(m_detailData, offset, meaning, errorMessage);
}

bool WordDefinitionEngine::readMeaning(const QByteArray &data, int offset,
                                       QByteArray *meaning,
                                       QString *errorMessage) const
{
    if (!meaning) {
        setError(errorMessage, QStringLiteral("未提供词义输出缓冲区。"));
        return false;
    }
    meaning->clear();
    if (offset < 0) {
        if (errorMessage) {
            errorMessage->clear();
        }
        return true;
    }

    int length = 0;
    int contentOffset = 0;
    if (!decodeLength(data, offset, &length, &contentOffset)
        || length > data.size() - contentOffset) {
        setError(errorMessage,
                 QStringLiteral("词义记录长度或偏移无效，数据文件可能已损坏。"));
        return false;
    }

    *meaning = data.mid(contentOffset, length);
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

namespace {

bool saveDefinitionFile(const QByteArray &data, const QString &sourceFilePath,
                        const QString &outputPath, const QString &fileDesc,
                        QString *errorMessage)
{
    if (data.isEmpty()) {
        setError(errorMessage,
                 QStringLiteral("%1数据为空或尚未初始化，无法导出。").arg(fileDesc));
        return false;
    }
    if (outputPath.isEmpty()) {
        setError(errorMessage,
                 QStringLiteral("未指定%1解压文件的保存路径。").arg(fileDesc));
        return false;
    }
    if (!sourceFilePath.isEmpty() && QFileInfo(sourceFilePath) == QFileInfo(outputPath)) {
        setError(errorMessage, QStringLiteral("解压文件不能覆盖当前词典源文件。"));
        return false;
    }

    QSaveFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly)) {
        setError(errorMessage,
                 QStringLiteral("无法创建解压后的%1文件：%2")
                     .arg(fileDesc, output.errorString()));
        return false;
    }
    if (output.write(data) != data.size()) {
        setError(errorMessage,
                 QStringLiteral("写入解压后的%1文件失败：%2")
                     .arg(fileDesc, output.errorString()));
        return false;
    }
    if (!output.commit()) {
        setError(errorMessage,
                 QStringLiteral("保存解压后的%1文件失败：%2")
                     .arg(fileDesc, output.errorString()));
        return false;
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

} // namespace

bool WordDefinitionEngine::exportSimpleUncompressed(const QString &outputPath,
                                                   QString *errorMessage) const
{
    return saveDefinitionFile(m_simpleData, m_simpleSourceFilePath, outputPath,
                              QStringLiteral("简明词义"), errorMessage);
}

bool WordDefinitionEngine::exportDetailUncompressed(const QString &outputPath,
                                                   QString *errorMessage) const
{
    return saveDefinitionFile(m_detailData, m_detailSourceFilePath, outputPath,
                              QStringLiteral("详细释义"), errorMessage);
}

bool WordDefinitionEngine::exportUncompressed(const QString &simpleOutputPath,
                                             const QString &detailOutputPath,
                                             QString *errorMessage) const
{
    if (!exportSimpleUncompressed(simpleOutputPath, errorMessage)) {
        return false;
    }
    return exportDetailUncompressed(detailOutputPath, errorMessage);
}

bool WordDefinitionEngine::exportUncompressedToDirectory(const QString &outputDirectory,
                                                        QString *errorMessage) const
{
    if (outputDirectory.isEmpty()) {
        setError(errorMessage, QStringLiteral("未指定目标导出目录。"));
        return false;
    }
    const QDir dir(outputDirectory);
    const QString simplePath = dir.filePath(QStringLiteral("data1.dat"));
    const QString detailPath = dir.filePath(QStringLiteral("data2.dat"));
    return exportUncompressed(simplePath, detailPath, errorMessage);
}
