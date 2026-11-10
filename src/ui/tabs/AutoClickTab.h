#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace ergo {

class AutoClickController;
class FeatureAvailabilityService;
class ParameterRow;
class SettingsStore;
class StatusBadge;
class ToggleSwitch;

// "Auto Click" tab content (B.4). The availability banner is added around it
// by FeaturePage.
class AutoClickTab : public QWidget {
    Q_OBJECT
public:
    AutoClickTab(AutoClickController &controller, SettingsStore &settings, FeatureAvailabilityService &availability,
                 QWidget *parent = nullptr);

private:
    void refresh();
    void showState(int state, int reason);

    AutoClickController &m_controller;
    SettingsStore &m_settings;
    FeatureAvailabilityService &m_availability;
    StatusBadge *m_badge = nullptr;
    QPushButton *m_toggle = nullptr;
    QLabel *m_stateLine = nullptr;
    QLabel *m_lastError = nullptr;
    ParameterRow *m_timer = nullptr;
    ParameterRow *m_distance = nullptr;
    ParameterRow *m_jitter = nullptr;
    ToggleSwitch *m_feedback = nullptr;
    QLabel *m_feedbackHint = nullptr;
};

} // namespace ergo
