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
};

QTEST_GUILESS_MAIN(TestInstallPolicy)
#include "tst_installpolicy.moc"
