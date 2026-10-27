#include "phonetickeyboard.h"
#include "phoneticlineedit.h"

#include <QFont>
#include <QGridLayout>
#include <QGuiApplication>
#include <QPushButton>
#include <QScreen>

namespace {

struct KeyDefinition
{
    const char *label;
    const char *input;
    int width;
    bool enabled;
    bool phonetic;
};

const KeyDefinition keyRows[][15] = {
    {
        {"~", "", 10, false, false}, {"A", "1", 10, true, true},
        {"W", "2", 10, true, true}, {"E", "3", 10, true, true},
        {"F", "4", 10, true, true}, {"5", "", 10, false, false},
        {"6", "", 10, false, false}, {"7", "", 10, false, false},
        {"8", "", 10, false, false}, {"9", "", 10, false, false},
        {"0", "", 10, false, false}, {"-", "", 10, false, false},
        {"=", "", 10, false, false}, {"退格", "\b", 12, true, false},
        {"", "", 0, false, false},
    },
    {
        {"Tab", "", 14, false, false}, {"Q", "q", 10, true, true},
        {"w", "w", 10, true, true}, {"e", "e", 10, true, true},
        {"r", "r", 10, true, true}, {"t", "t", 10, true, true},
        {"V", "y", 10, true, true}, {"U", "u", 10, true, true},
        {"i", "i", 10, true, true}, {"C", "o", 10, true, true},
        {"p", "p", 10, true, true}, {"[", "", 10, false, false},
        {"]", "", 10, false, false}, {"\\", "", 12, false, false},
        {"", "", 0, false, false},
    },
    {
        {"Caps", "", 18, false, false}, {"B", "a", 10, true, true},
        {"s", "s", 10, true, true}, {"d", "d", 10, true, true},
        {"f", "f", 10, true, true}, {"g", "g", 10, true, true},
        {"h", "h", 10, true, true}, {"j", "j", 10, true, true},
        {"k", "k", 10, true, true}, {"l", "l", 10, true, true},
        {":", ":", 10, true, true}, {"'", "", 10, false, false},
        {"Enter", "\n", 12, true, false}, {"", "", 0, false, false},
        {"", "", 0, false, false},
    },
    {
        {"Shift", "", 22, false, false}, {"z", "z", 10, true, true},
        {"T", "x", 10, true, true}, {"Z", "c", 10, true, true},
        {"v", "v", 10, true, true}, {"b", "b", 10, true, true},
        {"n", "n", 10, true, true}, {"m", "m", 10, true, true},
        {"N", ",", 10, true, true}, {".", "", 10, false, false},
        {"/", "", 10, false, false}, {"", "", 0, false, false},
        {"", "", 0, false, false}, {"", "", 0, false, false},
        {"", "", 0, false, false},
    },
};

} // namespace

PhoneticKeyboard::PhoneticKeyboard(PhoneticLineEdit *target, QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint
                          | Qt::WindowDoesNotAcceptFocus)
    , m_target(target)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowTitle(QStringLiteral("音标键盘"));
    resize(600, 180);

    const QFont phoneticFont(QStringLiteral("Okeyen Phonetic"));
    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setHorizontalSpacing(2);
    layout->setVerticalSpacing(2);

    constexpr int columnCount = 150;
    for (int column = 0; column < columnCount; ++column) {
        layout->setColumnStretch(column, 1);
    }
    for (int row = 0; row < 4; ++row) {
        layout->setRowStretch(row, 1);
        int column = 0;
        for (const KeyDefinition &key : keyRows[row]) {
            if (key.width == 0) {
                continue;
            }

            auto *button = new QPushButton(QString::fromUtf8(key.label), this);
            button->setEnabled(key.enabled);
            button->setFocusPolicy(Qt::NoFocus);
            button->setMinimumHeight(30);
            if (key.phonetic) {
                button->setFont(phoneticFont);
            }
            layout->addWidget(button, row, column, 1, key.width);
            column += key.width;

            if (key.enabled) {
                const QString input = QString::fromLatin1(key.input);
                connect(button, &QPushButton::clicked, this, [this, input] {
                    if (!m_target) {
                        return;
                    }
                    m_target->insertPhoneticKey(input);
                });
            }
        }
    }
}

void PhoneticKeyboard::showFor(QWidget *anchor)
{
    if (anchor) {
        const QPoint belowWindow = anchor->mapToGlobal(QPoint(0, anchor->height()));
        QScreen *screen = QGuiApplication::screenAt(belowWindow);
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        if (screen) {
            const QRect available = screen->availableGeometry();
            const int maxX = qMax(available.left(),
                                  available.right() - width() + 1);
            int x = qBound(available.left(), belowWindow.x(), maxX);
            int y = belowWindow.y();
            if (y + height() > available.bottom() + 1) {
                y = anchor->mapToGlobal(QPoint(0, 0)).y() - height();
            }
            const int maxY = qMax(available.top(),
                                  available.bottom() - height() + 1);
            y = qBound(available.top(), y, maxY);
            move(x, y);
        }
    }

    show();
    raise();
    if (m_target) {
        m_target->setFocus();
    }
}
