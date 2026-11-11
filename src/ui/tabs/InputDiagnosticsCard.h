#pragma once

#include "ui/widgets/CardWidget.h"

#include <QTimer>

class QGridLayout;
class QLabel;
class QToolButton;

namespace ergo {

class InputService;
class StatusBadge;

// Collapsible "Input diagnostics" card of the General tab (stage 2): live view
// of the global input hook (the manual click / F9 test buttons were removed in
// 1.7.5). Refreshes 5x per second while it is expanded and visible.
class InputDiagnosticsCard : public CardWidget {
    Q_OBJECT
public:
    explicit InputDiagnosticsCard(InputService &input, QWidget *parent = nullptr);

private:
    void setExpanded(bool expanded);
    void refresh();
    QLabel *addRow(QGridLayout *grid, int row, const QString &name);

    InputService &m_input;
    QToolButton *m_toggle = nullptr;
    QWidget *m_body = nullptr;
    QTimer m_refresh;

    StatusBadge *m_hookBadge = nullptr;
    QLabel *m_backend = nullptr;
    QLabel *m_pointer = nullptr;
    QLabel *m_lastKey = nullptr;
    QLabel *m_lastButton = nullptr;
    QLabel *m_synthetic = nullptr;
    QLabel *m_rate = nullptr;
    QLabel *m_watchdog = nullptr;
};

} // namespace ergo
