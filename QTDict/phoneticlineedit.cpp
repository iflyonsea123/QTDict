#include "phoneticlineedit.h"

#include <QKeyEvent>

PhoneticLineEdit::PhoneticLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
}

void PhoneticLineEdit::insertPhoneticKey(const QString &key)
{
    if (key == QStringLiteral("\b")) {
        backspace();
    } else if (key == QStringLiteral("\n")) {
        emit returnPressed();
    } else if (!key.isEmpty()) {
        const QChar mapped = mapKey(key.at(0).toUpper().unicode());
        if (!mapped.isNull()) {
            insert(mapped);
        }
    }
    setFocus();
}

void PhoneticLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier
                              | Qt::MetaModifier)) {
        QLineEdit::keyPressEvent(event);
        return;
    }

    const int key = event->key();
    if (key == Qt::Key_Backspace || key == Qt::Key_Delete
        || key == Qt::Key_Return || key == Qt::Key_Enter
        || key == Qt::Key_Left || key == Qt::Key_Right
        || key == Qt::Key_Home || key == Qt::Key_End
        || key == Qt::Key_Tab || key == Qt::Key_Escape) {
        QLineEdit::keyPressEvent(event);
        return;
    }

    const QChar mapped = mapKey(key);
    if (mapped.isNull()) {
        event->accept();
        return;
    }
    insert(mapped);
    event->accept();
}

QChar PhoneticLineEdit::mapKey(int key)
{
    switch (key) {
    case Qt::Key_1: return QLatin1Char('A');
    case Qt::Key_2: return QLatin1Char('W');
    case Qt::Key_3: return QLatin1Char('E');
    case Qt::Key_4: return QLatin1Char('F');
    case Qt::Key_A: return QLatin1Char('B');
    case Qt::Key_C: return QLatin1Char('Z');
    case Qt::Key_Q: return QLatin1Char('Q');
    case Qt::Key_U: return QLatin1Char('U');
    case Qt::Key_X: return QLatin1Char('T');
    case Qt::Key_Y: return QLatin1Char('V');
    case Qt::Key_Comma: return QLatin1Char('N');
    case Qt::Key_Colon:
    case Qt::Key_Semicolon: return QLatin1Char(':');
    default:
        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            return QLatin1Char(char(key + ('a' - 'A')));
        }
        return QChar();
    }
}
