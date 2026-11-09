#pragma once

#include <QDir>
#include <QString>

namespace ergo {

// Native path with zero-width spaces after each separator, so word-wrapped
// labels can break long paths instead of growing wider than the window.
inline QString wrappablePath(const QString &path)
{
    QString native = QDir::toNativeSeparators(path);
    const QChar separator = QDir::separator();
    native.replace(separator, QString(separator) + QChar(0x200B));
    return native;
}

} // namespace ergo
