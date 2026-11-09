#pragma once

// File helpers shared by the installer implementations of all OSes.

#include "platform/PlatformResult.h"

#include <QString>

namespace ergo::fileops {

// Removes a file or a folder (recursively). Succeeds if the path does not exist.
PlatformResult removePath(const QString &path);

// Copies a folder recursively (dst must not exist). Symbolic links are copied
// as links where the OS supports it.
PlatformResult copyDirectory(const QString &source, const QString &destination);

// Copies a file, replacing the destination, and optionally marks it executable.
PlatformResult copyFile(const QString &source, const QString &destination, bool executable);

// Writes a text file atomically (QSaveFile).
PlatformResult writeTextFile(const QString &path, const QString &content, bool executable = false);

// Starts a detached helper that waits until this process has exited and then
// deletes 'path' (used by Uninstall when running from the installed copy).
PlatformResult scheduleDeletionAfterExit(const QString &path);

// True if both paths refer to the same existing file/folder.
bool samePath(const QString &a, const QString &b);

} // namespace ergo::fileops
