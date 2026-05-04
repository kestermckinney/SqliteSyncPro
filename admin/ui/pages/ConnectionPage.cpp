// Copyright (C) 2026 Paul McKinney
#include "ConnectionPage.h"
#include "../../backend/AdminController.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QFrame>
#include <QPushButton>
#include <QIntValidator>
#include <QMessageBox>

ConnectionPage::ConnectionPage(AdminController *controller, QWidget *parent)
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
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(32, 0, 32, 0);
    headerLayout->setAlignment(Qt::AlignVCenter);
    headerLayout->setSpacing(2);

    QLabel *title = new QLabel(QStringLiteral("Connect to PostgreSQL"));
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 500; color: #37474f;"));

    QLabel *subtitle = new QLabel(QStringLiteral(
        "Enter your PostgreSQL superuser credentials. "
        "Choose Self-hosted for a local/VPS setup, or Supabase for cloud hosting."));
    subtitle->setStyleSheet(QStringLiteral("font-size: 13px; color: #607d8b;"));
    subtitle->setWordWrap(true);

    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    outer->addWidget(header);

    // ── Scrollable form ───────────────────────────────────────────────────────
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll, 1);

    QWidget *formContainer = new QWidget;
    scroll->setWidget(formContainer);

    QVBoxLayout *form = new QVBoxLayout(formContainer);
    form->setContentsMargins(32, 24, 32, 24);
    form->setSpacing(16);

    // Server Mode combo
    {
        QVBoxLayout *col = new QVBoxLayout;
        col->setSpacing(4);
        col->addWidget(new QLabel(QStringLiteral("Server Mode")));
        m_modeCombo = new QComboBox;
        m_modeCombo->addItem(QStringLiteral("Self-hosted PostgreSQL"));
        m_modeCombo->addItem(QStringLiteral("Supabase"));
        m_modeCombo->setCurrentIndex(m_controller->serverMode());
        col->addWidget(m_modeCombo);
        form->addLayout(col);
    }

    // Host + Port row
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(12);

        QVBoxLayout *hostCol = new QVBoxLayout;
        hostCol->setSpacing(4);
        hostCol->addWidget(new QLabel(QStringLiteral("Host")));
        m_hostEdit = new QLineEdit;
        m_hostEdit->setPlaceholderText(QStringLiteral("localhost"));
        m_hostEdit->setText(m_controller->host());
        hostCol->addWidget(m_hostEdit);
        row->addLayout(hostCol, 1);

        QVBoxLayout *portCol = new QVBoxLayout;
        portCol->setSpacing(4);
        portCol->addWidget(new QLabel(QStringLiteral("Port")));
        m_portEdit = new QLineEdit;
        m_portEdit->setPlaceholderText(QStringLiteral("5432"));
        m_portEdit->setText(QString::number(m_controller->port()));
        m_portEdit->setValidator(new QIntValidator(1, 65535, m_portEdit));
        m_portEdit->setFixedWidth(100);
        portCol->addWidget(m_portEdit);
        row->addLayout(portCol);

        form->addLayout(row);
    }

    // Database Name
    {
        QVBoxLayout *col = new QVBoxLayout;
        col->setSpacing(4);
        col->addWidget(new QLabel(QStringLiteral("Database Name")));
        m_dbNameEdit = new QLineEdit;
        m_dbNameEdit->setPlaceholderText(QStringLiteral("postgres"));
        m_dbNameEdit->setText(m_controller->dbName());
        col->addWidget(m_dbNameEdit);
        form->addLayout(col);
    }

    // Superuser
    {
        QVBoxLayout *col = new QVBoxLayout;
        col->setSpacing(4);
        col->addWidget(new QLabel(QStringLiteral("Superuser")));
        m_superuserEdit = new QLineEdit;
        m_superuserEdit->setPlaceholderText(QStringLiteral("postgres"));
        m_superuserEdit->setText(m_controller->superuser());
        col->addWidget(m_superuserEdit);
        form->addLayout(col);
    }

    // Password
    {
        QVBoxLayout *col = new QVBoxLayout;
        col->setSpacing(4);
        col->addWidget(new QLabel(QStringLiteral("Password")));
        m_passwordEdit = new QLineEdit;
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_passwordEdit->setPlaceholderText(QStringLiteral("••••••••"));
        m_passwordEdit->setText(m_controller->superPass());
        col->addWidget(m_passwordEdit);
        form->addLayout(col);
    }

    // ── Supabase-specific fields (shown only in Supabase mode) ───────────────
    m_supabaseFieldsWidget = new QWidget;
    {
        QFrame *supabaseFrame = new QFrame(m_supabaseFieldsWidget);
        supabaseFrame->setStyleSheet(QStringLiteral(
            "QFrame { background-color: #e8f5e9; border: 1px solid #a5d6a7; border-radius: 4px; }"));
        QVBoxLayout *sbLayout = new QVBoxLayout(supabaseFrame);
        sbLayout->setContentsMargins(12, 10, 12, 10);
        sbLayout->setSpacing(12);

        QLabel *sbDesc = new QLabel(QStringLiteral(
            "Connect to your Supabase project. The Service Role key is used to manage sync users "
            "via the Supabase Auth admin API."));
        sbDesc->setStyleSheet(QStringLiteral("font-size: 12px; color: #1b5e20; border: none;"));
        sbDesc->setWordWrap(true);
        sbLayout->addWidget(sbDesc);

        // Supabase Project URL
        {
            QVBoxLayout *col = new QVBoxLayout;
            col->setSpacing(4);
            QLabel *label = new QLabel(QStringLiteral("Supabase Project URL"));
            label->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 500; color: #1b5e20; border: none;"));
            col->addWidget(label);
            m_supabaseUrlEdit = new QLineEdit;
            m_supabaseUrlEdit->setPlaceholderText(QStringLiteral("https://xxxxxxxxxxxx.supabase.co"));
            m_supabaseUrlEdit->setText(m_controller->supabaseUrl());
            col->addWidget(m_supabaseUrlEdit);
            sbLayout->addLayout(col);
        }

        // Supabase Service Role Key
        {
            QVBoxLayout *col = new QVBoxLayout;
            col->setSpacing(4);
            QLabel *label = new QLabel(QStringLiteral("Service Role Key"));
            label->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 500; color: #1b5e20; border: none;"));
            col->addWidget(label);
            m_supabaseServiceKeyEdit = new QLineEdit;
            m_supabaseServiceKeyEdit->setEchoMode(QLineEdit::Password);
            m_supabaseServiceKeyEdit->setPlaceholderText(QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9…"));
            m_supabaseServiceKeyEdit->setText(m_controller->supabaseServiceKey());
            col->addWidget(m_supabaseServiceKeyEdit);
            sbLayout->addLayout(col);
        }

        QVBoxLayout *wrapLayout = new QVBoxLayout(m_supabaseFieldsWidget);
        wrapLayout->setContentsMargins(0, 0, 0, 0);
        wrapLayout->addWidget(supabaseFrame);
    }
    form->addWidget(m_supabaseFieldsWidget);

    // Set initial visibility based on current mode
    m_supabaseFieldsWidget->setVisible(m_controller->serverMode() == 1);

    // Status banner (hidden until test runs)
    m_statusFrame = new QFrame;
    m_statusFrame->setVisible(false);
    QHBoxLayout *statusLayout = new QHBoxLayout(m_statusFrame);
    statusLayout->setContentsMargins(12, 8, 12, 8);
    statusLayout->setSpacing(8);
    m_statusIcon = new QLabel;
    m_statusIcon->setStyleSheet(QStringLiteral("font-size: 16px;"));
    statusLayout->addWidget(m_statusIcon);
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("font-size: 13px;"));
    statusLayout->addWidget(m_statusLabel, 1);
    form->addWidget(m_statusFrame);

    // Buttons row
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(12);

        m_testButton = new QPushButton(QStringLiteral("Test Connection"));
        m_testButton->setFlat(true);
        row->addWidget(m_testButton);

        row->addStretch(1);

        m_nextButton = new QPushButton(QStringLiteral("Next →"));
        m_nextButton->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: #2196f3; color: white;"
            "  padding: 6px 16px; border-radius: 4px; border: none; }"
            "QPushButton:hover { background-color: #1976d2; }"
            "QPushButton:disabled { background-color: #b0bec5; }"));
        row->addWidget(m_nextButton);

        form->addLayout(row);
    }

    form->addStretch(1);

    // Wire field changes to controller
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectionPage::onModeChanged);
    connect(m_hostEdit,      &QLineEdit::textChanged,
            m_controller,    &AdminController::setHost);
    connect(m_portEdit,      &QLineEdit::textChanged, this, [this](const QString &t) {
        bool ok;
        const int v = t.toInt(&ok);
        if (ok) m_controller->setPort(v);
    });
    connect(m_dbNameEdit,    &QLineEdit::textChanged,
            m_controller,    &AdminController::setDbName);
    connect(m_superuserEdit, &QLineEdit::textChanged,
            m_controller,    &AdminController::setSuperuser);
    connect(m_passwordEdit,  &QLineEdit::textChanged,
            m_controller,    &AdminController::setSuperPass);
    connect(m_supabaseUrlEdit, &QLineEdit::textChanged,
            m_controller, &AdminController::setSupabaseUrl);
    connect(m_supabaseServiceKeyEdit, &QLineEdit::textChanged,
            m_controller, &AdminController::setSupabaseServiceKey);

    connect(m_testButton, &QPushButton::clicked, this, &ConnectionPage::onTestClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &ConnectionPage::onNextClicked);

    connect(m_controller, &AdminController::connectionTestResult,
            this, &ConnectionPage::onConnectionTestResult);
    connect(m_controller, &AdminController::alreadySetup,
            this, &ConnectionPage::onAlreadySetup);
    connect(m_controller, &AdminController::upgradeFinished,
            this, &ConnectionPage::onUpgradeFinished);
}

void ConnectionPage::onModeChanged(int index)
{
    m_controller->setServerMode(index);
    m_supabaseFieldsWidget->setVisible(index == 1); // 1 = Supabase
}

void ConnectionPage::onTestClicked()
{
    m_testButton->setText(QStringLiteral("Testing…"));
    m_testButton->setEnabled(false);
    hideStatus();
    m_controller->testConnection();
}

void ConnectionPage::onNextClicked()
{
    m_controller->saveConnectionSettings();

    if (!maybePromptForUpgrade())
        return;
    // If an upgrade is in flight, onUpgradeFinished() will continue the flow.
    if (m_nextButton->text() == QStringLiteral("Upgrading…"))
        return;

    m_controller->checkIfAlreadySetup();
}

bool ConnectionPage::maybePromptForUpgrade()
{
    if (!m_controller->needsUpgrade())
        return true;

    const auto choice = QMessageBox::warning(
        this,
        QStringLiteral("Database upgrade required"),
        QStringLiteral(
            "This database was set up with an older version of Project Notes "
            "and must be upgraded to work with Project Notes 5.0.1.\n\n"
            "The upgrade adds a server-managed timestamp to the sync_data "
            "table so changes are no longer missed when records carry "
            "backdated dates. It runs in a single transaction — if anything "
            "fails the database is left untouched.\n\n"
            "Upgrade the database now?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (choice != QMessageBox::Yes) {
        showStatus(true, QStringLiteral(
            "Cannot continue — Project Notes 5.0.1 requires the database upgrade. "
            "Click Next again to retry."));
        return false;
    }

    m_nextButton->setText(QStringLiteral("Upgrading…"));
    m_nextButton->setEnabled(false);
    m_testButton->setEnabled(false);
    hideStatus();

    m_controller->startUpgrade();
    return true;
}

void ConnectionPage::onUpgradeFinished(bool success, const QString &message)
{
    m_nextButton->setText(QStringLiteral("Next →"));
    m_nextButton->setEnabled(true);
    m_testButton->setEnabled(true);

    if (!success) {
        showStatus(true, QStringLiteral("Database upgrade failed: %1").arg(message));
        return;
    }

    showStatus(false, QStringLiteral("Database upgraded to Project Notes 5.0.1."));
    m_controller->checkIfAlreadySetup();
}

void ConnectionPage::onConnectionTestResult(bool success, const QString &message)
{
    m_testButton->setText(QStringLiteral("Test Connection"));
    m_testButton->setEnabled(true);
    showStatus(!success, message);
}

void ConnectionPage::onAlreadySetup(bool isSetup)
{
    if (isSetup)
        emit navigateTo(4); // go straight to Users page
    else
        emit navigateNext();
}

void ConnectionPage::showStatus(bool isError, const QString &message)
{
    m_statusIcon->setText(isError ? QStringLiteral("✗") : QStringLiteral("✓"));
    m_statusLabel->setText(message);

    if (isError) {
        m_statusFrame->setStyleSheet(QStringLiteral(
            "QFrame { background-color: #ffebee; border: 1px solid #ef9a9a; border-radius: 4px; }"));
        m_statusIcon->setStyleSheet(QStringLiteral("font-size: 16px; color: #c62828;"));
        m_statusLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #b71c1c;"));
    } else {
        m_statusFrame->setStyleSheet(QStringLiteral(
            "QFrame { background-color: #e8f5e9; border: 1px solid #a5d6a7; border-radius: 4px; }"));
        m_statusIcon->setStyleSheet(QStringLiteral("font-size: 16px; color: #2e7d32;"));
        m_statusLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #1b5e20;"));
    }
    m_statusFrame->setVisible(true);
}

void ConnectionPage::hideStatus()
{
    m_statusFrame->setVisible(false);
}
