#pragma once

#include "core/Clock.h"

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QtGlobal>

namespace ergo {

// Keep Screen Active (B.10, stage 9): decides WHEN a keep-active cycle is due.
// Pure: driven by real-input notifications, pause conditions, the enabled flag
// and the injectable clock (wall clock, so sleep counts as idle time). The
// caller calls update() about once per second and runs a cycle whenever it
// returns true; WHAT a cycle does (Teams / Zoom corner click, harmless input) is
// done by the UI controller with the platform layer.
//
// Rules:
//  - Disabled: never a cycle.
//  - Enabled and idle (no real input) for startAfterMs, no pause condition:
//    state Working; the first cycle is due at once, then every intervalMs.
//  - Any real input ends Working (state Idle) and restarts the idle time.
//  - A pause condition (screen locked / sleep, break window, drag lock) makes
//    the state Idle; when it ends and the user is still idle, Working starts
//    again with an immediate cycle.
class KeepScreenActiveEngine {
public:
    struct Config {
        qint64 startAfterMs = 3 * 60000;
        qint64 intervalMs = 3 * 60000;
    };

    enum class State { Disabled, Idle, Working };

    enum PauseReason : quint8 { None = 0, Locked = 1 << 0, BreakWindow = 1 << 1, DragLock = 1 << 2 };

    explicit KeepScreenActiveEngine(const IClock &clock);

    void setConfig(const Config &config);
    const Config &config() const { return m_config; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Real user input only (the caller filters synthetic events).
    void onRealInput();

    void setPaused(PauseReason reason, bool paused);
    quint8 pauseReasons() const { return m_pause; }

    // Evaluates the rules; returns true when a cycle must run now.
    bool update();

    State state() const;
    qint64 idleMs() const;
    int cycleCount() const { return m_cycles; } // cycles since the last real input

private:
    qint64 now() const { return m_clock.now().toMSecsSinceEpoch(); }

    const IClock &m_clock;
    Config m_config;
    bool m_enabled = false;
    bool m_working = false;
    quint8 m_pause = None;
    qint64 m_lastInputMs = 0;
    qint64 m_nextCycleMs = 0;
    int m_cycles = 0;
};

// Which application window a cycle targets.
enum class KeepActiveApp { Teams, Zoom };

// Point of an app window that is clicked (logical pixels): exactly its
// top-left corner, the resize corner (1.9.1). Kept here so the position can be
// adjusted in one place. Returns false when there is no visible frame.
bool keepActiveClickPoint(const QRectF &frame, QPointF *point);

// True when the Key Control hotkey for this feature became usable: it was
// added (or another binding took its place) or its binding was turned back on
// in the hotkey table. The feature is then switched on (1.9.1, owner request).
bool keepActiveHotkeyBecameActive(const QString &oldId, bool oldEnabled, const QString &newId, bool newEnabled);

} // namespace ergo
