#include "ui/statistics/StatisticsCharts.h"

#include "ui/Theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include <algorithm>
#include <cmath>

namespace ergo {

namespace {

QColor seriesColor(int index)
{
    const ThemeColors &c = Theme::instance().colors();
    switch (index) {
    case 0: return c.accent;
    case 1: return c.dark ? QColor(0xA7, 0x8B, 0xFA) : QColor(0x7C, 0x3A, 0xED); // violet
    default: return c.warning;
    }
}

QString eventName(BreakEvent e)
{
    switch (e) {
    case BreakEvent::Shown: return QObject::tr("Break shown");
    case BreakEvent::Dismissed: return QObject::tr("Break dismissed");
    case BreakEvent::Postponed: return QObject::tr("Break postponed");
    case BreakEvent::Completed: return QObject::tr("Break completed");
    }
    return {};
}

QString formatValue(double v)
{
    return v >= 100 || v == std::floor(v) ? QString::number(qRound(v)) : QString::number(v, 'f', 1);
}

} // namespace

QString formatClock(int minuteOfDay)
{
    return QStringLiteral("%1:%2")
        .arg(minuteOfDay / 60, 2, 10, QLatin1Char('0'))
        .arg(minuteOfDay % 60, 2, 10, QLatin1Char('0'));
}

QString formatDurationHms(qint64 seconds)
{
    return QStringLiteral("%1:%2:%3")
        .arg(seconds / 3600)
        .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

QString formatDistanceCm(double cm)
{
    if (cm < 100)
        return QObject::tr("≈ %1 cm").arg(qRound(cm));
    if (cm < 100000)
        return QObject::tr("≈ %1 m").arg(cm / 100.0, 0, 'f', cm < 1000 ? 2 : 1);
    return QObject::tr("≈ %1 km").arg(cm / 100000.0, 0, 'f', 2);
}

// ---------------------------------------------------------------- time axis

TimeAxisChart::TimeAxisChart(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(&Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

void TimeAxisChart::setDay(const DayStats &day, const HourRange &range)
{
    m_day = day;
    m_range = range;
    dayChanged();
    update();
}

double TimeAxisChart::xForMinute(double minute, const QRectF &plot) const
{
    return plot.left() + (minute - m_range.startMinute()) / double(m_range.minutes()) * plot.width();
}

double TimeAxisChart::minuteForX(double x, const QRectF &plot) const
{
    return m_range.startMinute() + (x - plot.left()) / plot.width() * m_range.minutes();
}

void TimeAxisChart::paintTimeAxis(QPainter &p, const QRectF &plot) const
{
    const ThemeColors &c = Theme::instance().colors();
    const int hours = m_range.endHour - m_range.startHour;
    const double perHour = plot.width() / hours;
    const int every = perHour >= 34 ? 1 : perHour >= 17 ? 2 : perHour >= 9 ? 3 : 6;
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.85);
    p.setFont(small);
    for (int h = m_range.startHour; h <= m_range.endHour; ++h) {
        const double x = xForMinute(h * 60, plot);
        QColor grid = c.border;
        grid.setAlphaF(0.6);
        p.setPen(QPen(grid, 1));
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        if ((h - m_range.startHour) % every != 0)
            continue;
        p.setPen(c.textMuted);
        const QRectF label(x - 24, plot.bottom() + 4, 48, kAxisHeight - 4);
        p.drawText(label, Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("%1:00").arg(h % 24, 2, 10, QLatin1Char('0')));
    }
}

// ---------------------------------------------------------------- usage chart

UsageChart::UsageChart(QWidget *parent)
    : TimeAxisChart(parent)
{
    setFixedHeight(250);
}

QString UsageChart::markerGlyph(BreakEvent e)
{
    switch (e) {
    case BreakEvent::Shown: return QStringLiteral("▲");
    case BreakEvent::Dismissed: return QStringLiteral("✕");
    case BreakEvent::Postponed: return QStringLiteral("◆");
    case BreakEvent::Completed: return QStringLiteral("✓");
    }
    return {};
}

QColor UsageChart::markerColor(BreakEvent e)
{
    const ThemeColors &c = Theme::instance().colors();
    switch (e) {
    case BreakEvent::Shown: return c.warning;
    case BreakEvent::Dismissed: return c.danger;
    case BreakEvent::Postponed: return seriesColor(1);
    case BreakEvent::Completed: return c.success;
    }
    return c.text;
}

void UsageChart::dayChanged()
{
    double maxValue = 0;
    for (const MinuteRecord &r : m_day.minutes)
        maxValue = std::max(maxValue, r.usagePercent);
    m_yMax = percentAxisMax(maxValue, &m_yStep);
}

QRectF UsageChart::plotRect() const
{
    return QRectF(kLeft, 22, width() - kLeft - kRight, height() - 22 - kAxisHeight);
}

double UsageChart::yFor(double percent, const QRectF &plot) const
{
    return plot.bottom() - percent / m_yMax * plot.height();
}

void UsageChart::paintEvent(QPaintEvent *)
{
    const ThemeColors &c = Theme::instance().colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF plot = plotRect();

    // Horizontal grid and percent labels.
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.85);
    p.setFont(small);
    for (double v = 0; v <= m_yMax + 0.1; v += m_yStep) {
        const double y = yFor(v, plot);
        QColor grid = c.border;
        grid.setAlphaF(0.6);
        p.setPen(QPen(grid, 1));
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        p.setPen(c.textMuted);
        p.drawText(QRectF(0, y - 9, kLeft - 8, 18), Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("%1 %").arg(qRound(v)));
    }
    paintTimeAxis(p, plot);

    // 100 % reference line.
    QPen hundred(c.warning, 1.6, Qt::DashLine);
    p.setPen(hundred);
    p.drawLine(QPointF(plot.left(), yFor(100, plot)), QPointF(plot.right(), yFor(100, plot)));

    if (m_day.isEmpty()) {
        p.setPen(c.textMuted);
        p.setFont(font());
        p.drawText(plot, Qt::AlignCenter, tr("No data for this day"));
        return;
    }

    // Usage line: consecutive minutes are connected, gaps are not.
    QColor fill = c.accent;
    fill.setAlphaF(0.14);
    QPen line(c.accent, 2.0);
    line.setJoinStyle(Qt::RoundJoin);
    const auto pointFor = [&](const MinuteRecord &r) {
        return QPointF(xForMinute(r.minute + 0.5, plot), yFor(r.usagePercent, plot));
    };
    int i = 0;
    const auto &minutes = m_day.minutes;
    while (i < minutes.size()) {
        int j = i;
        while (j + 1 < minutes.size() && minutes[j + 1].minute == minutes[j].minute + 1)
            ++j;
        if (i == j) {
            p.setPen(Qt::NoPen);
            p.setBrush(c.accent);
            p.drawEllipse(pointFor(minutes[i]), 2.2, 2.2);
        } else {
            QPainterPath path(pointFor(minutes[i]));
            for (int k = i + 1; k <= j; ++k)
                path.lineTo(pointFor(minutes[k]));
            QPainterPath area = path;
            area.lineTo(pointFor(minutes[j]).x(), plot.bottom());
            area.lineTo(pointFor(minutes[i]).x(), plot.bottom());
            area.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawPath(area);
            p.setBrush(Qt::NoBrush);
            p.setPen(line);
            p.drawPath(path);
        }
        i = j + 1;
    }

    // Break-event markers along the top edge.
    QFont markerFont = font();
    markerFont.setBold(true);
    p.setFont(markerFont);
    for (const MinuteRecord &r : minutes) {
        int stack = 0;
        for (const QString &token : r.events) {
            BreakEvent e;
            if (!breakEventFromToken(token, &e))
                continue;
            const double x = xForMinute(r.minute + 0.5, plot);
            p.setPen(markerColor(e));
            p.drawText(QRectF(x - 8 + stack * 11, 2, 16, 18), Qt::AlignCenter, markerGlyph(e)); // side by side
            ++stack;
        }
    }
}

void UsageChart::mouseMoveEvent(QMouseEvent *event)
{
    const QRectF plot = plotRect();
    const QPointF pos = event->position();
    if (pos.x() < plot.left() || pos.x() > plot.right() || m_day.isEmpty()) {
        QToolTip::hideText();
        return;
    }
    const int minute = int(std::floor(minuteForX(pos.x(), plot)));
    const MinuteRecord *best = nullptr;
    for (int d : {0, -1, 1}) {
        if ((best = m_day.find(minute + d)))
            break;
    }
    if (!best) {
        QToolTip::showText(event->globalPosition().toPoint(), tr("%1 – no data").arg(formatClock(minute)), this);
        return;
    }
    QString text = tr("%1 – usage %2 %").arg(formatClock(best->minute)).arg(best->usagePercent, 0, 'f', 1);
    for (const QString &token : best->events) {
        BreakEvent e;
        if (breakEventFromToken(token, &e))
            text += QLatin1Char('\n') + markerGlyph(e) + QLatin1Char(' ') + eventName(e);
    }
    QToolTip::showText(event->globalPosition().toPoint(), text, this);
}

// ---------------------------------------------------------------- activity chart

ActivityChart::ActivityChart(QWidget *parent)
    : TimeAxisChart(parent)
{
    setFixedHeight(230);
}

void ActivityChart::dayChanged()
{
    m_buckets = fiveMinuteBuckets(m_day);
    double maxima[3] = {0, 0, 0};
    for (const ActivityBucket &b : m_buckets) {
        maxima[0] = std::max(maxima[0], double(b.clicks));
        maxima[1] = std::max(maxima[1], double(b.keyPresses));
        maxima[2] = std::max(maxima[2], b.distanceCm);
    }
    for (int i = 0; i < 3; ++i)
        m_max[i] = niceCeiling(maxima[i]);
}

QRectF ActivityChart::plotArea() const
{
    return QRectF(kLeft, 4, width() - kLeft - kRight, height() - 4 - kAxisHeight);
}

QRectF ActivityChart::rowRect(int row) const
{
    const QRectF area = plotArea();
    const double gap = 10;
    const double h = (area.height() - 2 * gap) / 3;
    return QRectF(area.left(), area.top() + row * (h + gap), area.width(), h);
}

void ActivityChart::paintEvent(QPaintEvent *)
{
    const ThemeColors &c = Theme::instance().colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF area = plotArea();
    paintTimeAxis(p, area);

    const QString names[3] = {tr("Clicks"), tr("Keys"), tr("Distance")};
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() * 0.85);
    for (int row = 0; row < 3; ++row) {
        const QRectF r = rowRect(row);
        p.setPen(QPen(c.border, 1));
        p.drawLine(QPointF(r.left(), r.bottom()), QPointF(r.right(), r.bottom()));
        // Row label + scale.
        QFont bold = font();
        bold.setBold(true);
        p.setFont(bold);
        p.setPen(seriesColor(row));
        p.drawText(QRectF(0, r.top(), kLeft - 8, r.height() / 2), Qt::AlignRight | Qt::AlignBottom, names[row]);
        p.setFont(small);
        p.setPen(c.textMuted);
        const QString scale = row == 2 ? formatDistanceCm(m_max[2]).remove(QStringLiteral("≈ "))
                                       : formatValue(m_max[row]);
        p.drawText(QRectF(0, r.top() + r.height() / 2, kLeft - 8, r.height() / 2), Qt::AlignRight | Qt::AlignTop,
                   tr("max %1").arg(scale));

        p.setPen(Qt::NoPen);
        p.setBrush(seriesColor(row));
        for (const ActivityBucket &b : std::as_const(m_buckets)) {
            if (b.startMinute + kBucketMinutes <= m_range.startMinute()
                || b.startMinute >= m_range.startMinute() + m_range.minutes())
                continue;
            const double value = row == 0 ? b.clicks : row == 1 ? b.keyPresses : b.distanceCm;
            if (value <= 0)
                continue;
            const double x0 = xForMinute(b.startMinute, r);
            const double x1 = xForMinute(b.startMinute + kBucketMinutes, r);
            const double h = std::max(1.5, value / m_max[row] * r.height());
            p.drawRect(QRectF(x0 + 0.5, r.bottom() - h, std::max(1.0, x1 - x0 - 1.0), h));
        }
    }
    if (m_day.isEmpty()) {
        p.setPen(c.textMuted);
        p.setFont(font());
        p.drawText(area, Qt::AlignCenter, tr("No data for this day"));
    }
}

void ActivityChart::mouseMoveEvent(QMouseEvent *event)
{
    const QRectF area = plotArea();
    const QPointF pos = event->position();
    if (pos.x() < area.left() || pos.x() > area.right() || m_buckets.isEmpty()) {
        QToolTip::hideText();
        return;
    }
    const int minute = std::clamp(int(std::floor(minuteForX(pos.x(), area))), 0, kMinutesPerDay - 1);
    const ActivityBucket &b = m_buckets[minute / kBucketMinutes];
    QToolTip::showText(event->globalPosition().toPoint(),
                       tr("%1–%2\nClicks: %3\nKey presses: %4\nMouse distance: %5")
                           .arg(formatClock(b.startMinute), formatClock((b.startMinute + kBucketMinutes) % kMinutesPerDay))
                           .arg(b.clicks)
                           .arg(b.keyPresses)
                           .arg(formatDistanceCm(b.distanceCm)),
                       this);
}

} // namespace ergo
