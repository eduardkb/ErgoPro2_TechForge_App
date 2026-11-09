#include "ui/ErgoApplication.h"

#include "core/CrashHandler.h"

#include <exception>

namespace ergo {

ErgoApplication::ErgoApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
}

bool ErgoApplication::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (...) {
        logUncaughtException("Qt event handler", std::current_exception());
        // Qt does not support exceptions crossing the event loop; leave cleanly.
        QCoreApplication::exit(EXIT_FAILURE);
        return false;
    }
}

} // namespace ergo
