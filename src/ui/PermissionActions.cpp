#include "ui/PermissionActions.h"

#include "core/LogCategories.h"
#include "platform/PlatformInterfaces.h"
#include "ui/FeatureAvailabilityService.h"

#include <QCoreApplication>
#include <algorithm>
#include <QMessageBox>
#include <QPointer>

namespace ergo::PermissionActions {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::PermissionActions", text);
}

} // namespace

bool performFix(QWidget *parent, FeatureAvailabilityService &service, const QString &permissionId)
{
    const QList<PermissionState> permissions = service.permissions();
    const auto it = std::find_if(permissions.begin(), permissions.end(),
                                 [&](const PermissionState &p) { return p.id == permissionId; });
    if (it == permissions.end()) {
        qCCritical(lcUi) << "performFix: unknown permission" << permissionId;
        return false;
    }

    if (it->fixKind == FixKind::OpenSettings) {
        const PlatformResult result = service.openSettings(permissionId);
        if (!result.ok()) {
            QMessageBox::warning(parent, tr("Cannot open settings"), result.message);
            return false;
        }
        return true;
    }

    if (it->fixKind == FixKind::RunFix) {
        // Administrator rights are needed: always ask first (B.1.3 / B.1.4).
        const auto answer = QMessageBox::question(
            parent, tr("Administrator rights needed"),
            tr("To use ErgoPro2 on Wayland, your user must be allowed to read the input devices and to use "
               "/dev/uinput.\n\nErgoPro2 will ask the system for the administrator password and then:\n"
               "  • add your user to the 'input' group\n"
               "  • install a udev rule giving that group access to /dev/uinput\n\n"
               "ErgoPro2 never sees your password. Afterwards you must log out and log in again.\n\nContinue?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            qCInfo(lcUi) << "User declined the administrator fix for" << permissionId;
            return false;
        }
        QPointer<QWidget> guard(parent);
        service.runFix(permissionId, [guard](PlatformResult result) {
            QWidget *p = guard.data();
            if (result.ok()) {
                QMessageBox::information(p, tr("Access granted"),
                                         tr("Done. Log out and log in again so the change takes effect, then "
                                            "start ErgoPro2."));
            } else {
                qCWarning(lcUi).noquote() << "Fix failed:" << result.message;
                QMessageBox::warning(p, tr("Fix not applied"), result.message);
            }
        });
        return true;
    }

    QMessageBox::information(parent, it->displayName,
                             it->howToGrant.isEmpty() ? it->explanation : it->howToGrant);
    return false;
}

} // namespace ergo::PermissionActions
