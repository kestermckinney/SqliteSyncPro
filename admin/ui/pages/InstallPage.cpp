#include "InstallPage.h"
#include "../../backend/AdminController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include <QFrame>
#include <QShowEvent>
#include <QScrollBar>
#include <QFont>

InstallPage::InstallPage(AdminController *controller, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────────
    QWidget *header = new QWidget;
    header->setFixedHeight(72);
    header->setStyleSheet(QStringLiteral("QWidget { background-color: #eceff1; }"));
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(32, 0, 32, 0);
    headerLayout->setAlignment(Qt::AlignVCenter);
    headerLayout->setSpacing(2);

    m_titleLabel = new QLabel(QStringLiteral("Installing…"));
    m_titleLabel->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 500; color: #37474f;"));

    m_subtitleLabel = new QLabel(QStringLiteral(
        "Please wait while the database is being configured."));
    m_subtitleLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #607d8b;"));

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);
    layout->addWidget(header);

    // ── Progress bar ──────────────────────────────────────────────────────────
    QWidget *progressArea = new QWidget;
    QVBoxLayout *progressLayout = new QVBoxLayout(progressArea);
    progressLayout->setContentsMargins(32, 16, 32, 0);
    progressLayout->setSpacing(4);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, m_controller->totalSteps());
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background-color: #cfd8dc; border-radius: 3px; border: none; }"
        "QProgressBar::chunk { background-color: #2196f3; border-radius: 3px; }"));
    progressLayout->addWidget(m_progressBar);

    m_counterLabel = new QLabel(QStringLiteral("0 / %1 steps").arg(m_controller->totalSteps()));
    m_counterLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #607d8b;"));
    progressLayout->addWidget(m_counterLabel);

    layout->addWidget(progressArea);

    // ── Step log ──────────────────────────────────────────────────────────────
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
    logLayout->setContentsMargins(32, 12, 32, 0);
    logLayout->addWidget(m_log);
    layout->addWidget(logArea, 1);

    // ── Bottom buttons ────────────────────────────────────────────────────────
    QWidget *buttonArea = new QWidget;
    QHBoxLayout *buttonRow = new QHBoxLayout(buttonArea);
    buttonRow->setContentsMargins(32, 8, 32, 16);
    buttonRow->setSpacing(12);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"));
    m_cancelButton->setFlat(true);
    buttonRow->addWidget(m_cancelButton);

    buttonRow->addStretch(1);

    m_backButton = new QPushButton(QStringLiteral("← Back to Configure"));
    m_backButton->setFlat(true);
    m_backButton->setVisible(false);
    buttonRow->addWidget(m_backButton);

    layout->addWidget(buttonArea);

    // Signals
    connect(m_controller, &AdminController::stepCompleted,
            this, &InstallPage::onStepCompleted);
    connect(m_controller, &AdminController::setupFinished,
            this, &InstallPage::onSetupFinished);
    connect(m_cancelButton, &QPushButton::clicked,
            m_controller,   &AdminController::cancelSetup);
    connect(m_backButton,   &QPushButton::clicked,
            this, [this] { emit navigateTo(1); });
}

void InstallPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Reset state each time installation starts
    m_log->clear();
    m_completedSteps = 0;
    m_finished       = false;
    m_progressBar->setValue(0);
    m_counterLabel->setText(QStringLiteral("0 / %1 steps").arg(m_controller->totalSteps()));
    m_titleLabel->setText(QStringLiteral("Installing…"));
    m_titleLabel->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 500; color: #37474f;"));
    m_subtitleLabel->setText(QStringLiteral(
        "Please wait while the database is being configured."));
    m_cancelButton->setVisible(true);
    m_backButton->setVisible(false);
}

void InstallPage::onStepCompleted(int /*index*/, bool ok,
                                  const QString &stepName,
                                  const QString &detail)
{
    if (ok) {
        ++m_completedSteps;
        m_progressBar->setValue(m_completedSteps);
        m_counterLabel->setText(
            QStringLiteral("%1 / %2 steps").arg(m_completedSteps).arg(m_controller->totalSteps()));

        m_log->append(
            QStringLiteral("<span style='color:#4caf50;'>✓</span>"
                           "<span style='color:#e0e0e0;'> %1</span>")
            .arg(stepName.toHtmlEscaped()));
    } else {
        QString msg = QStringLiteral("<span style='color:#f44336;'>✗</span>"
                                     "<span style='color:#ff8a80;'> %1</span>")
                          .arg(stepName.toHtmlEscaped());
        if (!detail.isEmpty())
            msg += QStringLiteral("<br><span style='color:#ff8a80;'>&nbsp;&nbsp;%1</span>")
                       .arg(detail.toHtmlEscaped());
        m_log->append(msg);
    }

    // Scroll to bottom
    m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
}

void InstallPage::onSetupFinished(bool ok, const QString &message)
{
    m_finished = true;
    m_cancelButton->setVisible(false);

    if (ok) {
        m_titleLabel->setText(QStringLiteral("Installation Complete"));
        m_titleLabel->setStyleSheet(QStringLiteral(
            "font-size: 20px; font-weight: 500; color: #37474f;"));
        m_subtitleLabel->setText(QStringLiteral("All steps completed successfully."));
        m_counterLabel->setText(QStringLiteral("Done"));
        // MainWindow auto-navigates to SummaryPage via setupFinished signal
    } else {
        m_titleLabel->setText(QStringLiteral("Installation Failed"));
        m_titleLabel->setStyleSheet(QStringLiteral(
            "font-size: 20px; font-weight: 500; color: #c62828;"));
        m_subtitleLabel->setText(message);
        m_counterLabel->setText(QStringLiteral("Failed"));
        m_backButton->setVisible(true);
    }
}
