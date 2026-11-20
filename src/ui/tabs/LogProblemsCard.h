#pragma once

#include "core/LogProblems.h"
#include "ui/widgets/CardWidget.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>

class QLabel;
class QPlainTextEdit;
class QThread;

namespace ergo {

class Logger;
class SettingsStore;

// General tab card "Warnings and errors in the log" (2.2.0, owner request,
// spec B.3): lists the WARNING / ERROR / CRITICAL entries of the last
// general.logViewerDays days, newest first. It searches in a worker thread,
// only while the General tab is shown, at most every
// general.logViewerRefreshMinutes minutes. While searching it shows
// "Refreshing..." above the previous result; a search without entries removes
// the card. Not shown when the log level writes no warnings/errors.
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

    SettingsStore &m_settings;
    Logger &m_logger;
    QLabel *m_refreshing = nullptr;
    QLabel *m_summary = nullptr;
    QPlainTextEdit *m_list = nullptr;
    QTimer m_tick;
    QElapsedTimer m_clock;
    LogProblemSchedule m_schedule;
    QPointer<QThread> m_thread;
    bool m_tabShown = false;
    bool m_hasData = false;
};

} // namespace ergo
