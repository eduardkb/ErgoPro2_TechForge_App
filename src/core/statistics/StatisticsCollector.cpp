#include "core/statistics/StatisticsCollector.h"

#include <QDateTime>
#include <QLineF>

#include <algorithm>
#include <cmath>

namespace ergo {

StatisticsCollector::StatisticsCollector(const IClock &clock)
    : m_clock(clock)
{
    openMinuteAt(now());
    m_today.date = m_minuteDate;
}

void StatisticsCollector::setConfig(const Config &config)
{
    m_config = config;
    if (m_config.maxWorkMs <= 0)
        m_config.maxWorkMs = 60000;
}

void StatisticsCollector::setPixelsPerCm(double pixelsPerCm)
{
    if (pixelsPerCm > 0)
        m_pixelsPerCm = pixelsPerCm;
}

void StatisticsCollector::restore(const DayStats &day, qint64 lastDataEndMs, double lastPercent)
{
    advanceTo(now());
    if (day.date == m_today.date) {
        for (const MinuteRecord &r : day.minutes)
            m_today.add(r);
    }
    if (lastDataEndMs > 0 && now() - lastDataEndMs < m_config.resetAfterMs && lastPercent > 0) {
        // Restarted within the reset time: the usage percent continues.
        m_usedSeconds = qint64(std::llround(lastPercent / 100.0 * double(m_config.maxWorkMs) / 1000.0));
        m_lastInputMs = lastDataEndMs;
    }
}

void StatisticsCollector::openMinuteAt(qint64 t)
{
    const QDateTime local = QDateTime::fromMSecsSinceEpoch(t);
    const QTime time = local.time();
    const QDateTime start(local.date(), QTime(time.hour(), time.minute()));
    m_minuteDate = local.date();
    m_minuteStartMs = start.toMSecsSinceEpoch();
    m_minuteEndMs = start.addSecs(60).toMSecsSinceEpoch();
    m_minute = MinuteRecord();
    m_minute.minute = time.hour() * 60 + time.minute();
}

double StatisticsCollector::percentAt(qint64 t) const
{
    if (m_lastInputMs == 0 || t - m_lastInputMs >= m_config.resetAfterMs)
        return 0.0; // rested long enough (also: never used)
    return double(m_usedSeconds) * 1000.0 * 100.0 / double(m_config.maxWorkMs);
}

void StatisticsCollector::commitSecond()
{
    if (m_second < 0)
        return;
    if (m_secondMouse)
        ++m_minute.mouseSeconds;
    if (m_secondKeyboard)
        ++m_minute.keyboardSeconds;
    if (m_secondMouse || m_secondKeyboard) {
        ++m_minute.activeSeconds;
        ++m_usedSeconds;
    }
    m_second = -1;
    m_secondMouse = m_secondKeyboard = false;
}

void StatisticsCollector::finishMinute()
{
    if (m_minute.hasData()) {
        m_minute.usagePercent = percentAt(m_minuteEndMs - 1);
        if (m_minuteDate == m_today.date)
            m_today.add(m_minute);
        m_finished.append({m_minuteDate, m_minute});
    }
}

void StatisticsCollector::advanceTo(qint64 t)
{
    if (m_second >= 0 && t / 1000 != m_second)
        commitSecond();
    if (t >= m_minuteStartMs && t < m_minuteEndMs)
        return;
    finishMinute();
    openMinuteAt(t);
    if (m_minuteDate != m_today.date) {
        // Midnight: a new day starts. The usage percent continues.
        m_today = DayStats();
        m_today.date = m_minuteDate;
    }
}

void StatisticsCollector::onInputEvent(const InputEvent &event)
{
    if (event.synthetic || m_locked)
        return;
    const qint64 t = now();
    advanceTo(t);
    if (m_lastInputMs != 0 && t - m_lastInputMs >= m_config.resetAfterMs)
        m_usedSeconds = 0; // B.8.2: a pause of BREAKTIMER_BREAK_TIME resets the usage
    m_lastInputMs = t;
    if (m_second < 0)
        m_second = t / 1000;

    switch (event.type) {
    case InputEventType::MouseMove:
        m_secondMouse = true;
        if (m_hasPosition) {
            const double d = QLineF(m_lastPosition, event.position).length();
            m_minute.distancePx += d;
            m_minute.distanceCm += d / m_pixelsPerCm;
        }
        m_lastPosition = event.position;
        m_hasPosition = true;
        break;
    case InputEventType::MouseDown:
        m_secondMouse = true;
        if (event.button == MouseButtonId::Left)
            ++m_minute.clicksLeft;
        else if (event.button == MouseButtonId::Right)
            ++m_minute.clicksRight;
        else
            ++m_minute.clicksOther;
        break;
    case InputEventType::MouseUp:
        m_secondMouse = true;
        break;
    case InputEventType::Wheel:
        m_secondMouse = true;
        ++m_minute.wheel;
        break;
    case InputEventType::KeyDown:
        m_secondKeyboard = true;
        if (!event.autoRepeat)
            ++m_minute.keyPresses;
        break;
    case InputEventType::KeyUp:
        m_secondKeyboard = true;
        break;
    }
}

void StatisticsCollector::onClicksDoneForYou(int count)
{
    if (count <= 0)
        return;
    advanceTo(now());
    m_minute.clicksForYou += count;
}

void StatisticsCollector::onBreakEvent(BreakEvent event)
{
    advanceTo(now());
    m_minute.events.append(breakEventToken(event));
}

void StatisticsCollector::tick()
{
    advanceTo(now());
}

void StatisticsCollector::flush()
{
    commitSecond();
    finishMinute();
    openMinuteAt(now());
    // The partial minute is reported now; a restart in the same minute adds a
    // second row for it, which is merged when the day is read.
    if (m_minuteDate != m_today.date) {
        m_today = DayStats();
        m_today.date = m_minuteDate;
    }
}

QList<StatisticsCollector::FinishedMinute> StatisticsCollector::takeFinished()
{
    QList<FinishedMinute> list;
    list.swap(m_finished);
    return list;
}

} // namespace ergo
