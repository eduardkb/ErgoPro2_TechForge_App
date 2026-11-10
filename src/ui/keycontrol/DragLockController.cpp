#include "ui/keycontrol/DragLockController.h"

#include "core/LogCategories.h"
#include "core/settings/KeyControlSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformFactory.h"
#include "platform/common/EmergencyRelease.h"
#include "ui/InputService.h"
#include "ui/autoclick/AutoClickController.h"

#include <QCoreApplication>

namespace ergo {

namespace {

constexpr int kPollIntervalMs = 100;

OpResult toOp(const PlatformResult &r)
{
    return r.ok() ? OpResult::success() : OpResult::failure(r.message);
}

// Crash context: only post the button-up (see EmergencyRelease.h).
void emergencyRelease(void *context)
{
    static_cast<IClickInjector *>(context)->release(MouseButton::Left);
}

} // namespace

// The system side of the drag lock: click injector + input layer + crash safety.
class DragLockController::Device final : public IDragLockDevice {
public:
    Device(Platform &platform, InputService &input) : m_platform(platform), m_input(input) {}

    OpResult pressLeft() override { return toOp(m_platform.clickInjector->press(MouseButton::Left)); }
    OpResult releaseLeft() override { return toOp(m_platform.clickInjector->release(MouseButton::Left)); }
    void setHolding(bool holding) override
    {
        m_input.setDragLockHolding(holding);
        if (holding)
            armEmergencyRelease(&emergencyRelease, m_platform.clickInjector.get());
        else
            disarmEmergencyRelease();
    }

private:
    Platform &m_platform;
    InputService &m_input;
};

DragLockController::DragLockController(SettingsStore &settings, InputService &input, AutoClickController &autoclick,
                                       Platform &platform, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_input(input)
    , m_autoclick(autoclick)
    , m_platform(platform)
    , m_device(std::make_unique<Device>(platform, input))
    , m_engine(m_clock, *m_device)
{
    m_engine.setHoldingCallback([this](bool holding, const QPointF &position) { onHoldingChanged(holding, position); });
    m_timer.setInterval(kPollIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, [this] { m_engine.poll(); });
}

DragLockController::~DragLockController()
{
    shutdown();
}

void DragLockController::start()
{
    if (m_started)
        return;
    m_started = true;
    installEmergencyHandlers(); // never leave the button down on a crash or kill

    // Events arrive on the GUI thread in hook order.
    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this,
            [this](const InputEvent &event) { m_engine.onInputEvent(event); });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == KeyControlSettings::DragLockTimeoutS)
            applyTimeout();
    });
    // Hook reinstall failed / hook stopped: the releasing click could no longer
    // be seen, so do not keep holding.
    connect(&m_input, &InputService::statusChanged, this, [this] {
        if (!m_input.stats().running)
            release(Reason::InputLost);
    });
    connect(&m_autoclick, &AutoClickController::enabledChanged, this, [this](bool enabled) {
        if (!enabled)
            release(Reason::AutoClickOff);
    });
    // Covers every way out of the event loop (Exit, logout, fatal Qt error).
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { release(Reason::AppExit); });
    applyTimeout();
}

void DragLockController::shutdown()
{
    release(Reason::AppExit);
    m_timer.stop();
}

void DragLockController::applyTimeout()
{
    const int seconds = m_settings.intValue(KeyControlSettings::DragLockTimeoutS);
    m_engine.setTimeoutMs(seconds * 1000);
    qCInfo(lcApp) << "Drag lock timeout:" << seconds << "s without mouse movement";
}

OpResult DragLockController::toggle()
{
    const OpResult r = m_engine.toggle(m_input.stats().pointer);
    if (!r.ok) {
        qCWarning(lcApp).noquote() << "Drag lock: the left button could not be pressed:" << r.message;
        emit failed(r.message);
    }
    return r;
}

bool DragLockController::release(Reason reason)
{
    return m_engine.release(reason);
}

void DragLockController::onHoldingChanged(bool holding, const QPointF &position)
{
    // AutoClick: no click while holding, and after the release the pointer must
    // travel AUTOCLICK_TRIGGER_DISTANCE from the release position (B.4 rule 7, D-04).
    m_autoclick.setDragLockActive(holding, position);
    if (holding) {
        m_timer.start();
        qCInfo(lcApp).noquote() << QStringLiteral("Drag lock: left button held at (%1, %2)")
                                       .arg(position.x())
                                       .arg(position.y());
        if (!m_input.canSuppressMouseClicks())
            qCInfo(lcApp) << "Drag lock: this input backend cannot swallow clicks; the releasing click also "
                             "reaches the application";
    } else {
        m_timer.stop();
        const OpResult result = m_engine.lastReleaseResult();
        if (m_engine.lastReleaseReason() == Reason::Error) {
            qCInfo(lcApp) << "Drag lock: not started";
        } else if (result.ok) {
            qCInfo(lcApp).noquote() << QStringLiteral("Drag lock: left button released at (%1, %2) - %3")
                                           .arg(position.x())
                                           .arg(position.y())
                                           .arg(DragLockEngine::reasonText(m_engine.lastReleaseReason()));
        } else {
            qCCritical(lcApp).noquote() << "Drag lock: the left button could not be released ("
                                        << DragLockEngine::reasonText(m_engine.lastReleaseReason())
                                        << "):" << result.message;
            emit failed(tr("The left mouse button could not be released: %1 Click the left mouse button once "
                           "to release it.")
                            .arg(result.message));
        }
    }
    emit holdingChanged(holding);
}

} // namespace ergo
