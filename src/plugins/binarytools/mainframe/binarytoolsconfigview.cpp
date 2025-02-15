// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "binarytoolsconfigview.h"
#include "binarytoolssetting.h"
#include "environmentview.h"

#include "common/widget/collapsewidget.h"
#include "common/util/custompaths.h"
#include "common/dialog/contextdialog.h"

#include <DWidget>
#include <DComboBox>
#include <DPushButton>
#include <DLineEdit>
#include <DFileDialog>
#include <DLabel>
#include <DScrollArea>
#include <DSuggestButton>
#include <dinputdialog.h>
#include <DFrame>

#include <QIcon>
#include <qmessagebox.h>
#include <QGridLayout>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPalette>

static QString CURRENT_COMMAND = "Current command";
static QString ALL_COMMAND = "All command";
static QString ENVIRONMENT = "Environment";

DWIDGET_USE_NAMESPACE

class BinaryToolsConfigViewPrivate
{
    friend class BinaryToolsConfigView;

    QGridLayout *gridLayout = nullptr;
    DWidget *compatConfigWidget = nullptr;
    DComboBox *runComandCombo = nullptr;
    DLineEdit *toolArgsEdit = nullptr;
    QDialog *combinationDialog = nullptr;
    DLabel *commandCombination = nullptr;
    DLabel *nameLabel = nullptr;
    DLabel *commandLabel = nullptr;
    DLineEdit *executableDirEdit = nullptr;
    DLineEdit *workingDirEdit = nullptr;
    EnvironmentView *envView = nullptr;
    DPushButton *addButton = nullptr;
    DPushButton *deleteButton = nullptr;
    DPushButton *renameButton = nullptr;
    DPushButton *combineButton = nullptr;
    DPushButton *useCombinationButton = nullptr;
    BinaryToolsSetting *settings = nullptr;
    QList<QString> programList;
    QList<QStringList> argsList;
    QList<QString> workingDirList;
    QList<QMap<QString, QVariant>> envList;
    QFormLayout *formLayout =nullptr;
};

BinaryToolsConfigView::BinaryToolsConfigView(QWidget *parent)
    : QWidget(parent)
    , d(new BinaryToolsConfigViewPrivate)
{
    d->compatConfigWidget = new DWidget(this);
    d->nameLabel = new DLabel();

    d->runComandCombo = new DComboBox(this);
    d->runComandCombo->setMinimumContentsLength(15);
    d->runComandCombo->setSizeAdjustPolicy(DComboBox::AdjustToContents);
    d->runComandCombo->setFixedWidth(298);

    // Add
    d->addButton = new DPushButton(this);
    d->addButton->setIcon(QIcon::fromTheme("binarytools_add"));
    d->addButton->setFixedSize(36, 36);
    d->addButton->setToolTip(tr("add"));

    // Delete
    d->deleteButton = new DPushButton(this);
    d->deleteButton->setIcon(QIcon::fromTheme("binarytools_delete"));
    d->deleteButton->setFixedSize(36, 36);
    d->deleteButton->setToolTip(tr("delete"));

    // Rename
    d->renameButton = new DPushButton(this);
    d->renameButton->setIcon(QIcon::fromTheme("binarytools_rename"));
    d->renameButton->setFixedSize(36, 36);
    d->renameButton->setToolTip(tr("rename"));

    // Combine
    d->combineButton = new DPushButton(this);
    d->combineButton->setIcon(QIcon::fromTheme("binarytools_combine"));
    d->combineButton->setFixedSize(36, 36);
    d->combineButton->setToolTip(tr("combine"));
    d->formLayout = new QFormLayout(this);
    d->formLayout->setLabelAlignment(Qt::AlignLeft);
    d->formLayout->setFormAlignment(Qt::AlignLeft);
    d->formLayout->setSpacing(10);

    auto configLabel = new DLabel(this);
    configLabel->setText(tr("Run configuration:"));

    // Create a horizontal layout for buttons
    DWidget *btnWidget = new DWidget();
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins(0,0,0,0);
    btnWidget->setLayout(buttonsLayout);

    buttonsLayout->addWidget(d->runComandCombo);
    buttonsLayout->addWidget(d->addButton);
    buttonsLayout->addWidget(d->deleteButton);
    buttonsLayout->addWidget(d->renameButton);
    buttonsLayout->addWidget(d->combineButton);
    buttonsLayout->setAlignment(Qt::AlignLeft);


    // Add the label and the horizontal layout to the form layout
    d->formLayout->addRow(configLabel,btnWidget);
    d->formLayout->addRow(d->compatConfigWidget);

    setConfigWidget();

    setLayout(d->formLayout);
    d->combinationDialog = new QDialog(this);
    d->commandCombination = new DLabel(tr("command combination:"), d->combinationDialog);
    d->useCombinationButton = new DPushButton(tr("Use Combination Command"), d->combinationDialog);
    initializeCombinationDialog();

    if (!d->settings) {
        QString iniPath = CustomPaths::user(CustomPaths::Flags::Configures) + QDir::separator() + QString("binarytools.ini");
        bool setDefaultVaule = false;
        if (!QFile::exists(iniPath)) {
            setDefaultVaule = true;
        }
        d->settings = new BinaryToolsSetting(iniPath, this);
        if (setDefaultVaule) {
            initializeCombo();
        }
    }
    readConfig();

    connect(d->runComandCombo, static_cast<void (DComboBox::*)(const QString &)>(&DComboBox::currentIndexChanged),
            this, &BinaryToolsConfigView::currentConfigChanged);

    connect(d->addButton, &DPushButton::clicked,
            this, &BinaryToolsConfigView::addCompatConfig);

    connect(d->deleteButton, &DPushButton::clicked,
            this, &BinaryToolsConfigView::deleteCompatConfig);

    connect(d->renameButton, &DPushButton::clicked,
            this, &BinaryToolsConfigView::renameCompatConfig);

    connect(d->combineButton, &DPushButton::clicked,
            this, &BinaryToolsConfigView::combineCompatConfig);
}

BinaryToolsConfigView::~BinaryToolsConfigView()
{
    if (d)
        delete d;
}

bool BinaryToolsConfigView::saveConfig()
{
    QString curCommand = d->runComandCombo->currentText();
    if (curCommand.isEmpty())
        return false;

    if (d->executableDirEdit->text().isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Warning"),
                           tr("Please select working directory."),
                           QMessageBox::Ok, d->compatConfigWidget);
        msgBox.exec();
        return false;
    }

    if (d->toolArgsEdit->text().isEmpty())
        d->toolArgsEdit->setText("");
    QStringList commandList = QStringList() << d->executableDirEdit->text() << d->toolArgsEdit->text()
                                            << d->nameLabel->text() << d->workingDirEdit->text();

    d->settings->setValue(CURRENT_COMMAND, curCommand);
    d->settings->setValue(curCommand, commandList);
    d->settings->setValue(curCommand + ENVIRONMENT, d->envView->getEnvironment());
    return true;
}

void BinaryToolsConfigView::readConfig()
{
    QStringList allCommand = qvariant_cast<QStringList>(d->settings->getValue(ALL_COMMAND));
    for (QString command : allCommand) {
        if (d->runComandCombo->findText(command) == -1)
            d->runComandCombo->addItem(command);
    }

    QString curCommand = qvariant_cast<QString>(d->settings->getValue(CURRENT_COMMAND, ""));
    if (curCommand.isEmpty())
        return;
    int index = d->runComandCombo->findText(curCommand);
    d->runComandCombo->setCurrentIndex(index);
    updateView(curCommand);
}

QList<QString> BinaryToolsConfigView::getProgramList()
{
    saveConfig();
    if (d->programList.isEmpty())
        d->programList.push_back(d->executableDirEdit->text());
    return d->programList;
}

QList<QStringList> BinaryToolsConfigView::getArgumentsList()
{
    if (d->argsList.isEmpty())
        d->argsList.push_back(d->toolArgsEdit->text().split(" "));
    return d->argsList;
}

QList<QString> BinaryToolsConfigView::getWorkingDirList()
{
    if (d->workingDirList.isEmpty())
        d->workingDirList.push_back(d->workingDirEdit->text());
    return d->workingDirList;
}

QList<QMap<QString, QVariant>> BinaryToolsConfigView::getEnvironmentList()
{
    if (d->envList.isEmpty())
        d->envList.push_back(d->envView->getEnvironment());
    return d->envList;
}

void BinaryToolsConfigView::updateView(const QString &command)
{
    if (command.isEmpty()) {
        d->nameLabel->setText("");
        d->commandLabel->setText("");
        d->toolArgsEdit->setText("");
        d->executableDirEdit->setText("");
        d->workingDirEdit->setText("");
        d->envView->setValue({});
        return;
    }

    BinaryToolsSetting settings;
    QStringList argList = qvariant_cast<QStringList>(d->settings->getValue(command, "/usr/bin/" + command));
    if (argList.isEmpty())
        return;
    d->executableDirEdit->setText(argList.at(0));
    d->toolArgsEdit->setText(argList.size() > 1 ? argList.at(1) : "");
    d->nameLabel->setText(argList.size() > 2 ? argList.at(2) : "");
    d->workingDirEdit->setText(argList.size() > 3 ? argList.at(3) : "");
    d->commandLabel->setText(d->executableDirEdit->text() + " " + d->toolArgsEdit->text());

    QMap<QString, QVariant> map = qvariant_cast<QMap<QString, QVariant>>(d->settings->getValue(command + ENVIRONMENT));
    d->envView->setValue(map);
}

void BinaryToolsConfigView::currentConfigChanged(const QString &text)
{
    if (d->settings->getValue(CURRENT_COMMAND).toString() == text)
        return;

    d->settings->setValue(CURRENT_COMMAND, text);
    updateView(text);
    emit comboChanged();
}

void BinaryToolsConfigView::initializeCombo()
{
    QString fileName = CustomPaths::global(CustomPaths::Flags::Configures)
            + QDir::separator() + QString("binarytool.support");
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray array = doc.array();
        for (auto node : array) {
            auto obj = node.toObject();
            QString name = obj.value("name").toString();
            appendCommand(name);
        }
    }
}

void BinaryToolsConfigView::initializeCombinationDialog()
{
    QRect parentRect = this->geometry();
    int parentX = parentRect.x();
    int parentY = parentRect.y();
    int parentWidth = parentRect.width();
    int parentHeight = parentRect.height();
    QRect dialogRect = d->combinationDialog->geometry();
    int dialogWidth = dialogRect.width();
    int dialogHeight = dialogRect.height();
    int x = parentX + (parentWidth - dialogWidth) / 2;
    int y = parentY + (parentHeight - dialogHeight) / 2;
    setGeometry(x, y, dialogWidth, dialogHeight);

    auto gridLayout = new QGridLayout(d->combinationDialog);
    d->combinationDialog->setLayout(gridLayout);
    gridLayout->setSpacing(30);
    gridLayout->setContentsMargins(30, 30, 30, 30);
    gridLayout->addWidget(d->commandCombination, 0, 0, 1, 2);
    gridLayout->addWidget(d->useCombinationButton, 1, 1, 1, 1);
    QObject::connect(d->useCombinationButton, &DPushButton::clicked, [=](){
        d->combinationDialog->close();
        emit useCombinationCommand();
    });
}

void BinaryToolsConfigView::addCompatConfig()
{
    if (!d->runComandCombo->currentText().isEmpty() && !saveConfig())
        return;

    bool ok;

    QString name = QInputDialog::getText(this, tr("Add new command"),
                                       tr("New command name:"),
                                       QLineEdit::Normal, "", &ok);
    if (!ok)
        return;

    appendCommand(name);
}

void BinaryToolsConfigView::deleteCompatConfig()
{
    bool doSave = false, doCancle = false;
    auto ok = [&](bool checked) {
        Q_UNUSED(checked);
        doSave = true;
    };
    auto no = [&](bool checked) {
        Q_UNUSED(checked);
        doSave = false;
    };
    auto cancel = [&](bool checked) {
        Q_UNUSED(checked);
    };

    ContextDialog::question(tr("Delete Configuration?"),
                            tr("Do you really want to delete the command <b>%1</b>?").arg(d->nameLabel->text()),
                            QMessageBox::Question,
                            ok, no, cancel);
    if (!doSave || doCancle)
        return;

    QStringList allCommand = qvariant_cast<QStringList>(d->settings->getValue(ALL_COMMAND));
    allCommand.removeOne(d->runComandCombo->currentText());
    d->settings->setValue(ALL_COMMAND, allCommand);
    d->settings->deleteKey(d->runComandCombo->currentText());
    d->settings->deleteKey(d->runComandCombo->currentText() + ENVIRONMENT);

    int curIndex = d->runComandCombo->currentIndex();
    d->runComandCombo->removeItem(curIndex);
    d->runComandCombo->setCurrentIndex((curIndex - 1) >= 0 ? curIndex - 1 : 0);
    if (d->runComandCombo->currentText().isEmpty())
        d->deleteButton->setEnabled(false);
}

void BinaryToolsConfigView::renameCompatConfig()
{
    bool ok;

    QString name = QInputDialog::getText(this, tr("Rename..."),
                                        tr("New name for command <b>%1</b>:").
                                        arg(d->runComandCombo->currentText()),
                                        QLineEdit::Normal,
                                        d->runComandCombo->currentText(), &ok);
    if (!ok)
        return;

    if (name == d->runComandCombo->currentText())
        return;

    QString uniName = uniqueName(name);
    if (uniName.isEmpty())
        return;

    QStringList allCommand = qvariant_cast<QStringList>(d->settings->getValue(ALL_COMMAND));
    int index = allCommand.indexOf(d->runComandCombo->currentText());
    if (index != -1) {
        allCommand.replace(index, uniName);
    } else {
        allCommand.append(uniName);
    }
    d->settings->setValue(CURRENT_COMMAND, uniName);
    d->settings->setValue(ALL_COMMAND, allCommand);
    d->nameLabel->setText(name);
    QStringList commandList = QStringList() << d->executableDirEdit->text()<< d->toolArgsEdit->text()
                                            << d->nameLabel->text() << d->workingDirEdit->text();
    d->settings->setValue(uniName, commandList);
    d->settings->setValue(uniName + ENVIRONMENT, d->envView->getEnvironment());
    d->settings->deleteKey(d->runComandCombo->currentText());
    d->settings->deleteKey(d->runComandCombo->currentText() + ENVIRONMENT);

    int itemIndex = d->runComandCombo->currentIndex();
    d->runComandCombo->insertItem(itemIndex + 1, uniName);
    d->runComandCombo->setCurrentText(uniName);
    d->runComandCombo->removeItem(itemIndex);
}

void BinaryToolsConfigView::combineCompatConfig()
{
    saveConfig();
    d->combinationDialog->showNormal();
    d->programList.push_back(d->executableDirEdit->text());
    d->argsList.push_back(d->toolArgsEdit->text().split(" "));
    d->workingDirList.push_back(d->workingDirEdit->text());
    d->envList.push_back(d->envView->getEnvironment());
    d->commandCombination->setText(d->commandCombination->text() + " " + d->nameLabel->text());
}

void BinaryToolsConfigView::setConfigWidget()
{
    auto cmdLabel = new DLabel(d->compatConfigWidget);
    cmdLabel->setText(tr("Command:"));
    d->commandLabel = new DLabel(d->compatConfigWidget);
    QHBoxLayout *cmdLayout =new QHBoxLayout();
    cmdLayout->addWidget(d->commandLabel);
    cmdLayout->setContentsMargins(12,0,0,0);

    auto argsLabel = new DLabel(d->compatConfigWidget);
    argsLabel->setText(tr("Tool arguments:"));
    d->toolArgsEdit = new DLineEdit(d->compatConfigWidget);
    d->toolArgsEdit->setPlaceholderText(tr("Input your arguments"));

    auto exeLabel = new DLabel(d->compatConfigWidget);
    exeLabel->setText(tr("Executable:"));
    d->executableDirEdit = new DLineEdit(d->compatConfigWidget);
    auto browseButton1 = new DSuggestButton("...", d->compatConfigWidget);
    browseButton1->setFixedSize(36,36);

    // Wrap executableDirEdit and browseButton1 in a QHBoxLayout
    auto exeLayout = new QHBoxLayout();
    exeLayout->addWidget(d->executableDirEdit);
    exeLayout->addWidget(browseButton1);

    auto workLabel = new DLabel(d->compatConfigWidget);
    workLabel->setText(tr("Working directory:"));
    d->workingDirEdit = new DLineEdit(d->compatConfigWidget);
    auto browseButton2 = new DSuggestButton("...", d->compatConfigWidget);
    browseButton2->setFixedSize(36,36);

    // Wrap workingDirEdit and browseButton2 in a QHBoxLayout
    auto workLayout = new QHBoxLayout();
    workLayout->addWidget(d->workingDirEdit);
    workLayout->addWidget(browseButton2);

    auto envLabel = new DLabel(d->compatConfigWidget);
    envLabel->setText(tr("Configuration environment for the current command:"));

    auto *envViewFrame = new DFrame();
    d->envView = new EnvironmentView(envViewFrame);
    QVBoxLayout *envViewlayout = new QVBoxLayout(envViewFrame);
    envViewlayout->setContentsMargins(5,5,5,5);
    envViewFrame->setLayout(envViewlayout);
    envViewlayout->addWidget(d->envView);
    envViewlayout->setAlignment(Qt::AlignLeft);

    d->envView->setFixedHeight(280);
    auto formLayout = new QFormLayout(d->compatConfigWidget);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(0, 0, 0, 0);

    formLayout->addRow(cmdLabel, cmdLayout);
    formLayout->addRow(argsLabel, d->toolArgsEdit);
    formLayout->addRow(exeLabel, exeLayout); // Add the QHBoxLayout for executable
    formLayout->addRow(workLabel, workLayout); // Add the QHBoxLayout for working directory

    // Add a spacer item to create vertical space
    auto spacerItem = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    formLayout->addItem(spacerItem);

    formLayout->addRow(envLabel);

   // formLayout->addRow(d->envView);
    formLayout->addRow(envViewFrame);

    d->compatConfigWidget->setLayout(formLayout);
    connect(browseButton1, &DPushButton::clicked, [=]() {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString filePath = DFileDialog::getOpenFileName(nullptr, tr("Select Executabel Path"), dir);
        if (filePath.isEmpty() && !QFileInfo(filePath).exists())
            return;
        d->executableDirEdit->setText(filePath);
    });

    connect(browseButton2, &DPushButton::clicked, [=]() {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString filePath = DFileDialog::getExistingDirectory(nullptr, tr("Select Working Directory"), dir);
        if (filePath.isEmpty() && !QFileInfo(filePath).exists())
            return;
        d->workingDirEdit->setText(filePath);
    });

    connect(d->toolArgsEdit, &DLineEdit::textChanged, [=](){
        d->commandLabel->setText(d->executableDirEdit->text() + " " + d->toolArgsEdit->text());
    });

    connect(d->executableDirEdit, &DLineEdit::textChanged, [=](){
        d->commandLabel->setText(d->executableDirEdit->text() + " " + d->toolArgsEdit->text());
    });

    connect(this, &BinaryToolsConfigView::comboChanged, d->envView ,&EnvironmentView::disableDleteButton);
}

void BinaryToolsConfigView::appendCommand(const QString &name)
{
    QString uniName = uniqueName(name);
    if (uniName.isEmpty())
        return;

    QStringList allCommand = qvariant_cast<QStringList>(d->settings->getValue(ALL_COMMAND));
    allCommand.append(uniName);
    d->settings->setValue(CURRENT_COMMAND, uniName);
    d->settings->setValue(ALL_COMMAND, allCommand);

    d->nameLabel->setText(name);
    d->toolArgsEdit->setText("");
    d->executableDirEdit->setText("/usr/bin/" + name);
    d->workingDirEdit->setText(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    d->envView->initModel();
    QStringList commandList = QStringList() << d->executableDirEdit->text()<< d->toolArgsEdit->text()
                                            << d->nameLabel->text() << d->workingDirEdit->text();
    d->settings->setValue(uniName, commandList);
    d->settings->setValue(uniName + ENVIRONMENT, d->envView->getEnvironment());

    //call currentConfigChanged()
    d->runComandCombo->addItem(uniName);
    d->runComandCombo->setCurrentText(uniName);
    d->deleteButton->setEnabled(true);
}

QString BinaryToolsConfigView::uniqueName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        if (d->runComandCombo->findText(result) == -1)
            return result;
        int i = 2;

        QString tryName = result + QString::number(i);
        while (d->runComandCombo->findText(tryName) != -1) {
            tryName = result + QString::number(++i);
        }
        return tryName;
    }
    return result;
}
