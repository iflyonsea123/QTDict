#include "wordlistmodel.h"

#include "wordsearchengine.h"

WordListModel::WordListModel(const WordSearchEngine *engine, QObject *parent)
    : QAbstractListModel(parent)
    , m_engine(engine)
{
}

void WordListModel::refresh()
{
    beginResetModel();
    endResetModel();
}

int WordListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_engine || !m_engine->isInitialized()) {
        return 0;
    }
    return m_engine->wordCount();
}

QVariant WordListModel::data(const QModelIndex &index, int role) const
{
    if (!m_engine || !index.isValid() || role != Qt::DisplayRole
        || index.row() < 0 || index.row() >= m_engine->wordCount()) {
        return QVariant();
    }
    return m_engine->wordAt(index.row());
}
