// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QWidget>

class AdminController;
class QComboBox;
class QLineEdit;
class QLabel;
class QFrame;
class QPushButton;
class QWidget;

class ConnectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(AdminController *controller, QWidget *parent = nullptr);

signals:
    void navigateNext();
    void navigateTo(int index);

private slots:
    void onTestClicked();
    void onNextClicked();
    void onConnectionTestResult(bool success, const QString &message);
    void onAlreadySetup(bool isSetup);
    void onModeChanged(int index);
    void onUpgradeFinished(bool success, const QString &message);

private:
    /**
     * If the database has an old sync_data schema, prompt the user to upgrade
     * and run the UpgradeWorker. Returns true if the caller should continue
     * (no upgrade needed, or upgrade kicked off and we'll resume on completion).
     * Returns false if the caller should stop (user declined the upgrade).
     */
    bool maybePromptForUpgrade();

    void showStatus(bool isError, const QString &message);
    void hideStatus();

    AdminController *m_controller;
    QComboBox       *m_modeCombo;
    QLineEdit       *m_hostEdit;
    QLineEdit       *m_portEdit;
    QLineEdit       *m_dbNameEdit;
    QLineEdit       *m_superuserEdit;
    QLineEdit       *m_passwordEdit;
    QWidget         *m_supabaseFieldsWidget;
    QLineEdit       *m_supabaseUrlEdit;
    QLineEdit       *m_supabaseServiceKeyEdit;
    QFrame          *m_statusFrame;
    QLabel          *m_statusIcon;
    QLabel          *m_statusLabel;
    QPushButton     *m_testButton;
    QPushButton     *m_nextButton;
};
