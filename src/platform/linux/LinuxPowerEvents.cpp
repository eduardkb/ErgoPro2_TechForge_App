// Linux sleep/resume and lock/unlock notifications over D-Bus:
//   - logind (system bus): Manager.PrepareForSleep(bool), Session.Lock / Unlock
//   - desktop screensaver (session bus): org.gnome.ScreenSaver and
//     org.freedesktop.ScreenSaver ActiveChanged(bool)
// Compiled with QtDBus when available (ERGOPRO2_HAVE_QTDBUS).

#include "platform/linux/LinuxInput.h"

#include "core/LogCategories.h"

#include <QCoreApplication>

#ifdef ERGOPRO2_HAVE_QTDBUS
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QObject>

#include <unistd.h>
#endif

namespace ergo {

#ifdef ERGOPRO2_HAVE_QTDBUS

namespace {

// Receives the D-Bus signals (QtDBus delivers signals to QObject slots).
class PowerSignalReceiver : public QObject {
    Q_OBJECT
public:
    explicit PowerSignalReceiver(std::function<void(PowerEvent)> callback)
        : m_callback(std::move(callback))
    {
    }

public slots:
    void prepareForSleep(bool sleeping) { m_callback(sleeping ? PowerEvent::Sleep : PowerEvent::Resume); }
    void locked() { m_callback(PowerEvent::ScreenLocked); }
    void unlocked() { m_callback(PowerEvent::ScreenUnlocked); }
    void screenSaverActive(bool active)
    {
        m_callback(active ? PowerEvent::ScreenLocked : PowerEvent::ScreenUnlocked);
    }

private:
    std::function<void(PowerEvent)> m_callback;
};

class LinuxPowerEvents final : public IPowerEvents {
public:
    ~LinuxPowerEvents() override { stop(); }

    PlatformResult start(std::function<void(PowerEvent)> callback) override
    {
        if (m_receiver)
            return PlatformResult::success();
        m_receiver = std::make_unique<PowerSignalReceiver>(std::move(callback));
        const QString login1 = QStringLiteral("org.freedesktop.login1");
        QDBusConnection system = QDBusConnection::systemBus();
        QDBusConnection session = QDBusConnection::sessionBus();

        bool any = false;
        if (system.isConnected()) {
            any |= system.connect(login1, QStringLiteral("/org/freedesktop/login1"),
                                  QStringLiteral("org.freedesktop.login1.Manager"), QStringLiteral("PrepareForSleep"),
                                  m_receiver.get(), SLOT(prepareForSleep(bool)));
            // This process's login session object.
            QDBusInterface manager(login1, QStringLiteral("/org/freedesktop/login1"),
                                   QStringLiteral("org.freedesktop.login1.Manager"), system);
            const QDBusReply<QDBusObjectPath> path = manager.call(QStringLiteral("GetSessionByPID"), uint(::getpid()));
            if (path.isValid()) {
                const QString sessionPath = path.value().path();
                const QString iface = QStringLiteral("org.freedesktop.login1.Session");
                any |= system.connect(login1, sessionPath, iface, QStringLiteral("Lock"), m_receiver.get(), SLOT(locked()));
                any |= system.connect(login1, sessionPath, iface, QStringLiteral("Unlock"), m_receiver.get(),
                                      SLOT(unlocked()));
            } else {
                qCInfo(lcPlatform).noquote() << "logind session not found:" << path.error().message();
            }
        }
        if (session.isConnected()) {
            any |= session.connect(QStringLiteral("org.gnome.ScreenSaver"), QStringLiteral("/org/gnome/ScreenSaver"),
                                   QStringLiteral("org.gnome.ScreenSaver"), QStringLiteral("ActiveChanged"),
                                   m_receiver.get(), SLOT(screenSaverActive(bool)));
            any |= session.connect(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"),
                                   QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("ActiveChanged"),
                                   m_receiver.get(), SLOT(screenSaverActive(bool)));
        }
        if (!any) {
            m_receiver.reset();
            return PlatformResult::notSupported(QCoreApplication::translate(
                "ergo::LinuxPowerEvents", "No D-Bus service for sleep/lock notifications is available."));
        }
        qCInfo(lcPlatform) << "Listening for sleep/resume and lock notifications over D-Bus";
        return PlatformResult::success();
    }

    void stop() override { m_receiver.reset(); }

private:
    std::unique_ptr<PowerSignalReceiver> m_receiver;
};

} // namespace

std::unique_ptr<IPowerEvents> createLinuxPowerEvents()
{
    return std::make_unique<LinuxPowerEvents>();
}

#else // no QtDBus

namespace {
class NoPowerEvents final : public IPowerEvents {
public:
    PlatformResult start(std::function<void(PowerEvent)>) override
    {
        return PlatformResult::notSupported(QCoreApplication::translate(
            "ergo::LinuxPowerEvents", "ErgoPro2 was built without QtDBus: no sleep/lock notifications."));
    }
    void stop() override {}
};
} // namespace

std::unique_ptr<IPowerEvents> createLinuxPowerEvents()
{
    return std::make_unique<NoPowerEvents>();
}

#endif

} // namespace ergo

#ifdef ERGOPRO2_HAVE_QTDBUS
#include "LinuxPowerEvents.moc"
#endif
