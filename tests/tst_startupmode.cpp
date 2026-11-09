#include "core/StartupMode.h"
#include "core/settings/GeneralSettings.h"

#include <QTest>

using namespace ergo;

class TestStartupMode : public QObject {
    Q_OBJECT
private slots:
    void decide_data()
    {
        QTest::addColumn<bool>("autostart");
        QTest::addColumn<QString>("autoMode");
        QTest::addColumn<QString>("manualMode");
        QTest::addColumn<bool>("tray");
        QTest::addColumn<bool>("showWindow");

        const QString tray = GeneralSettings::StartupTray;
        const QString window = GeneralSettings::StartupWindow;
        // Defaults: hide at login, show when started manually.
        QTest::newRow("login, default") << true << tray << window << true << false;
        QTest::newRow("manual, default") << false << tray << window << true << true;
        // User changed the options.
        QTest::newRow("login, show") << true << window << window << true << true;
        QTest::newRow("manual, hide") << false << tray << tray << true << false;
        // No system tray: always show, otherwise the app would be unreachable.
        QTest::newRow("login, no tray") << true << tray << tray << false << true;
        QTest::newRow("manual, no tray") << false << tray << tray << false << true;
    }

    void decide()
    {
        QFETCH(bool, autostart);
        QFETCH(QString, autoMode);
        QFETCH(QString, manualMode);
        QFETCH(bool, tray);
        QFETCH(bool, showWindow);
        const StartupWindowMode mode = decideStartupWindowMode(autostart, autoMode, manualMode, tray);
        QCOMPARE(mode == StartupWindowMode::ShowWindow, showWindow);
    }
};

QTEST_GUILESS_MAIN(TestStartupMode)
#include "tst_startupmode.moc"
