#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

namespace ergo {

class InputService;
class KeyControlController;
class ParameterRow;
class SettingsStore;

// "Key Control" tab content (B.5.2): Add Hotkey and the list of hotkeys with
// Enable/Disable, Modify, Reset binding and Delete, and the Drag Lock timeout
// (B.5.3). The availability banner is
// added around it by FeaturePage.
class KeyControlTab : public QWidget {
    Q_OBJECT
public:
    KeyControlTab(KeyControlController &controller, InputService &input, SettingsStore &settings,
                  QWidget *parent = nullptr);

private:
    void rebuild();
    void openDialog(const QString &bindingId);
    void confirmDelete(const QString &bindingId);
    void reportFailure(const QString &title, const QString &message);

    KeyControlController &m_controller;
    InputService &m_input;
    SettingsStore &m_settings;
    ParameterRow *m_dragTimeout = nullptr;
    QPushButton *m_add = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_empty = nullptr;
    QLabel *m_allAssigned = nullptr;
    QLabel *m_lastError = nullptr;
};

} // namespace ergo
