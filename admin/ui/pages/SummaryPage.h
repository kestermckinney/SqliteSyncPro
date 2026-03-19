#pragma once

#include <QWidget>

class AdminController;
class QLineEdit;
class QTextEdit;
class QWidget;
class QLabel;

class SummaryPage : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryPage(AdminController *controller, QWidget *parent = nullptr);

signals:
    void navigateTo(int index);

protected:
    void showEvent(QShowEvent *event) override;

private:
    AdminController *m_controller;
    QLineEdit       *m_authPasswordEdit;
    QLineEdit       *m_jwtSecretEdit;
    QTextEdit       *m_configEdit;
    QWidget         *m_selfHostedCredentials;  // shown only in self-hosted mode
    QLabel          *m_supabaseNote;           // shown only in Supabase mode
};
