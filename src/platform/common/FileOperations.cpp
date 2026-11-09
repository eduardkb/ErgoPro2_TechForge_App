#include "platform/common/FileOperations.h"

#include "core/LogCategories.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>

namespace ergo::fileops {

PlatformResult removePath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() && !info.isSymLink())
        return PlatformResult::success();

    bool ok = false;
    if (info.isDir() && !info.isSymLink())
        ok = QDir(path).removeRecursively();
    else
        ok = QFile::remove(path);

    if (!ok) {
        const QString message = QCoreApplication::translate("ergo::fileops", "Cannot delete '%1'.")
                                    .arg(QDir::toNativeSeparators(path));
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    qCInfo(lcPlatform).noquote() << "Deleted" << QDir::toNativeSeparators(path);
    return PlatformResult::success();
}

PlatformResult copyDirectory(const QString &source, const QString &destination)
{
    const QDir src(source);
    if (!src.exists())
        return PlatformResult::failed(
            QCoreApplication::translate("ergo::fileops", "Folder '%1' does not exist.").arg(source));
    if (!QDir().mkpath(destination))
        return PlatformResult::failed(QCoreApplication::translate("ergo::fileops", "Cannot create folder '%1'.")
                                          .arg(QDir::toNativeSeparators(destination)));

    QDirIterator it(source, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        const QString target = QDir(destination).filePath(src.relativeFilePath(info.filePath()));
        bool ok = true;
        if (info.isSymLink()) {
            ok = QFile::link(info.symLinkTarget(), target);
        } else if (info.isDir()) {
            ok = QDir().mkpath(target);
        } else {
            ok = QFile::copy(info.filePath(), target);
            if (ok)
                QFile::setPermissions(target, info.permissions());
        }
        if (!ok) {
            const QString message = QCoreApplication::translate("ergo::fileops", "Cannot copy '%1' to '%2'.")
                                        .arg(QDir::toNativeSeparators(info.filePath()),
                                             QDir::toNativeSeparators(target));
            qCCritical(lcPlatform).noquote() << message;
            return PlatformResult::failed(message);
        }
    }
    return PlatformResult::success();
}

PlatformResult copyFile(const QString &source, const QString &destination, bool executable)
{
    if (const PlatformResult removed = removePath(destination); !removed.ok())
        return removed;
    if (!QDir().mkpath(QFileInfo(destination).absolutePath()) || !QFile::copy(source, destination)) {
        const QString message = QCoreApplication::translate("ergo::fileops", "Cannot copy '%1' to '%2'.")
                                    .arg(QDir::toNativeSeparators(source), QDir::toNativeSeparators(destination));
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    if (executable) {
        QFile::setPermissions(destination, QFile::permissions(destination) | QFileDevice::ExeOwner
                                               | QFileDevice::ExeUser | QFileDevice::ExeGroup | QFileDevice::ExeOther);
    }
    return PlatformResult::success();
}

PlatformResult writeTextFile(const QString &path, const QString &content, bool executable)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return PlatformResult::failed(QCoreApplication::translate("ergo::fileops", "Cannot create folder '%1'.")
                                          .arg(QDir::toNativeSeparators(QFileInfo(path).absolutePath())));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString message = QCoreApplication::translate("ergo::fileops", "Cannot write '%1': %2")
                                    .arg(QDir::toNativeSeparators(path), file.errorString());
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        const QString message = QCoreApplication::translate("ergo::fileops", "Cannot save '%1': %2")
                                    .arg(QDir::toNativeSeparators(path), file.errorString());
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    if (executable) {
        QFile::setPermissions(path, QFile::permissions(path) | QFileDevice::ExeOwner | QFileDevice::ExeUser
                                        | QFileDevice::ExeGroup | QFileDevice::ExeOther);
    }
    return PlatformResult::success();
}

PlatformResult scheduleDeletionAfterExit(const QString &path)
{
    const QString pid = QString::number(QCoreApplication::applicationPid());
    QString program;
    QStringList arguments;
#if defined(Q_OS_WIN)
    // PowerShell waits for this process to end, then deletes the folder.
    QString quoted = QDir::toNativeSeparators(path);
    quoted.replace(QLatin1String("'"), QLatin1String("''"));
    program = QStringLiteral("powershell.exe");
    arguments << QStringLiteral("-NoProfile") << QStringLiteral("-NonInteractive") << QStringLiteral("-WindowStyle")
              << QStringLiteral("Hidden") << QStringLiteral("-Command")
              << QStringLiteral("Wait-Process -Id %1 -ErrorAction SilentlyContinue; Start-Sleep -Milliseconds 500; "
                                "Remove-Item -LiteralPath '%2' -Recurse -Force")
                     .arg(pid, quoted);
#else
    // The path and PID are passed as positional arguments ($1, $2), never
    // interpolated into the script, so no quoting issues are possible.
    program = QStringLiteral("/bin/sh");
    arguments << QStringLiteral("-c")
              << QStringLiteral("while kill -0 \"$1\" 2>/dev/null; do sleep 0.5; done; rm -rf -- \"$2\"")
              << QStringLiteral("ergopro2-uninstall") << pid << path;
#endif
    if (!QProcess::startDetached(program, arguments)) {
        const QString message = QCoreApplication::translate("ergo::fileops",
                                                            "Cannot schedule the deletion of '%1' after exit.")
                                    .arg(QDir::toNativeSeparators(path));
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    qCInfo(lcPlatform).noquote() << "Deletion of" << QDir::toNativeSeparators(path) << "scheduled after exit";
    return PlatformResult::success();
}

bool samePath(const QString &a, const QString &b)
{
    const QFileInfo fa(a);
    const QFileInfo fb(b);
    if (!fa.exists() || !fb.exists())
        return false;
    return fa.canonicalFilePath() == fb.canonicalFilePath();
}

} // namespace ergo::fileops
