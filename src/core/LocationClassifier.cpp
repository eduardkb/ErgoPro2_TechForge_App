#include "core/LocationClassifier.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace ergo {

namespace {

// Normalizes a path: absolute, symlinks resolved through the deepest existing
// ancestor (so "/tmp/x" and the root "/tmp" both become "/private/tmp/..." on
// macOS even if "x" does not exist), '/' separators, no trailing '/'.
QString normalize(const QString &path)
{
    QString existing = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    QString remainder;
    while (!QFileInfo::exists(existing)) {
        const int slash = existing.lastIndexOf(QLatin1Char('/'));
        if (slash <= 0) {
            existing.clear();
            break;
        }
        remainder = existing.mid(slash) + remainder;
        existing.truncate(slash);
    }
    QString result = existing.isEmpty() ? QDir::cleanPath(QFileInfo(path).absoluteFilePath())
                                        : QFileInfo(existing).canonicalFilePath() + remainder;
    result = QDir::fromNativeSeparators(QDir::cleanPath(result));
    while (result.size() > 1 && result.endsWith(QLatin1Char('/')))
        result.chop(1);
    return result;
}

bool isUnder(const QString &path, const QString &root, Qt::CaseSensitivity cs)
{
    if (root.isEmpty())
        return false;
    const QString r = normalize(root);
    if (r == QLatin1String("/"))
        return false; // never treat the whole file system as a root
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
