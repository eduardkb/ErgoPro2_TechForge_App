#include "core/LogCategories.h"

// Categories are enabled for all levels; filtering is done by ergo::Logger
// according to the user's log level selection.
Q_LOGGING_CATEGORY(lcApp, "app", QtDebugMsg)
Q_LOGGING_CATEGORY(lcLogging, "logging", QtDebugMsg)
Q_LOGGING_CATEGORY(lcSettings, "settings", QtDebugMsg)
Q_LOGGING_CATEGORY(lcPlatform, "platform", QtDebugMsg)
Q_LOGGING_CATEGORY(lcUi, "ui", QtDebugMsg)
