#ifndef PHONETICKEYBOARD_H
#define PHONETICKEYBOARD_H

#include <QPointer>
#include <QWidget>

class PhoneticLineEdit;

class PhoneticKeyboard : public QWidget
{
public:
    explicit PhoneticKeyboard(PhoneticLineEdit *target, QWidget *parent = nullptr);

    void showFor(QWidget *anchor);

private:
    QPointer<PhoneticLineEdit> m_target;
};

#endif // PHONETICKEYBOARD_H
