#include "ui/widgets/ParameterRow.h"

#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>

namespace ergo {

ParameterRow::ParameterRow(const QString &label, int minimum, int maximum, const QString &unit, QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(2);

    auto *name = new QLabel(label, this);
    name->setObjectName(QStringLiteral("valueLabel"));
    name->setMinimumWidth(170);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(minimum, maximum);
    m_slider->setPageStep(qMax(1, (maximum - minimum) / 10));

    m_spin = new QSpinBox(this);
    m_spin->setRange(minimum, maximum);
    m_spin->setFixedWidth(96); // same width in every row, so the sliders line up
    m_spin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *unitLabel = new QLabel(unit, this);
    unitLabel->setObjectName(QStringLiteral("hint"));
    unitLabel->setMinimumWidth(36);

    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("hint"));
    m_hint->setWordWrap(true);
    m_hint->setVisible(false);

    grid->addWidget(name, 0, 0);
    grid->addWidget(m_slider, 0, 1);
    grid->addWidget(m_spin, 0, 2);
    grid->addWidget(unitLabel, 0, 3);
    grid->addWidget(m_hint, 1, 0, 1, 4);
    grid->setColumnStretch(1, 1);

    connect(m_slider, &QSlider::valueChanged, this, &ParameterRow::onUserValue);
    connect(m_spin, &QSpinBox::valueChanged, this, &ParameterRow::onUserValue);
}

int ParameterRow::value() const
{
    return m_spin->value();
}

void ParameterRow::setValue(int value)
{
    const QSignalBlocker blockSlider(m_slider);
    const QSignalBlocker blockSpin(m_spin);
    m_slider->setValue(value);
    m_spin->setValue(value);
}

void ParameterRow::setHint(const QString &hint)
{
    m_hint->setText(hint);
    m_hint->setVisible(!hint.isEmpty());
}

void ParameterRow::onUserValue(int value)
{
    if (m_updating)
        return;
    m_updating = true;
    m_slider->setValue(value);
    m_spin->setValue(value);
    m_updating = false;
    emit valueChanged(value);
}

} // namespace ergo
