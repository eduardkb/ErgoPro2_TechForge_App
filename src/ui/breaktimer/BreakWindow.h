#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

namespace ergo {

// One full-screen, semi-transparent break overlay for one screen (B.7). The
// main screen shows the message, the mm:ss countdown, stretching tips and the
// Dismiss / Postpone buttons; other screens show a short note and the
// countdown. It cannot be closed with Esc / Alt+F4 / Cmd+W - only by the
// buttons or when the break is over (BreakWindowManager).
class BreakWindow : public QWidget {
    Q_OBJECT
public:
    BreakWindow(bool mainScreen, QWidget *parent = nullptr);

    void setRemaining(qint64 ms);
    void setPostponeMinutes(int minutes);
    void allowClose() { m_allowClose = true; }

signals:
    void dismissClicked();
    void postponeClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QWidget *buildMainCard();
    QWidget *buildTips(QWidget *parent);

    bool m_main = false;
    bool m_allowClose = false;
    QLabel *m_countdown = nullptr;
    QPushButton *m_postpone = nullptr;
};

} // namespace ergo
