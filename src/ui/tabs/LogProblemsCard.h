#pragma once

#include "core/LogProblems.h"
#include "ui/widgets/CardWidget.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QThread;

namespace ergo {

class Logger;
class SettingsStore;

// General tab card "Errors in the log" (2.2.0, owner request, spec B.3;
// warnings removed in 2.2.2): lists the ERROR / CRITICAL entries of the last
// general.logViewerDays days, newest first. It searches in a worker thread,
// only while the General tab is shown, at most every
// general.logViewerRefreshMinutes minutes. While searching it shows
// "Refreshing..." above the previous result; a search without entries removes
// the card. Not shown when the log level writes no errors.
// Filters (2.2.1): a check box per level found (all ticked by default) and a
// date list ("All dates" + each date with entries, newest first).
class LogProblemsCard : public CardWidget {
    Q_OBJECT
public:
    LogProblemsCard(SettingsStore &settings, Logger &logger, QWidget *parent = nullptr);
    ~LogProblemsCard() override;

    // Called by the General tab from its show/hide events.
    void setTabShown(bool shown);

private:
    void check();
    void startSearch();
    void showResult(const LogProblemScan &scan, int days);
    void clearAndHide();
    void applySettings();
    void rebuildFilters();
    void render();

    SettingsStore &m_settings;
    Logger &m_logger;
    QLabel *m_refreshing = nullptr;
    QLabel *m_summary = nullptr;
    QPlainTextEdit *m_list = nullptr;
    // 2.2.1 (owner request): filters above the list.
    QWidget *m_filters = nullptr;
    QList<QPair<LogLevel, QCheckBox *>> m_levelBoxes;
    QComboBox *m_date = nullptr;
    QList<LogProblem> m_problems; // last result (newest first, at most kMaxShownLogProblems)
    int m_totalFound = 0;
    int m_days = 0;
    QTimer m_tick;
    QElapsedTimer m_clock;
    LogProblemSchedule m_schedule;
    QPointer<QThread> m_thread;
    bool m_tabShown = false;
    bool m_hasData = false;
};

} // namespace ergo
