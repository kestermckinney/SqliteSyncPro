// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QWidget>

class AdminController;
class QTableWidget;
class QLabel;
class QTimer;
class QStackedWidget;

class UsersPage : public QWidget
{
    Q_OBJECT

public:
    explicit UsersPage(AdminController *controller, QWidget *parent = nullptr);

private slots:
    void onUsersChanged();
    void onUserAdded(bool success, const QString &message);
    void onUserRemoved(bool success, const QString &username);
    void onUserEdited(bool success, const QString &message);

private:
    void showAddDialog();
    void showEditDialog(const QString &username);
    void showRemoveDialog(const QString &username);
    void showSnack(bool isError, const QString &message);

    AdminController *m_controller;
    QStackedWidget  *m_contentStack; // 0=empty state, 1=table
    QTableWidget    *m_table;
    QLabel          *m_snackLabel;
    QTimer          *m_snackTimer;

    // Holds the username pending add/edit (for error display in dialog)
    QString m_pendingAddError;
    QString m_pendingEditError;
};
