#pragma once

#include <QColor>
#include <QObject>

class QApplication;

namespace ergo {

// Color tokens of the current theme. Used by the stylesheet (@token@
// placeholders in resources/style.qss) and by custom-painted widgets.
struct ThemeColors {
    bool dark = false;
    QColor background;
    QColor surface;       // cards
    QColor surfaceAlt;    // inputs, tab bar
    QColor border;
    QColor text;
    QColor textMuted;
    QColor accent;
    QColor accentHover;
    QColor accentText;    // text on accent backgrounds
    QColor success;
    QColor warning;
    QColor warningBg;     // background of warning banners
    QColor danger;
    QColor dangerHover;
};

// Applies the Fusion style, a palette and resources/style.qss, and follows
// the OS light/dark color scheme at runtime.
class Theme : public QObject {
    Q_OBJECT
public:
    static Theme &instance();

    void install(QApplication &app);
    const ThemeColors &colors() const { return m_colors; }

signals:
    void changed();

private:
    Theme() = default;
    void apply();
    static ThemeColors lightColors();
    static ThemeColors darkColors();
    static bool systemPrefersDark();

    QApplication *m_app = nullptr;
    ThemeColors m_colors;
};

} // namespace ergo
