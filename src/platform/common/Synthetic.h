#pragma once

#include <QtGlobal>

#include <atomic>

namespace ergo {

// Marker attached to every event ErgoPro2 injects (B.6):
//   macOS   - kCGEventSourceUserData
//   Windows - dwExtraInfo of SendInput
// The value spells "ERG2".
inline constexpr qint64 kSyntheticEventMarker = 0x45524732;

// X11 / uinput cannot tag events. The injector announces how many button
// events it is about to inject; the hook marks that many matching events that
// arrive within kWindowMs as synthetic.
class SyntheticTracker {
public:
    static constexpr qint64 kWindowMs = 500;

    void expect(int count, qint64 nowMs)
    {
        m_pending.fetch_add(count);
        m_deadlineMs.store(nowMs + kWindowMs);
    }

    // Returns true (and consumes one) if a synthetic event is expected now.
    bool consume(qint64 nowMs)
    {
        if (m_pending.load() <= 0)
            return false;
        if (nowMs > m_deadlineMs.load()) {
            m_pending.store(0); // expired: events did not arrive
            return false;
        }
        return m_pending.fetch_sub(1) > 0;
    }

private:
    std::atomic<int> m_pending{0};
    std::atomic<qint64> m_deadlineMs{0};
};

} // namespace ergo
