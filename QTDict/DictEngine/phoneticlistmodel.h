#ifndef PHONETICLISTMODEL_H
#define PHONETICLISTMODEL_H

#include <QAbstractListModel>

class PhoneticSearchEngine;

class PhoneticListModel : public QAbstractListModel
{
public:
    explicit PhoneticListModel(const PhoneticSearchEngine *engine,
                               QObject *parent = nullptr);

    void refresh();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    const PhoneticSearchEngine *m_engine;
};

#endif // PHONETICLISTMODEL_H
