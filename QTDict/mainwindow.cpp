#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "phonetickeyboard.h"
#include "phoneticlineedit.h"
#include "DictEngine/phoneticlistmodel.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>
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

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_phoneticLineEdit = new PhoneticLineEdit(ui->phoneticComboBox);
    ui->phoneticComboBox->setLineEdit(m_phoneticLineEdit);
    const QFont phoneticFont(QStringLiteral("Okeyen Phonetic"));
    ui->phoneticComboBox->setFont(phoneticFont);
    m_phoneticLineEdit->setFont(phoneticFont);
    ui->phoneticListView->setFont(phoneticFont);

    auto *contentLayout = new QVBoxLayout(ui->dictionaryContentView);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(10);

    auto *contentTitle = new QLabel(QStringLiteral("音标词条信息"),
                                    ui->dictionaryContentView);
    QFont titleFont = contentTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    contentTitle->setFont(titleFont);
    contentLayout->addWidget(contentTitle);

    auto *wordCaption = new QLabel(QStringLiteral("单词"), ui->dictionaryContentView);
    contentLayout->addWidget(wordCaption);
    m_dictionaryWordLabel = new QLabel(QStringLiteral("请选择音标条目"),
                                       ui->dictionaryContentView);
    QFont wordFont = m_dictionaryWordLabel->font();
    wordFont.setPointSize(wordFont.pointSize() + 4);
    wordFont.setBold(true);
    m_dictionaryWordLabel->setFont(wordFont);
    m_dictionaryWordLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_dictionaryWordLabel);

    auto *pronunciationCaption = new QLabel(QStringLiteral("音标"),
                                            ui->dictionaryContentView);
    contentLayout->addWidget(pronunciationCaption);
    m_dictionaryPronunciationLabel = new QLabel(ui->dictionaryContentView);
    m_dictionaryPronunciationLabel->setFont(phoneticFont);
    m_dictionaryPronunciationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_dictionaryPronunciationLabel);

    auto *entryNumberCaption = new QLabel(QStringLiteral("索引序号"),
                                          ui->dictionaryContentView);
    contentLayout->addWidget(entryNumberCaption);
    m_dictionaryEntryNumberLabel = new QLabel(ui->dictionaryContentView);
    m_dictionaryEntryNumberLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_dictionaryEntryNumberLabel);
    contentLayout->addStretch();

    m_phoneticModel = new PhoneticListModel(&m_phoneticEngine, this);
    ui->phoneticListView->setModel(m_phoneticModel);

    m_phoneticKeyboard = new PhoneticKeyboard(m_phoneticLineEdit, this);

    connect(ui->wordIndexButton, &QToolButton::clicked, this, [this] {
        ui->indexStack->setCurrentIndex(0);
        ui->wordComboBox->show();
        ui->phoneticComboBox->hide();
        ui->screenKeyboardButton->setEnabled(false);
        hidePhoneticKeyboard();
    });
    connect(ui->phoneticIndexButton, &QToolButton::clicked, this, [this] {
        ui->indexStack->setCurrentIndex(1);
        ui->wordComboBox->hide();
        ui->phoneticComboBox->show();
        ui->screenKeyboardButton->setEnabled(true);
        ensurePhoneticEngine();
        ui->phoneticComboBox->setFocus();
        showPhoneticKeyboard();
    });
    connect(ui->structureIndexButton, &QToolButton::clicked, this, [this] {
        ui->indexStack->setCurrentIndex(2);
        ui->wordComboBox->show();
        ui->phoneticComboBox->hide();
        ui->screenKeyboardButton->setEnabled(false);
        hidePhoneticKeyboard();
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

void MainWindow::showPhoneticEntry(int row)
{
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
