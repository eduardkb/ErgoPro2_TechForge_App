#pragma once

#include <QString>

namespace ergo {

class AutoClickController;
class FeatureAvailabilityService;
class InputService;
class InstallController;
class Logger;
class SettingsStore;
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
};

} // namespace ergo
