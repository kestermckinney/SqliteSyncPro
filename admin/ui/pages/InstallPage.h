#pragma once

#include <QWidget>

class AdminController;
class QLabel;
class QProgressBar;
class QTextEdit;
class QPushButton;

class InstallPage : public QWidget
{
    Q_OBJECT

public:
    explicit InstallPage(AdminController *controller, QWidget *parent = nullptr);

signals:
    void navigateTo(int index);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onStepCompleted(int index, bool ok, const QString &stepName, const QString &detail);
    void onSetupFinished(bool ok, const QString &message);

private:
    AdminController *m_controller;
    QLabel          *m_titleLabel;
    QLabel          *m_subtitleLabel;
    QProgressBar    *m_progressBar;
    QLabel          *m_counterLabel;
    QTextEdit       *m_log;
    QPushButton     *m_cancelButton;
    QPushButton     *m_backButton;

    int  m_completedSteps = 0;
    bool m_finished       = false;
};
