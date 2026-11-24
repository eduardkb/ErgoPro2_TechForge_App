#pragma once

#include <QWidget>

class QLabel;

namespace ergo {

class MicroBreakController;
class ParameterRow;
class SettingsStore;
class StatusBadge;
class ToggleSwitch;

// "Micro-breaks" tab (2.5.0, owner request, spec B.7a): on/off switch with a
// status badge on top, live status, and the two values (every N minutes of
// active use, shown for N seconds).
class MicroBreaksTab : public QWidget {
    Q_OBJECT
public:
    MicroBreaksTab(MicroBreakController &controller, SettingsStore &settings, QWidget *parent = nullptr);

private:
    void refresh();
    void refreshStatus();

    MicroBreakController &m_controller;
    SettingsStore &m_settings;
    ToggleSwitch *m_enabled = nullptr;
    StatusBadge *m_badge = nullptr;
    ParameterRow *m_interval = nullptr;
    ParameterRow *m_show = nullptr;
    QLabel *m_active = nullptr;
    QLabel *m_until = nullptr;
    QLabel *m_state = nullptr;
};

} // namespace ergo
