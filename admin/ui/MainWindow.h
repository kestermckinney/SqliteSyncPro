#pragma once

#include <QMainWindow>

class AdminController;
class WizardSidebar;
class QStackedWidget;
class TeardownPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void goToPage(int index);
    void nextPage();

private slots:
    void onSetupFinished(bool success, const QString &message);

private:
    AdminController *m_controller;
    WizardSidebar   *m_sidebar;
    QStackedWidget  *m_stack;
    int              m_currentStep = 0;

    static constexpr int kTotalPages = 6;
};
