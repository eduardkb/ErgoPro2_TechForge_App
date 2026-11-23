#pragma once

#include <QWidget>

class QLabel;

namespace ergo {

// Small, non-blocking reminder before a break (2.3.0, owner request, spec B.7):
// "Break in 0:58" with a countdown, in the screen corner next to the tray. It
// stays on top, never takes the keyboard focus and is dismissed by a click.
class BreakReminderWindow : public QWidget {
    Q_OBJECT
public:
    explicit BreakReminderWindow(QWidget *parent = nullptr);

    // Shows / updates the countdown. trayIcon: tray icon geometry if known.
    void showReminder(qint64 remainingMs, bool paused, const QRect &trayIcon);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *m_title = nullptr;
    QLabel *m_hint = nullptr;
};

} // namespace ergo
