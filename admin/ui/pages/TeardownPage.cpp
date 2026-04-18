// Copyright (C) 2026 Paul McKinney
#include "TeardownPage.h"
#include "../../backend/AdminController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QStackedWidget>
#include <QScrollArea>
#include <QFrame>
#include <QFont>
#include <QScrollBar>

static const QString kConfirmPhrase = QStringLiteral("YES PLEASE");

TeardownPage::TeardownPage(AdminController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ── Deep-red danger header ────────────────────────────────────────────────
    QWidget *header = new QWidget;
    header->setFixedHeight(80);
    header->setStyleSheet(QStringLiteral("QWidget { background-color: #b71c1c; }"));

    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(32, 0, 32, 0);
    headerLayout->setAlignment(Qt::AlignVCenter);
    headerLayout->setSpacing(2);

    QLabel *headerTitle = new QLabel(QStringLiteral("\u26a0  Remove All Objects"));
    headerTitle->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 600; color: white;"));

    QLabel *headerSub = new QLabel(QStringLiteral(
        "Permanently delete all SQLSync database objects and user data."));
    headerSub->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: rgba(255,255,255,0.8);"));

    headerLayout->addWidget(headerTitle);
    headerLayout->addWidget(headerSub);
    outerLayout->addWidget(header);

    // ── Two-panel stack (confirmation / progress) ─────────────────────────────
    m_panels = new QStackedWidget;
    outerLayout->addWidget(m_panels, 1);

    // ═══════════════════════════════════════════════════════════════════════════
    // PANEL 0 — Confirmation
    // ═══════════════════════════════════════════════════════════════════════════
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(QStringLiteral("background: white;"));

    QWidget *scrollContent = new QWidget;
    QVBoxLayout *confLayout = new QVBoxLayout(scrollContent);
    confLayout->setContentsMargins(40, 28, 40, 28);
    confLayout->setSpacing(20);

    // ── What will be deleted box ──────────────────────────────────────────────
    QFrame *deleteBox = new QFrame;
    deleteBox->setStyleSheet(QStringLiteral(
        "QFrame { border: 2px solid #e53935; border-radius: 6px; background: #fff8f8; }"));
    QVBoxLayout *deleteLayout = new QVBoxLayout(deleteBox);
    deleteLayout->setContentsMargins(20, 16, 20, 16);
    deleteLayout->setSpacing(10);

    QLabel *deleteTitle = new QLabel(QStringLiteral(
        "\u26a0\u00a0\u00a0WARNING — The following will be PERMANENTLY DELETED:"));
    deleteTitle->setStyleSheet(QStringLiteral(
        "font-size: 14px; font-weight: 700; color: #b71c1c; border: none;"));
    deleteLayout->addWidget(deleteTitle);

    auto addSection = [&](const QString &heading, const QStringList &items) {
        QLabel *h = new QLabel(heading);
        h->setStyleSheet(QStringLiteral(
            "font-size: 12px; font-weight: 700; color: #37474f; "
            "margin-top: 6px; border: none;"));
        deleteLayout->addWidget(h);
        for (const QString &item : items) {
            QLabel *l = new QLabel(QStringLiteral("\u2022\u2002") + item);
            l->setStyleSheet(QStringLiteral(
                "font-size: 13px; color: #37474f; border: none;"));
            l->setWordWrap(true);
            deleteLayout->addWidget(l);
        }
    };

    addSection(QStringLiteral("Tables — ALL DATA WILL BE LOST:"), {
        QStringLiteral("sync_data — every sync record from every device"),
        QStringLiteral("auth_users — every user account and password hash"),
    });

    addSection(QStringLiteral("Functions:"), {
        QStringLiteral("rpc_login — user authentication"),
        QStringLiteral("_sign_jwt, _verify, _algorithm_sign, _try_cast_double, _url_encode"),
    });

    addSection(QStringLiteral("Database Roles (self-hosted only):"), {
        QStringLiteral("pnanon, pnapp_user, pnauthenticator, postgrest_db"),
    });

    confLayout->addWidget(deleteBox);

    // ── Cannot be undone box ──────────────────────────────────────────────────
    QFrame *undoneBox = new QFrame;
    undoneBox->setStyleSheet(QStringLiteral(
        "QFrame { border: 2px solid #f57f17; border-radius: 6px; background: #fffde7; }"));
    QVBoxLayout *undoneLayout = new QVBoxLayout(undoneBox);
    undoneLayout->setContentsMargins(20, 14, 20, 14);
    undoneLayout->setSpacing(4);

    QLabel *undoneTitle = new QLabel(QStringLiteral(
        "\u26a0\u00a0\u00a0This operation CANNOT BE UNDONE"));
    undoneTitle->setStyleSheet(QStringLiteral(
        "font-size: 13px; font-weight: 700; color: #e65100; border: none;"));

    QLabel *undoneBody = new QLabel(QStringLiteral(
        "There is no recovery from this action. All sync records and user accounts "
        "will be gone permanently. Make sure you have a database backup before proceeding."));
    undoneBody->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: #5d4037; border: none;"));
    undoneBody->setWordWrap(true);

    undoneLayout->addWidget(undoneTitle);
    undoneLayout->addWidget(undoneBody);
    confLayout->addWidget(undoneBox);

    // ── Confirmation input ────────────────────────────────────────────────────
    QFrame *confirmBox = new QFrame;
    confirmBox->setStyleSheet(QStringLiteral(
        "QFrame { border: 1px solid #cfd8dc; border-radius: 6px; background: #fafafa; }"));
    QVBoxLayout *confirmLayout = new QVBoxLayout(confirmBox);
    confirmLayout->setContentsMargins(20, 16, 20, 16);
    confirmLayout->setSpacing(8);

    QLabel *confirmPrompt = new QLabel(QStringLiteral(
        "To confirm, type  <b>YES PLEASE</b>  exactly as shown:"));
    confirmPrompt->setStyleSheet(QStringLiteral(
        "font-size: 13px; color: #37474f; border: none;"));

    m_confirmEdit = new QLineEdit;
    m_confirmEdit->setPlaceholderText(QStringLiteral("Type YES PLEASE here"));
    m_confirmEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { font-size: 14px; font-weight: 600; padding: 8px 12px; "
        "  border: 2px solid #ef9a9a; border-radius: 4px; background: white; color: #37474f; }"
        "QLineEdit:focus { border-color: #e53935; }"));

    confirmLayout->addWidget(confirmPrompt);
    confirmLayout->addWidget(m_confirmEdit);
    confLayout->addWidget(confirmBox);

    confLayout->addStretch();

    // ── Button row ────────────────────────────────────────────────────────────
    QWidget *buttonArea = new QWidget;
    QHBoxLayout *buttonRow = new QHBoxLayout(buttonArea);
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(12);
    buttonRow->addStretch();

    m_removeButton = new QPushButton(QStringLiteral("\u26a0  Remove All Objects"));
    m_removeButton->setEnabled(false);
    m_removeButton->setFixedHeight(38);
    m_removeButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #b71c1c; color: white; font-size: 13px; font-weight: 600;"
        "  border-radius: 4px; padding: 0 24px; }"
        "QPushButton:hover    { background-color: #c62828; }"
        "QPushButton:disabled { background-color: #ef9a9a; color: rgba(255,255,255,0.6); }"));
    buttonRow->addWidget(m_removeButton);

    confLayout->addWidget(buttonArea);

    scrollArea->setWidget(scrollContent);
    m_panels->addWidget(scrollArea); // index 0

    // ═══════════════════════════════════════════════════════════════════════════
    // PANEL 1 — Progress
    // ═══════════════════════════════════════════════════════════════════════════
    QWidget *progressPanel = new QWidget;
    QVBoxLayout *progressLayout = new QVBoxLayout(progressPanel);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(0);

    // Progress bar area
    QWidget *progressArea = new QWidget;
    QVBoxLayout *paLayout = new QVBoxLayout(progressArea);
    paLayout->setContentsMargins(32, 16, 32, 0);
    paLayout->setSpacing(4);

    m_statusTitle = new QLabel(QStringLiteral("Removing objects\u2026"));
    m_statusTitle->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 500; color: #37474f;"));

    m_statusSubtitle = new QLabel(QStringLiteral(
        "Please wait while database objects are being removed."));
    m_statusSubtitle->setStyleSheet(QStringLiteral("font-size: 12px; color: #607d8b;"));

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, m_controller->teardownTotalSteps());
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background-color: #cfd8dc; border-radius: 3px; border: none; }"
        "QProgressBar::chunk { background-color: #e53935; border-radius: 3px; }"));

    m_counterLabel = new QLabel(QStringLiteral("0 / %1 steps")
                                    .arg(m_controller->teardownTotalSteps()));
    m_counterLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #607d8b;"));

    paLayout->addWidget(m_statusTitle);
    paLayout->addWidget(m_statusSubtitle);
    paLayout->addSpacing(8);
    paLayout->addWidget(m_progressBar);
    paLayout->addWidget(m_counterLabel);
    progressLayout->addWidget(progressArea);

    // Log
    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    m_log->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: #1a1a2e; color: #e0e0e0;"
        "  border: none; border-radius: 4px; padding: 8px; }"));
    QFont monoFont(QStringLiteral("monospace"));
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(10);
    m_log->setFont(monoFont);

    QWidget *logArea = new QWidget;
    QVBoxLayout *logLayout = new QVBoxLayout(logArea);
    logLayout->setContentsMargins(32, 12, 32, 16);
    logLayout->addWidget(m_log);
    progressLayout->addWidget(logArea, 1);

    m_panels->addWidget(progressPanel); // index 1

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_confirmEdit, &QLineEdit::textChanged,
            this, &TeardownPage::onConfirmTextChanged);

    connect(m_removeButton, &QPushButton::clicked,
            this, &TeardownPage::onRemoveClicked);

    connect(m_controller, &AdminController::teardownStepCompleted,
            this, &TeardownPage::onStepCompleted);

    connect(m_controller, &AdminController::teardownFinished,
            this, &TeardownPage::onTeardownFinished);
}

void TeardownPage::onConfirmTextChanged(const QString &text)
{
    m_removeButton->setEnabled(text == kConfirmPhrase);
}

void TeardownPage::onRemoveClicked()
{
    // Switch to progress panel and start
    m_completedSteps = 0;
    m_log->clear();
    m_progressBar->setRange(0, m_controller->teardownTotalSteps());
    m_progressBar->setValue(0);
    m_counterLabel->setText(QStringLiteral("0 / %1 steps")
                                .arg(m_controller->teardownTotalSteps()));
    m_statusTitle->setText(QStringLiteral("Removing objects\u2026"));
    m_statusTitle->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 500; color: #37474f;"));
    m_statusSubtitle->setText(QStringLiteral(
        "Please wait while database objects are being removed."));

    m_panels->setCurrentIndex(1);
    m_controller->startTeardown();
}

void TeardownPage::onStepCompleted(int /*index*/, bool ok,
                                    const QString &stepName,
                                    const QString &detail)
{
    ++m_completedSteps;
    m_progressBar->setValue(m_completedSteps);
    m_counterLabel->setText(
        QStringLiteral("%1 / %2 steps")
            .arg(m_completedSteps)
            .arg(m_controller->teardownTotalSteps()));

    if (ok) {
        m_log->append(
            QStringLiteral("<span style='color:#4caf50;'>\u2713</span>"
                           "<span style='color:#e0e0e0;'> %1</span>")
            .arg(stepName.toHtmlEscaped()));
    } else {
        QString msg = QStringLiteral("<span style='color:#f44336;'>\u2717</span>"
                                     "<span style='color:#ff8a80;'> %1</span>")
                          .arg(stepName.toHtmlEscaped());
        if (!detail.isEmpty())
            msg += QStringLiteral("<br><span style='color:#ff8a80;'>"
                                  "&nbsp;&nbsp;%1</span>")
                       .arg(detail.toHtmlEscaped());
        m_log->append(msg);
    }

    m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
}

void TeardownPage::onTeardownFinished(bool success, const QString &message)
{
    if (success) {
        m_statusTitle->setText(QStringLiteral("Removal Complete"));
        m_statusTitle->setStyleSheet(QStringLiteral(
            "font-size: 15px; font-weight: 500; color: #2e7d32;"));
        m_statusSubtitle->setText(QStringLiteral(
            "All SQLSync objects have been removed from the database."));
        m_counterLabel->setText(QStringLiteral("Done"));
    } else {
        m_statusTitle->setText(QStringLiteral("Removal Failed"));
        m_statusTitle->setStyleSheet(QStringLiteral(
            "font-size: 15px; font-weight: 500; color: #b71c1c;"));
        m_statusSubtitle->setText(message);
        m_counterLabel->setText(QStringLiteral("Failed"));
    }
}
