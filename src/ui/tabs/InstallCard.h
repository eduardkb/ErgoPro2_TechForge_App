#pragma once

#include "ui/widgets/CardWidget.h"

class QLabel;
class QPushButton;

namespace ergo {

class InstallController;

// "Installation" card of the General tab: status, location warning and the
// Install / Uninstall buttons (B.1.3).
class InstallCard : public CardWidget {
    Q_OBJECT
public:
    InstallCard(InstallController &controller, QWidget *parent = nullptr);

    void refresh();

private:
    InstallController &m_controller;
    QLabel *m_status = nullptr;
    QLabel *m_warning = nullptr;
    QPushButton *m_install = nullptr;
    QPushButton *m_uninstall = nullptr;
};

} // namespace ergo
