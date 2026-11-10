#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

namespace ergo {

class InputService;
class KeyControlController;

// "Key Control" tab content (B.5.2): Add Hotkey and the list of hotkeys with
// Enable/Disable, Modify, Reset binding and Delete. The page scrolls; the
// table always has the height of all its rows (1.6.1). The availability banner is
// added around it by FeaturePage.
class KeyControlTab : public QWidget {
    Q_OBJECT
public:
    KeyControlTab(KeyControlController &controller, InputService &input, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void rebuild();
    void fitTableHeight();
    void openDialog(const QString &bindingId);
    void confirmDelete(const QString &bindingId);
    void reportFailure(const QString &title, const QString &message);

    KeyControlController &m_controller;
    InputService &m_input;
    QPushButton *m_add = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_empty = nullptr;
    QLabel *m_allAssigned = nullptr;
    QLabel *m_lastError = nullptr;
};

} // namespace ergo
