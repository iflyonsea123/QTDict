#ifndef WORDDEFINITIONENGINE_H
#define WORDDEFINITIONENGINE_H

#include <QByteArray>
#include <QString>

class WordDefinitionEngine
{
public:
    bool initialize(const QString &simpleFilePath, const QString &detailFilePath,
                    QString *errorMessage = nullptr);
    bool exportUncompressed(const QString &simpleOutputPath,
                            const QString &detailOutputPath,
                            QString *errorMessage = nullptr) const;
    bool exportUncompressedToDirectory(const QString &outputDirectory,
                                       QString *errorMessage = nullptr) const;
    bool exportSimpleUncompressed(const QString &outputPath,
                                  QString *errorMessage = nullptr) const;
    bool exportDetailUncompressed(const QString &outputPath,
                                  QString *errorMessage = nullptr) const;
    bool isInitialized() const;
    bool readSimpleMeaning(int offset, QByteArray *meaning,
                           QString *errorMessage = nullptr) const;
    bool readDetailMeaning(int offset, QByteArray *meaning,
                           QString *errorMessage = nullptr) const;

private:
    bool readMeaning(const QByteArray &data, int offset, QByteArray *meaning,
                     QString *errorMessage) const;

    QByteArray m_simpleData;
    QByteArray m_detailData;
    QString m_simpleSourceFilePath;
    QString m_detailSourceFilePath;
};

#endif // WORDDEFINITIONENGINE_H
