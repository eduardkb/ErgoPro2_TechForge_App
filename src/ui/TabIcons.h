#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace ergo {

// Tab icon from a single-color SVG (the stroke/fill color #0D9488 is replaced):
// 'normalColor' for unselected tabs, 'selectedColor' for the selected tab
// (QTabBar draws the selected tab's icon in the QIcon::On state).
QIcon makeTabIcon(const QString &svgResource, const QColor &normalColor, const QColor &selectedColor);

} // namespace ergo
