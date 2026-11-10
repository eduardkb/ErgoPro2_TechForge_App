#include "ui/keycontrol/HotkeyDialog.h"

#include "core/LogCategories.h"
#include "ui/InputService.h"
#include "ui/keycontrol/KeyControlController.h"

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

namespace ergo {

namespace {

QLabel *bigLabel(QWidget *parent)
{
    auto *l = new QLabel(parent);
    l->setObjectName(QStringLiteral("pageTitle"));
    l->setAlignment(Qt::AlignCenter);
    l->setWordWrap(true);
    return l;
}

QLabel *hint(const QString &text, QWidget *parent)
{
    auto *l = new QLabel(text, parent);
    l->setObjectName(QStringLiteral("hint"));
    l->setAlignment(Qt::AlignCenter);
    l->setWordWrap(true);
    return l;
}

QPushButton *button(const QString &text, QWidget *parent, bool primary = false)
{
    auto *b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    b->setAutoDefault(false); // Enter must not press a button while capturing
    b->setDefault(false);
    if (primary)
        b->setProperty("variant", QStringLiteral("primary"));
    return b;
}

} // namespace

HotkeyDialog::HotkeyDialog(KeyControlController &controller, InputService &input, const QString &bindingId,
                           QWidget *parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_input(input)
    , m_bindingId(bindingId)
{
    setWindowTitle(bindingId.isEmpty() ? tr("Add Hotkey") : tr("Modify Hotkey"));
    setModal(true);
    setFixedWidth(520);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(buildCapturePage());
    m_pages->addWidget(buildConfirmPage());
    m_pages->addWidget(buildActionPage());
    layout->addWidget(m_pages);

    m_controller.setPaused(true); // K-14: the user can press any key, nothing fires

    // Modify: start on the action page with the current key.
    for (const KeyBinding &b : m_controller.bindings()) {
        if (b.id == bindingId && b.isValid()) {
            m_combo = b.combo;
            m_haveCombo = true;
            const int index = m_action->findData(b.actionId);
            if (index >= 0)
                m_action->setCurrentIndex(index);
        }
    }
    showPage(m_haveCombo ? ActionPage : CapturePage);
}

HotkeyDialog::~HotkeyDialog()
{
    disconnect(m_inputConnection);
    m_controller.setPaused(false); // restore all hotkeys
}

QWidget *HotkeyDialog::buildCapturePage()
{
    auto *page = new QWidget(this);
    auto *l = new QVBoxLayout(page);
    l->setSpacing(12);
    auto *title = bigLabel(page);
    title->setText(tr("Press the key or key combination…"));
    l->addWidget(title);
    l->addWidget(hint(tr("Hold the modifier keys (Shift, Ctrl, Alt/Option, Cmd/Win) and press the key. "
                         "Function keys F1–F24 and keypad keys can also be used alone."),
                      page));
    m_heldLabel = bigLabel(page);
    m_heldLabel->setMinimumHeight(36);
    l->addWidget(m_heldLabel);
    m_captureError = new QLabel(page);
    m_captureError->setObjectName(QStringLiteral("warningText"));
    m_captureError->setWordWrap(true);
    m_captureError->setAlignment(Qt::AlignCenter);
    m_captureError->hide();
    l->addWidget(m_captureError);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto *cancel = button(tr("Cancel"), page);
    buttons->addWidget(cancel);
    l->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    return page;
}

QWidget *HotkeyDialog::buildConfirmPage()
{
    auto *page = new QWidget(this);
    auto *l = new QVBoxLayout(page);
    l->setSpacing(12);
    l->addWidget(hint(tr("You pressed:"), page));
    m_confirmCombo = bigLabel(page);
    l->addWidget(m_confirmCombo);
    m_confirmMessage = new QLabel(page);
    m_confirmMessage->setWordWrap(true);
    m_confirmMessage->setAlignment(Qt::AlignCenter);
    l->addWidget(m_confirmMessage);
    auto *buttons = new QHBoxLayout;
    auto *cancel = button(tr("Cancel"), page);
    auto *retry = button(tr("Retry"), page);
    m_confirm = button(tr("Confirm"), page, true);
    buttons->addWidget(cancel);
    buttons->addStretch(1);
    buttons->addWidget(retry);
    buttons->addWidget(m_confirm);
    l->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(retry, &QPushButton::clicked, this, [this] { showPage(CapturePage); });
    connect(m_confirm, &QPushButton::clicked, this, [this] { showPage(ActionPage); });
    return page;
}

QWidget *HotkeyDialog::buildActionPage()
{
    auto *page = new QWidget(this);
    auto *l = new QVBoxLayout(page);
    l->setSpacing(12);
    auto *row = new QHBoxLayout;
    auto *keyName = new QLabel(tr("Key"), page);
    keyName->setObjectName(QStringLiteral("valueLabel"));
    keyName->setMinimumWidth(90);
    m_actionCombo = new QLabel(page);
    m_actionCombo->setObjectName(QStringLiteral("cardTitle"));
    auto *change = button(tr("Change key…"), page);
    row->addWidget(keyName);
    row->addWidget(m_actionCombo, 1);
    row->addWidget(change);
    l->addLayout(row);

    auto *actionRow = new QHBoxLayout;
    auto *actionName = new QLabel(tr("Action"), page);
    actionName->setObjectName(QStringLiteral("valueLabel"));
    actionName->setMinimumWidth(90);
    m_action = new QComboBox(page);
    for (IKeyAction *a : m_controller.selectableActions())
        m_action->addItem(a->displayName(), a->id());
    actionRow->addWidget(actionName);
    actionRow->addWidget(m_action, 1);
    l->addLayout(actionRow);
    l->addWidget(hint(tr("Clicks happen at the current mouse pointer position."), page));

    m_saveError = new QLabel(page);
    m_saveError->setObjectName(QStringLiteral("warningText"));
    m_saveError->setWordWrap(true);
    m_saveError->hide();
    l->addWidget(m_saveError);

    auto *buttons = new QHBoxLayout;
    auto *cancel = button(tr("Cancel"), page);
    auto *saveButton = button(tr("Save"), page, true);
    buttons->addWidget(cancel);
    buttons->addStretch(1);
    buttons->addWidget(saveButton);
    l->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(change, &QPushButton::clicked, this, [this] { showPage(CapturePage); });
    connect(saveButton, &QPushButton::clicked, this, &HotkeyDialog::save);
    return page;
}

void HotkeyDialog::showPage(Page page)
{
    disconnect(m_inputConnection);
    if (page == CapturePage) {
        m_held = Modifier::None;
        m_heldLabel->setText(QString());
        m_inputConnection = connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this,
                                    &HotkeyDialog::onInputEvent);
    } else if (page == ConfirmPage) {
        const ComboCheck c = m_controller.check(m_combo, m_bindingId);
        m_confirmCombo->setText(m_controller.displayText(m_combo));
        m_confirmMessage->setText(c.message);
        m_confirmMessage->setObjectName(c.result == ComboCheck::Result::Ok ? QStringLiteral("hint")
                                                                           : QStringLiteral("warningText"));
        m_confirmMessage->style()->unpolish(m_confirmMessage);
        m_confirmMessage->style()->polish(m_confirmMessage);
        m_confirmMessage->setVisible(!c.message.isEmpty());
        m_confirm->setEnabled(c.accepted());
        m_confirm->setText(c.result == ComboCheck::Result::Warning ? tr("Use anyway") : tr("Confirm"));
    } else {
        m_actionCombo->setText(m_controller.displayText(m_combo));
        m_saveError->hide();
    }
    m_pages->setCurrentIndex(page);
    adjustSize();
}

bool HotkeyDialog::event(QEvent *event)
{
    // While capturing, keys belong to the capture (Esc / Enter must not close
    // the dialog or press a button); they are read from the global hook.
    if (m_pages && m_pages->currentIndex() == CapturePage
        && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease
            || event->type() == QEvent::ShortcutOverride)) {
        event->accept();
        return true;
    }
    return QDialog::event(event);
}

void HotkeyDialog::onInputEvent(const InputEvent &event)
{
    if (!event.isKey() || event.synthetic || event.autoRepeat)
        return;
    if (isModifierKey(event.key)) {
        // Live display of the held modifiers.
        const Modifiers flag = modifierForKey(event.key);
        if (event.type == InputEventType::KeyDown)
            m_held |= flag;
        else
            m_held &= Modifiers(~flag);
        m_heldLabel->setText(m_controller.modifiersText(m_held));
        return;
    }
    if (event.type != InputEventType::KeyDown)
        return;
    onCaptured(KeyCombo{event.key, Modifiers(event.modifiers & ~modifierForKey(event.key)), event.keypad});
}

void HotkeyDialog::onCaptured(const KeyCombo &combo)
{
    m_combo = combo;
    m_haveCombo = true;
    m_captureError->hide();
    qCDebug(lcUi).noquote() << "Hotkey dialog captured" << m_controller.displayText(combo);
    showPage(ConfirmPage);
}

void HotkeyDialog::save()
{
    const QString actionId = m_action->currentData().toString();
    // The dialog paused all hotkeys; registration happens with them resumed so
    // the new key is registered like the others.
    m_controller.setPaused(false);
    const OpResult r = m_bindingId.isEmpty() ? m_controller.add(m_combo, actionId)
                                             : m_controller.modify(m_bindingId, m_combo, actionId);
    if (r.ok) {
        accept();
        return;
    }
    m_controller.setPaused(true);
    // Keep the dialog open and let the user choose another key.
    m_captureError->setText(tr("%1 cannot be used: %2\nPress another key or combination.")
                                .arg(m_controller.displayText(m_combo), r.message));
    m_captureError->show();
    showPage(CapturePage);
}

} // namespace ergo
