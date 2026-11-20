#include "core/InstallPolicy.h"

#include <QDir>
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

} // namespace ergo
