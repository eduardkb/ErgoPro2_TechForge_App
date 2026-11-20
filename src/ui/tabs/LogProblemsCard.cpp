#include "ui/tabs/LogProblemsCard.h"

#include "core/LogCategories.h"
#include "core/Logger.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsStore.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QHBoxLayout>
#include <QLocale>
#include <QLabel>
#include <QPlainTextEdit>
#include <QThread>
#include <QVBoxLayout>

#include <memory>

namespace ergo {

namespace {
constexpr int kTickMs = 1000;
constexpr int kListHeight = 220; // fixed height; more entries scroll (owner request)
} // namespace

LogProblemsCard::LogProblemsCard(SettingsStore &settings, Logger &logger, QWidget *parent)
    : CardWidget(tr("Errors in the log"), {}, parent)
    , m_settings(settings)
    , m_logger(logger)
{
    m_refreshing = new QLabel(tr("Refreshing..."), this);
    m_refreshing->setObjectName(QStringLiteral("warningText"));
    contentLayout()->addWidget(m_refreshing);

    // Filters (2.2.1): level boxes and the date list; default shows everything.
    m_filters = new QWidget(this);
    auto *filterRow = new QHBoxLayout(m_filters);
    filterRow->setContentsMargins(0, 0, 0, 0);
    filterRow->setSpacing(12);
    auto *show = new QLabel(tr("Show:"), m_filters);
    show->setObjectName(QStringLiteral("hint"));
    filterRow->addWidget(show);
    for (LogLevel level : {LogLevel::Error, LogLevel::Critical}) { // 2.2.2: no warnings
        auto *box = new QCheckBox(logLevelName(level), m_filters);
        box->setChecked(true);
        box->setCursor(Qt::PointingHandCursor);
        connect(box, &QCheckBox::toggled, this, &LogProblemsCard::render);
        filterRow->addWidget(box);
        m_levelBoxes.append({level, box});
    }
    filterRow->addStretch(1);
    auto *dateLabel = new QLabel(tr("Date:"), m_filters);
    dateLabel->setObjectName(QStringLiteral("hint"));
    filterRow->addWidget(dateLabel);
    m_date = new QComboBox(m_filters);
    m_date->setCursor(Qt::PointingHandCursor);
    connect(m_date, &QComboBox::currentIndexChanged, this, &LogProblemsCard::render);
    filterRow->addWidget(m_date);
    contentLayout()->addWidget(m_filters);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("hint"));
    m_summary->setWordWrap(true);
    contentLayout()->addWidget(m_summary);

    m_list = new QPlainTextEdit(this);
    m_list->setReadOnly(true);
    m_list->setFixedHeight(kListHeight);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_list->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    contentLayout()->addWidget(m_list);

    m_clock.start();
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &LogProblemsCard::check);
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == GeneralSettings::LogLevel || key == GeneralSettings::LogViewerDays
            || key == GeneralSettings::LogViewerRefreshMinutes) {
            applySettings();
            m_schedule.invalidate(); // search again with the new values
            check();
        }
    });
    applySettings();
    setVisible(false); // appears with the first search
}

LogProblemsCard::~LogProblemsCard()
{
    // A running search only reads files; wait briefly so it does not outlive the app.
    if (m_thread && m_thread->isRunning() && !m_thread->wait(3000))
        qCWarning(lcUi) << "Log check still running at exit";
}

void LogProblemsCard::applySettings()
{
    m_schedule.setIntervalMs(qint64(m_settings.intValue(GeneralSettings::LogViewerRefreshMinutes)) * 60000);
}

void LogProblemsCard::setTabShown(bool shown)
{
    if (shown == m_tabShown)
        return;
    m_tabShown = shown;
    if (shown) {
        m_tick.start();
        check(); // at once when due (first time, or the interval passed while hidden)
    } else {
        m_tick.stop(); // the card is not refreshed while the tab is not shown
    }
}

void LogProblemsCard::check()
{
    const auto selection = logLevelSelectionFromId(m_settings.stringValue(GeneralSettings::LogLevel));
    if (!selection || !logProblemsCardEnabled(*selection)) {
        if (isVisibleTo(parentWidget()) || m_hasData)
            qCDebug(lcUi) << "Log check: the log level writes no errors - card removed";
        clearAndHide();
        return;
    }
    if (m_schedule.searchDue(m_clock.elapsed(), m_tabShown, !m_thread.isNull()))
        startSearch();
}

void LogProblemsCard::startSearch()
{
    m_schedule.searchStarted(m_clock.elapsed());
    const QString directory = m_logger.directory();
    const int days = m_settings.intValue(GeneralSettings::LogViewerDays);
    const QDate today = QDate::currentDate();
    qCDebug(lcUi).noquote() << "Log check: searching" << directory << "for the last" << days << "day(s)";

    // Previous result stays visible below "Refreshing..." (or only "Refreshing..." the first time).
    m_refreshing->setVisible(true);
    m_filters->setVisible(m_hasData);
    m_summary->setVisible(m_hasData);
    m_list->setVisible(m_hasData);
    setVisible(true);

    auto result = std::make_shared<LogProblemScan>();
    QThread *thread = QThread::create([result, directory, today, days] {
        *result = scanLogProblems(directory, today, days, kMaxShownLogProblems);
    });
    thread->setObjectName(QStringLiteral("LogCheck"));
    m_thread = thread;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, result, days] { showResult(*result, days); });
    thread->start(QThread::LowPriority);
}

void LogProblemsCard::showResult(const LogProblemScan &scan, int days)
{
    m_thread.clear();
    const auto selection = logLevelSelectionFromId(m_settings.stringValue(GeneralSettings::LogLevel));
    if (!selection || !logProblemsCardEnabled(*selection)) {
        clearAndHide(); // the log level changed during the search
        return;
    }
    for (const QString &file : scan.unreadableFiles)
        qCWarning(lcUi).noquote() << "Log check: cannot read" << file;
    qCDebug(lcUi) << "Log check:" << scan.totalFound << "error entries in" << scan.filesSearched << "file(s)";
    if (scan.totalFound == 0) {
        clearAndHide(); // no warnings / errors: the whole card disappears
        return;
    }
    m_problems = scan.problems;
    m_totalFound = scan.totalFound;
    m_days = days;
    rebuildFilters();
    render();
    m_hasData = true;
    m_refreshing->setVisible(false);
    m_filters->setVisible(true);
    m_summary->setVisible(true);
    m_list->setVisible(true);
    setVisible(true);
}

// Level boxes only for the levels found; the date list keeps the chosen date
// while it still has entries, otherwise "All dates".
void LogProblemsCard::rebuildFilters()
{
    const QList<LogLevel> levels = logProblemLevels(m_problems);
    for (const auto &[level, box] : std::as_const(m_levelBoxes))
        box->setVisible(levels.contains(level));

    const QDate previous = m_date->currentData().toDate();
    const QSignalBlocker blocker(m_date);
    m_date->clear();
    m_date->addItem(tr("All dates"), QDate());
    const QLocale locale;
    for (const QDate &date : logProblemDates(m_problems))
        m_date->addItem(locale.toString(date, QLocale::ShortFormat), date);
    const int index = previous.isValid() ? m_date->findData(previous) : 0;
    m_date->setCurrentIndex(index < 0 ? 0 : index);
}

void LogProblemsCard::render()
{
    LogProblemFilter filter;
    for (const auto &[level, box] : std::as_const(m_levelBoxes)) {
        if (!box->isChecked())
            filter.hiddenLevels << level;
    }
    if (const QDate date = m_date->currentData().toDate(); date.isValid())
        filter.date = date;
    const QList<LogProblem> shown = filterLogProblems(m_problems, filter);

    QStringList lines;
    lines.reserve(shown.size());
    for (const LogProblem &p : shown)
        lines << p.text;
    m_list->setPlainText(lines.join(QLatin1Char('\n')));
    m_list->moveCursor(QTextCursor::Start); // newest entry on top

    QString summary = m_totalFound > m_problems.size()
                          ? tr("Showing the newest %1 of %2 entries from the last %3 days.")
                                .arg(m_problems.size())
                                .arg(m_totalFound)
                                .arg(m_days)
                      : m_totalFound == 1 ? tr("1 entry from the last %1 days.").arg(m_days)
                                          : tr("%1 entries from the last %2 days, newest first.")
                                                .arg(m_totalFound)
                                                .arg(m_days);
    if (shown.size() != m_problems.size())
        summary += QLatin1Char(' ') + tr("Filter: %1 shown.").arg(shown.size());
    m_summary->setText(summary);
    qCDebug(lcUi) << "Log check: filter shows" << shown.size() << "of" << m_problems.size() << "entries";
}

void LogProblemsCard::clearAndHide()
{
    m_hasData = false;
    m_problems.clear();
    m_totalFound = 0;
    m_list->clear();
    m_summary->clear();
    m_refreshing->setVisible(false);
    setVisible(false);
}

} // namespace ergo
