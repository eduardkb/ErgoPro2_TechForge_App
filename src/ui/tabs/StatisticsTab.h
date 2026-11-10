#pragma once

#include <QDate>
#include <QList>
#include <QWidget>

class QLabel;
class QPushButton;

namespace ergo {

class ActivityChart;
class ParameterRow;
class SettingsStore;
class StatisticsController;
class UsageChart;

// "Statistics" tab content (B.8.3): date header, usage chart, activity chart,
// daily totals, day navigation and the options card. The availability banner
// is added around it by FeaturePage.
class StatisticsTab : public QWidget {
    Q_OBJECT
public:
    StatisticsTab(StatisticsController &controller, SettingsStore &settings, QWidget *parent = nullptr);

private:
    void showDay(const QDate &date);
    void refresh();
    void refreshNavigation();
    void openFolder();

    StatisticsController &m_controller;
    SettingsStore &m_settings;
    QDate m_date;
    bool m_followToday = true; // the shown day is "today" and moves on at midnight

    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    UsageChart *m_usage = nullptr;
    ActivityChart *m_activity = nullptr;
    QList<QLabel *> m_values;
    QPushButton *m_previous = nullptr;
    QPushButton *m_next = nullptr;
    QPushButton *m_today = nullptr;
    ParameterRow *m_retention = nullptr;
};

} // namespace ergo
