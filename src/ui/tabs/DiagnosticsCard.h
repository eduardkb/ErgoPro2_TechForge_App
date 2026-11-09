#pragma once

#include "ui/widgets/CardWidget.h"

class QGridLayout;

namespace ergo {

struct AppContext;

// "Permissions & diagnostics" card of the General tab (B.1.3): OS, session,
// permission statuses with fix buttons, input hook status, start-at-login
// registration. Updates live when permissions change.
class DiagnosticsCard : public CardWidget {
    Q_OBJECT
public:
    DiagnosticsCard(AppContext &context, QWidget *parent = nullptr);

    void refresh();

private:
    void addRow(int row, const QString &name, QWidget *value, QWidget *action = nullptr);

    AppContext &m_context;
    QWidget *m_body = nullptr;
    QGridLayout *m_grid = nullptr;
};

} // namespace ergo
