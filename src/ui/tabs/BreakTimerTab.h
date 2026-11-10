#pragma once

#include <QWidget>

class QLabel;

namespace ergo {

class BreakTimerController;
class ParameterRow;
class SettingsStore;
class StatusBadge;
class ToggleSwitch;

// "Break Timer" tab content (B.7): enable switch + status badge, parameters,
// and a live status panel (work time, time until the next break, state)
// updated every second. No "break now" / "reset" buttons (B-04).
class BreakTimerTab : public QWidget {
    Q_OBJECT
public:
    BreakTimerTab(BreakTimerController &controller, SettingsStore &settings, QWidget *parent = nullptr);

private:
    void refresh();
    void refreshStatus();

    BreakTimerController &m_controller;
    SettingsStore &m_settings;
    ToggleSwitch *m_enabled = nullptr;
    StatusBadge *m_badge = nullptr;
    ParameterRow *m_idle = nullptr;
    ParameterRow *m_work = nullptr;
    ParameterRow *m_break = nullptr;
    ParameterRow *m_postpone = nullptr;
    ToggleSwitch *m_fullscreen = nullptr;
    QLabel *m_fullscreenHint = nullptr;
    QLabel *m_workTime = nullptr;
    QLabel *m_untilBreak = nullptr;
    QLabel *m_state = nullptr;
};

} // namespace ergo
