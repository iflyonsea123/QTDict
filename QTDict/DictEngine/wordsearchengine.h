#ifndef WORDSEARCHENGINE_H
#define WORDSEARCHENGINE_H

#include <QByteArray>
#include <QString>
#include <QVector>

class WordSearchEngine
{
public:
    struct DefinitionOffsets
    {
        int simple = -1;
        int detail = -1;
    };

    struct SearchResult
    {
        int row = -1;
        bool exact = false;
    };

    bool initialize(const QString &filePath, QString *errorMessage = nullptr);
    bool exportUncompressed(const QString &outputPath,
                            QString *errorMessage = nullptr) const;
    bool isInitialized() const;
    int wordCount() const;
    QString wordAt(int row) const;
    DefinitionOffsets definitionOffsetsAt(int row) const;
    SearchResult find(const QByteArray &query) const;

private:
    int compareWordAt(int row, const QByteArray &query) const;

    QByteArray m_data;
    QVector<int> m_wordOffsets;
    QVector<DefinitionOffsets> m_definitionOffsets;
    QString m_sourceFilePath;
};

#endif // WORDSEARCHENGINE_H
