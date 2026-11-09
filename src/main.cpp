#include "core/Clock.h"
#include "core/CrashHandler.h"
#include "core/LogCategories.h"
#include "core/Logger.h"
#include "core/settings/SettingsRegistry.h"
#include "core/settings/SettingsSchema.h"
#include "core/settings/SettingsStore.h"
#include "ergopro2/Version.h"
#include "platform/PlatformFactory.h"
#include "ui/AppController.h"
#include "ui/ErgoApplication.h"
#include "ui/SingleInstance.h"
#include "ui/Theme.h"

#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>

using namespace ergo;

int main(int argc, char *argv[])
{
    installTerminateHandler();

    ErgoApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral(ERGOPRO2_ORGANIZATION));
    QCoreApplication::setApplicationName(QStringLiteral(ERGOPRO2_APP_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral(ERGOPRO2_VERSION_STRING));
    QThread::currentThread()->setObjectName(QStringLiteral("main"));

    // --- Command line -------------------------------------------------------
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("ErgoPro2 - auto click, key control and break reminders"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption autostartOption(
        QStringLiteral("autostart"), QStringLiteral("Set by the OS login item: the app was started automatically."));
    parser.addOption(autostartOption);
    parser.process(app);

    LaunchOptions launch;
    launch.autostart = parser.isSet(autostartOption);

    // --- Logging --------------------------------------------------------------
    SystemClock clock;
    Logger logger(clock);
    Logger::Config logConfig;
    logConfig.directory =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("logs"));
#ifndef NDEBUG
    logConfig.consoleOutput = true; // debug builds also log to the console
#endif
    logger.open(logConfig); // failures are reported on stderr; logging falls back to stderr
    Logger::installQtMessageHandler(&logger);

    qCInfo(lcApp).noquote() << "ErgoPro2" << ERGOPRO2_VERSION_STRING << "starting on" << QSysInfo::prettyProductName()
                            << QSysInfo::currentCpuArchitecture() << "- Qt" << qVersion();

    // --- Single instance ------------------------------------------------------
    SingleInstance instance(QStringLiteral("ergopro2"));
    switch (instance.acquire()) {
    case SingleInstance::Result::Primary:
        break;
    case SingleInstance::Result::OtherActivated:
        qCInfo(lcApp) << "Second launch: the running instance was activated, exiting";
        return 0;
    case SingleInstance::Result::Failed:
        QMessageBox::warning(nullptr, QStringLiteral("ErgoPro2"), instance.errorMessage());
        return 1;
    }

    // --- Settings -------------------------------------------------------------
    SettingsSchema schema;
    registerAllSettings(schema);
    const QString settingsPath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath(QStringLiteral("settings.json"));
    SettingsStore settings(schema, settingsPath);
    const SettingsLoadResult loadResult = settings.load();

    // --- Platform + UI --------------------------------------------------------
    Platform platform = createPlatform();
    Theme::instance().install(app);

    AppController controller(settings, logger, platform, instance, launch);
    controller.start(loadResult);

    qCInfo(lcApp).noquote() << "Settings file:" << QDir::toNativeSeparators(settingsPath);
    qCInfo(lcApp).noquote() << "Log folder:" << QDir::toNativeSeparators(logger.directory());

    const int exitCode = app.exec();

    if (!settings.saveNow())
        qCCritical(lcApp) << "Settings could not be saved on shutdown";
    qCInfo(lcApp) << "ErgoPro2 exited with code" << exitCode;
    Logger::installQtMessageHandler(nullptr);
    return exitCode;
}
