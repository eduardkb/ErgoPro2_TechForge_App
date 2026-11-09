#include "ui/widgets/CardWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace ergo {

CardWidget::CardWidget(const QString &title, const QString &subtitle, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(20, 16, 20, 18);
    outer->setSpacing(12);

    m_header = new QHBoxLayout;
    m_header->setSpacing(8);
    auto *titles = new QVBoxLayout;
    titles->setSpacing(2);
    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    titles->addWidget(titleLabel);
    if (!subtitle.isEmpty()) {
        auto *subtitleLabel = new QLabel(subtitle, this);
        subtitleLabel->setObjectName(QStringLiteral("cardSubtitle"));
        subtitleLabel->setWordWrap(true);
        titles->addWidget(subtitleLabel);
    }
    m_header->addLayout(titles, 1);
    outer->addLayout(m_header);

    m_content = new QVBoxLayout;
    m_content->setSpacing(10);
    outer->addLayout(m_content);
}

void CardWidget::setHeaderWidget(QWidget *widget)
{
    m_header->addWidget(widget, 0, Qt::AlignTop | Qt::AlignRight);
}

} // namespace ergo
