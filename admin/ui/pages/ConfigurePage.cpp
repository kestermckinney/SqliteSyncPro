// Copyright (C) 2026 Paul McKinney
#include "ConfigurePage.h"
#include "../../backend/AdminController.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QShowEvent>

ConfigurePage::ConfigurePage(AdminController *controller, QWidget *parent)
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

    QLabel *title = new QLabel(QStringLiteral("Review Configuration"));
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 500; color: #37474f;"));

    QLabel *subtitle = new QLabel(QStringLiteral(
        "Confirm the settings below, then click Install to proceed."));
    subtitle->setStyleSheet(QStringLiteral("font-size: 13px; color: #607d8b;"));

    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
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

    // ── Summary card ──────────────────────────────────────────────────────────
    QFrame *card = new QFrame;
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #eceff1; border: 1px solid #b0bec5; border-radius: 6px; }"));

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(10);

    QLabel *cardTitle = new QLabel(QStringLiteral("PostgreSQL Connection"));
    cardTitle->setStyleSheet(QStringLiteral(
        "font-size: 13px; font-weight: 500; color: #455a64; border: none;"));
    cardLayout->addWidget(cardTitle);

    QGridLayout *grid = new QGridLayout;
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(6);

    auto addRow = [&](int row, const QString &key, QLabel **valueLabel) {
        QLabel *k = new QLabel(key);
        k->setStyleSheet(QStringLiteral("font-size: 13px; color: #607d8b; border: none;"));
        *valueLabel = new QLabel;
        (*valueLabel)->setStyleSheet(QStringLiteral("font-size: 13px; border: none;"));
        grid->addWidget(k,           row, 0);
        grid->addWidget(*valueLabel, row, 1);
    };

    addRow(0, QStringLiteral("Host:"),      &m_hostPortLabel);
    QLabel *dbLabel = nullptr;
    addRow(1, QStringLiteral("Database:"),  &dbLabel);
    dbLabel->setText(QStringLiteral("sqlitesyncpro (auto-created)"));
    addRow(2, QStringLiteral("Superuser:"), &m_superuserLabel);

    cardLayout->addLayout(grid);
    layout->addWidget(card);

    // ── What will be installed ─────────────────────────────────────────────────
    QLabel *installTitle = new QLabel(QStringLiteral(
        "The following will be installed in your PostgreSQL database:"));
    installTitle->setStyleSheet(QStringLiteral("font-size: 13px; color: #455a64;"));
    installTitle->setWordWrap(true);
    layout->addWidget(installTitle);

    const QStringList items = {
        QStringLiteral("pgcrypto extension"),
        QStringLiteral("Roles: authenticator, anon, app_user (skipped automatically on hosted services)"),
        QStringLiteral("JWT helper functions (_base64url_encode, _sign_jwt)"),
        QStringLiteral("auth_users table with bcrypt password hashing"),
        QStringLiteral("sync_data table with Row-Level Security"),
        QStringLiteral("rpc_login function (JWT secret embedded in function body)")
    };

    QVBoxLayout *bulletLayout = new QVBoxLayout;
    bulletLayout->setSpacing(6);
    for (const QString &item : items) {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(8);
        QLabel *bullet = new QLabel(QStringLiteral("•"));
        bullet->setStyleSheet(QStringLiteral("font-size: 16px; color: #2196f3;"));
        bullet->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        QLabel *text = new QLabel(item);
        text->setStyleSheet(QStringLiteral("font-size: 13px;"));
        text->setWordWrap(true);
        row->addWidget(bullet);
        row->addWidget(text, 1);
        bulletLayout->addLayout(row);
    }
    layout->addLayout(bulletLayout);

    // ── Warning note ──────────────────────────────────────────────────────────
    QFrame *warning = new QFrame;
    warning->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #fff8e1; border: 1px solid #ffd54f; border-radius: 4px; }"));

    QHBoxLayout *warnLayout = new QHBoxLayout(warning);
    warnLayout->setContentsMargins(12, 10, 12, 10);
    warnLayout->setSpacing(8);

    QLabel *warnIcon = new QLabel(QStringLiteral("⚠"));
    warnIcon->setStyleSheet(QStringLiteral("font-size: 18px; color: #e65100; border: none;"));
    warnIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    warnLayout->addWidget(warnIcon);

    QLabel *warnText = new QLabel(QStringLiteral(
        "The installation is idempotent — running it on an existing database is safe. "
        "Roles and tables that already exist will be updated, not duplicated."));
    warnText->setStyleSheet(QStringLiteral("font-size: 13px; color: #bf360c; border: none;"));
    warnText->setWordWrap(true);
    warnLayout->addWidget(warnText, 1);

    layout->addWidget(warning);

    // ── Buttons ───────────────────────────────────────────────────────────────
    {
        QHBoxLayout *row = new QHBoxLayout;

        QPushButton *backBtn = new QPushButton(QStringLiteral("← Back"));
        backBtn->setFlat(true);
        row->addWidget(backBtn);

        row->addStretch(1);

        QPushButton *installBtn = new QPushButton(QStringLiteral("Install →"));
        installBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: #2196f3; color: white;"
            "  padding: 6px 16px; border-radius: 4px; border: none; }"
            "QPushButton:hover { background-color: #1976d2; }"));
        row->addWidget(installBtn);

        layout->addLayout(row);

        connect(backBtn,    &QPushButton::clicked, this, [this] { emit navigateTo(0); });
        connect(installBtn, &QPushButton::clicked, this, [this] {
            emit navigateNext();        // advance to InstallPage
            m_controller->startSetup(); // kick off worker
        });
    }

    layout->addStretch(1);
}

void ConfigurePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Refresh labels from the controller each time the page is shown
    m_hostPortLabel->setText(
        QStringLiteral("%1:%2").arg(m_controller->host()).arg(m_controller->port()));
    m_superuserLabel->setText(m_controller->superuser());
}
