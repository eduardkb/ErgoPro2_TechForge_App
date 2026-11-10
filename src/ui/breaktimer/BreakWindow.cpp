#include "ui/breaktimer/BreakWindow.h"

#include "core/LogCategories.h"
#include "ui/TabIcons.h"
#include "ui/Theme.h"

#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace ergo {

namespace {

struct Tip {
    const char *image;
    const char *title;
    const char *text;
};

const Tip kTips[] = {
    {":/icons/stretch/wrist-flexor.svg", QT_TRANSLATE_NOOP("ergo::BreakWindow", "Wrist flexor stretch"),
     QT_TRANSLATE_NOOP("ergo::BreakWindow", "Arm straight, palm up. Gently pull the fingers down with the other "
                                            "hand. 15 s each side.")},
    {":/icons/stretch/wrist-extensor.svg", QT_TRANSLATE_NOOP("ergo::BreakWindow", "Wrist extensor stretch"),
     QT_TRANSLATE_NOOP("ergo::BreakWindow", "Arm straight, palm down. Gently press the back of the hand down. "
                                            "15 s each side.")},
    {":/icons/stretch/fist-fan.svg", QT_TRANSLATE_NOOP("ergo::BreakWindow", "Fist and fan"),
     QT_TRANSLATE_NOOP("ergo::BreakWindow", "Make a soft fist, then spread the fingers wide. Repeat 10 times.")},
    {":/icons/stretch/shoulder-rolls.svg", QT_TRANSLATE_NOOP("ergo::BreakWindow", "Shoulder rolls"),
     QT_TRANSLATE_NOOP("ergo::BreakWindow", "Roll your shoulders slowly backwards 10 times.")},
    {":/icons/stretch/look-away.svg", QT_TRANSLATE_NOOP("ergo::BreakWindow", "Look into the distance"),
     QT_TRANSLATE_NOOP("ergo::BreakWindow", "Look at something about 6 m away for 20 seconds.")},
};

QString formatRemaining(qint64 ms)
{
    const qint64 seconds = (std::max<qint64>(0, ms) + 999) / 1000; // round up: never shows 00:00 early
    return QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

} // namespace

BreakWindow::BreakWindow(bool mainScreen, QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint)
    , m_main(mainScreen)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(tr("ErgoPro2 – Break"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->addStretch(1);
    if (m_main) {
        layout->addWidget(buildMainCard(), 0, Qt::AlignHCenter);
    } else {
        auto *note = new QLabel(tr("Time for a break – see the main screen"), this);
        note->setObjectName(QStringLiteral("breakNote"));
        note->setAlignment(Qt::AlignCenter);
        layout->addWidget(note);
        m_countdown = new QLabel(this);
        m_countdown->setObjectName(QStringLiteral("breakCountdownSmall"));
        m_countdown->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_countdown);
    }
    layout->addStretch(1);
}

QWidget *BreakWindow::buildMainCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("breakCard"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setMaximumWidth(980);
    auto *l = new QVBoxLayout(card);
    l->setContentsMargins(36, 30, 36, 30);
    l->setSpacing(14);

    auto *title = new QLabel(tr("Time to rest your wrists"), card);
    title->setObjectName(QStringLiteral("breakTitle"));
    title->setAlignment(Qt::AlignCenter);
    l->addWidget(title);
    auto *subtitle = new QLabel(tr("Take your hands off the mouse and keyboard. Try these stretches:"), card);
    subtitle->setObjectName(QStringLiteral("hint"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    l->addWidget(subtitle);

    m_countdown = new QLabel(card);
    m_countdown->setObjectName(QStringLiteral("breakCountdown"));
    m_countdown->setAlignment(Qt::AlignCenter);
    l->addWidget(m_countdown);

    l->addWidget(buildTips(card));

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto *dismiss = new QPushButton(tr("Dismiss"), card);
    m_postpone = new QPushButton(card);
    m_postpone->setProperty("variant", QStringLiteral("primary"));
    for (QPushButton *b : {dismiss, m_postpone}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumWidth(170);
        b->setAutoDefault(false);
        buttons->addWidget(b);
    }
    buttons->addStretch(1);
    l->addLayout(buttons);
    auto *note = new QLabel(tr("Dismiss skips this break and starts a new work period."), card);
    note->setObjectName(QStringLiteral("hint"));
    note->setAlignment(Qt::AlignCenter);
    l->addWidget(note);

    connect(dismiss, &QPushButton::clicked, this, &BreakWindow::dismissClicked);
    connect(m_postpone, &QPushButton::clicked, this, &BreakWindow::postponeClicked);
    return card;
}

QWidget *BreakWindow::buildTips(QWidget *parent)
{
    auto *tips = new QWidget(parent);
    auto *grid = new QGridLayout(tips);
    grid->setContentsMargins(0, 6, 0, 6);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(10);
    const QColor accent = Theme::instance().colors().accent;
    int column = 0;
    for (const Tip &tip : kTips) {
        auto *tile = new QFrame(tips);
        tile->setObjectName(QStringLiteral("breakTip"));
        tile->setAttribute(Qt::WA_StyledBackground, true);
        auto *v = new QVBoxLayout(tile);
        v->setContentsMargins(12, 12, 12, 12);
        v->setSpacing(6);
        auto *image = new QLabel(tile);
        image->setAlignment(Qt::AlignCenter);
        image->setPixmap(renderTintedSvg(QString::fromLatin1(tip.image), accent, QSize(128, 88), devicePixelRatioF()));
        image->setMinimumHeight(92);
        auto *name = new QLabel(tr(tip.title), tile);
        name->setObjectName(QStringLiteral("valueLabel"));
        name->setAlignment(Qt::AlignCenter);
        name->setWordWrap(true);
        auto *text = new QLabel(tr(tip.text), tile);
        text->setObjectName(QStringLiteral("hint"));
        text->setAlignment(Qt::AlignCenter);
        text->setWordWrap(true);
        v->addWidget(image);
        v->addWidget(name);
        v->addWidget(text, 1);
        tile->setMinimumWidth(150);
        grid->setColumnStretch(column, 1); // equal columns
        grid->addWidget(tile, 0, column++);
    }
    return tips;
}

void BreakWindow::setRemaining(qint64 ms)
{
    if (m_countdown)
        m_countdown->setText(formatRemaining(ms));
}

void BreakWindow::setPostponeMinutes(int minutes)
{
    if (m_postpone)
        m_postpone->setText(tr("Postpone %n min", nullptr, minutes));
}

void BreakWindow::paintEvent(QPaintEvent *)
{
    // Semi-transparent dimming of the whole screen.
    QPainter p(this);
    p.fillRect(rect(), QColor(8, 14, 20, Theme::instance().colors().dark ? 215 : 190));
}

void BreakWindow::closeEvent(QCloseEvent *event)
{
    if (m_allowClose) {
        event->accept();
        return;
    }
    // Alt+F4, Cmd+W, window-manager close: the break can only end via the
    // buttons or the countdown (B-05).
    qCInfo(lcUi) << "Break window: close request ignored";
    event->ignore();
}

void BreakWindow::keyPressEvent(QKeyEvent *event)
{
    // Esc / Enter must not end the break.
    event->accept();
}

} // namespace ergo
