#pragma once

#include <QApplication>

namespace ergo {

// QApplication that logs exceptions escaping from event handlers as CRITICAL
// and exits with an error instead of silently continuing in an unknown state.
class ErgoApplication : public QApplication {
    Q_OBJECT
public:
    ErgoApplication(int &argc, char **argv);

    bool notify(QObject *receiver, QEvent *event) override;
};

} // namespace ergo
