#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "DictEngine/phoneticsearchengine.h"

class PhoneticKeyboard;
class PhoneticLineEdit;
class PhoneticListModel;
class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    PhoneticSearchEngine m_phoneticEngine;
    PhoneticListModel *m_phoneticModel = nullptr;
    PhoneticLineEdit *m_phoneticLineEdit = nullptr;
    PhoneticKeyboard *m_phoneticKeyboard = nullptr;
    QLabel *m_dictionaryWordLabel = nullptr;
    QLabel *m_dictionaryPronunciationLabel = nullptr;
    QLabel *m_dictionaryEntryNumberLabel = nullptr;
    bool m_settingSearchSelection = false;

    void showPhoneticKeyboard();
    void hidePhoneticKeyboard();
    bool ensurePhoneticEngine();
    void searchPhonetic(const QString &text);
    void showPhoneticEntry(int row);
};
#endif // MAINWINDOW_H
