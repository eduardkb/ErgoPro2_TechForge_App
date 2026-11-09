// Renders an SVG into PNG sizes for a macOS .iconset, a Windows .ico and a
// Linux PNG icon.
//
// Usage: ergopro2_icongen <input.svg> <output-dir>
// Output: <output-dir>/ErgoPro2.iconset/*.png, <output-dir>/ErgoPro2.ico,
//         <output-dir>/ergopro2.png (256x256)

#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QImageWriter>
#include <QPainter>
#include <QSvgRenderer>
#include <QTextStream>

namespace {

QImage render(QSvgRenderer &renderer, int size)
{
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    renderer.render(&painter);
    return image;
}

bool save(const QImage &image, const QString &path, const char *format)
{
    QImageWriter writer(path, format);
    if (!writer.write(image)) {
        QTextStream(stderr) << "Cannot write " << path << ": " << writer.errorString() << "\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() != 3) {
        QTextStream(stderr) << "Usage: ergopro2_icongen <input.svg> <output-dir>\n";
        return 2;
    }

    QSvgRenderer renderer(args.at(1));
    if (!renderer.isValid()) {
        QTextStream(stderr) << "Invalid SVG: " << args.at(1) << "\n";
        return 1;
    }

    const QDir out(args.at(2));
    const QString iconset = out.filePath(QStringLiteral("ErgoPro2.iconset"));
    if (!QDir().mkpath(iconset)) {
        QTextStream(stderr) << "Cannot create " << iconset << "\n";
        return 1;
    }

    // macOS iconset: 16, 32, 128, 256, 512 at @1x and @2x.
    bool ok = true;
    for (int base : {16, 32, 128, 256, 512}) {
        ok &= save(render(renderer, base), QDir(iconset).filePath(QStringLiteral("icon_%1x%1.png").arg(base)), "png");
        ok &= save(render(renderer, base * 2), QDir(iconset).filePath(QStringLiteral("icon_%1x%1@2x.png").arg(base)),
                   "png");
    }
    ok &= save(render(renderer, 256), out.filePath(QStringLiteral("ErgoPro2.ico")), "ico");
    ok &= save(render(renderer, 256), out.filePath(QStringLiteral("ergopro2.png")), "png");
    return ok ? 0 : 1;
}
