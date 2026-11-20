#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace ergo {

// Pure decisions of the Install workflow (B.1.3). The OS code collects the
// facts (which copies exist, where start at login points to) and these
// functions decide; so the rules are unit-tested (tests/tst_installpolicy.cpp).

// A place where an installed copy may be (macOS 2.0.0: /Applications/ErgoPro2.app
// and ~/Applications/ErgoPro2.app).
struct InstallCandidate {
    QString path;
    bool exists = false;
};

// Which copy counts as "the installed copy":
// 1. the running copy, when it is one of the candidates;
// 2. otherwise the first existing candidate (candidates in order of preference);
// 3. otherwise none (empty string).
QString chooseInstalledCopy(const QString &runningPath, const QList<InstallCandidate> &candidates,
                            Qt::CaseSensitivity caseSensitivity);

// Where Install copies to when no installed copy exists yet: the preferred
// folder when it can be written, else the per-user fallback.
QString chooseInstallTarget(const QString &preferred, bool preferredWritable, const QString &fallback);

// macOS (2.0.0): a copy dragged from the .dmg into an Applications folder is
// installed already. On its start it registers start at login (or moves it from
// an older copy) and offers once to remove an older copy (~/Applications).
struct InstallStartupFacts {
    bool runningFromInstalledCopy = false;
    bool registerStartAtLoginOnStart = false; // the OS installs by copying (macOS drag and drop)
    bool startsAtLogin = false;
    bool startAtLoginPointsToInstalledCopy = false;
    bool olderCopyExists = false;
    bool olderCopyKept = false; // the user answered "keep" before
    // 2.1.0: the ErgoPro2 installer disk image is still mounted / a
    // downloaded ErgoPro2-<version>-macos.dmg was found (mounted or not).
    bool installerImageMounted = false;
    bool installerImageFound = false;
};

struct InstallStartupActions {
    bool registerStartAtLogin = false;
    bool askToRemoveOlderCopy = false;
    // 2.1.0 (owner request): "ErgoPro2 is installed" on the first start of the
    // dragged copy, and the offer to eject the .dmg and move it to the Trash.
    bool confirmInstalled = false;
    bool offerToDeleteInstaller = false;
};

InstallStartupActions decideInstallStartupActions(const InstallStartupFacts &facts);

// True for the name of a downloaded macOS installer disk image:
// "ErgoPro2-<x.y.z>-macos.dmg", also with a browser suffix like " (1)".
bool isInstallerDiskImageName(const QString &fileName);

// Disk images listed by "hdiutil info -plist" with their mount points (empty
// when attached but not mounted). Empty list for unreadable output.
struct MountedImage {
    QString imagePath;
    QStringList mountPoints;
};
QList<MountedImage> parseMountedImages(const QString &hdiutilInfoPlist);

// First program argument (the executable) of a macOS LaunchAgent plist
// (ProgramArguments array); empty when it cannot be found.
QString launchAgentExecutable(const QString &plistXml);

} // namespace ergo
