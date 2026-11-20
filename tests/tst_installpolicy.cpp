#include "core/InstallPolicy.h"

#include <QtTest>

using namespace ergo;

namespace {
const QString kSystem = QStringLiteral("/Applications/ErgoPro2.app");
const QString kUser = QStringLiteral("/Users/me/Applications/ErgoPro2.app");
const QString kDmg = QStringLiteral("/Volumes/ErgoPro2 2.0.0/ErgoPro2.app");
} // namespace

class TestInstallPolicy : public QObject {
    Q_OBJECT

private slots:
    void runningCopyInAnApplicationsFolderIsTheInstalledCopy()
    {
        // Dragged into /Applications and started from there (2.0.0).
        QCOMPARE(chooseInstalledCopy(kSystem, {{kSystem, true}, {kUser, true}}, Qt::CaseInsensitive), kSystem);
        // Running the per-user copy while /Applications has one too.
        QCOMPARE(chooseInstalledCopy(kUser, {{kSystem, true}, {kUser, true}}, Qt::CaseInsensitive), kUser);
        // Same path written differently (case-insensitive file system, trailing slash).
        QCOMPARE(chooseInstalledCopy(QStringLiteral("/applications/ErgoPro2.app/"), {{kSystem, true}},
                                     Qt::CaseInsensitive),
                 kSystem);
    }

    void otherwiseTheFirstExistingCandidate()
    {
        QCOMPARE(chooseInstalledCopy(kDmg, {{kSystem, true}, {kUser, true}}, Qt::CaseInsensitive), kSystem);
        QCOMPARE(chooseInstalledCopy(kDmg, {{kSystem, false}, {kUser, true}}, Qt::CaseInsensitive), kUser);
        QCOMPARE(chooseInstalledCopy(kDmg, {{kSystem, false}, {kUser, false}}, Qt::CaseInsensitive), QString());
    }

    void installTargetPrefersTheWritableSystemFolder()
    {
        QCOMPARE(chooseInstallTarget(kSystem, true, kUser), kSystem);
        QCOMPARE(chooseInstallTarget(kSystem, false, kUser), kUser); // standard account
    }

    void draggedCopyRegistersStartAtLogin()
    {
        InstallStartupFacts f;
        f.runningFromInstalledCopy = true;
        f.registerStartAtLoginOnStart = true;
        QVERIFY(decideInstallStartupActions(f).registerStartAtLogin); // first start
        f.startsAtLogin = true;
        QVERIFY(decideInstallStartupActions(f).registerStartAtLogin); // still points to the old copy
        f.startAtLoginPointsToInstalledCopy = true;
        QVERIFY(!decideInstallStartupActions(f).registerStartAtLogin); // already done
    }

    void noRegistrationOutsideTheInstalledCopyOrOnOtherSystems()
    {
        InstallStartupFacts f;
        f.registerStartAtLoginOnStart = true;
        QVERIFY(!decideInstallStartupActions(f).registerStartAtLogin); // running from the .dmg
        f.runningFromInstalledCopy = true;
        f.registerStartAtLoginOnStart = false; // Windows setup: its own option decides
        QVERIFY(!decideInstallStartupActions(f).registerStartAtLogin);
    }

    void olderCopyIsOfferedOnceFromTheInstalledCopy()
    {
        InstallStartupFacts f;
        f.olderCopyExists = true;
        QVERIFY(!decideInstallStartupActions(f).askToRemoveOlderCopy); // not from the installed copy
        f.runningFromInstalledCopy = true;
        QVERIFY(decideInstallStartupActions(f).askToRemoveOlderCopy);
        f.olderCopyKept = true;
        QVERIFY(!decideInstallStartupActions(f).askToRemoveOlderCopy); // answered "keep" before
        f.olderCopyKept = false;
        f.olderCopyExists = false;
        QVERIFY(!decideInstallStartupActions(f).askToRemoveOlderCopy);
    }

    void launchAgentExecutableIsReadFromThePlist()
    {
        const QString plist = QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<plist version=\"1.0\">\n<dict>\n"
            "    <key>Label</key>\n    <string>com.techforge.ergopro2</string>\n"
            "    <key>ProgramArguments</key>\n    <array>\n"
            "        <string>/Users/me/Applications/ErgoPro2.app/Contents/MacOS/ErgoPro2</string>\n"
            "        <string>--autostart</string>\n    </array>\n"
            "    <key>RunAtLoad</key>\n    <true/>\n</dict>\n</plist>\n");
        QCOMPARE(launchAgentExecutable(plist), QStringLiteral("/Users/me/Applications/ErgoPro2.app/Contents/MacOS/ErgoPro2"));
        QCOMPARE(launchAgentExecutable(QStringLiteral("<plist><dict><key>Label</key><string>x</string></dict></plist>")),
                 QString());
        QCOMPARE(launchAgentExecutable(QStringLiteral("not xml")), QString());
    }

    // 2.1.0 (owner request): confirmation and the offer to delete the .dmg.
    void draggedCopyConfirmsTheInstallationOnce()
    {
        InstallStartupFacts f;
        f.runningFromInstalledCopy = true;
        f.registerStartAtLoginOnStart = true;
        QVERIFY(decideInstallStartupActions(f).confirmInstalled); // first start of the dragged copy
        f.startsAtLogin = true;
        f.startAtLoginPointsToInstalledCopy = true;
        QVERIFY(!decideInstallStartupActions(f).confirmInstalled); // later starts
    }

    void installerImageIsOfferedForDeletion()
    {
        InstallStartupFacts f;
        f.runningFromInstalledCopy = true;
        f.registerStartAtLoginOnStart = true;
        f.installerImageFound = true; // not mounted, e.g. in Downloads
        QVERIFY(decideInstallStartupActions(f).offerToDeleteInstaller); // right after installing
        f.startsAtLogin = true;
        f.startAtLoginPointsToInstalledCopy = true;
        QVERIFY(!decideInstallStartupActions(f).offerToDeleteInstaller); // not on every start
        f.installerImageMounted = true; // update dragged from a mounted .dmg, or Install + restart
        QVERIFY(decideInstallStartupActions(f).offerToDeleteInstaller);
        f.runningFromInstalledCopy = false; // running from the .dmg itself
        QVERIFY(!decideInstallStartupActions(f).offerToDeleteInstaller);
        f.runningFromInstalledCopy = true;
        f.registerStartAtLoginOnStart = false; // Windows / Linux
        QVERIFY(!decideInstallStartupActions(f).offerToDeleteInstaller);
    }

    void installerDiskImageNames()
    {
        QVERIFY(isInstallerDiskImageName(QStringLiteral("ErgoPro2-2.0.0-macos.dmg")));
        QVERIFY(isInstallerDiskImageName(QStringLiteral("ErgoPro2-2.1.0-macos (1).dmg"))); // browser copy
        QVERIFY(!isInstallerDiskImageName(QStringLiteral("ErgoPro2-2.0.0-windows-x64-setup.exe")));
        QVERIFY(!isInstallerDiskImageName(QStringLiteral("Other-2.0.0-macos.dmg")));
        QVERIFY(!isInstallerDiskImageName(QStringLiteral("ErgoPro2-notes.dmg")));
    }

    void mountedImagesAreReadFromHdiutilInfo()
    {
        // Shortened output of "hdiutil info -plist" (macOS 26).
        const QString plist = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<plist version="1.0"><dict>
  <key>framework</key><string>683.160.3.701.2</string>
  <key>images</key><array>
    <dict>
      <key>image-path</key><string>/Users/me/Downloads/ErgoPro2-2.1.0-macos.dmg</string>
      <key>system-entities</key><array>
        <dict><key>content-hint</key><string>GUID_partition_scheme</string><key>dev-entry</key><string>/dev/disk4</string></dict>
        <dict><key>dev-entry</key><string>/dev/disk4s1</string><key>mount-point</key><string>/Volumes/ErgoPro2 2.1.0</string></dict>
      </array>
      <key>writeable</key><false/>
    </dict>
    <dict>
      <key>image-path</key><string>/Users/me/Other.dmg</string>
      <key>system-entities</key><array><dict><key>dev-entry</key><string>/dev/disk5</string></dict></array>
    </dict>
  </array>
  <key>revision</key><string>683.160.3.701.2</string>
</dict></plist>)");
        const QList<MountedImage> images = parseMountedImages(plist);
        QCOMPARE(images.size(), 2);
        QCOMPARE(images[0].imagePath, QStringLiteral("/Users/me/Downloads/ErgoPro2-2.1.0-macos.dmg"));
        QCOMPARE(images[0].mountPoints, QStringList{QStringLiteral("/Volumes/ErgoPro2 2.1.0")});
        QCOMPARE(images[1].imagePath, QStringLiteral("/Users/me/Other.dmg"));
        QVERIFY(images[1].mountPoints.isEmpty()); // attached, not mounted
        QVERIFY(parseMountedImages(QStringLiteral("garbage")).isEmpty());
    }

    // 2.2.1 (owner request): an update dragged over the installed copy is
    // confirmed too - once per new version.
    void eachNewVersionIsConfirmedOnce()
    {
        InstallStartupFacts f;
        f.runningFromInstalledCopy = true;
        f.registerStartAtLoginOnStart = true;
        f.startsAtLogin = true;
        f.startAtLoginPointsToInstalledCopy = true; // installed before (e.g. 2.1.0 -> 2.2.1)
        f.currentVersion = versionNumber(2, 2, 1);
        f.confirmedVersion = versionNumber(2, 1, 0);
        QVERIFY(decideInstallStartupActions(f).confirmInstalled);
        f.installerImageFound = true; // .dmg in Downloads, already ejected
        QVERIFY(decideInstallStartupActions(f).offerToDeleteInstaller);
        f.confirmedVersion = f.currentVersion; // confirmed at the previous start
        QVERIFY(!decideInstallStartupActions(f).confirmInstalled);
        QVERIFY(!decideInstallStartupActions(f).offerToDeleteInstaller);
        f.runningFromInstalledCopy = false; // started from the .dmg
        f.confirmedVersion = 0;
        QVERIFY(!decideInstallStartupActions(f).confirmInstalled);
        QCOMPARE(versionNumber(2, 2, 1), 20201);
    }
};

QTEST_GUILESS_MAIN(TestInstallPolicy)
#include "tst_installpolicy.moc"
