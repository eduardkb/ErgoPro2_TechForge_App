#include "ui/tabs/StatisticsTab.h"

#include "core/LogCategories.h"
#include "core/settings/SettingsStore.h"
#include "core/settings/StatisticsSettings.h"
#include "core/statistics/ChartAxis.h"
#include "ui/statistics/StatisticsCharts.h"
#include "ui/statistics/StatisticsController.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/ParameterRow.h"

#include <QDesktopServices>
#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

namespace ergo {

namespace {

enum Tile {
    Clicks,
    ClicksForYou,
    Keys,
    Wheel,
    Distance,
    MouseTime,
    KeyboardTime,
    ActiveTime,
    MaxUsage,
    TileCount
};

QPushButton *navButton(const QString &text, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

} // namespace

StatisticsTab::StatisticsTab(StatisticsController &controller, SettingsStore &settings, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_settings(settings)
{
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageLayout->addWidget(scroll);
    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 12, 0, 20);
    layout->setSpacing(16);

    // --- 1. date header
    auto *header = new QVBoxLayout;
    header->setContentsMargins(4, 0, 4, 0);
    header->setSpacing(2);
    m_title = new QLabel(content);
    m_title->setObjectName(QStringLiteral("cardTitle"));
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
    m_title->setFont(titleFont);
    m_subtitle = new QLabel(content);
    m_subtitle->setObjectName(QStringLiteral("hint"));
    header->addWidget(m_title);
    header->addWidget(m_subtitle);
    layout->addLayout(header);

    // --- 2. chart 1
    auto *usageCard = new CardWidget(
        tr("Computer usage"),
        tr("100 % = mouse/keyboard use for the whole “Break after working” time. It keeps growing when a break "
           "is dismissed or postponed and goes back to 0 % after a pause as long as the break length."),
        content);
    m_usage = new UsageChart(usageCard);
    usageCard->contentLayout()->addWidget(m_usage);
    QStringList legend;
    for (BreakEvent e : {BreakEvent::Shown, BreakEvent::Dismissed, BreakEvent::Postponed, BreakEvent::Completed}) {
        const QString names[] = {tr("break shown"), tr("dismissed"), tr("postponed"), tr("completed")};
        legend << QStringLiteral("<span style=\"color:%1; font-weight:600\">%2</span> %3")
                      .arg(UsageChart::markerColor(e).name(), UsageChart::markerGlyph(e), names[int(e)]);
    }
    auto *legendLabel = new QLabel(legend.join(QStringLiteral("&nbsp;&nbsp;&nbsp;")), usageCard);
    legendLabel->setObjectName(QStringLiteral("hint"));
    legendLabel->setTextFormat(Qt::RichText);
    usageCard->contentLayout()->addWidget(legendLabel);
    layout->addWidget(usageCard);

    // --- 3. chart 2
    auto *activityCard = new CardWidget(tr("Activity"),
                                        tr("Mouse clicks, key presses and mouse distance in 5-minute steps. Point "
                                           "at a bar for the exact values."),
                                        content);
    m_activity = new ActivityChart(activityCard);
    activityCard->contentLayout()->addWidget(m_activity);
    layout->addWidget(activityCard);

    // --- 4. daily totals
    auto *totals = new CardWidget(tr("Day totals"), {}, content);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(12);
    const QString names[TileCount] = {tr("Mouse clicks"),       tr("Clicks done for you"),
                                      tr("Key presses"),        tr("Mouse wheel"),
                                      tr("Mouse distance"),     tr("Mouse use"),
                                      tr("Keyboard use"),       tr("Total active use"),
                                      tr("Highest usage")};
    for (int i = 0; i < TileCount; ++i) {
        auto *tile = new QVBoxLayout;
        tile->setSpacing(2);
        auto *name = new QLabel(names[i], totals);
        name->setObjectName(QStringLiteral("hint"));
        auto *value = new QLabel(QStringLiteral("–"), totals);
        value->setObjectName(QStringLiteral("cardTitle"));
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        tile->addWidget(name);
        tile->addWidget(value);
        grid->addLayout(tile, i / 3, i % 3);
        m_values << value;
    }
    for (int col = 0; col < 3; ++col)
        grid->setColumnStretch(col, 1);
    totals->contentLayout()->addLayout(grid);
    auto *note = new QLabel(tr("Only counts and times are recorded – never which keys you pressed or where you "
                               "clicked. Mouse distance is approximate (screen size). Mouse and keyboard use can add "
                               "up to more than the total when both are used in the same second."),
                            totals);
    note->setObjectName(QStringLiteral("hint"));
    note->setWordWrap(true);
    totals->contentLayout()->addWidget(note);
    layout->addWidget(totals);

    // --- 5. day navigation (end of the daily section)
    auto *nav = new QHBoxLayout;
    m_previous = navButton(tr("‹ Previous day"), content);
    m_today = navButton(tr("Today"), content);
    m_next = navButton(tr("Next day ›"), content);
    m_today->setProperty("variant", QStringLiteral("primary"));
    nav->addStretch(1);
    nav->addWidget(m_previous);
    nav->addWidget(m_today);
    nav->addWidget(m_next);
    nav->addStretch(1);
    layout->addLayout(nav);

    // --- 6. options
    namespace SS = StatisticsSettings;
    auto *options = new CardWidget(tr("Options"), {}, content);
    auto *folderRow = new QHBoxLayout;
    auto *folder = new QLabel(tr("Statistics folder: %1").arg(QDir::toNativeSeparators(m_controller.directory())),
                              options);
    folder->setObjectName(QStringLiteral("hint"));
    folder->setWordWrap(true);
    folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *open = navButton(tr("Open statistics folder"), options);
    folderRow->addWidget(folder, 1);
    folderRow->addWidget(open);
    options->contentLayout()->addLayout(folderRow);
    m_retention = new ParameterRow(tr("Keep statistics for"), SS::kRetentionMin, SS::kRetentionMax, tr("days"),
                                   options);
    m_retention->setHint(tr("One file per day. Older files are deleted (also right away when you lower this)."));
    m_retention->setValue(m_settings.intValue(SS::RetentionDays));
    options->contentLayout()->addWidget(m_retention);
    layout->addWidget(options);
    layout->addStretch(1);

    // --- wiring
    connect(m_previous, &QPushButton::clicked, this, [this] {
        if (const auto d = m_controller.previousDay(m_date))
            showDay(*d);
    });
    connect(m_next, &QPushButton::clicked, this, [this] {
        if (const auto d = m_controller.nextDay(m_date))
            showDay(*d);
    });
    connect(m_today, &QPushButton::clicked, this, [this] { showDay(m_controller.today()); });
    connect(open, &QPushButton::clicked, this, &StatisticsTab::openFolder);
    connect(m_retention, &ParameterRow::valueChanged, this,
            [this](int days) { m_settings.setValue(StatisticsSettings::RetentionDays, days); });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == StatisticsSettings::RetentionDays)
            m_retention->setValue(m_settings.intValue(key));
    });
    connect(&m_controller, &StatisticsController::todayChanged, this, [this] {
        if (m_followToday && m_date != m_controller.today())
            m_date = m_controller.today(); // midnight while "today" is shown
        if (m_date == m_controller.today())
            refresh(); // live, once per minute
    });
    connect(&m_controller, &StatisticsController::daysChanged, this, [this] {
        if (m_date != m_controller.today() && m_controller.day(m_date).isEmpty())
            showDay(m_controller.today()); // the shown day was deleted (retention)
        else
            refreshNavigation();
    });
    showDay(m_controller.today());
}

void StatisticsTab::showDay(const QDate &date)
{
    m_date = date;
    m_followToday = date == m_controller.today();
    qCDebug(lcUi) << "Statistics: showing" << date.toString(Qt::ISODate);
    refresh();
}

void StatisticsTab::refresh()
{
    const DayStats day = m_controller.day(m_date);
    const bool isToday = m_date == m_controller.today();
    m_title->setText(isToday ? tr("Today – %1").arg(QLocale().toString(m_date, QLocale::LongFormat))
                             : QLocale().toString(m_date, QLocale::LongFormat));
    m_subtitle->setText(isToday ? tr("Live – updated every minute.") : tr("Saved statistics (read-only)."));

    const HourRange range = hourRange(day);
    m_usage->setDay(day, range);
    m_activity->setDay(day, range);

    const DayTotals t = computeTotals(day);
    m_values[Clicks]->setText(tr("%1  (left %2 · right %3 · other %4)")
                                  .arg(t.clicks())
                                  .arg(t.clicksLeft)
                                  .arg(t.clicksRight)
                                  .arg(t.clicksOther));
    m_values[ClicksForYou]->setText(QString::number(t.clicksForYou));
    m_values[Keys]->setText(QString::number(t.keyPresses));
    m_values[Wheel]->setText(QString::number(t.wheel));
    m_values[Distance]->setText(formatDistanceCm(t.distanceCm));
    m_values[MouseTime]->setText(formatDurationHms(t.mouseSeconds));
    m_values[KeyboardTime]->setText(formatDurationHms(t.keyboardSeconds));
    m_values[ActiveTime]->setText(formatDurationHms(t.activeSeconds));
    m_values[MaxUsage]->setText(QStringLiteral("%1 %").arg(t.maxUsagePercent, 0, 'f', 1));
    refreshNavigation();
}

void StatisticsTab::refreshNavigation()
{
    m_previous->setEnabled(m_controller.previousDay(m_date).has_value());
    m_next->setEnabled(m_controller.nextDay(m_date).has_value());
    m_today->setEnabled(m_date != m_controller.today());
}

void StatisticsTab::openFolder()
{
    const QString path = m_controller.directory();
    if (!QDir().mkpath(path) || !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        qCCritical(lcUi).noquote() << "Cannot open the statistics folder" << QDir::toNativeSeparators(path);
        QMessageBox::warning(window(), tr("Cannot open folder"),
                             tr("The folder could not be opened. Its location is:\n%1")
                                 .arg(QDir::toNativeSeparators(path)));
        return;
    }
    qCInfo(lcUi).noquote() << "Opened folder" << QDir::toNativeSeparators(path);
}

} // namespace ergo
