#pragma once

#include <QWidget>

class AdminController;
class QLabel;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QTextEdit;
class QStackedWidget;

class TeardownPage : public QWidget
{
    Q_OBJECT

public:
    explicit TeardownPage(AdminController *controller, QWidget *parent = nullptr);

private slots:
    void onConfirmTextChanged(const QString &text);
    void onRemoveClicked();
    void onStepCompleted(int index, bool success,
                         const QString &stepName,
                         const QString &detail);
    void onTeardownFinished(bool success, const QString &message);

private:
    AdminController *m_controller;

    // Confirmation panel widgets
    QStackedWidget *m_panels;
    QLineEdit      *m_confirmEdit;
    QPushButton    *m_removeButton;

    // Progress panel widgets
    QLabel         *m_statusTitle;
    QLabel         *m_statusSubtitle;
    QProgressBar   *m_progressBar;
    QLabel         *m_counterLabel;
    QTextEdit      *m_log;

    int  m_completedSteps = 0;
};
