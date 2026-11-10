#include "ui/tabs/KeyControlTab.h"

#include "core/LogCategories.h"
#include "core/keycontrol/KeyAction.h"
#include "ui/keycontrol/HotkeyDialog.h"
#include "ui/keycontrol/KeyControlController.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/StatusBadge.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace ergo {

namespace {

enum Column { KeyColumn, ActionColumn, StatusColumn, ButtonsColumn };

QPushButton *rowButton(const QString &text, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    b->setProperty("size", QStringLiteral("small"));
    return b;
}

} // namespace

KeyControlTab::KeyControlTab(KeyControlController &controller, InputService &input, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_input(input)
{
    // The whole page scrolls (like the other tabs); before 1.6.1 the page was
    // squeezed to the window height and the hotkey list could not grow.
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageLayout->addWidget(scroll);
    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);
    auto *outer = new QVBoxLayout(content);
    outer->setContentsMargins(0, 12, 0, 20);
    outer->setSpacing(16);

    auto *card = new CardWidget(tr("Hotkeys"),
                                tr("Keyboard keys that click the mouse for you. While a hotkey is enabled, other "
                                   "applications do not receive that key."),
                                content);
    m_add = new QPushButton(tr("Add Hotkey"), card);
    m_add->setProperty("variant", QStringLiteral("primary"));
    m_add->setCursor(Qt::PointingHandCursor);
    card->setHeaderWidget(m_add);

    m_table = new QTableWidget(0, 4, card);
    m_table->setHorizontalHeaderLabels({tr("Key"), tr("Action"), tr("Status"), QString()});
    m_table->verticalHeader()->hide();
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setShowGrid(false);
    m_table->setWordWrap(true);
    m_table->horizontalHeader()->setSectionResizeMode(KeyColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ActionColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(StatusColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ButtonsColumn, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // No own scroll bar: the table is always as tall as its rows (see fitTableHeight).
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // Rows change height when wrapped texts need more or fewer lines (layout,
    // window resized): follow them.
    connect(m_table->verticalHeader(), &QHeaderView::sectionResized, this, [this] { fitTableHeight(); });
    m_table->installEventFilter(this);

    // Above the table, as a warning (1.6.1).
    m_allAssigned = new QLabel(tr("Every action already has a hotkey. Delete a hotkey to assign its action to "
                                  "another key."),
                               card);
    m_allAssigned->setObjectName(QStringLiteral("warningText"));
    m_allAssigned->setWordWrap(true);
    m_allAssigned->hide();
    card->contentLayout()->addWidget(m_allAssigned);
    card->contentLayout()->addWidget(m_table);

    m_empty = new QLabel(tr("No hotkeys yet. Click “Add Hotkey” and press the key you want to use."), card);
    m_empty->setObjectName(QStringLiteral("hint"));
    m_empty->setAlignment(Qt::AlignCenter);
    card->contentLayout()->addWidget(m_empty);

    m_lastError = new QLabel(card);
    m_lastError->setObjectName(QStringLiteral("warningText"));
    m_lastError->setWordWrap(true);
    m_lastError->hide();
    card->contentLayout()->addWidget(m_lastError);
    outer->addWidget(card);

    auto *tips = new CardWidget(tr("Tips"), {}, content);
    auto *text = new QLabel(tr("• Function keys (F1–F24) and keypad keys work best; other keys need a modifier "
                               "(Shift, Ctrl, Alt/Option, Cmd/Win).\n"
                               "• On Mac keyboards the top row may need the fn key for F1–F12.\n"
                               "• Disable a hotkey to give the key back to other applications; “Reset binding” "
                               "registers it again if it stopped working.\n"
                               "• Clicks happen at the current pointer position and count as manual clicks for "
                               "Auto Click."),
                            tips);
    text->setObjectName(QStringLiteral("hint"));
    text->setWordWrap(true);
    tips->contentLayout()->addWidget(text);
    outer->addWidget(tips);
    outer->addStretch(1);

    connect(m_add, &QPushButton::clicked, this, [this] { openDialog({}); });
    connect(&m_controller, &KeyControlController::changed, this, &KeyControlTab::rebuild);
    connect(&m_controller, &KeyControlController::actionFailed, this, [this](const QString &message) {
        m_lastError->setText(tr("Last hotkey action failed: %1").arg(message));
        m_lastError->show();
    });
    rebuild();
}

void KeyControlTab::rebuild()
{
    const bool available = m_controller.isAvailable();
    // Controls of an unavailable feature are disabled (B.1.4); one hotkey per
    // action, so Add is also disabled when every action has a hotkey.
    const bool actionsLeft = !m_controller.selectableActions().isEmpty();
    m_add->setEnabled(available && actionsLeft);
    m_add->setToolTip(actionsLeft ? QString()
                                  : tr("Every action already has a hotkey. Delete a hotkey to free its action."));
    m_allAssigned->setVisible(!actionsLeft);

    const QList<KeyBinding> &bindings = m_controller.bindings();
    m_table->setRowCount(int(bindings.size()));
    m_table->setVisible(!bindings.isEmpty());
    m_empty->setVisible(bindings.isEmpty());

    for (int row = 0; row < bindings.size(); ++row) {
        const KeyBinding &b = bindings.at(row);
        const BindingState s = m_controller.state(b.id);

        auto *keyItem = new QTableWidgetItem(b.isValid() ? m_controller.displayText(b.combo)
                                                         : b.raw.value(QStringLiteral("keyCombo")).toString());
        QFont font = keyItem->font();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF() * 1.15);
        keyItem->setFont(font);
        m_table->setItem(row, KeyColumn, keyItem);
        m_table->setItem(row, ActionColumn,
                         new QTableWidgetItem(b.actionId.isEmpty() ? tr("?") : m_controller.actionName(b.actionId)));

        auto *statusCell = new QWidget(m_table);
        auto *statusLayout = new QVBoxLayout(statusCell);
        statusLayout->setContentsMargins(4, 6, 4, 6);
        statusLayout->setSpacing(2);
        auto *badge = new StatusBadge(statusCell);
        switch (s.status) {
        case BindingStatus::Active: badge->setState(StatusBadge::State::Active, tr("ACTIVE")); break;
        case BindingStatus::Disabled: badge->setState(StatusBadge::State::Inactive, tr("DISABLED")); break;
        case BindingStatus::Paused: badge->setState(StatusBadge::State::Inactive, tr("PAUSED")); break;
        case BindingStatus::Unavailable: badge->setState(StatusBadge::State::Warning, tr("UNAVAILABLE")); break;
        case BindingStatus::Error: badge->setState(StatusBadge::State::Error, tr("ERROR")); break;
        }
        // Drag lock holding the left button (B.5.3): shown on its row.
        const bool holding = s.status == BindingStatus::Active && b.actionId == KeyActionRegistry::DragLock
                             && m_controller.isDragLockHolding();
        if (holding)
            badge->setState(StatusBadge::State::Warning, tr("HOLDING"));
        statusLayout->addWidget(badge, 0, Qt::AlignLeft);
        if (holding) {
            auto *hint = new QLabel(tr("Left button held. Press the hotkey, any key or a mouse button to let go."),
                                    statusCell);
            hint->setObjectName(QStringLiteral("hint"));
            hint->setWordWrap(true);
            statusLayout->addWidget(hint);
        }
        if (!s.reason.isEmpty() && s.status == BindingStatus::Error) {
            auto *reason = new QLabel(s.reason, statusCell);
            reason->setObjectName(QStringLiteral("hint"));
            reason->setWordWrap(true);
            statusLayout->addWidget(reason);
        }
        m_table->setCellWidget(row, StatusColumn, statusCell);

        auto *buttons = new QWidget(m_table);
        auto *h = new QHBoxLayout(buttons);
        h->setContentsMargins(4, 4, 4, 4);
        h->setSpacing(6);
        const QString id = b.id;
        auto *toggle = rowButton(b.enabled ? tr("Disable") : tr("Enable"), buttons);
        auto *modify = rowButton(tr("Modify…"), buttons);
        auto *reset = rowButton(tr("Reset binding"), buttons);
        auto *remove = rowButton(tr("Delete…"), buttons);
        toggle->setEnabled(available && b.isValid());
        modify->setEnabled(available);
        reset->setEnabled(available && b.isValid() && b.enabled);
        remove->setEnabled(available);
        for (QPushButton *btn : {toggle, modify, reset, remove})
            h->addWidget(btn);
        connect(toggle, &QPushButton::clicked, this, [this, id, enabled = b.enabled] {
            const OpResult r = m_controller.setEnabled(id, !enabled);
            if (!r.ok)
                reportFailure(tr("Hotkey not enabled"), r.message);
        });
        connect(modify, &QPushButton::clicked, this, [this, id] { openDialog(id); });
        connect(reset, &QPushButton::clicked, this, [this, id] {
            const OpResult r = m_controller.resetBinding(id);
            if (!r.ok)
                reportFailure(tr("Binding not restored"), r.message);
        });
        connect(remove, &QPushButton::clicked, this, [this, id] { confirmDelete(id); });
        m_table->setCellWidget(row, ButtonsColumn, buttons);
    }
    m_table->resizeRowsToContents();
    fitTableHeight();
    // Row heights are final only after the layout has run.
    QTimer::singleShot(0, this, [this] {
        m_table->resizeRowsToContents();
        fitTableHeight();
    });
}

bool KeyControlTab::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_table && event->type() == QEvent::Resize) {
        auto *resize = static_cast<QResizeEvent *>(event);
        if (resize->size().width() != resize->oldSize().width()) // height changes come from fitTableHeight
            QTimer::singleShot(0, this, [this] {
                m_table->resizeRowsToContents();
                fitTableHeight();
            });
    }
    return QWidget::eventFilter(watched, event);
}

void KeyControlTab::fitTableHeight()
{
    // Header + every row + frame: all hotkeys are always visible and the page
    // scroll bar appears when the window is too small.
    const int height = m_table->horizontalHeader()->sizeHint().height() + m_table->verticalHeader()->length()
                       + 2 * m_table->frameWidth();
    if (m_table->height() == height && m_table->minimumHeight() == height)
        return;
    m_table->setFixedHeight(height);
    qCDebug(lcUi) << "Key Control table:" << m_table->rowCount() << "row(s)," << height << "px";
}

void KeyControlTab::openDialog(const QString &bindingId)
{
    m_lastError->hide();
    HotkeyDialog dialog(m_controller, m_input, bindingId, window());
    dialog.exec();
}

void KeyControlTab::confirmDelete(const QString &bindingId)
{
    QString name;
    for (const KeyBinding &b : m_controller.bindings()) {
        if (b.id == bindingId)
            name = b.isValid() ? m_controller.displayText(b.combo) : tr("this entry");
    }
    const auto answer = QMessageBox::question(window(), tr("Delete hotkey"),
                                              tr("Delete the hotkey %1? The key works normally again in other "
                                                 "applications.")
                                                  .arg(name),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    const OpResult r = m_controller.remove(bindingId);
    if (!r.ok)
        reportFailure(tr("Hotkey not deleted"), r.message);
}

void KeyControlTab::reportFailure(const QString &title, const QString &message)
{
    qCWarning(lcUi).noquote() << title << "-" << message;
    QMessageBox::warning(window(), title, message);
}

} // namespace ergo
