#include "phoneticlistmodel.h"

#include "phoneticsearchengine.h"

PhoneticListModel::PhoneticListModel(const PhoneticSearchEngine *engine,
                                     QObject *parent)
    : QAbstractListModel(parent)
    , m_engine(engine)
{
}

void PhoneticListModel::refresh()
{
    beginResetModel();
    endResetModel();
}

int PhoneticListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() || !m_engine ? 0 : m_engine->wordCount();
}

QVariant PhoneticListModel::data(const QModelIndex &index, int role) const
{
    if (!m_engine || !index.isValid() || index.row() < 0
        || index.row() >= m_engine->wordCount()) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        return m_engine->pronunciationAt(index.row());
    }
    return {};
}
