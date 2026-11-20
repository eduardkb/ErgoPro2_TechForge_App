#include "ui/tabs/LogProblemsCard.h"

#include "core/LogCategories.h"
#include "core/Logger.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsStore.h"

#include <QDate>
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
    : CardWidget(tr("Warnings and errors in the log"), {}, parent)
    , m_settings(settings)
    , m_logger(logger)
{
    m_refreshing = new QLabel(tr("Refreshing..."), this);
    m_refreshing->setObjectName(QStringLiteral("warningText"));
    contentLayout()->addWidget(m_refreshing);

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
            qCDebug(lcUi) << "Log check: the log level writes no warnings or errors - card removed";
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
    qCDebug(lcUi) << "Log check:" << scan.totalFound << "warning/error entries in" << scan.filesSearched << "file(s)";
    if (scan.totalFound == 0) {
        clearAndHide(); // no warnings / errors: the whole card disappears
        return;
    }
    QStringList lines;
    lines.reserve(scan.problems.size());
    for (const LogProblem &p : scan.problems)
        lines << p.text;
    m_list->setPlainText(lines.join(QLatin1Char('\n')));
    m_list->moveCursor(QTextCursor::Start); // newest entry on top
    m_summary->setText(scan.totalFound > scan.problems.size()
                           ? tr("Showing the newest %1 of %2 entries from the last %3 days.")
                                 .arg(scan.problems.size())
                                 .arg(scan.totalFound)
                                 .arg(days)
                       : scan.totalFound == 1 ? tr("1 entry from the last %1 days.").arg(days)
                                              : tr("%1 entries from the last %2 days, newest first.")
                                                    .arg(scan.totalFound)
                                                    .arg(days));
    m_hasData = true;
    m_refreshing->setVisible(false);
    m_summary->setVisible(true);
    m_list->setVisible(true);
    setVisible(true);
}

void LogProblemsCard::clearAndHide()
{
    m_hasData = false;
    m_list->clear();
    m_summary->clear();
    m_refreshing->setVisible(false);
    setVisible(false);
}

} // namespace ergo
