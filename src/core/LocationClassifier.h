#pragma once

#include <QString>
#include <QStringList>

namespace ergo {

// Detects whether the application runs from a location that is not meant to be
// permanent: a temporary folder, the Downloads folder, a mounted disk image or
// macOS App Translocation (B.1.3: warn and suggest Install).
//
// Pure function: the OS folders are passed in so it can be unit-tested.
struct LocationCheck {
    enum class Kind { Permanent, Temporary, Downloads, DiskImageOrRemovable, Translocated };
    Kind kind = Kind::Permanent;

    bool isWarning() const { return kind != Kind::Permanent; }
};

struct LocationRoots {
    QStringList temporary; // e.g. QStandardPaths::TempLocation, /tmp
    QStringList downloads; // e.g. QStandardPaths::DownloadLocation
    QStringList removable; // e.g. "/Volumes" on macOS (mounted .dmg files, USB drives)
    Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
};

LocationCheck classifyLocation(const QString &path, const LocationRoots &roots);

// Collects the roots for the current OS (uses QStandardPaths; no OS headers).
LocationRoots currentLocationRoots();

} // namespace ergo
