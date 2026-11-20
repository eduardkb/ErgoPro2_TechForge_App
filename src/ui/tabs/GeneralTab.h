#pragma once

#include <QWidget>

class QComboBox;

namespace ergo {

class LogProblemsCard;
class ParameterRow;
struct AppContext;

// "General" tab (B.1.3): application info, installation, startup behavior,
// logging (level, retention, open folders), permissions & diagnostics,
// reset to defaults and Exit.
class GeneralTab : public QWidget {
    Q_OBJECT
public:
    GeneralTab(AppContext &context, QWidget *parent = nullptr);

signals:
    void exitRequested();

protected:
    // The log problems card refreshes only while this tab is shown (2.2.0).
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    QWidget *buildApplicationCard(QWidget *parent);
    QWidget *buildStartupCard(QWidget *parent);
    QWidget *buildLoggingCard(QWidget *parent);
    QWidget *buildMaintenanceCard(QWidget *parent);
    void openFolder(const QString &path);
    void resetSettings();
    void syncFromSettings();

    AppContext &m_context;
    QComboBox *m_logLevel = nullptr;
    ParameterRow *m_retention = nullptr;
    QComboBox *m_startupAuto = nullptr;
    QComboBox *m_startupManual = nullptr;
    LogProblemsCard *m_logProblems = nullptr;
};

} // namespace ergo
