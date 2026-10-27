#ifndef PHONETICSEARCHENGINE_H
#define PHONETICSEARCHENGINE_H

#include <QByteArray>
#include <QString>
#include <QVector>

class PhoneticSearchEngine
{
public:
    struct SearchResult
    {
        int row = -1;
        bool exact = false;
    };

    bool initialize(const QString &filePath, QString *errorMessage = nullptr);
    // Saves the decompressed binary payload unchanged; initialize() accepts it directly.
    bool exportUncompressed(const QString &outputPath,
                            QString *errorMessage = nullptr) const;
    bool isInitialized() const;
    int wordCount() const;
    QString pronunciationAt(int row) const;
    QString wordAt(int row) const;
    int entryNumberAt(int row) const;
    SearchResult find(const QByteArray &query, bool includeSyllableMarks = false) const;

private:
    struct Entry
    {
        int pronunciationOffset;
        int pronunciationLength;
        int wordOffset;
        int wordLength;
    };

    bool inflateDictZip(const QByteArray &compressed, QByteArray *uncompressed,
                        QString *errorMessage) const;
    int comparePronunciations(const QByteArray &left, const Entry &right,
                              bool includeSyllableMarks) const;
    int comparePronunciations(const QByteArray &left, const QByteArray &right,
                              bool includeSyllableMarks) const;

    QByteArray m_data;
    QVector<Entry> m_entries;
    QString m_sourceFilePath;
};

#endif // PHONETICSEARCHENGINE_H
