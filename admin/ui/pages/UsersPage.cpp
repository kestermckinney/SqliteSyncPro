#include "UsersPage.h"
#include "../../backend/AdminController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QStackedWidget>
#include <QFrame>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDateTime>
#include <QMessageBox>
#include <QVariantMap>

UsersPage::UsersPage(AdminController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────────
    QWidget *header = new QWidget;
    header->setFixedHeight(72);
    header->setStyleSheet(QStringLiteral("QWidget { background-color: #eceff1; }"));
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(32, 0, 32, 0);

    QVBoxLayout *headerText = new QVBoxLayout;
    headerText->setSpacing(2);
    QLabel *title = new QLabel(QStringLiteral("Sync Users"));
    title->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 500; color: #37474f;"));
    QLabel *subtitle = new QLabel(QStringLiteral(
        "Each user is a PostgreSQL role with access to sync_data."));
    subtitle->setStyleSheet(QStringLiteral("font-size: 13px; color: #607d8b;"));
    headerText->addWidget(title);
    headerText->addWidget(subtitle);
    headerLayout->addLayout(headerText, 1);

    QPushButton *addBtn = new QPushButton(QStringLiteral("+ Add User"));
    addBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #2196f3; color: white;"
        "  padding: 6px 16px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #1976d2; }"));
    headerLayout->addWidget(addBtn);
    outer->addWidget(header);

    // ── Content (empty state or table) ────────────────────────────────────────
    QFrame *contentFrame = new QFrame;
    contentFrame->setStyleSheet(QStringLiteral(
        "QFrame { border: 1px solid #b0bec5; border-radius: 6px; }"));

    QVBoxLayout *contentFrameLayout = new QVBoxLayout(contentFrame);
    contentFrameLayout->setContentsMargins(0, 0, 0, 0);

    m_contentStack = new QStackedWidget;
    contentFrameLayout->addWidget(m_contentStack);

    // Page 0: empty state
    QWidget *emptyState = new QWidget;
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyState);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(8);
    QLabel *emptyIcon = new QLabel(QStringLiteral("👤"));
    emptyIcon->setStyleSheet(QStringLiteral("font-size: 36px;"));
    emptyIcon->setAlignment(Qt::AlignCenter);
    QLabel *emptyTitle = new QLabel(QStringLiteral("No sync users yet"));
    emptyTitle->setStyleSheet(QStringLiteral("font-size: 15px; color: #90a4ae;"));
    emptyTitle->setAlignment(Qt::AlignCenter);
    QLabel *emptyHint = new QLabel(QStringLiteral("Click \"+ Add User\" to create one."));
    emptyHint->setStyleSheet(QStringLiteral("font-size: 13px; color: #90a4ae;"));
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyHint);
    m_contentStack->addWidget(emptyState);

    // Page 1: users table
    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Username (PostgreSQL Role)"),
        QStringLiteral("Created"),
        QStringLiteral(""),
        QStringLiteral("")
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(1, 180);
    m_table->horizontalHeader()->resizeSection(2, 70);
    m_table->horizontalHeader()->resizeSection(3, 80);
    m_table->horizontalHeader()->setStyleSheet(QStringLiteral(
        "QHeaderView::section { background-color: #cfd8dc; font-size: 12px;"
        "  font-weight: 500; color: #546e7a; border: none;"
        "  padding: 8px; border-bottom: 1px solid #b0bec5; }"));
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget { border: none; alternate-background-color: #eceff1; }"
        "QTableWidget::item { padding: 4px 8px; font-size: 13px; border: none; }"));
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(48);
    m_contentStack->addWidget(m_table);

    QWidget *contentWrapper = new QWidget;
    QVBoxLayout *wrapLayout = new QVBoxLayout(contentWrapper);
    wrapLayout->setContentsMargins(32, 16, 32, 0);
    wrapLayout->addWidget(contentFrame, 1);
    outer->addWidget(contentWrapper, 1);

    // ── Snackbar ──────────────────────────────────────────────────────────────
    m_snackLabel = new QLabel;
    m_snackLabel->setAlignment(Qt::AlignCenter);
    m_snackLabel->setFixedHeight(48);
    m_snackLabel->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: white; background-color: #2196f3;"));
    m_snackLabel->setVisible(false);
    outer->addWidget(m_snackLabel);

    m_snackTimer = new QTimer(this);
    m_snackTimer->setSingleShot(true);
    m_snackTimer->setInterval(3500);
    connect(m_snackTimer, &QTimer::timeout, this, [this] {
        m_snackLabel->setVisible(false);
    });

    // Signals
    connect(addBtn, &QPushButton::clicked, this, &UsersPage::showAddDialog);
    connect(m_controller, &AdminController::usersChanged,  this, &UsersPage::onUsersChanged);
    connect(m_controller, &AdminController::userAdded,     this, &UsersPage::onUserAdded);
    connect(m_controller, &AdminController::userRemoved,   this, &UsersPage::onUserRemoved);
    connect(m_controller, &AdminController::userEdited,    this, &UsersPage::onUserEdited);
}

void UsersPage::onUsersChanged()
{
    const QVariantList users = m_controller->users();

    if (users.isEmpty()) {
        m_contentStack->setCurrentIndex(0);
        return;
    }

    m_contentStack->setCurrentIndex(1);
    m_table->setRowCount(users.size());

    for (int i = 0; i < users.size(); ++i) {
        const QVariantMap row = users.at(i).toMap();
        const QString username  = row.value(QStringLiteral("username")).toString();
        const qint64  createdAt = row.value(QStringLiteral("created_at")).toLongLong();

        // Username (monospace)
        QTableWidgetItem *nameItem = new QTableWidgetItem(username);
        QFont mono(QStringLiteral("monospace"));
        mono.setStyleHint(QFont::Monospace);
        nameItem->setFont(mono);
        m_table->setItem(i, 0, nameItem);

        // Created date
        QString dateStr;
        if (createdAt > 0) {
            dateStr = QDateTime::fromMSecsSinceEpoch(createdAt).toLocalTime().toString(
                QStringLiteral("yyyy-MM-dd"));
        }
        QTableWidgetItem *dateItem = new QTableWidgetItem(dateStr);
        dateItem->setForeground(QColor(0x60, 0x7d, 0x8b));
        m_table->setItem(i, 1, dateItem);

        // Edit button
        QPushButton *editBtn = new QPushButton(QStringLiteral("Edit"));
        editBtn->setFlat(true);
        editBtn->setStyleSheet(QStringLiteral("QPushButton { color: #2196f3; border: none; }"));
        connect(editBtn, &QPushButton::clicked, this, [this, username] {
            showEditDialog(username);
        });
        m_table->setCellWidget(i, 2, editBtn);

        // Remove button
        QPushButton *removeBtn = new QPushButton(QStringLiteral("Remove"));
        removeBtn->setFlat(true);
        removeBtn->setStyleSheet(QStringLiteral("QPushButton { color: #c62828; border: none; }"));
        connect(removeBtn, &QPushButton::clicked, this, [this, username] {
            showRemoveDialog(username);
        });
        m_table->setCellWidget(i, 3, removeBtn);
    }
}

void UsersPage::onUserAdded(bool success, const QString &message)
{
    if (success)
        showSnack(false, QStringLiteral("User added successfully."));
    else
        QMessageBox::critical(this, QStringLiteral("Add User Failed"), message);
}

void UsersPage::onUserRemoved(bool success, const QString &username)
{
    if (success)
        showSnack(false, QStringLiteral("%1 removed.").arg(username));
    else
        QMessageBox::critical(this, QStringLiteral("Remove User Failed"), username);
}

void UsersPage::onUserEdited(bool success, const QString &message)
{
    if (success)
        showSnack(false, QStringLiteral("Password updated."));
    else
        QMessageBox::critical(this, QStringLiteral("Edit User Failed"), message);
}

void UsersPage::showAddDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Add Sync User"));
    dlg.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(QStringLiteral("Username (PostgreSQL role name)")));

    QLineEdit *usernameEdit = new QLineEdit;
    usernameEdit->setPlaceholderText(QStringLiteral("e.g. alice"));
    layout->addWidget(usernameEdit);

    layout->addWidget(new QLabel(QStringLiteral("Password (min. 8 characters)")));

    QLineEdit *passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(QStringLiteral("••••••••"));
    layout->addWidget(passwordEdit);

    QLabel *hint = new QLabel(QStringLiteral(
        "Usernames must start with a letter or underscore,\n"
        "followed by letters, digits, or underscores only."));
    hint->setStyleSheet(QStringLiteral("font-size: 11px; color: #607d8b;"));
    layout->addWidget(hint);

    QLabel *errorLabel = new QLabel;
    errorLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #c62828;"));
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&] {
        const QString uname = usernameEdit->text().trimmed().toLower();
        const QString pass  = passwordEdit->text();

        if (uname.isEmpty()) {
            errorLabel->setText(QStringLiteral("Username cannot be empty."));
            errorLabel->setVisible(true);
            return;
        }
        if (pass.length() < 8) {
            errorLabel->setText(QStringLiteral("Password must be at least 8 characters."));
            errorLabel->setVisible(true);
            return;
        }
        dlg.accept();
        m_controller->addUser(uname, pass);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // Show error inline if addUser fails while dialog is still "open" conceptually.
    // (In practice the dialog is already closed when userAdded fires, so we rely
    //  on the snackbar. The errorLabel is used only for local validation above.)

    dlg.exec();
}

void UsersPage::showEditDialog(const QString &username)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Change Password"));
    dlg.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    layout->addWidget(new QLabel(QStringLiteral("User")));

    QLineEdit *usernameDisplay = new QLineEdit(username);
    usernameDisplay->setReadOnly(true);
    usernameDisplay->setStyleSheet(QStringLiteral(
        "QLineEdit { background-color: #eceff1; border-radius: 4px; }"));
    layout->addWidget(usernameDisplay);

    layout->addWidget(new QLabel(QStringLiteral("New Password (min. 8 characters)")));

    QLineEdit *passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(QStringLiteral("••••••••"));
    layout->addWidget(passwordEdit);

    QLabel *errorLabel = new QLabel;
    errorLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #c62828;"));
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&] {
        const QString pass = passwordEdit->text();
        if (pass.length() < 8) {
            errorLabel->setText(QStringLiteral("Password must be at least 8 characters."));
            errorLabel->setVisible(true);
            return;
        }
        dlg.accept();
        m_controller->editUser(username, pass);
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}

void UsersPage::showRemoveDialog(const QString &username)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Remove User"));
    dlg.setMinimumWidth(440);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    // Warning icon + bold username
    QLabel *heading = new QLabel(
        QStringLiteral("⚠️  Remove <b>%1</b>?").arg(username));
    heading->setStyleSheet(QStringLiteral("font-size: 15px; color: #b71c1c;"));
    layout->addWidget(heading);

    QLabel *warning = new QLabel(QStringLiteral(
        "This will permanently delete the PostgreSQL role and "
        "<b>all sync data</b> for this user. "
        "Every device that syncs as <b>%1</b> will lose all server-side records. "
        "This action cannot be undone.").arg(username));
    warning->setWordWrap(true);
    warning->setStyleSheet(QStringLiteral("font-size: 13px; color: #37474f;"));
    layout->addWidget(warning);

    QLabel *prompt = new QLabel(QStringLiteral("Type <b>YES</b> to confirm:"));
    prompt->setStyleSheet(QStringLiteral("font-size: 13px; color: #37474f; margin-top: 6px;"));
    layout->addWidget(prompt);

    QLineEdit *confirmEdit = new QLineEdit;
    confirmEdit->setPlaceholderText(QStringLiteral("YES"));
    confirmEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { border: 2px solid #ef9a9a; border-radius: 4px; padding: 6px; font-size: 14px; }"));
    layout->addWidget(confirmEdit);

    QLabel *errorLabel = new QLabel(QStringLiteral("You must type YES to confirm."));
    errorLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #c62828;"));
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okBtn = buttons->button(QDialogButtonBox::Ok);
    okBtn->setText(QStringLiteral("Remove User"));
    okBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #c62828; color: white;"
        "  padding: 6px 16px; border-radius: 4px; border: none; }"
        "QPushButton:hover { background-color: #b71c1c; }"
        "QPushButton:disabled { background-color: #ef9a9a; }"));
    layout->addWidget(buttons);

    // Turn the border green as the user types and enable OK only when input == "YES"
    connect(confirmEdit, &QLineEdit::textChanged, &dlg,
            [confirmEdit, okBtn, errorLabel](const QString &text) {
        const bool confirmed = (text == QStringLiteral("YES"));
        confirmEdit->setStyleSheet(QStringLiteral(
            "QLineEdit { border: 2px solid %1; border-radius: 4px;"
            " padding: 6px; font-size: 14px; }")
            .arg(confirmed ? QStringLiteral("#2e7d32") : QStringLiteral("#ef9a9a")));
        okBtn->setEnabled(confirmed);
        if (confirmed)
            errorLabel->setVisible(false);
    });
    okBtn->setEnabled(false);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&] {
        if (confirmEdit->text() != QStringLiteral("YES")) {
            errorLabel->setVisible(true);
            return;
        }
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted)
        m_controller->removeUser(username);
}

void UsersPage::showSnack(bool isError, const QString &message)
{
    m_snackLabel->setText(message);
    m_snackLabel->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: white; background-color: %1;")
        .arg(isError ? QStringLiteral("#c62828") : QStringLiteral("#2e7d32")));
    m_snackLabel->setVisible(true);
    m_snackTimer->start();
}
