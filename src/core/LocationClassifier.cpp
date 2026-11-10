#include "core/LocationClassifier.h"

#include "core/LogCategories.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace ergo {

namespace {

// Normalizes a path: absolute, symlinks resolved through the deepest existing
// ancestor (so "/tmp/x" and the root "/tmp" both become "/private/tmp/..." on
// macOS even if "x" does not exist), '/' separators, no trailing '/' except for
// a file-system root ("/" or a Windows drive such as "C:/").
//
// Parents are taken with QFileInfo::path() and roots detected with
// QDir::isRoot(): cutting at the last '/' would turn "C:/Users" into "C:",
// which on Windows means "the current folder of drive C" (bug fixed in 1.1.1).
QString normalize(const QString &path)
{
    QString existing = QDir::cleanPath(QDir::fromNativeSeparators(QFileInfo(path).absoluteFilePath()));
    QStringList missing; // trailing components that do not exist (yet)
    while (!QFileInfo::exists(existing) && !QDir(existing).isRoot()) {
        const QFileInfo info(existing);
        const QString parent = info.path();
        if (parent == existing || parent.isEmpty())
            break;
        missing.prepend(info.fileName());
        existing = parent;
    }
    const QString base = QFileInfo::exists(existing) ? QFileInfo(existing).canonicalFilePath() : existing;
    QString result = missing.isEmpty() ? base : QDir(base).filePath(missing.join(QLatin1Char('/')));
    result = QDir::fromNativeSeparators(QDir::cleanPath(result));
    while (result.endsWith(QLatin1Char('/')) && !QDir(result).isRoot())
        result.chop(1);
    return result;
}

bool isUnder(const QString &path, const QString &root, Qt::CaseSensitivity cs)
{
    if (root.isEmpty())
        return false;
    const QString r = normalize(root);
    if (QDir(r).isRoot())
        return false; // never treat a whole file system / drive as a root
    return path.compare(r, cs) == 0 || path.startsWith(r + QLatin1Char('/'), cs);
}

bool isUnderAny(const QString &path, const QStringList &roots, Qt::CaseSensitivity cs)
{
    for (const QString &root : roots) {
        if (isUnder(path, root, cs))
            return true;
    }
    return false;
}

} // namespace

LocationCheck classifyLocation(const QString &path, const LocationRoots &roots)
{
    LocationCheck check;
    const QString p = normalize(path);

    // macOS runs quarantined apps from a random read-only copy ("App Translocation").
    if (p.contains(QLatin1String("/AppTranslocation/"), roots.caseSensitivity))
        check.kind = LocationCheck::Kind::Translocated;
    else if (isUnderAny(p, roots.downloads, roots.caseSensitivity))
        check.kind = LocationCheck::Kind::Downloads;
    else if (isUnderAny(p, roots.temporary, roots.caseSensitivity))
        check.kind = LocationCheck::Kind::Temporary;
    else if (isUnderAny(p, roots.removable, roots.caseSensitivity))
        check.kind = LocationCheck::Kind::DiskImageOrRemovable;
    qCDebug(lcApp).noquote() << "Location check:" << p << "-> kind" << static_cast<int>(check.kind);
    return check;
}

LocationRoots currentLocationRoots()
{
    LocationRoots roots;
    roots.temporary << QStandardPaths::standardLocations(QStandardPaths::TempLocation) << QDir::tempPath();
    roots.downloads << QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
    // Unix temporary folders that may differ from TMPDIR.
    for (const char *dir : {"/tmp", "/var/tmp", "/private/tmp", "/private/var/tmp"}) {
        if (QFileInfo::exists(QString::fromLatin1(dir)))
            roots.temporary << QString::fromLatin1(dir);
    }
    // macOS mounts disk images and external drives under /Volumes;
    // Linux uses /media and /run/media for removable media.
    for (const char *dir : {"/Volumes", "/media", "/run/media"}) {
        if (QFileInfo::exists(QString::fromLatin1(dir)))
            roots.removable << QString::fromLatin1(dir);
    }
    // Windows file systems are case-insensitive.
    roots.caseSensitivity = QFileInfo::exists(QStringLiteral("C:/Windows")) ? Qt::CaseInsensitive : Qt::CaseSensitive;
    roots.temporary.removeDuplicates();
    return roots;
}

} // namespace ergo
