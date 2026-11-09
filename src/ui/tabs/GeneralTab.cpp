#include "ui/tabs/GeneralTab.h"

#include "core/LogCategories.h"
#include "core/LogLevel.h"
#include "core/Logger.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsStore.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/ParameterRow.h"
#include "ui/widgets/StatusBadge.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
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

} // namespace

GeneralTab::GeneralTab(SettingsStore &settings, const QString &logDirectory, QWidget *parent)
    : QWidget(parent)
    , m_settings(settings)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(20, 12, 20, 20);
    layout->setSpacing(16);

    // --- Application card -------------------------------------------------
    auto *appCard = new CardWidget(tr("ErgoPro2"),
                                   tr("Helps reduce mouse and keyboard strain with auto click, "
                                      "hotkeys and break reminders."),
                                   content);
    auto *running = new StatusBadge(appCard);
    running->setState(StatusBadge::State::Active, tr("RUNNING"));
    appCard->setHeaderWidget(running);

    auto *versionRow = new QHBoxLayout;
    auto *versionName = new QLabel(tr("Version"), appCard);
    versionName->setObjectName(QStringLiteral("valueLabel"));
    versionName->setMinimumWidth(170);
    auto *versionValue = new QLabel(QApplication::applicationVersion(), appCard);
    versionValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
    versionRow->addWidget(versionName);
    versionRow->addWidget(versionValue, 1);
    appCard->contentLayout()->addLayout(versionRow);
    layout->addWidget(appCard);

    // --- Logging card -----------------------------------------------------
    auto *logCard = new CardWidget(tr("Logging"),
                                   tr("Choose which messages are written to the log files and how long "
                                      "log files are kept."),
                                   content);

    auto *levelRow = new QHBoxLayout;
    levelRow->setSpacing(12);
    auto *levelName = new QLabel(tr("Log level"), logCard);
    levelName->setObjectName(QStringLiteral("valueLabel"));
    levelName->setMinimumWidth(170);
    m_logLevel = new QComboBox(logCard);
    for (const auto &option : kLevelOptions)
        m_logLevel->addItem(tr(option.text), logLevelSelectionId(option.selection));
    levelRow->addWidget(levelName);
    levelRow->addWidget(m_logLevel, 1);
    logCard->contentLayout()->addLayout(levelRow);

    m_retention = new ParameterRow(tr("Keep log files for"), Logger::kMinRetentionDays, Logger::kMaxRetentionDays,
                                   tr("days"), logCard);
    logCard->contentLayout()->addWidget(m_retention);

    auto *folder = new QLabel(tr("Log folder: %1").arg(QDir::toNativeSeparators(logDirectory)), logCard);
    folder->setObjectName(QStringLiteral("hint"));
    folder->setWordWrap(true);
    folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
    logCard->contentLayout()->addWidget(folder);
    layout->addWidget(logCard);

    layout->addStretch(1);

    // --- Exit -------------------------------------------------------------
    auto *exitRow = new QHBoxLayout;
    auto *exitHint = new QLabel(tr("Closing the window keeps ErgoPro2 running in the tray."), content);
    exitHint->setObjectName(QStringLiteral("hint"));
    auto *exitButton = new QPushButton(tr("Exit ErgoPro2"), content);
    exitButton->setProperty("variant", QStringLiteral("danger"));
    exitButton->setCursor(Qt::PointingHandCursor);
    exitRow->addWidget(exitHint, 1);
    exitRow->addWidget(exitButton);
    layout->addLayout(exitRow);

    syncFromSettings();

    connect(m_logLevel, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString id = m_logLevel->itemData(index).toString();
        if (!m_settings.setValue(GeneralSettings::LogLevel, id))
            qCWarning(lcUi) << "Log level selection rejected by settings:" << id;
    });
    connect(m_retention, &ParameterRow::valueChanged, this, [this](int days) {
        if (!m_settings.setValue(GeneralSettings::LogRetentionDays, days))
            qCWarning(lcUi) << "Log retention rejected by settings:" << days;
    });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key.startsWith(QLatin1String("general.")))
            syncFromSettings();
    });
    connect(exitButton, &QPushButton::clicked, this, &GeneralTab::exitRequested);
}

void GeneralTab::syncFromSettings()
{
    {
        const QSignalBlocker block(m_logLevel);
        const int index = m_logLevel->findData(m_settings.stringValue(GeneralSettings::LogLevel));
        m_logLevel->setCurrentIndex(index >= 0 ? index : 3);
    }
    m_retention->setValue(m_settings.intValue(GeneralSettings::LogRetentionDays));
}

} // namespace ergo
