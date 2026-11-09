#pragma once

#include <QDateTime>
#include <QElapsedTimer>
#include <QtGlobal>

namespace ergo {

// Injectable time source. All core logic that depends on time must use an
// IClock instead of calling QDateTime/QElapsedTimer directly, so that state
// machines can be unit-tested with FakeClock without real waiting.
class IClock {
public:
    virtual ~IClock() = default;

    // Local wall-clock date/time (used for log file names, timestamps).
    virtual QDateTime now() const = 0;

    // Monotonic milliseconds since an arbitrary epoch (used for durations).
    virtual qint64 monotonicMs() const = 0;
};

// Real clock used by the application.
class SystemClock final : public IClock {
public:
    SystemClock() { m_elapsed.start(); }

    QDateTime now() const override { return QDateTime::currentDateTime(); }
    qint64 monotonicMs() const override { return m_elapsed.elapsed(); }

private:
    QElapsedTimer m_elapsed;
};

// Fully controllable clock for unit tests.
class FakeClock final : public IClock {
public:
    explicit FakeClock(const QDateTime &start = QDateTime(QDate(2026, 1, 1), QTime(9, 0)))
        : m_now(start) {}

    QDateTime now() const override { return m_now; }
    qint64 monotonicMs() const override { return m_monotonicMs; }

    // Moves both wall-clock and monotonic time forward.
    void advanceMs(qint64 ms)
    {
        m_now = m_now.addMSecs(ms);
        m_monotonicMs += ms;
    }

    // Changes the wall-clock only (e.g. to simulate a new day); monotonic time is untouched.
    void setNow(const QDateTime &now) { m_now = now; }

private:
    QDateTime m_now;
    qint64 m_monotonicMs = 0;
};

} // namespace ergo
