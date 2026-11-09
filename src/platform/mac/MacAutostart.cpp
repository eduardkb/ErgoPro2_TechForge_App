#include "platform/mac/MacAutostart.h"

#include "core/LogCategories.h"
#include "platform/common/FileOperations.h"

#include <QDir>
#include <QFileInfo>

namespace ergo {

namespace {
const QString kLabel = QStringLiteral("com.techforge.ergopro2");
}

QString MacAutostart::plistPath()
{
    return QDir(QDir::homePath()).filePath(QStringLiteral("Library/LaunchAgents/%1.plist").arg(kLabel));
}

bool MacAutostart::isEnabled() const
{
    return QFileInfo::exists(plistPath());
}

QString MacAutostart::location() const
{
    return plistPath();
}

PlatformResult MacAutostart::enable(const QString &executablePath, const QStringList &arguments)
{
    QString programArguments = QStringLiteral("        <string>%1</string>\n").arg(executablePath.toHtmlEscaped());
    for (const QString &argument : arguments)
        programArguments += QStringLiteral("        <string>%1</string>\n").arg(argument.toHtmlEscaped());

    // RunAtLoad starts the app when the agent is loaded at login. The agent is
    // deliberately not loaded now (that would start a second instance).
    const QString plist = QStringLiteral(
                              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                              "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                              "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                              "<plist version=\"1.0\">\n"
                              "<dict>\n"
                              "    <key>Label</key>\n"
                              "    <string>%1</string>\n"
                              "    <key>ProgramArguments</key>\n"
                              "    <array>\n"
                              "%2"
                              "    </array>\n"
                              "    <key>RunAtLoad</key>\n"
                              "    <true/>\n"
                              "    <key>ProcessType</key>\n"
                              "    <string>Interactive</string>\n"
                              "    <key>LimitLoadToSessionType</key>\n"
                              "    <string>Aqua</string>\n"
                              "</dict>\n"
                              "</plist>\n")
                              .arg(kLabel, programArguments);

    const PlatformResult result = fileops::writeTextFile(plistPath(), plist);
    if (result.ok())
        qCInfo(lcPlatform).noquote() << "Start at login enabled:" << plistPath() << "->" << executablePath;
    return result;
}

PlatformResult MacAutostart::disable()
{
    // The plist is only removed (not "launchctl bootout"): booting out the job
    // would terminate ErgoPro2 if this process was started by it.
    const PlatformResult result = fileops::removePath(plistPath());
    if (result.ok())
        qCInfo(lcPlatform) << "Start at login disabled";
    return result;
}

} // namespace ergo
