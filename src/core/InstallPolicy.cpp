#include "core/InstallPolicy.h"

#include <QDir>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace ergo {

namespace {

bool samePath(const QString &a, const QString &b, Qt::CaseSensitivity cs)
{
    return !a.isEmpty() && !b.isEmpty() && QDir::cleanPath(a).compare(QDir::cleanPath(b), cs) == 0;
}

} // namespace

QString chooseInstalledCopy(const QString &runningPath, const QList<InstallCandidate> &candidates,
                            Qt::CaseSensitivity caseSensitivity)
{
    for (const InstallCandidate &c : candidates) {
        if (c.exists && samePath(runningPath, c.path, caseSensitivity))
            return c.path;
    }
    for (const InstallCandidate &c : candidates) {
        if (c.exists)
            return c.path;
    }
    return {};
}

QString chooseInstallTarget(const QString &preferred, bool preferredWritable, const QString &fallback)
{
    return preferredWritable ? preferred : fallback;
}

InstallStartupActions decideInstallStartupActions(const InstallStartupFacts &facts)
{
    InstallStartupActions a;
    if (!facts.runningFromInstalledCopy)
        return a; // e.g. started from the .dmg: only the warning on the General tab
    a.registerStartAtLogin =
        facts.registerStartAtLoginOnStart && (!facts.startsAtLogin || !facts.startAtLoginPointsToInstalledCopy);
    a.askToRemoveOlderCopy = facts.olderCopyExists && !facts.olderCopyKept;
    // 2.1.0: the first start of a copy dragged into Applications is the moment
    // it became installed (start at login is registered for it now).
    // 2.2.1: also the first start of each new version (an update dragged over
    // the installed copy), remembered in general.installConfirmedVersion.
    a.confirmInstalled = facts.registerStartAtLoginOnStart
                         && (a.registerStartAtLogin || facts.currentVersion != facts.confirmedVersion);
    // The .dmg is offered right after installing (mounted or found in a
    // download folder), or whenever it is still mounted (update / Install +
    // restart) - not on every start because of an old download.
    a.offerToDeleteInstaller = facts.registerStartAtLoginOnStart
                               && (facts.installerImageMounted || (a.confirmInstalled && facts.installerImageFound));
    return a;
}

QString launchAgentExecutable(const QString &plistXml)
{
    // <key>ProgramArguments</key><array><string>EXECUTABLE</string>...
    QXmlStreamReader xml(plistXml);
    bool afterKey = false;
    bool inArray = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement())
            continue;
        const QStringView name = xml.name();
        if (name == QLatin1String("key")) {
            afterKey = xml.readElementText() == QLatin1String("ProgramArguments");
            inArray = false;
        } else if (afterKey && name == QLatin1String("array")) {
            inArray = true;
        } else if (inArray && name == QLatin1String("string")) {
            return xml.readElementText().trimmed();
        } else {
            afterKey = false;
        }
    }
    return {};
}

int versionNumber(int major, int minor, int patch)
{
    return major * 10000 + minor * 100 + patch;
}

bool isInstallerDiskImageName(const QString &fileName)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^ErgoPro2-\d+\.\d+\.\d+-macos( \(\d+\))?\.dmg$)"));
    return pattern.match(fileName).hasMatch();
}

QList<MountedImage> parseMountedImages(const QString &hdiutilInfoPlist)
{
    // <key>images</key><array> <dict> image-path, system-entities[ {mount-point} ] </dict> ... </array>
    QList<MountedImage> images;
    QXmlStreamReader xml(hdiutilInfoPlist);
    int depth = 0;       // element depth
    int imageDepth = -1; // depth of the current image <dict>
    int arrayDepth = -1; // depth of the "images" <array>
    QString lastKey;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) {
            if (depth == imageDepth)
                imageDepth = -1;
            else if (depth == arrayDepth)
                arrayDepth = -1;
            --depth;
            continue;
        }
        if (!xml.isStartElement())
            continue;
        ++depth;
        const QStringView name = xml.name();
        if (name == QLatin1String("key")) {
            lastKey = xml.readElementText();
            --depth; // readElementText consumed the end element
            continue;
        }
        if (name == QLatin1String("array") && arrayDepth < 0 && lastKey == QLatin1String("images")) {
            arrayDepth = depth;
        } else if (name == QLatin1String("dict") && arrayDepth >= 0 && depth == arrayDepth + 1) {
            imageDepth = depth;
            images.append(MountedImage{});
        } else if (name == QLatin1String("string") && imageDepth >= 0) {
            const QString value = xml.readElementText();
            --depth;
            if (lastKey == QLatin1String("image-path") && depth == imageDepth)
                images.last().imagePath = value;
            else if (lastKey == QLatin1String("mount-point"))
                images.last().mountPoints << value;
            continue;
        }
        lastKey.clear();
    }
    if (xml.hasError())
        return {};
    return images;
}

} // namespace ergo
