#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "DictEngine/phoneticsearchengine.h"
#include "DictEngine/worddefinitionengine.h"
#include "DictEngine/wordsearchengine.h"

class PhoneticKeyboard;
class PhoneticLineEdit;
class PhoneticListModel;
class WordListModel;
class QLabel;
class QPlainTextEdit;

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
    WordSearchEngine m_wordEngine;
    WordDefinitionEngine m_wordDefinitionEngine;
    PhoneticListModel *m_phoneticModel = nullptr;
    WordListModel *m_wordModel = nullptr;
    PhoneticLineEdit *m_phoneticLineEdit = nullptr;
    PhoneticKeyboard *m_phoneticKeyboard = nullptr;
    QLabel *m_dictionaryTitleLabel = nullptr;
    QLabel *m_dictionaryWordCaption = nullptr;
    QLabel *m_dictionaryWordLabel = nullptr;
    QLabel *m_dictionaryPronunciationCaption = nullptr;
    QLabel *m_dictionaryPronunciationLabel = nullptr;
    QLabel *m_dictionaryEntryNumberCaption = nullptr;
    QLabel *m_dictionaryEntryNumberLabel = nullptr;
    QLabel *m_dictionaryMatchLabel = nullptr;
    QLabel *m_simpleMeaningCaption = nullptr;
    QPlainTextEdit *m_simpleMeaningEdit = nullptr;
    QLabel *m_detailMeaningCaption = nullptr;
    QPlainTextEdit *m_detailMeaningEdit = nullptr;
    QString m_wordDataDirectory;
    bool m_settingSearchSelection = false;
    bool m_settingWordSelection = false;

    void showPhoneticKeyboard();
    void hidePhoneticKeyboard();
    bool ensurePhoneticEngine();
    bool ensureWordEngine();
    bool ensureWordDefinitionEngine();
    void searchPhonetic(const QString &text);
    void searchWord(const QString &text);
    void showPhoneticEntry(int row);
    bool showWordEntry(int row, bool exact);
};
#endif // MAINWINDOW_H
