#ifndef PHONETICLINEEDIT_H
#define PHONETICLINEEDIT_H

#include <QLineEdit>

class QKeyEvent;

class PhoneticLineEdit : public QLineEdit
{
public:
    explicit PhoneticLineEdit(QWidget *parent = nullptr);

    void insertPhoneticKey(const QString &key);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    static QChar mapKey(int key);
};

#endif // PHONETICLINEEDIT_H
