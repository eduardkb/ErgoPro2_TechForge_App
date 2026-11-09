#pragma once

#include <QWidget>

namespace ergo {

// Centered placeholder for tabs whose feature is implemented in a later stage.
class PlaceholderPage : public QWidget {
    Q_OBJECT
public:
    PlaceholderPage(const QString &iconResource, const QString &title, const QString &text,
                    QWidget *parent = nullptr);
};

} // namespace ergo
