#include "ui/tabs/GeneralTab.h"

#include "core/LogCategories.h"
#include "core/LogLevel.h"
#include "core/Logger.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsStore.h"
#include "ui/AppContext.h"
#include "ui/TextUtils.h"
#include "ui/tabs/DiagnosticsCard.h"
#include "ui/tabs/InputDiagnosticsCard.h"
#include "ui/tabs/InstallCard.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/ParameterRow.h"
#include "ui/widgets/StatusBadge.h"

#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

namespace ergo {

namespace {

struct LevelOption {
    LogLevelSelection selection;
    const char *text;
};

// Order and wording follow the owner's request (S-06).
const LevelOption kLevelOptions[] = {
    {LogLevelSelection::InfoOnly, QT_TRANSLATE_NOOP("ergo::GeneralTab", "Info only")},
    {LogLevelSelection::InfoWarning, QT_TRANSLATE_NOOP("ergo::GeneralTab", "Info + Warning")},
    {LogLevelSelection::InfoWarningError, QT_TRANSLATE_NOOP("ergo::GeneralTab", "Info + Warning + Error")},
    {LogLevelSelection::InfoWarningErrorCritical,
     QT_TRANSLATE_NOOP("ergo::GeneralTab", "Info + Warning + Error + Critical (recommended)")},
    {LogLevelSelection::All, QT_TRANSLATE_NOOP("ergo::GeneralTab", "Info + Warning + Error + Critical + Debug")},
};

QHBoxLayout *labeledRow(const QString &label, QWidget *field, QWidget *parent)
{
    auto *row = new QHBoxLayout;
    row->setSpacing(12);
    auto *name = new QLabel(label, parent);
    name->setObjectName(QStringLiteral("valueLabel"));
    name->setMinimumWidth(170);
    row->addWidget(name);
    row->addWidget(field, 1);
    return row;
}

QPushButton *button(const QString &text, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

void bindStartupCombo(QComboBox *combo, SettingsStore &settings, const QString &key)
{
    QObject::connect(combo, &QComboBox::currentIndexChanged, combo, [combo, &settings, key](int index) {
        const QString value = combo->itemData(index).toString();
        if (!settings.setValue(key, value))
            qCWarning(lcUi) << "Startup mode rejected by settings:" << key << value;
    });
}

} // namespace

GeneralTab::GeneralTab(AppContext &context, QWidget *parent)
    : QWidget(parent)
    , m_context(context)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // content wraps to the window width
    outer->addWidget(scroll);

    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(20, 12, 20, 20);
    layout->setSpacing(16);

    layout->addWidget(buildApplicationCard(content));
    layout->addWidget(new InstallCard(m_context.install, content));
    layout->addWidget(buildStartupCard(content));
    layout->addWidget(buildLoggingCard(content));
    layout->addWidget(new DiagnosticsCard(m_context, content));
    layout->addWidget(new InputDiagnosticsCard(m_context.input, content));
    layout->addWidget(buildMaintenanceCard(content));
    layout->addStretch(1);

    // --- Exit -------------------------------------------------------------
    auto *exitRow = new QHBoxLayout;
    auto *exitHint = new QLabel(tr("Closing the window keeps ErgoPro2 running in the tray."), content);
    exitHint->setObjectName(QStringLiteral("hint"));
    auto *exitButton = button(tr("Exit ErgoPro2"), content);
    exitButton->setProperty("variant", QStringLiteral("danger"));
    exitRow->addWidget(exitHint, 1);
    exitRow->addWidget(exitButton);
    layout->addLayout(exitRow);
    connect(exitButton, &QPushButton::clicked, this, &GeneralTab::exitRequested);

    syncFromSettings();
    connect(&m_context.settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key.startsWith(QLatin1String("general.")))
            syncFromSettings();
    });
}

QWidget *GeneralTab::buildApplicationCard(QWidget *parent)
{
    auto *card = new CardWidget(tr("ErgoPro2"),
                                tr("Helps reduce mouse and keyboard strain with auto click, hotkeys and break "
                                   "reminders."),
                                parent);
    auto *running = new StatusBadge(card);
    running->setState(StatusBadge::State::Active, tr("RUNNING"));
    card->setHeaderWidget(running);

    auto *version = new QLabel(QApplication::applicationVersion(), card);
    version->setTextInteractionFlags(Qt::TextSelectableByMouse);
    card->contentLayout()->addLayout(labeledRow(tr("Version"), version, card));
    return card;
}

QWidget *GeneralTab::buildStartupCard(QWidget *parent)
{
    auto *card = new CardWidget(tr("Startup"), tr("What ErgoPro2 does with its window when it starts."), parent);

    m_startupAuto = new QComboBox(card);
    m_startupAuto->addItem(tr("Hide in tray"), GeneralSettings::StartupTray);
    m_startupAuto->addItem(tr("Show window"), GeneralSettings::StartupWindow);
    card->contentLayout()->addLayout(labeledRow(tr("Started at login"), m_startupAuto, card));

    m_startupManual = new QComboBox(card);
    m_startupManual->addItem(tr("Show window"), GeneralSettings::StartupWindow);
    m_startupManual->addItem(tr("Hide in tray"), GeneralSettings::StartupTray);
    card->contentLayout()->addLayout(labeledRow(tr("Started manually"), m_startupManual, card));

    bindStartupCombo(m_startupAuto, m_context.settings, GeneralSettings::StartupAutoMode);
    bindStartupCombo(m_startupManual, m_context.settings, GeneralSettings::StartupManualMode);
    return card;
}

QWidget *GeneralTab::buildLoggingCard(QWidget *parent)
{
    auto *card = new CardWidget(tr("Logging"),
                                tr("Choose which messages are written to the log files and how long log files "
                                   "are kept."),
                                parent);

    m_logLevel = new QComboBox(card);
    for (const auto &option : kLevelOptions)
        m_logLevel->addItem(tr(option.text), logLevelSelectionId(option.selection));
    card->contentLayout()->addLayout(labeledRow(tr("Log level"), m_logLevel, card));

    m_retention = new ParameterRow(tr("Keep log files for"), Logger::kMinRetentionDays, Logger::kMaxRetentionDays,
                                   tr("days"), card);
    card->contentLayout()->addWidget(m_retention);

    const QString logDir = m_context.logger.directory();
    const QString settingsDir = QFileInfo(m_context.settings.filePath()).absolutePath();

    auto *folder = new QLabel(tr("Log folder: %1").arg(wrappablePath(logDir)), card);
    folder->setObjectName(QStringLiteral("hint"));
    folder->setWordWrap(true);
    folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
    card->contentLayout()->addWidget(folder);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto *openSettings = button(tr("Open settings folder"), card);
    auto *openLogs = button(tr("Open log folder"), card);
    buttons->addWidget(openSettings);
    buttons->addWidget(openLogs);
    card->contentLayout()->addLayout(buttons);

    connect(m_logLevel, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString id = m_logLevel->itemData(index).toString();
        if (!m_context.settings.setValue(GeneralSettings::LogLevel, id))
            qCWarning(lcUi) << "Log level selection rejected by settings:" << id;
    });
    connect(m_retention, &ParameterRow::valueChanged, this, [this](int days) {
        if (!m_context.settings.setValue(GeneralSettings::LogRetentionDays, days))
            qCWarning(lcUi) << "Log retention rejected by settings:" << days;
    });
    connect(openLogs, &QPushButton::clicked, this, [this, logDir] { openFolder(logDir); });
    connect(openSettings, &QPushButton::clicked, this, [this, settingsDir] { openFolder(settingsDir); });
    return card;
}

QWidget *GeneralTab::buildMaintenanceCard(QWidget *parent)
{
    auto *card = new CardWidget(tr("Reset"), tr("Restore every ErgoPro2 setting to its default value."), parent);
    auto *row = new QHBoxLayout;
    row->addStretch(1);
    auto *reset = button(tr("Reset all settings to defaults…"), card);
    row->addWidget(reset);
    card->contentLayout()->addLayout(row);
    connect(reset, &QPushButton::clicked, this, &GeneralTab::resetSettings);
    return card;
}

void GeneralTab::openFolder(const QString &path)
{
    if (!QDir().mkpath(path)) {
        qCCritical(lcUi) << "Cannot create folder" << path;
        QMessageBox::warning(window(), tr("Cannot open folder"),
                             tr("The folder %1 does not exist and cannot be created.")
                                 .arg(QDir::toNativeSeparators(path)));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        qCCritical(lcUi) << "Cannot open folder" << path;
        QMessageBox::warning(window(), tr("Cannot open folder"),
                             tr("The folder could not be opened. Its location is:\n%1")
                                 .arg(QDir::toNativeSeparators(path)));
        return;
    }
    qCInfo(lcUi).noquote() << "Opened folder" << QDir::toNativeSeparators(path);
}

void GeneralTab::resetSettings()
{
    const auto answer = QMessageBox::question(
        window(), tr("Reset all settings"),
        tr("Restore every setting to its default value?\n\nInstallation and start at login are not changed. This "
           "cannot be undone."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    // Also clears the hotkey list; Key Control releases the keys (B.1.3).
    m_context.settings.resetToDefaults();
    qCInfo(lcUi) << "All settings were reset to defaults by the user";
}

void GeneralTab::syncFromSettings()
{
    const auto selectData = [](QComboBox *combo, const QString &value) {
        const QSignalBlocker block(combo);
        const int index = combo->findData(value);
        if (index >= 0)
            combo->setCurrentIndex(index);
    };
    selectData(m_logLevel, m_context.settings.stringValue(GeneralSettings::LogLevel));
    selectData(m_startupAuto, m_context.settings.stringValue(GeneralSettings::StartupAutoMode));
    selectData(m_startupManual, m_context.settings.stringValue(GeneralSettings::StartupManualMode));
    m_retention->setValue(m_context.settings.intValue(GeneralSettings::LogRetentionDays));
}

} // namespace ergo
