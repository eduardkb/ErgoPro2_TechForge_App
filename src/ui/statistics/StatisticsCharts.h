#pragma once

#include "core/statistics/ChartAxis.h"
#include "core/statistics/StatisticsTypes.h"

#include <QWidget>

namespace ergo {

// Display helpers (B.8.3).
QString formatClock(int minuteOfDay);          // "09:05"
QString formatDurationHms(qint64 seconds);     // "1:05:09"
QString formatDistanceCm(double centimetres);  // "≈ 45 cm", "≈ 12.3 m"

// Common x axis of both charts: same left/right margins so the hours line up.
class TimeAxisChart : public QWidget {
    Q_OBJECT
public:
    explicit TimeAxisChart(QWidget *parent = nullptr);
    void setDay(const DayStats &day, const HourRange &range);

protected:
    static constexpr int kLeft = 80;
    static constexpr int kRight = 24;
    static constexpr int kAxisHeight = 22;

    double xForMinute(double minute, const QRectF &plot) const;
    double minuteForX(double x, const QRectF &plot) const;
    // Hour labels and vertical grid lines below / inside 'plot'.
    void paintTimeAxis(QPainter &p, const QRectF &plot) const;
    virtual void dayChanged() {}

    DayStats m_day;
    HourRange m_range;
};

// Chart 1: computer usage percent, one point per minute, gaps not connected,
// 100 % line, break-event markers, hover tooltip.
class UsageChart final : public TimeAxisChart {
    Q_OBJECT
public:
    explicit UsageChart(QWidget *parent = nullptr);
    QSize sizeHint() const override { return {640, 250}; }
    QSize minimumSizeHint() const override { return {320, 200}; }

    static QString markerGlyph(BreakEvent event);
    static QColor markerColor(BreakEvent event);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dayChanged() override;

private:
    QRectF plotRect() const;
    double yFor(double percent, const QRectF &plot) const;

    double m_yMax = 100;
    double m_yStep = 25;
};

// Chart 2: three thin bar charts (clicks, key presses, mouse distance) in
// 5-minute buckets on the same time axis, each with its own scale.
class ActivityChart final : public TimeAxisChart {
    Q_OBJECT
public:
    explicit ActivityChart(QWidget *parent = nullptr);
    QSize sizeHint() const override { return {640, 230}; }
    QSize minimumSizeHint() const override { return {320, 200}; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void dayChanged() override;

private:
    QRectF rowRect(int row) const;
    QRectF plotArea() const;

    QList<ActivityBucket> m_buckets;
    double m_max[3] = {1, 1, 1};
};

} // namespace ergo
