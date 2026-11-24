#pragma once

#include "core/Clock.h"

#include <QtGlobal>

namespace ergo {

// Micro-breaks (2.5.0, owner request; spec B.7a): a short pause suggested every
// few minutes of active use, shown in a small non-blocking window. Pure logic,
// driven by real input, lock/sleep, the Break Timer and the injectable clock
// (tests/tst_microbreak.cpp). The caller calls update() every second.
//
// Rules (owner decisions 2026-11-24):
//  - Active use counts like the Break Timer work time: from the first input,
//    while input continues; a pause counts only up to pauseAfterMs.
//  - No input for showMs (the micro-break length) = the micro-break was taken:
//    the count restarts with the next input.
//  - Active use >= intervalMs -> Shown for showMs, then the count restarts with
//    the next input. dismiss() (click on the window) closes it at once. Input
//    during a shown micro-break changes nothing.
//  - Gives way to the Break Timer: while blocked (break window or pre-break
//    reminder shown) no micro-break starts and a shown one closes; restart()
//    (Break Timer cycle reset) restarts the count (a shown one stays).
//  - Lock / sleep: the count restarts, a shown one closes; input is ignored
//    until unlock / resume.
class MicroBreakEngine {
public:
    struct Config {
        qint64 intervalMs = 10 * 60000;
        qint64 showMs = 20000;
        qint64 pauseAfterMs = 20000;
    };

    enum class State { Disabled, Waiting, Working, Shown };

    explicit MicroBreakEngine(const IClock &clock);

    void setConfig(const Config &config); // applies immediately
    const Config &config() const { return m_config; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_state != State::Disabled; }

    void onActivity(); // real input only
    void setLocked(bool locked);
    void setBlocked(bool blocked);
    void restart();
    void dismiss();
    void update();

    State state() const { return m_state; }
    qint64 activeMs() const;         // active use of the current count (0 while Waiting / Disabled)
    qint64 timeUntilMs() const;      // Working only, else -1
    qint64 shownRemainingMs() const; // Shown only, else -1
    bool isPaused() const;           // Working, but no input for pauseAfterMs

private:
    qint64 now() const { return m_clock.now().toMSecsSinceEpoch(); }
    qint64 workedAt(qint64 t) const;
    void reset();

    const IClock &m_clock;
    Config m_config;
    State m_state = State::Disabled;
    qint64 m_workedMs = 0;
    qint64 m_markMs = 0;
    qint64 m_lastActivityMs = 0;
    qint64 m_shownAtMs = 0;
    bool m_locked = false;
    bool m_blocked = false;
};

} // namespace ergo
