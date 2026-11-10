#pragma once

#include "ui/widgets/CardWidget.h"

#include <QElapsedTimer>
#include <QTimer>

class QGridLayout;
class QLabel;
class QPushButton;
class QToolButton;

namespace ergo {

class InputService;
class StatusBadge;

// Collapsible "Input diagnostics" card of the General tab (stage 2): live view
// of the global input hook and manual tests for click injection and key
// blocking. Refreshes 5x per second while it is expanded and visible.
class InputDiagnosticsCard : public CardWidget {
    Q_OBJECT
public:
    explicit InputDiagnosticsCard(InputService &input, QWidget *parent = nullptr);

private:
    void setExpanded(bool expanded);
    void refresh();
    void startClickTest();
    void startSwallowTest();
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

    QPushButton *m_clickButton = nullptr;
    QLabel *m_clickResult = nullptr;
    QTimer m_clickCountdown;
    int m_clickSecondsLeft = 0;
    QElapsedTimer m_clickSent;
    bool m_waitingForSyntheticClick = false;

    QPushButton *m_swallowButton = nullptr;
    QLabel *m_swallowResult = nullptr;
    QTimer m_swallowCountdown;
    int m_swallowSecondsLeft = 0;
    quint64 m_swallowedAtStart = 0;
};

} // namespace ergo
