#include "core/LocationClassifier.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

using namespace ergo;

class TestLocationClassifier : public QObject {
    Q_OBJECT
private:
    static LocationRoots roots()
    {
        LocationRoots r;
        r.temporary = {QStringLiteral("/private/var/folders/ab/T"), QStringLiteral("/tmp")};
        r.downloads = {QStringLiteral("/Users/me/Downloads")};
        r.removable = {QStringLiteral("/Volumes")};
        return r;
    }

private slots:
    void classify_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<int>("kind");

        using K = LocationCheck::Kind;
        QTest::newRow("applications") << "/Users/me/Applications/ErgoPro2.app" << int(K::Permanent);
        QTest::newRow("build folder") << "/Users/me/dev/ErgoPro2/build/bin/ErgoPro2.app" << int(K::Permanent);
        QTest::newRow("downloads") << "/Users/me/Downloads/ErgoPro2.app" << int(K::Downloads);
        QTest::newRow("downloads subfolder") << "/Users/me/Downloads/x/ErgoPro2.app" << int(K::Downloads);
        QTest::newRow("downloads lookalike") << "/Users/me/DownloadsArchive/ErgoPro2.app" << int(K::Permanent);
        QTest::newRow("temp") << "/private/var/folders/ab/T/ErgoPro2.app" << int(K::Temporary);
        QTest::newRow("tmp") << "/tmp/ErgoPro2" << int(K::Temporary);
        QTest::newRow("dmg") << "/Volumes/ErgoPro2/ErgoPro2.app" << int(K::DiskImageOrRemovable);
        QTest::newRow("translocated")
            << "/private/var/folders/xy/T/AppTranslocation/1234/d/ErgoPro2.app" << int(K::Translocated);
    }

    void classify()
    {
        QFETCH(QString, path);
        QFETCH(int, kind);
        QCOMPARE(int(classifyLocation(path, roots()).kind), kind);
        QCOMPARE(classifyLocation(path, roots()).isWarning(), kind != int(LocationCheck::Kind::Permanent));
    }

    void caseInsensitiveOnWindows()
    {
        LocationRoots r;
        r.downloads = {QStringLiteral("C:/Users/Me/Downloads")};
        r.caseSensitivity = Qt::CaseInsensitive;
        QCOMPARE(classifyLocation(QStringLiteral("c:/users/me/downloads/ErgoPro2/ErgoPro2.exe"), r).kind,
                 LocationCheck::Kind::Downloads);
        r.caseSensitivity = Qt::CaseSensitive;
        QCOMPARE(classifyLocation(QStringLiteral("c:/users/me/downloads/ErgoPro2/ErgoPro2.exe"), r).kind,
                 LocationCheck::Kind::Permanent);
    }

    void rootSlashIsIgnored()
    {
        LocationRoots r;
        r.temporary = {QStringLiteral("/")};
        QCOMPARE(classifyLocation(QStringLiteral("/Applications/ErgoPro2.app"), r).kind,
                 LocationCheck::Kind::Permanent);
    }

    void fileSystemRootIsIgnored()
    {
        // Regression (v1.1.1): on Windows the root is a drive ("C:/"), which was
        // trimmed to "C:" (= current folder of drive C) and then matched every path.
        // QDir::rootPath() is "/" on macOS/Linux and the system drive on Windows.
        LocationRoots r;
        r.temporary = {QDir::rootPath()};
        r.downloads = {QDir::rootPath()};
        r.removable = {QDir::rootPath()};
        r.caseSensitivity = Qt::CaseInsensitive;
        const QString app = QDir(QDir::rootPath()).filePath(QStringLiteral("SomeFolder/ErgoPro2/ErgoPro2"));
        QCOMPARE(classifyLocation(app, r).kind, LocationCheck::Kind::Permanent);
    }

    void trailingSlashOnRootsIsIgnored()
    {
        LocationRoots r;
        r.downloads = {QStringLiteral("/Users/me/Downloads/")};
        QCOMPARE(classifyLocation(QStringLiteral("/Users/me/Downloads/ErgoPro2.app"), r).kind,
                 LocationCheck::Kind::Downloads);
    }

    void realTemporaryFolderIsDetected()
    {
        // Uses the real OS roots (symlinks such as /var -> /private/var are resolved).
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString app = QDir(dir.path()).filePath(QStringLiteral("ErgoPro2"));
        QVERIFY(QDir().mkpath(app));
        QCOMPARE(classifyLocation(app, currentLocationRoots()).kind, LocationCheck::Kind::Temporary);
    }
};

QTEST_GUILESS_MAIN(TestLocationClassifier)
#include "tst_locationclassifier.moc"
