#pragma once

#include <QFrame>

class QHBoxLayout;
class QLabel;
class QVBoxLayout;

namespace ergo {

// Rounded "card" section with a title, optional subtitle, optional header
// widget on the right (e.g. a status badge) and a content area.
class CardWidget : public QFrame {
    Q_OBJECT
public:
    explicit CardWidget(const QString &title, const QString &subtitle = {}, QWidget *parent = nullptr);

    // Layout to which the card's content is added.
    QVBoxLayout *contentLayout() const { return m_content; }

    // Places a widget at the right side of the title row.
    void setHeaderWidget(QWidget *widget);

private:
    QHBoxLayout *m_header = nullptr;
    QVBoxLayout *m_content = nullptr;
};

} // namespace ergo
