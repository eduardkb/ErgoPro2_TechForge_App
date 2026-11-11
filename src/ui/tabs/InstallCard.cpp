#include "ui/tabs/InstallCard.h"

#include "ui/InstallController.h"
#include "ui/TextUtils.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace ergo {

InstallCard::InstallCard(InstallController &controller, QWidget *parent)
    : CardWidget(tr("Installation"),
                 tr("Install copies ErgoPro2 to a permanent folder in your user account and starts it "
                    "automatically when you log in. No administrator rights are needed."),
                 parent)
    , m_controller(controller)
{
    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout()->addWidget(m_status);

    m_warning = new QLabel(this);
    m_warning->setObjectName(QStringLiteral("warningText"));
    m_warning->setWordWrap(true);
    contentLayout()->addWidget(m_warning);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    m_uninstall = new QPushButton(tr("Uninstall"), this);
    m_install = new QPushButton(tr("Install"), this);
    m_install->setProperty("variant", QStringLiteral("primary"));
    for (QPushButton *b : {m_uninstall, m_install}) {
        b->setCursor(Qt::PointingHandCursor);
        buttons->addWidget(b);
    }
    contentLayout()->addLayout(buttons);

    connect(m_install, &QPushButton::clicked, this, [this] { m_controller.install(window()); });
    connect(m_uninstall, &QPushButton::clicked, this, [this] { m_controller.uninstall(window()); });
    connect(&m_controller, &InstallController::changed, this, &InstallCard::refresh);
    refresh();
}

void InstallCard::refresh()
{
    const InstallController::Status s = m_controller.status();
    const QString installed = wrappablePath(s.install.installLocation);
    const QString current = wrappablePath(s.install.currentLocation);

    if (s.install.installed && s.startsAtLogin)
        m_status->setText(tr("✅ Installed at %1, starts at login.").arg(installed));
    else if (s.install.installed)
        m_status->setText(tr("Installed at %1, but not starting at login. Click Install to repair.").arg(installed));
    else if (s.startsAtLogin)
        m_status->setText(tr("Not installed, but registered to start at login. Click Install to repair or "
                             "Uninstall to remove the registration."));
    else
        m_status->setText(tr("Not installed – running from %1.").arg(current));

    if (s.install.managedByInstaller)
        m_status->setText(m_status->text() + QLatin1Char('\n') + tr("Installed with the ErgoPro2 setup program."));
    if (!s.install.runningFromInstalledCopy && s.install.installed)
        m_status->setText(m_status->text() + QLatin1Char('\n') + tr("This window runs from %1.").arg(current));

    QString warning;
    switch (s.location.kind) {
    case LocationCheck::Kind::Permanent:
        break;
    case LocationCheck::Kind::Temporary:
        warning = tr("⚠ ErgoPro2 is running from a temporary folder, which may be deleted. Click Install to copy it "
                     "to a permanent folder.");
        break;
    case LocationCheck::Kind::Downloads:
        warning = tr("⚠ ErgoPro2 is running from your Downloads folder. Click Install to copy it to a permanent "
                     "folder.");
        break;
    case LocationCheck::Kind::DiskImageOrRemovable:
        warning = tr("⚠ ErgoPro2 is running from a disk image or removable drive. Click Install to copy it to "
                     "your computer.");
        break;
    case LocationCheck::Kind::Translocated:
        warning = tr("⚠ macOS is running ErgoPro2 from a temporary, read-only copy (App Translocation). Click "
                     "Install to copy it to your Applications folder.");
        break;
    }
    m_warning->setText(warning);
    m_warning->setVisible(!warning.isEmpty());

    const bool complete = s.install.installed && s.startsAtLogin;
    m_install->setText(s.install.installed ? tr("Reinstall") : tr("Install"));
    m_install->setEnabled(!(complete && s.install.runningFromInstalledCopy));
    m_uninstall->setEnabled(s.install.installed || s.startsAtLogin);
}

} // namespace ergo
