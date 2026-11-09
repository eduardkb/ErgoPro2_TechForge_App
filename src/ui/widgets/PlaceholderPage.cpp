#include "ui/widgets/PlaceholderPage.h"

#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

namespace ergo {

PlaceholderPage::PlaceholderPage(const QString &iconResource, const QString &title, const QString &text,
                                 QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(10);
    layout->addStretch(1);

    auto *icon = new QLabel(this);
    icon->setPixmap(QIcon(iconResource).pixmap(QSize(72, 72), devicePixelRatioF()));
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName(QStringLiteral("placeholderTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto *textLabel = new QLabel(text, this);
    textLabel->setObjectName(QStringLiteral("placeholderText"));
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    layout->addStretch(2);
}

} // namespace ergo
