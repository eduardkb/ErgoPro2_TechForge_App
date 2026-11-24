#pragma once

#include <QWidget>

class QLabel;

namespace ergo {

// Micro-break suggestion (2.5.0, owner request, spec B.7a): a small window in
// the screen corner next to the tray, always on top, never takes the keyboard
// focus. Shows a countdown in its bottom-right corner until it closes by itself;
// a click dismisses it.
class MicroBreakWindow : public QWidget {
    Q_OBJECT
public:
    explicit MicroBreakWindow(QWidget *parent = nullptr);

    // Shows / updates the window. trayIcon: tray icon geometry if known.
    void showMicroBreak(qint64 remainingMs, int lengthSeconds, const QRect &trayIcon);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *m_hint = nullptr;
    QLabel *m_countdown = nullptr;
};

} // namespace ergo
