#include "ui/TabIcons.h"

#include "core/LogCategories.h"

#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

namespace ergo {

namespace {

QPixmap render(const QByteArray &svg, int size)
{
    QSvgRenderer renderer(svg);
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    return pixmap;
}

} // namespace

QIcon makeTabIcon(const QString &svgResource, const QColor &normalColor, const QColor &selectedColor)
{
    QFile file(svgResource);
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(lcUi) << "Tab icon resource missing:" << svgResource;
        return {};
    }
    const QByteArray source = file.readAll();
    QIcon icon;
    for (const auto &[state, color] : {std::pair{QIcon::Off, normalColor}, std::pair{QIcon::On, selectedColor}}) {
        QByteArray svg = source;
        svg.replace("#0D9488", color.name(QColor::HexRgb).toLatin1());
        // 1x and 2x (Retina) sizes around the tab icon size.
        for (int size : {16, 18, 20, 32, 36, 40})
            icon.addPixmap(render(svg, size), QIcon::Normal, state);
    }
    return icon;
}

} // namespace ergo
