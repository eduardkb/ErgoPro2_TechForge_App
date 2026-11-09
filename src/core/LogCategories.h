#pragma once

#include <QLoggingCategory>

// One logging category per module. The category name is written as [module]
// in every log line. Use e.g. qCWarning(lcSettings) << "...".
Q_DECLARE_LOGGING_CATEGORY(lcApp)
Q_DECLARE_LOGGING_CATEGORY(lcLogging)
Q_DECLARE_LOGGING_CATEGORY(lcSettings)
Q_DECLARE_LOGGING_CATEGORY(lcPlatform)
Q_DECLARE_LOGGING_CATEGORY(lcUi)
