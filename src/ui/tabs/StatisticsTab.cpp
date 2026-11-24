#include "ui/tabs/StatisticsTab.h"

#include "core/LogCategories.h"
#include "core/settings/SettingsStore.h"
#include "core/settings/StatisticsSettings.h"
#include "core/statistics/ChartAxis.h"
#include "core/statistics/DateText.h"
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
    // 1.7.9 (owner request): the date and the day navigation are fixed at the
    // top; the three day sections share one card below them; titles only.
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // --- 1. fixed top bar: date, then slim navigation buttons
    auto *topBar = new QWidget(this);
    auto *top = new QVBoxLayout(topBar);
    top->setContentsMargins(4, 10, 4, 10);
    top->setSpacing(6);
    m_title = new QLabel(topBar);
    m_title->setObjectName(QStringLiteral("cardTitle"));
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
    m_title->setFont(titleFont);
    top->addWidget(m_title);
    auto *nav = new QHBoxLayout;
    nav->setSpacing(6);
    m_previous = navButton(tr("‹ Previous day"), topBar);
    m_today = navButton(tr("Today"), topBar);
    m_next = navButton(tr("Next day ›"), topBar);
    m_today->setProperty("variant", QStringLiteral("primary"));
    for (QPushButton *b : {m_previous, m_today, m_next}) {
        b->setProperty("size", QStringLiteral("slim"));
        nav->addWidget(b);
    }
    nav->addStretch(1);
    top->addLayout(nav);
    pageLayout->addWidget(topBar);
    auto *topLine = new QFrame(this);
    topLine->setObjectName(QStringLiteral("sectionDivider"));
    topLine->setFrameShape(QFrame::NoFrame);
    topLine->setFixedHeight(1);
    pageLayout->addWidget(topLine);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // narrow screens (2.5.0)
    pageLayout->addWidget(scroll, 1);
    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 12, 0, 20);
    layout->setSpacing(16);

    // --- 2. one card with the three sections of the shown day
    auto *dayCard = new QFrame(content);
    dayCard->setObjectName(QStringLiteral("card"));
    dayCard->setAttribute(Qt::WA_StyledBackground, true);
    auto *day = new QVBoxLayout(dayCard);
    day->setContentsMargins(20, 16, 20, 18);
    day->setSpacing(10);
    const auto addSection = [&](const QString &title, bool divider) {
        if (divider) {
            auto *line = new QFrame(dayCard);
            line->setObjectName(QStringLiteral("sectionDivider"));
            line->setFrameShape(QFrame::NoFrame);
            line->setFixedHeight(1);
            day->addSpacing(6);
            day->addWidget(line);
            day->addSpacing(6);
        }
        auto *label = new QLabel(title, dayCard);
        label->setObjectName(QStringLiteral("cardTitle"));
        day->addWidget(label);
    };

    // 2a. computer usage chart + legend
    addSection(tr("Computer usage"), false);
    m_usage = new UsageChart(dayCard);
    day->addWidget(m_usage);
    QStringList legend;
    for (BreakEvent e : {BreakEvent::Shown, BreakEvent::Dismissed, BreakEvent::Postponed, BreakEvent::Completed}) {
        const QString names[] = {tr("break shown"), tr("dismissed"), tr("postponed"), tr("completed")};
        legend << QStringLiteral("<span style=\"color:%1; font-weight:600\">%2</span> %3")
                      .arg(UsageChart::markerColor(e).name(), UsageChart::markerGlyph(e), names[int(e)]);
    }
    auto *legendLabel = new QLabel(legend.join(QStringLiteral("&nbsp;&nbsp;&nbsp;")), dayCard);
    legendLabel->setObjectName(QStringLiteral("hint"));
    legendLabel->setTextFormat(Qt::RichText);
    day->addWidget(legendLabel);

    // 2b. activity chart
    addSection(tr("Activity"), true);
    m_activity = new ActivityChart(dayCard);
    day->addWidget(m_activity);

    // 2c. day totals
    addSection(tr("Day totals"), true);
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
        auto *name = new QLabel(names[i], dayCard);
        name->setObjectName(QStringLiteral("hint"));
        auto *value = new QLabel(QStringLiteral("–"), dayCard);
        value->setObjectName(QStringLiteral("cardTitle"));
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        tile->addWidget(name);
        tile->addWidget(value);
        grid->addLayout(tile, i / 3, i % 3);
        m_values << value;
    }
    for (int col = 0; col < 3; ++col)
        grid->setColumnStretch(col, 1);
    day->addLayout(grid);
    layout->addWidget(dayCard);

    // --- 3. options
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
    // "Today – 18 November 2026" / "Tuesday – 10 November 2026" (1.8.1, owner request).
    const QString date = longDateWithoutWeekday(m_date, QLocale());
    m_title->setText(isToday ? tr("Today – %1").arg(date)
                             : tr("%1 – %2", "weekday – date").arg(weekdayName(m_date, QLocale()), date));

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
