#pragma once

#include <QDialog>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace ergo {

class FeatureAvailabilityService;

// macOS startup window (REVIEW 1.2, B.1.4): explains each missing permission
// and why ErgoPro2 needs it, with buttons to open System Settings, "Check
// again" and "Restart ErgoPro2" (after the user changed a setting). Updates
// live and closes itself when everything is granted.
class PermissionsDialog : public QDialog {
    Q_OBJECT
public:
    explicit PermissionsDialog(FeatureAvailabilityService &service, QWidget *parent = nullptr);

signals:
    void restartRequested();

private:
    void rebuild();

    FeatureAvailabilityService &m_service;
    QVBoxLayout *m_rows = nullptr;
    QLabel *m_summary = nullptr;
    QPushButton *m_restart = nullptr;
    bool m_settingsOpened = false;
    bool m_closing = false;
};

} // namespace ergo
