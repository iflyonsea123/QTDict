#ifndef WORDLISTMODEL_H
#define WORDLISTMODEL_H

#include <QAbstractListModel>

class WordSearchEngine;

class WordListModel : public QAbstractListModel
{
public:
    explicit WordListModel(const WordSearchEngine *engine,
                           QObject *parent = nullptr);

    void refresh();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    const WordSearchEngine *m_engine;
};

#endif // WORDLISTMODEL_H
