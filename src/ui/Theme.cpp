#include "ui/Theme.h"

#include "core/LogCategories.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <QStyleHints>

namespace ergo {

Theme &Theme::instance()
{
    static Theme theme;
    return theme;
}

ThemeColors Theme::lightColors()
{
    ThemeColors c;
    c.dark = false;
    c.background = QColor(0xF3, 0xF5, 0xF8);
    c.surface = QColor(0xFF, 0xFF, 0xFF);
    c.surfaceAlt = QColor(0xEE, 0xF1, 0xF5);
    c.border = QColor(0xDF, 0xE4, 0xEB);
    c.text = QColor(0x0F, 0x17, 0x2A);
    c.textMuted = QColor(0x5B, 0x67, 0x7A);
    c.accent = QColor(0x0D, 0x94, 0x88);
    c.accentHover = QColor(0x0F, 0x76, 0x6E);
    c.accentText = QColor(0xFF, 0xFF, 0xFF);
    c.success = QColor(0x15, 0x80, 0x3D);
    c.warning = QColor(0xB4, 0x53, 0x09);
    c.warningBg = QColor(0xFF, 0xF7, 0xE6);
    c.danger = QColor(0xDC, 0x26, 0x26);
    c.dangerHover = QColor(0xB9, 0x1C, 0x1C);
    return c;
}

ThemeColors Theme::darkColors()
{
    ThemeColors c;
    c.dark = true;
    c.background = QColor(0x0F, 0x14, 0x19);
    c.surface = QColor(0x18, 0x1F, 0x27);
    c.surfaceAlt = QColor(0x21, 0x2A, 0x34);
    c.border = QColor(0x2C, 0x37, 0x43);
    c.text = QColor(0xE6, 0xED, 0xF3);
    c.textMuted = QColor(0x94, 0xA3, 0xB8);
    c.accent = QColor(0x2D, 0xD4, 0xBF);
    c.accentHover = QColor(0x5E, 0xEA, 0xD4);
    c.accentText = QColor(0x04, 0x2F, 0x2B);
    c.success = QColor(0x4A, 0xDE, 0x80);
    c.warning = QColor(0xFB, 0xBF, 0x24);
    c.warningBg = QColor(0x2E, 0x26, 0x12);
    c.danger = QColor(0xDC, 0x26, 0x26);
    c.dangerHover = QColor(0xEF, 0x44, 0x44);
    return c;
}

bool Theme::systemPrefersDark()
{
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark)
        return true;
    if (scheme == Qt::ColorScheme::Light)
        return false;
    // Unknown: fall back to the platform palette's window lightness.
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

void Theme::install(QApplication &app)
{
    m_app = &app;
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
        QApplication::setStyle(fusion);
    else
        qCWarning(lcUi) << "Fusion style not available, using the platform style";

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
        qCInfo(lcUi) << "OS color scheme changed:" << scheme;
        apply();
    });
    apply();
}

void Theme::apply()
{
    if (!m_app)
        return;
    m_colors = systemPrefersDark() ? darkColors() : lightColors();
    const ThemeColors &c = m_colors;

    QPalette pal;
    pal.setColor(QPalette::Window, c.background);
    pal.setColor(QPalette::WindowText, c.text);
    pal.setColor(QPalette::Base, c.surface);
    pal.setColor(QPalette::AlternateBase, c.surfaceAlt);
    pal.setColor(QPalette::Text, c.text);
    pal.setColor(QPalette::Button, c.surfaceAlt);
    pal.setColor(QPalette::ButtonText, c.text);
    pal.setColor(QPalette::Highlight, c.accent);
    pal.setColor(QPalette::HighlightedText, c.accentText);
    pal.setColor(QPalette::ToolTipBase, c.surface);
    pal.setColor(QPalette::ToolTipText, c.text);
    pal.setColor(QPalette::PlaceholderText, c.textMuted);
    pal.setColor(QPalette::Link, c.accent);
    pal.setColor(QPalette::Mid, c.border);
    pal.setColor(QPalette::Light, c.surface);
    pal.setColor(QPalette::Dark, c.border);
    pal.setColor(QPalette::Disabled, QPalette::Text, c.textMuted);
    pal.setColor(QPalette::Disabled, QPalette::WindowText, c.textMuted);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, c.textMuted);
    QApplication::setPalette(pal);

    QFile qss(QStringLiteral(":/style.qss"));
    if (!qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCCritical(lcUi) << "Cannot load stylesheet :/style.qss:" << qss.errorString();
    } else {
        QString sheet = QString::fromUtf8(qss.readAll());
        const auto token = [&sheet](const char *name, const QColor &color) {
            sheet.replace(QStringLiteral("@%1@").arg(QLatin1String(name)), color.name(QColor::HexRgb));
        };
        token("background", c.background);
        token("surface", c.surface);
        token("surfaceAlt", c.surfaceAlt);
        token("border", c.border);
        token("text", c.text);
        token("textMuted", c.textMuted);
        token("accent", c.accent);
        token("accentHover", c.accentHover);
        token("accentText", c.accentText);
        token("success", c.success);
        token("warningBg", c.warningBg);
        token("warning", c.warning);
        token("danger", c.danger);
        token("dangerHover", c.dangerHover);
        m_app->setStyleSheet(sheet);
    }

    qCInfo(lcUi) << "Theme applied:" << (c.dark ? "dark" : "light");
    emit changed();
}

} // namespace ergo
