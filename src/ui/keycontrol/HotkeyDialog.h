#pragma once

#include "core/input/InputEvent.h"
#include "core/input/KeyCombo.h"

#include <QDialog>
#include <QMetaObject>

class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;

namespace ergo {

class InputService;
class KeyControlController;

// Add / Modify Hotkey dialog (K-15):
//   1. "Press the key or key combination…" (Cancel)
//   2. shows the captured combination with Confirm / Retry (+ warning or the
//      reason why it cannot be used)
//   3. choose the action -> Save -> registered; on failure the reason is shown
//      and the user can pick another key.
// Keys are captured from the global input hook (same key identity as at
// runtime). All hotkeys are paused while the dialog is open (K-14).
class HotkeyDialog : public QDialog {
    Q_OBJECT
public:
    // Add mode when bindingId is empty, Modify mode otherwise.
    HotkeyDialog(KeyControlController &controller, InputService &input, const QString &bindingId = {},
                 QWidget *parent = nullptr);
    ~HotkeyDialog() override;

protected:
    bool event(QEvent *event) override;

private:
    enum Page { CapturePage, ConfirmPage, ActionPage };

    QWidget *buildCapturePage();
    QWidget *buildConfirmPage();
    QWidget *buildActionPage();
    void showPage(Page page);
    void onInputEvent(const InputEvent &event);
    void onCaptured(const KeyCombo &combo);
    void save();

    KeyControlController &m_controller;
    InputService &m_input;
    QString m_bindingId;
    QMetaObject::Connection m_inputConnection;

    KeyCombo m_combo;
    bool m_haveCombo = false;
    Modifiers m_held = Modifier::None;

    QStackedWidget *m_pages = nullptr;
    QLabel *m_captureError = nullptr;
    QLabel *m_heldLabel = nullptr;
    QLabel *m_confirmCombo = nullptr;
    QLabel *m_confirmMessage = nullptr;
    QPushButton *m_confirm = nullptr;
    QLabel *m_actionCombo = nullptr;
    QComboBox *m_action = nullptr;
    QLabel *m_saveError = nullptr;
};

} // namespace ergo
