#include "core/keycontrol/KeyBinding.h"

#include "core/keycontrol/KeyAction.h"
#include "core/keycontrol/KeyComboValidation.h"

#include <QCoreApplication>
#include <QJsonValue>
#include <QUuid>

namespace ergo::KeyBindingStore {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::KeyBindingStore", text);
}

} // namespace

QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QList<KeyBinding> fromJson(const QJsonArray &array, const KeyActionRegistry &actions, OsStyle os,
                           QStringList *problems)
{
    QList<KeyBinding> result;
    QSet<KeyCombo> seen;
    for (int i = 0; i < array.size(); ++i) {
        KeyBinding b;
        const QJsonValue value = array.at(i);
        if (!value.isObject()) {
            b.id = newId();
            b.loadError = tr("The stored entry is not a valid hotkey definition.");
            b.raw = QJsonObject{{QStringLiteral("invalid"), value}};
        } else {
            const QJsonObject o = value.toObject();
            b.raw = o;
            b.id = o.value(QStringLiteral("id")).toString();
            if (b.id.isEmpty())
                b.id = newId();
            b.actionId = o.value(QStringLiteral("action")).toString();
            b.enabled = o.value(QStringLiteral("enabled")).toBool(true);
            const QString comboText = o.value(QStringLiteral("keyCombo")).toString();
            const auto combo = comboFromPortableText(comboText);
            if (!combo) {
                b.loadError = tr("Unknown key combination \"%1\".").arg(comboText);
            } else {
                b.combo = *combo;
                const ComboCheck check = validateCombo(b.combo, os);
                if (!actions.find(b.actionId))
                    b.loadError = tr("Unknown action \"%1\".").arg(b.actionId);
                else if (!check.accepted())
                    b.loadError = check.message;
                else if (seen.contains(b.combo))
                    b.loadError = tr("%1 is assigned twice; only the first entry is used.")
                                      .arg(comboToDisplayText(b.combo, os));
                else
                    seen.insert(b.combo);
            }
        }
        if (!b.isValid() && problems)
            *problems << tr("Hotkey entry %1: %2").arg(i + 1).arg(b.loadError);
        result << b;
    }
    return result;
}

QJsonArray toJson(const QList<KeyBinding> &bindings)
{
    QJsonArray array;
    for (const KeyBinding &b : bindings) {
        if (!b.isValid()) {
            array.append(b.raw.contains(QStringLiteral("invalid")) ? b.raw.value(QStringLiteral("invalid"))
                                                                    : QJsonValue(b.raw));
            continue;
        }
        array.append(QJsonObject{{QStringLiteral("id"), b.id},
                                 {QStringLiteral("keyCombo"), comboToPortableText(b.combo)},
                                 {QStringLiteral("action"), b.actionId},
                                 {QStringLiteral("enabled"), b.enabled}});
    }
    return array;
}

} // namespace ergo::KeyBindingStore
