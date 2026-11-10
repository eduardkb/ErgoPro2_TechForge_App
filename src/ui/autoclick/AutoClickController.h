#pragma once

#include <QObject>
#include <QPointF>
#include <QThread>

#include <memory>

namespace ergo {

class AutoClickOverlay;
class AutoClickWorker;
class FeatureAvailabilityService;
class InputService;
class SettingsStore;
struct Platform;

// GUI-thread side of AutoClick (B.4): owns the worker thread, applies the
// settings, decides whether AutoClick is effectively active (enabled AND
// available AND the input hook runs) and drives the optional visual feedback.
class AutoClickController : public QObject {
    Q_OBJECT
public:
    AutoClickController(SettingsStore &settings, FeatureAvailabilityService &availability, InputService &input,
                        Platform &platform, QObject *parent = nullptr);
    ~AutoClickController() override;

    void start();
    void shutdown();

    bool isEnabled() const;           // user setting
    bool isActive() const { return m_active; } // enabled and able to run
    bool isAvailable() const;
    bool visualFeedbackSupported() const;
    QString visualFeedbackUnsupportedReason() const;

    void setEnabled(bool enabled);
    void toggle() { setEnabled(!isEnabled()); }

    // Key Control click (stage 4): treated like a manual click.
    void notifyExternalClick(const QPointF &position);
    // Drag Lock (stage 6).
    void setDragLockActive(bool active, const QPointF &position);

    int lastState() const { return m_state; }
    int lastSuppressReason() const { return m_reason; }

signals:
    void activeChanged(bool active);
    void enabledChanged(bool enabled);
    void stateChanged(int state, int suppressReason);
    void clickFailed(const QString &message);
    // An automatic click was made (Statistics: "clicks done for you").
    void clickPerformed();

private:
    void applyConfig();
    void updateActive();

    SettingsStore &m_settings;
    FeatureAvailabilityService &m_availability;
    InputService &m_input;
    Platform &m_platform;
    QThread m_thread;
    AutoClickWorker *m_worker = nullptr; // lives in m_thread
    std::unique_ptr<AutoClickOverlay> m_overlay;
    bool m_active = false;
    bool m_started = false;
    int m_state = 0;
    int m_reason = 0;
};

} // namespace ergo
