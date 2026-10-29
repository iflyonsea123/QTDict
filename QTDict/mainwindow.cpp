#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "phonetickeyboard.h"
#include "phoneticlineedit.h"
#include "DictEngine/phoneticlistmodel.h"
#include "DictEngine/wordlistmodel.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>
#include <QStringList>
#include <QTextCodec>
#include <QVBoxLayout>

namespace {

QString findPhoneticIndexFile()
{
    const QStringList startingDirectories = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath(),
    };
    const QStringList relativePaths = {
        QStringLiteral("data/Iword2.dat.dz"),
        QStringLiteral("bin/data/Iword2.dat.dz"),
        QStringLiteral("bin/data/all25/Iword2.dat.dz"),
    };

    for (const QString &startingDirectory : startingDirectories) {
        QDir directory(startingDirectory);
        for (int level = 0; level < 10; ++level) {
            for (const QString &relativePath : relativePaths) {
                const QString candidate = directory.filePath(relativePath);
                if (QFileInfo(candidate).isFile()) {
                    return QDir::cleanPath(candidate);
                }
            }
            if (!directory.cdUp()) {
                break;
            }
        }
    }
    return QString();
}

QString findWordIndexFile()
{
    const QStringList startingDirectories = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath(),
    };
    const QStringList relativePaths = {
        QStringLiteral("data/all25/iword.dat.dz"),
        QStringLiteral("bin/data/all25/iword.dat.dz"),
        QStringLiteral("data/iword.dat.dz"),
        QStringLiteral("bin/data/iword.dat.dz"),
        QStringLiteral("bin/data/all25demo/iword.dat.dz"),
        QStringLiteral("bin/ddata/iword.dat.dz"),
    };

    for (const QString &startingDirectory : startingDirectories) {
        QDir directory(startingDirectory);
        for (int level = 0; level < 10; ++level) {
            for (const QString &relativePath : relativePaths) {
                const QString candidate = directory.filePath(relativePath);
                if (QFileInfo(candidate).isFile()) {
                    return QDir::cleanPath(candidate);
                }
            }
            if (!directory.cdUp()) {
                break;
            }
        }
    }
    return QString();
}

bool decodeDictionaryText(const QByteArray &data, QString *text,
                          QString *errorMessage)
{
    QTextCodec *codec = QTextCodec::codecForName("GB18030");
    if (!codec) {
        codec = QTextCodec::codecForName("GBK");
    }
    if (!codec) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("当前 Qt 环境不支持 GB18030/GBK 编码。");
        }
        return false;
    }
    *text = codec->toUnicode(data.constData(), data.size());
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->indexStack->setCurrentIndex(0);
    ui->wordComboBox->show();
    ui->phoneticComboBox->hide();

    m_phoneticLineEdit = new PhoneticLineEdit(ui->phoneticComboBox);
    ui->phoneticComboBox->setLineEdit(m_phoneticLineEdit);
    const QFont phoneticFont(QStringLiteral("Okeyen Phonetic"));
    ui->phoneticComboBox->setFont(phoneticFont);
    m_phoneticLineEdit->setFont(phoneticFont);
    ui->phoneticListView->setFont(phoneticFont);

    auto *contentLayout = new QVBoxLayout(ui->dictionaryContentView);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(10);

    m_dictionaryTitleLabel = new QLabel(ui->dictionaryContentView);
    QFont titleFont = m_dictionaryTitleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_dictionaryTitleLabel->setFont(titleFont);
    contentLayout->addWidget(m_dictionaryTitleLabel);

    m_dictionaryWordCaption = new QLabel(QStringLiteral("单词"),
                                         ui->dictionaryContentView);
    contentLayout->addWidget(m_dictionaryWordCaption);
    m_dictionaryWordLabel = new QLabel(QStringLiteral("请选择音标条目"),
                                       ui->dictionaryContentView);
    QFont wordFont = m_dictionaryWordLabel->font();
    wordFont.setPointSize(wordFont.pointSize() + 4);
    wordFont.setBold(true);
    m_dictionaryWordLabel->setFont(wordFont);
    m_dictionaryWordLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_dictionaryWordLabel);

    m_dictionaryPronunciationCaption = new QLabel(QStringLiteral("音标"),
                                                  ui->dictionaryContentView);
    contentLayout->addWidget(m_dictionaryPronunciationCaption);
    m_dictionaryPronunciationLabel = new QLabel(ui->dictionaryContentView);
    m_dictionaryPronunciationLabel->setFont(phoneticFont);
    m_dictionaryPronunciationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_dictionaryPronunciationLabel);

    m_dictionaryEntryNumberCaption = new QLabel(QStringLiteral("索引序号"),
                                                ui->dictionaryContentView);
    contentLayout->addWidget(m_dictionaryEntryNumberCaption);
    m_dictionaryEntryNumberLabel = new QLabel(ui->dictionaryContentView);
    m_dictionaryEntryNumberLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_dictionaryEntryNumberLabel);
    m_dictionaryMatchLabel = new QLabel(ui->dictionaryContentView);
    m_dictionaryMatchLabel->setWordWrap(true);
    contentLayout->addWidget(m_dictionaryMatchLabel);

    m_simpleMeaningCaption = new QLabel(QStringLiteral("简明词义"),
                                        ui->dictionaryContentView);
    contentLayout->addWidget(m_simpleMeaningCaption);
    m_simpleMeaningEdit = new QPlainTextEdit(ui->dictionaryContentView);
    m_simpleMeaningEdit->setReadOnly(true);
    m_simpleMeaningEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_simpleMeaningEdit->setMinimumHeight(80);
    contentLayout->addWidget(m_simpleMeaningEdit, 2);

    m_detailMeaningCaption = new QLabel(QStringLiteral("详细释义"),
                                        ui->dictionaryContentView);
    contentLayout->addWidget(m_detailMeaningCaption);
    m_detailMeaningEdit = new QPlainTextEdit(ui->dictionaryContentView);
    m_detailMeaningEdit->setReadOnly(true);
    m_detailMeaningEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_detailMeaningEdit->setMinimumHeight(170);
    contentLayout->addWidget(m_detailMeaningEdit, 5);

    m_phoneticModel = new PhoneticListModel(&m_phoneticEngine, this);
    ui->phoneticListView->setModel(m_phoneticModel);
    m_wordModel = new WordListModel(&m_wordEngine, this);
    ui->wordListWidget->setModel(m_wordModel);
    ui->wordListWidget->setUniformItemSizes(true);

    m_phoneticKeyboard = new PhoneticKeyboard(m_phoneticLineEdit, this);
    showWordEntry(-1, false);

    connect(ui->wordIndexButton, &QToolButton::clicked, this, [this] {
        ui->indexStack->setCurrentIndex(0);
        ui->wordComboBox->show();
        ui->phoneticComboBox->hide();
        ui->screenKeyboardButton->setEnabled(false);
        hidePhoneticKeyboard();
        showWordEntry(-1, false);
        ensureWordEngine();
        searchWord(ui->wordComboBox->currentText());
    });
    connect(ui->phoneticIndexButton, &QToolButton::clicked, this, [this] {
        ui->indexStack->setCurrentIndex(1);
        ui->wordComboBox->hide();
        ui->phoneticComboBox->show();
        ui->screenKeyboardButton->setEnabled(true);
        if (ensurePhoneticEngine()) {
            searchPhonetic(ui->phoneticComboBox->currentText());
        } else {
            showPhoneticEntry(-1);
        }
        ui->phoneticComboBox->setFocus();
        showPhoneticKeyboard();
    });
    connect(ui->structureIndexButton, &QToolButton::clicked, this, [this] {
        ui->indexStack->setCurrentIndex(2);
        ui->wordComboBox->show();
        ui->phoneticComboBox->hide();
        ui->screenKeyboardButton->setEnabled(false);
        hidePhoneticKeyboard();
        m_dictionaryTitleLabel->setText(QStringLiteral("词根词缀索引"));
        m_dictionaryWordLabel->setText(QStringLiteral("该索引模块尚未移植"));
        m_dictionaryPronunciationCaption->hide();
        m_dictionaryPronunciationLabel->hide();
        m_dictionaryEntryNumberLabel->clear();
        m_dictionaryMatchLabel->clear();
        m_simpleMeaningCaption->hide();
        m_simpleMeaningEdit->hide();
        m_detailMeaningCaption->hide();
        m_detailMeaningEdit->hide();
    });
    connect(ui->screenKeyboardButton, &QToolButton::clicked, this, [this] {
        if (m_phoneticKeyboard->isVisible()) {
            hidePhoneticKeyboard();
        } else {
            showPhoneticKeyboard();
        }
    });
    connect(m_phoneticLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::searchPhonetic);
    connect(m_phoneticLineEdit, &QLineEdit::returnPressed,
            this, [this] {
                searchPhonetic(ui->phoneticComboBox->currentText());
            });
    connect(ui->wordComboBox->lineEdit(), &QLineEdit::textChanged,
            this, &MainWindow::searchWord);
    connect(ui->wordComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, [this] {
                searchWord(ui->wordComboBox->currentText());
            });
    connect(ui->searchButton, &QToolButton::clicked, this, [this] {
        if (ui->indexStack->currentIndex() == 0) {
            searchWord(ui->wordComboBox->currentText());
        } else if (ui->indexStack->currentIndex() == 1) {
            searchPhonetic(ui->phoneticComboBox->currentText());
        }
    });
    connect(ui->phoneticListView->selectionModel(),
            &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) {
                if (m_settingSearchSelection || !current.isValid()) {
                    return;
                }
                const QString pronunciation =
                    m_phoneticEngine.pronunciationAt(current.row());
                m_settingSearchSelection = true;
                m_phoneticLineEdit->setText(pronunciation);
                m_settingSearchSelection = false;
                showPhoneticEntry(current.row());
            });
    connect(ui->wordListWidget->selectionModel(),
            &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) {
                if (m_settingWordSelection || !current.isValid()) {
                    return;
                }
                const QString word = m_wordEngine.wordAt(current.row());
                if (word.isEmpty()) {
                    return;
                }
                m_settingWordSelection = true;
                ui->wordComboBox->setEditText(word);
                m_settingWordSelection = false;
                if (showWordEntry(current.row(), true)) {
                    statusBar()->showMessage(
                        QStringLiteral("已选择第 %1 条单词索引")
                            .arg(current.row() + 1));
                }
            });
    ensureWordEngine();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showPhoneticKeyboard()
{
    m_phoneticKeyboard->showFor(this);
}

void MainWindow::hidePhoneticKeyboard()
{
    if (m_phoneticKeyboard) {
        m_phoneticKeyboard->hide();
    }
}

bool MainWindow::ensurePhoneticEngine()
{
    if (m_phoneticEngine.isInitialized()) {
        return true;
    }

    const QString filePath = findPhoneticIndexFile();
    if (filePath.isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("找不到音标索引文件 Iword2.dat.dz。"));
        return false;
    }

    QString errorMessage;
    if (!m_phoneticEngine.initialize(filePath, &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        return false;
    }
    // m_phoneticEngine.exportUncompressed("C:\\learning\\okeyen\\PC\\okwordoldcode\\word\\project25\\bin\\IPhonetic.dat");

    m_phoneticModel->refresh();
    statusBar()->showMessage(
        QStringLiteral("音标索引已加载：%1 条（%2）")
            .arg(m_phoneticEngine.wordCount())
            .arg(QDir::toNativeSeparators(filePath)));
    return true;
}

bool MainWindow::ensureWordEngine()
{
    if (m_wordEngine.isInitialized()) {
        return true;
    }

    const QString filePath = findWordIndexFile();
    if (filePath.isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("找不到单词索引文件 iword.dat.dz。"));
        return false;
    }

    QString errorMessage;
    if (!m_wordEngine.initialize(filePath, &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        return false;
    }
    m_wordDataDirectory = QFileInfo(filePath).absolutePath();
    // m_wordEngine.exportUncompressed("C:\\learning\\okeyen\\PC\\okwordoldcode\\word\\project25\\bin\\go\\iword.dat");
    m_wordModel->refresh();
    statusBar()->showMessage(
        QStringLiteral("单词索引已加载：%1 条（%2）")
            .arg(m_wordEngine.wordCount())
            .arg(QDir::toNativeSeparators(filePath)));
    return true;
}

bool MainWindow::ensureWordDefinitionEngine()
{
    if (m_wordDefinitionEngine.isInitialized()) {
        return true;
    }
    if (m_wordDataDirectory.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("单词索引目录尚未确定。"));
        return false;
    }

    const QString simpleFilePath =
        QDir(m_wordDataDirectory).filePath(QStringLiteral("data1.dat.dz"));
    const QString detailFilePath =
        QDir(m_wordDataDirectory).filePath(QStringLiteral("data2.dat.dz"));
    QString errorMessage;
    if (!m_wordDefinitionEngine.initialize(simpleFilePath, detailFilePath,
                                           &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        return false;
    }
    // m_wordDefinitionEngine.exportUncompressedToDirectory("C:\\learning\\okeyen\\PC\\okwordoldcode\\word\\project25\\bin\\go");
    return true;
}

void MainWindow::searchPhonetic(const QString &text)
{
    if (m_settingSearchSelection) {
        return;
    }
    if (text.isEmpty()) {
        ui->phoneticListView->clearSelection();
        ui->phoneticListView->setCurrentIndex(QModelIndex());
        showPhoneticEntry(-1);
        return;
    }
    if (!ensurePhoneticEngine()) {
        return;
    }

    const QByteArray query = text.toLatin1();
    bool isAscii = true;
    for (QChar character : text) {
        if (character.unicode() > 0x7f) {
            isAscii = false;
            break;
        }
    }
    if (!isAscii) {
        ui->phoneticListView->clearSelection();
        ui->phoneticListView->setCurrentIndex(QModelIndex());
        showPhoneticEntry(-1);
        statusBar()->showMessage(QStringLiteral("音标检索仅支持音标键盘输入的字符。"));
        return;
    }

    const PhoneticSearchEngine::SearchResult result = m_phoneticEngine.find(query);
    if (result.row < 0) {
        ui->phoneticListView->clearSelection();
        showPhoneticEntry(-1);
        statusBar()->showMessage(QStringLiteral("没有可显示的音标条目。"));
        return;
    }

    m_settingSearchSelection = true;
    const QModelIndex index = m_phoneticModel->index(result.row, 0);
    ui->phoneticListView->setCurrentIndex(index);
    ui->phoneticListView->scrollTo(index, QAbstractItemView::PositionAtCenter);
    m_settingSearchSelection = false;
    showPhoneticEntry(result.row);
    statusBar()->showMessage(result.exact
        ? QStringLiteral("精确匹配：第 %1 条音标").arg(result.row + 1)
        : QStringLiteral("未找到完全匹配，已定位到最接近的音标条目。"));
}

void MainWindow::searchWord(const QString &text)
{
    if (m_settingWordSelection || ui->indexStack->currentIndex() != 0) {
        return;
    }
    if (text.isEmpty()) {
        ui->wordListWidget->clearSelection();
        ui->wordListWidget->setCurrentIndex(QModelIndex());
        showWordEntry(-1, false);
        return;
    }
    if (!ensureWordEngine()) {
        return;
    }

    bool isAscii = true;
    for (QChar character : text) {
        if (character.unicode() > 0x7f) {
            isAscii = false;
            break;
        }
    }
    if (!isAscii) {
        ui->wordListWidget->clearSelection();
        ui->wordListWidget->setCurrentIndex(QModelIndex());
        showWordEntry(-1, false);
        statusBar()->showMessage(QStringLiteral("单词索引目前仅支持 ASCII 词头。"));
        return;
    }

    const WordSearchEngine::SearchResult result =
        m_wordEngine.find(text.toLatin1());
    if (result.row < 0) {
        ui->wordListWidget->clearSelection();
        ui->wordListWidget->setCurrentIndex(QModelIndex());
        showWordEntry(-1, false);
        statusBar()->showMessage(QStringLiteral("没有可显示的单词索引条目。"));
        return;
    }

    const QModelIndex index = m_wordModel->index(result.row, 0);
    m_settingWordSelection = true;
    ui->wordListWidget->setCurrentIndex(index);
    ui->wordListWidget->scrollTo(index, QAbstractItemView::PositionAtCenter);
    m_settingWordSelection = false;
    if (showWordEntry(result.row, result.exact)) {
        statusBar()->showMessage(result.exact
            ? QStringLiteral("精确匹配：第 %1 条单词").arg(result.row + 1)
            : QStringLiteral("未找到完全匹配，已定位到最接近的单词：%1（第 %2 条）")
                  .arg(m_wordEngine.wordAt(result.row))
                  .arg(result.row + 1));
    }
}

void MainWindow::showPhoneticEntry(int row)
{
    m_dictionaryTitleLabel->setText(QStringLiteral("音标词条信息"));
    m_dictionaryWordCaption->show();
    m_dictionaryPronunciationCaption->show();
    m_dictionaryPronunciationLabel->show();
    m_dictionaryEntryNumberCaption->setText(QStringLiteral("索引序号"));
    m_dictionaryEntryNumberCaption->show();
    m_dictionaryEntryNumberLabel->show();
    m_dictionaryMatchLabel->hide();
    m_simpleMeaningCaption->hide();
    m_simpleMeaningEdit->hide();
    m_detailMeaningCaption->hide();
    m_detailMeaningEdit->hide();

    const QString word = m_phoneticEngine.wordAt(row);
    if (word.isEmpty()) {
        m_dictionaryWordLabel->setText(QStringLiteral("请选择音标条目"));
        m_dictionaryPronunciationLabel->clear();
        m_dictionaryEntryNumberLabel->clear();
        return;
    }

    m_dictionaryWordLabel->setText(word);
    m_dictionaryPronunciationLabel->setText(
        m_phoneticEngine.pronunciationAt(row));
    m_dictionaryEntryNumberLabel->setText(
        QString::number(m_phoneticEngine.entryNumberAt(row)));
}

bool MainWindow::showWordEntry(int row, bool exact)
{
    m_dictionaryTitleLabel->setText(QStringLiteral("单词索引信息"));
    m_dictionaryWordCaption->show();
    m_dictionaryPronunciationCaption->hide();
    m_dictionaryPronunciationLabel->hide();
    m_dictionaryEntryNumberCaption->setText(QStringLiteral("索引序号"));
    m_dictionaryEntryNumberCaption->show();
    m_dictionaryEntryNumberLabel->show();
    m_dictionaryMatchLabel->show();
    m_simpleMeaningCaption->show();
    m_simpleMeaningEdit->show();
    m_detailMeaningCaption->show();
    m_detailMeaningEdit->show();

    const QString word = m_wordEngine.wordAt(row);
    if (word.isEmpty()) {
        m_dictionaryWordLabel->setText(QStringLiteral("请输入单词进行搜索"));
        m_dictionaryEntryNumberLabel->clear();
        m_dictionaryMatchLabel->clear();
        m_simpleMeaningEdit->clear();
        m_detailMeaningEdit->clear();
        return true;
    }

    m_dictionaryWordLabel->setText(word);
    m_dictionaryEntryNumberLabel->setText(QString::number(row + 1));
    m_dictionaryMatchLabel->setText(exact
        ? QStringLiteral("匹配状态：精确匹配")
        : QStringLiteral("匹配状态：近似词条"));

    m_simpleMeaningEdit->clear();
    m_detailMeaningEdit->clear();
    if (!ensureWordDefinitionEngine()) {
        const QString message = statusBar()->currentMessage();
        m_simpleMeaningEdit->setPlainText(message);
        return false;
    }

    const WordSearchEngine::DefinitionOffsets offsets =
        m_wordEngine.definitionOffsetsAt(row);
    QByteArray meaningData;
    QString errorMessage;
    if (!m_wordDefinitionEngine.readSimpleMeaning(
            offsets.simple, &meaningData, &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        m_simpleMeaningEdit->setPlainText(errorMessage);
        return false;
    }
    QString meaningText;
    if (!decodeDictionaryText(meaningData, &meaningText, &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        m_simpleMeaningEdit->setPlainText(errorMessage);
        return false;
    }
    m_simpleMeaningEdit->setPlainText(meaningText);

    if (!m_wordDefinitionEngine.readDetailMeaning(
            offsets.detail, &meaningData, &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        m_detailMeaningEdit->setPlainText(errorMessage);
        return false;
    }
    if (!decodeDictionaryText(meaningData, &meaningText, &errorMessage)) {
        statusBar()->showMessage(errorMessage);
        m_detailMeaningEdit->setPlainText(errorMessage);
        return false;
    }
    m_detailMeaningEdit->setPlainText(meaningText);
    return true;
}
