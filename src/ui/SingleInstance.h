#pragma once

#include <QLockFile>
#include <QObject>
#include <QString>

#include <memory>

class QLocalServer;

namespace ergo {

// Ensures only one ErgoPro2 instance runs per user.
//
// The first instance holds a QLockFile and listens on a QLocalServer. A second
// instance fails to take the lock, sends "activate" to the first one (which
// brings its window to the front) and exits.
class SingleInstance : public QObject {
    Q_OBJECT
public:
    enum class Result {
        Primary,        // this process is the only instance
        OtherActivated, // another instance is running and was asked to show itself
        Failed          // another instance holds the lock but did not respond
    };

    explicit SingleInstance(const QString &key, QObject *parent = nullptr);
    ~SingleInstance() override;

    Result acquire();

    // Releases the lock and stops listening, so that a new process (restart,
    // installed copy) can become the primary instance.
    void release();
    QString errorMessage() const { return m_error; }

signals:
    void activationRequested();

private:
    bool notifyPrimary();
    void startServer();

    QString m_serverName;
    std::unique_ptr<QLockFile> m_lock;
    QLocalServer *m_server = nullptr;
    QString m_error;
};

} // namespace ergo
