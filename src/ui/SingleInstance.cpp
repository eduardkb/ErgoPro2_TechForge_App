#include "ui/SingleInstance.h"

#include "core/LogCategories.h"

#include <QCryptographicHash>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QThread>

namespace ergo {

namespace {

constexpr char kActivateMessage[] = "activate\n";

// Per-user identifier so different users on the same machine do not collide.
QString userScopedName(const QString &key)
{
    const QByteArray hash = QCryptographicHash::hash(QDir::homePath().toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("%1-%2").arg(key, QString::fromLatin1(hash.left(12)));
}

} // namespace

SingleInstance::SingleInstance(const QString &key, QObject *parent)
    : QObject(parent)
    , m_serverName(userScopedName(key))
{
    const QString lockPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                 .filePath(m_serverName + QStringLiteral(".lock"));
    m_lock = std::make_unique<QLockFile>(lockPath);
    // Never treat a lock as stale because of its age; QLockFile still detects
    // locks left by crashed processes (the PID no longer exists).
    m_lock->setStaleLockTime(0);
}

SingleInstance::~SingleInstance()
{
    if (m_server)
        m_server->close();
    if (m_lock && m_lock->isLocked())
        m_lock->unlock();
}

SingleInstance::Result SingleInstance::acquire()
{
    if (m_lock->tryLock(200)) {
        startServer();
        return Result::Primary;
    }

    if (m_lock->error() != QLockFile::LockFailedError) {
        // Lock file unusable (permissions, unknown error): continue as primary
        // rather than refusing to start, but report it.
        qCWarning(lcApp) << "Single-instance lock file unusable (error" << m_lock->error()
                         << "); continuing without single-instance protection";
        startServer();
        return Result::Primary;
    }

    qCInfo(lcApp) << "Another ErgoPro2 instance is running; asking it to show its window";
    // The other instance may still be starting: retry for a moment.
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (notifyPrimary())
            return Result::OtherActivated;
        QThread::msleep(300);
    }

    m_error = tr("ErgoPro2 is already running but does not respond. If this persists, end the other "
                 "ErgoPro2 process and start the application again.");
    qCCritical(lcApp).noquote() << m_error;
    return Result::Failed;
}

void SingleInstance::release()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    if (m_lock && m_lock->isLocked())
        m_lock->unlock();
    qCInfo(lcApp) << "Single-instance lock released";
}

bool SingleInstance::notifyPrimary()
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(500)) {
        qCDebug(lcApp) << "Cannot connect to the running instance:" << socket.errorString();
        return false;
    }
    socket.write(kActivateMessage);
    if (!socket.waitForBytesWritten(1000)) {
        qCWarning(lcApp) << "Cannot send activation to the running instance:" << socket.errorString();
        return false;
    }
    socket.disconnectFromServer();
    return true;
}

void SingleInstance::startServer()
{
    // Remove a socket file left behind by a crashed instance (Unix).
    QLocalServer::removeServer(m_serverName);

    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(m_serverName)) {
        qCCritical(lcApp) << "Single-instance server cannot listen on" << m_serverName << ":"
                          << m_server->errorString() << "- a second launch will not activate this window";
        return;
    }

    connect(m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *client = m_server->nextPendingConnection()) {
            connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            connect(client, &QLocalSocket::readyRead, this, [this, client] {
                const QByteArray message = client->readAll();
                if (message.startsWith("activate")) {
                    qCInfo(lcApp) << "Activation requested by a second launch";
                    emit activationRequested();
                } else {
                    qCWarning(lcApp) << "Unexpected single-instance message:" << message;
                }
            });
        }
    });
}

} // namespace ergo
