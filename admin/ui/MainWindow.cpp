#include "MainWindow.h"
#include "WizardSidebar.h"
#include "pages/ConnectionPage.h"
#include "pages/ConfigurePage.h"
#include "pages/InstallPage.h"
#include "pages/SummaryPage.h"
#include "pages/UsersPage.h"
#include "pages/TeardownPage.h"
#include "../backend/AdminController.h"

#include <QHBoxLayout>
#include <QFrame>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_controller(new AdminController(this))
{
    setWindowTitle(QStringLiteral("SQLSync Administrator"));
    resize(900, 620);
    setMinimumSize(780, 540);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Sidebar
    m_sidebar = new WizardSidebar(this);
    m_sidebar->setFixedWidth(200);
    layout->addWidget(m_sidebar);

    // 1px vertical divider
    QFrame *divider = new QFrame(this);
    divider->setFrameShape(QFrame::VLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setStyleSheet(QStringLiteral("QFrame { color: #b0bec5; }"));
    layout->addWidget(divider);

    // Content area
    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);

    // Pages (order must match goToPage() indices)
    auto *connPage    = new ConnectionPage(m_controller, this);
    auto *confPage    = new ConfigurePage(m_controller, this);
    auto *installPage = new InstallPage(m_controller, this);
    auto *summaryPage = new SummaryPage(m_controller, this);
    auto *usersPage     = new UsersPage(m_controller, this);
    auto *teardownPage  = new TeardownPage(m_controller, this);

    m_stack->addWidget(connPage);     // 0
    m_stack->addWidget(confPage);     // 1
    m_stack->addWidget(installPage);  // 2
    m_stack->addWidget(summaryPage);  // 3
    m_stack->addWidget(usersPage);    // 4
    m_stack->addWidget(teardownPage); // 5

    // Page navigation signals
    connect(connPage,    &ConnectionPage::navigateNext, this, &MainWindow::nextPage);
    connect(connPage,    &ConnectionPage::navigateTo,   this, &MainWindow::goToPage);
    connect(confPage,    &ConfigurePage::navigateNext,  this, &MainWindow::nextPage);
    connect(confPage,    &ConfigurePage::navigateTo,    this, &MainWindow::goToPage);
    connect(installPage, &InstallPage::navigateTo,      this, &MainWindow::goToPage);
    connect(summaryPage, &SummaryPage::navigateTo,      this, &MainWindow::goToPage);

    // Auto-advance to SummaryPage on successful install
    connect(m_controller, &AdminController::setupFinished,
            this, &MainWindow::onSetupFinished);

    // Sidebar step clicks — only navigate to visited pages (or Users when setup done, or Remove All always)
    connect(m_sidebar, &WizardSidebar::stepClicked, this, [this](int index) {
        if (index <= m_currentStep || (index == 4 && m_controller->isSetupDone()) || index == 5)
            goToPage(index);
    });

    // Keep sidebar's "setup done" state in sync
    connect(m_controller, &AdminController::setupStateChanged, this, [this]() {
        m_sidebar->setSetupDone(m_controller->isSetupDone());
    });

    goToPage(0);
}

void MainWindow::goToPage(int index)
{
    if (index < 0 || index >= kTotalPages)
        return;
    m_currentStep = index;
    m_stack->setCurrentIndex(index);
    m_sidebar->setCurrentStep(index);

    // Trigger users reload whenever we land on that page
    if (index == 4)
        m_controller->loadUsers();
}

void MainWindow::nextPage()
{
    goToPage(m_currentStep + 1);
}

void MainWindow::onSetupFinished(bool success, const QString &)
{
    if (success)
        goToPage(3); // SummaryPage
}
