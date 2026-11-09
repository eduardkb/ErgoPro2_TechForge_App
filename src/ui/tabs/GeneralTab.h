#pragma once

#include <QWidget>

class QComboBox;
class QLabel;

namespace ergo {

class ParameterRow;
class SettingsStore;

// "General" tab. Stage 0 content: application version, logging options and
// the Exit button. Stage 1 adds install/uninstall, startup behavior, folders,
// reset to defaults and permissions/diagnostics.
class GeneralTab : public QWidget {
    Q_OBJECT
public:
    GeneralTab(SettingsStore &settings, const QString &logDirectory, QWidget *parent = nullptr);

signals:
    void exitRequested();

private:
    void syncFromSettings();

    SettingsStore &m_settings;
    QComboBox *m_logLevel = nullptr;
    ParameterRow *m_retention = nullptr;
};

} // namespace ergo
