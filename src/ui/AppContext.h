#pragma once

#include <QString>

namespace ergo {

class AutoClickController;
class BreakTimerController;
class KeyControlController;
class FeatureAvailabilityService;
class InputService;
class InstallController;
class Logger;
class SettingsStore;
class StatisticsController;
struct Platform;

// Services shared by the windows and tabs (owned by AppController / main).
struct AppContext {
    SettingsStore &settings;
    Logger &logger;
    Platform &platform;
    FeatureAvailabilityService &availability;
    InstallController &install;
    InputService &input;
    AutoClickController &autoclick;
    KeyControlController &keycontrol;
    BreakTimerController &breaktimer;
    StatisticsController &statistics;
};

} // namespace ergo
