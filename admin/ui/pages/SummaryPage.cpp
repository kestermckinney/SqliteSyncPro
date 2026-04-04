// Copyright (C) 2026 Paul McKinney
#include "SummaryPage.h"
#include "../../backend/AdminController.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QFrame>
#include <QPushButton>
#include <QShowEvent>
#include <QFont>

// Helper: builds a read-only credential row (label + monospace field + Copy button)
static QWidget *credentialRow(const QString &labelText,
                               QLineEdit **fieldOut,
                               AdminController *controller,
                               std::function<QString()> getValue)
{
    QFrame *frame = new QFrame;
    frame->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #eceff1; border: 1px solid #b0bec5; border-radius: 4px; }"));

    QHBoxLayout *row = new QHBoxLayout(frame);
    row->setContentsMargins(12, 10, 12, 10);
    row->setSpacing(12);

    QVBoxLayout *col = new QVBoxLayout;
    col->setSpacing(2);

    QLabel *keyLabel = new QLabel(labelText);
    keyLabel->setStyleSheet(QStringLiteral(
        "font-size: 11px; font-weight: 500; color: #607d8b; border: none;"));
    col->addWidget(keyLabel);

    QLineEdit *field = new QLineEdit;
    field->setReadOnly(true);
    QFont mono(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    field->setFont(mono);
    field->setStyleSheet(QStringLiteral(
        "QLineEdit { border: none; background: transparent; color: #212121; }"));
    col->addWidget(field);
    *fieldOut = field;

    row->addLayout(col, 1);

    QPushButton *copyBtn = new QPushButton(QStringLiteral("Copy"));
    copyBtn->setFlat(true);
    copyBtn->setStyleSheet(QStringLiteral("QPushButton { color: #2196f3; }"));
    row->addWidget(copyBtn);

    QObject::connect(copyBtn, &QPushButton::clicked, [controller, getValue] {
        controller->copyToClipboard(getValue());
    });

    return frame;
}

SummaryPage::SummaryPage(AdminController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ── Header (green) ────────────────────────────────────────────────────────
    QWidget *header = new QWidget;
    header->setFixedHeight(72);
    header->setStyleSheet(QStringLiteral("QWidget { background-color: #e8f5e9; }"));
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(32, 0, 32, 0);
    headerLayout->setSpacing(12);

    QLabel *checkmark = new QLabel(QStringLiteral("✓"));
    checkmark->setStyleSheet(QStringLiteral("font-size: 28px; color: #43a047;"));
    headerLayout->addWidget(checkmark);

    QVBoxLayout *headerText = new QVBoxLayout;
    headerText->setSpacing(2);
    QLabel *title = new QLabel(QStringLiteral("Setup Complete"));
    title->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 500; color: #1b5e20;"));
    QLabel *subtitle = new QLabel(QStringLiteral(
        "Your PostgreSQL database is ready for SQLSync."));
    subtitle->setStyleSheet(QStringLiteral("font-size: 13px; color: #2e7d32;"));
    headerText->addWidget(title);
    headerText->addWidget(subtitle);
    headerLayout->addLayout(headerText, 1);

    outer->addWidget(header);

    // ── Scrollable content ────────────────────────────────────────────────────
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll, 1);

    QWidget *container = new QWidget;
    scroll->setWidget(container);

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(20);

    // ── Generated Credentials ─────────────────────────────────────────────────
    QLabel *credTitle = new QLabel(QStringLiteral("Generated Credentials"));
    credTitle->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 500; color: #37474f;"));
    layout->addWidget(credTitle);

    QLabel *saveWarning = new QLabel(QStringLiteral(
        "Save these values — the passwords cannot be recovered after you close this window."));
    saveWarning->setStyleSheet(QStringLiteral("font-size: 13px; color: #c62828;"));
    saveWarning->setWordWrap(true);
    layout->addWidget(saveWarning);

    // Self-hosted credential rows — hidden in Neon mode
    m_selfHostedCredentials = new QWidget;
    {
        QVBoxLayout *credLayout = new QVBoxLayout(m_selfHostedCredentials);
        credLayout->setContentsMargins(0, 0, 0, 0);
        credLayout->setSpacing(12);

        credLayout->addWidget(credentialRow(
            QStringLiteral("AUTHENTICATOR PASSWORD"),
            &m_authPasswordEdit,
            m_controller,
            [this] { return m_controller->authenticatorPassword(); }));

        credLayout->addWidget(credentialRow(
            QStringLiteral("JWT SECRET"),
            &m_jwtSecretEdit,
            m_controller,
            [this] { return m_controller->jwtSecret(); }));
    }
    layout->addWidget(m_selfHostedCredentials);

    // Supabase mode note (hidden in self-hosted mode)
    m_supabaseNote = new QLabel(QStringLiteral(
        "Supabase Auth manages authentication and user accounts. "
        "Configure PostgREST with your Supabase JWT secret "
        "(Supabase Dashboard → Settings → API → JWT Settings → JWT Secret). "
        "No authenticator password or JWT secret is generated by this setup."));
    m_supabaseNote->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: #1b5e20; background-color: #e8f5e9; "
        "border: 1px solid #a5d6a7; border-radius: 4px; padding: 10px;"));
    m_supabaseNote->setWordWrap(true);
    layout->addWidget(m_supabaseNote);

    // ── Divider ───────────────────────────────────────────────────────────────
    QFrame *div1 = new QFrame;
    div1->setFrameShape(QFrame::HLine);
    div1->setStyleSheet(QStringLiteral("color: #b0bec5;"));
    layout->addWidget(div1);

    // ── PostgREST Configuration ───────────────────────────────────────────────
    QLabel *configTitle = new QLabel(QStringLiteral("PostgREST Configuration"));
    configTitle->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 500; color: #37474f;"));
    layout->addWidget(configTitle);

    QLabel *configSub = new QLabel(QStringLiteral(
        "Paste this block into your postgrest.conf file:"));
    configSub->setStyleSheet(QStringLiteral("font-size: 13px; color: #607d8b;"));
    layout->addWidget(configSub);

    // Dark code block with Copy button overlaid at top-right
    QFrame *codeFrame = new QFrame;
    codeFrame->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #1a1a2e; border-radius: 4px; }"));

    QVBoxLayout *codeLayout = new QVBoxLayout(codeFrame);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setSpacing(0);

    // Top bar with Copy button
    QWidget *codeTopBar = new QWidget;
    codeTopBar->setStyleSheet(QStringLiteral("background: transparent;"));
    QHBoxLayout *codeTopLayout = new QHBoxLayout(codeTopBar);
    codeTopLayout->setContentsMargins(0, 6, 8, 0);
    codeTopLayout->addStretch(1);
    QPushButton *copyConfigBtn = new QPushButton(QStringLiteral("Copy"));
    copyConfigBtn->setFlat(true);
    copyConfigBtn->setStyleSheet(QStringLiteral(
        "QPushButton { color: #90caf9; background: transparent; border: none; }"));
    codeTopLayout->addWidget(copyConfigBtn);
    codeLayout->addWidget(codeTopBar);

    m_configEdit = new QTextEdit;
    m_configEdit->setReadOnly(true);
    m_configEdit->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: #1a1a2e; color: #e0e0e0;"
        "  border: none; padding: 4px 12px 12px 12px; }"));
    QFont mono(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    m_configEdit->setFont(mono);
    m_configEdit->setFixedHeight(140);
    codeLayout->addWidget(m_configEdit);

    layout->addWidget(codeFrame);

    connect(copyConfigBtn, &QPushButton::clicked, this, [this] {
        m_controller->copyToClipboard(m_controller->postgrestConfig());
    });

    // ── Divider ───────────────────────────────────────────────────────────────
    QFrame *div2 = new QFrame;
    div2->setFrameShape(QFrame::HLine);
    div2->setStyleSheet(QStringLiteral("color: #b0bec5;"));
    layout->addWidget(div2);

    // ── Next Steps ────────────────────────────────────────────────────────────
    QLabel *nextTitle = new QLabel(QStringLiteral("Next Steps"));
    nextTitle->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 500; color: #37474f;"));
    layout->addWidget(nextTitle);

    const QStringList steps = {
        QStringLiteral("Install PostgREST if you haven't already (postgrest.org)"),
        QStringLiteral("Create your postgrest.conf using the block above"),
        QStringLiteral("Start PostgREST: postgrest postgrest.conf"),
        QStringLiteral("Add sync users using the Users page →")
    };

    QVBoxLayout *stepsLayout = new QVBoxLayout;
    stepsLayout->setSpacing(6);
    for (int i = 0; i < steps.size(); ++i) {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(8);
        QLabel *num = new QLabel(QStringLiteral("%1.").arg(i + 1));
        num->setStyleSheet(QStringLiteral("font-size: 13px; color: #2196f3;"));
        num->setFixedWidth(20);
        QLabel *text = new QLabel(steps.at(i));
        text->setStyleSheet(QStringLiteral("font-size: 13px;"));
        text->setWordWrap(true);
        row->addWidget(num);
        row->addWidget(text, 1);
        stepsLayout->addLayout(row);
    }
    layout->addLayout(stepsLayout);

    // Manage Users button
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->addStretch(1);
        QPushButton *usersBtn = new QPushButton(QStringLiteral("Manage Users →"));
        usersBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: #2196f3; color: white;"
            "  padding: 6px 16px; border-radius: 4px; border: none; }"
            "QPushButton:hover { background-color: #1976d2; }"));
        row->addWidget(usersBtn);
        layout->addLayout(row);
        connect(usersBtn, &QPushButton::clicked, this, [this] { emit navigateTo(4); });
    }

    layout->addStretch(1);
}

void SummaryPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    const bool supabase = m_controller->isSupabaseMode();

    // Show/hide credential sections based on mode
    m_selfHostedCredentials->setVisible(!supabase);
    m_supabaseNote->setVisible(supabase);

    if (!supabase) {
        m_authPasswordEdit->setText(m_controller->authenticatorPassword());
        m_jwtSecretEdit->setText(m_controller->jwtSecret());
    }

    m_configEdit->setPlainText(m_controller->postgrestConfig());
}
