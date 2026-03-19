#include "MainWindow.h"
#include "backend/InvoiceController.h"
#include "sqlitesyncpro.h"

#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVariantMap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_controller(new InvoiceController(this))
{
    setWindowTitle(tr("Invoice Sync Demo"));
    resize(1100, 720);

    // ── Central widget ───────────────────────────────────────────────────────
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Header ───────────────────────────────────────────────────────────────
    QFrame *header = new QFrame;
    header->setFixedHeight(64);
    header->setStyleSheet(QStringLiteral("background-color: #546e7a;"));

    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(24, 8, 24, 8);
    headerLayout->setSpacing(2);

    QLabel *titleLabel = new QLabel(tr("Invoice Sync Demo"));
    titleLabel->setStyleSheet(QStringLiteral("color: white; font-size: 20px; font-weight: 500;"));

    QLabel *subtitleLabel = new QLabel(tr("SqliteSyncPro \u2014 two-device push / pull simulation"));
    subtitleLabel->setStyleSheet(QStringLiteral("color: #b0bec5; font-size: 12px;"));

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    QFrame *toolbar = new QFrame;
    toolbar->setFixedHeight(52);
    toolbar->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #eceff1; border-bottom: 1px solid #cfd8dc; }"));

    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 0, 16, 0);
    toolbarLayout->setSpacing(12);

    m_settingsBtn = new QPushButton(tr("Settings\u2026"));
    m_settingsBtn->setStyleSheet(QStringLiteral(
        "QPushButton { color: #37474f; background: transparent; "
        "              border: 1px solid #90a4ae; border-radius: 4px; padding: 5px 14px; }"
        "QPushButton:hover    { background: #cfd8dc; }"
        "QPushButton:disabled { color: #90a4ae; border-color: #cfd8dc; }"));

    m_progressBar = new QProgressBar;
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);   // indeterminate
    m_progressBar->setFixedWidth(120);
    m_progressBar->setFixedHeight(16);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);

    m_runTestBtn = new QPushButton(tr("Run Test"));
    m_runTestBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #009688; color: white; "
        "              border-radius: 4px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton:hover    { background-color: #00897b; }"
        "QPushButton:disabled { background-color: #b2dfdb; color: #80cbc4; }"));

    toolbarLayout->addWidget(m_settingsBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_progressBar);
    toolbarLayout->addWidget(m_runTestBtn);

    // ── Status bar ───────────────────────────────────────────────────────────
    m_statusLabel = new QLabel(tr("Ready \u2014 configure settings and click Run Test."));
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->setSizeGripEnabled(false);

    // ── Split panel ──────────────────────────────────────────────────────────
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(QStringLiteral("QSplitter::handle { background: #cfd8dc; }"));

    auto makePanel = [&](const QString &title, const QString &subtitle,
                         const QString &headerColor,
                         QTreeWidget *&treeOut) -> QWidget * {
        QWidget *panel = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Colored panel header
        QFrame *panelHeader = new QFrame;
        panelHeader->setFixedHeight(40);
        panelHeader->setStyleSheet(
            QStringLiteral("QFrame { background-color: %1; }").arg(headerColor));

        QHBoxLayout *phLayout = new QHBoxLayout(panelHeader);
        phLayout->setContentsMargins(16, 0, 16, 0);

        QLabel *titleLbl = new QLabel(title);
        titleLbl->setStyleSheet(
            QStringLiteral("color: white; font-size: 13px; font-weight: 500;"));
        QLabel *subtitleLbl = new QLabel(subtitle);
        subtitleLbl->setStyleSheet(
            QStringLiteral("color: rgba(255,255,255,0.7); font-size: 11px;"));

        phLayout->addWidget(titleLbl);
        phLayout->addStretch();
        phLayout->addWidget(subtitleLbl);

        // Tree widget
        treeOut = new QTreeWidget;
        treeOut->setColumnCount(3);
        treeOut->setHeaderLabels({tr("Invoice / Line"), tr("Details"), tr("Status")});
        treeOut->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        treeOut->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        treeOut->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        treeOut->setAlternatingRowColors(true);
        treeOut->setRootIsDecorated(true);
        treeOut->setSelectionMode(QAbstractItemView::NoSelection);
        treeOut->setFocusPolicy(Qt::NoFocus);

        layout->addWidget(panelHeader);
        layout->addWidget(treeOut, 1);

        return panel;
    };

    splitter->addWidget(makePanel(tr("SOURCE DATABASE"), tr("pushed to server \u2191"),
                                  QStringLiteral("#00796b"), m_sourceTree));
    splitter->addWidget(makePanel(tr("DESTINATION DATABASE"), tr("\u2193 pulled from server"),
                                  QStringLiteral("#3949ab"), m_destTree));
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    // ── Assemble layout ──────────────────────────────────────────────────────
    mainLayout->addWidget(header);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(splitter, 1);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_settingsBtn, &QPushButton::clicked, this, [this]() {
        SqliteSyncPro api;
        api.showSettingsDialog(this);
    });

    connect(m_runTestBtn, &QPushButton::clicked, m_controller, &InvoiceController::runSync);

    connect(m_controller, &InvoiceController::statusChanged, this, [this]() {
        m_statusLabel->setText(m_controller->statusText());
    });

    connect(m_controller, &InvoiceController::busyChanged, this, [this]() {
        const bool busy = m_controller->isBusy();
        m_settingsBtn->setEnabled(!busy);
        m_runTestBtn->setEnabled(!busy);
        m_progressBar->setVisible(busy);
    });

    connect(m_controller, &InvoiceController::sourceRecordsChanged, this, [this]() {
        populateTree(m_sourceTree, m_controller->sourceRecords());
    });

    connect(m_controller, &InvoiceController::destRecordsChanged, this, [this]() {
        populateTree(m_destTree, m_controller->destRecords());
    });
}

void MainWindow::populateTree(QTreeWidget *tree, const QVariantList &records)
{
    tree->clear();

    QFont boldFont = tree->font();
    boldFont.setBold(true);

    QTreeWidgetItem *currentInvoice = nullptr;

    for (const QVariant &v : records) {
        const QVariantMap row    = v.toMap();
        const bool        synced = row[QStringLiteral("synced")].toBool();
        const QString statusText = synced ? tr("\u2713 synced") : tr("\u23f3 pending");
        const QColor  statusColor = synced ? QColor(0x00897b) : QColor(0xf57c00);

        if (row[QStringLiteral("rowType")] == QLatin1String("invoice")) {
            currentInvoice = new QTreeWidgetItem(tree);
            currentInvoice->setText(0, row[QStringLiteral("invoiceNumber")].toString());
            currentInvoice->setText(1, row[QStringLiteral("address")].toString());
            currentInvoice->setText(2, statusText);
            currentInvoice->setFont(0, boldFont);
            currentInvoice->setForeground(2, statusColor);
        } else {
            if (!currentInvoice)
                continue;
            QTreeWidgetItem *line = new QTreeWidgetItem(currentInvoice);
            line->setText(0, tr("Line %1  \u2022  %2")
                .arg(row[QStringLiteral("lineNumber")].toInt())
                .arg(row[QStringLiteral("description")].toString()));
            line->setText(1, tr("qty: %1   price: $%2")
                .arg(row[QStringLiteral("quantity")].toDouble(), 0, 'f', 1)
                .arg(row[QStringLiteral("price")].toDouble(), 0, 'f', 2));
            line->setText(2, statusText);
            line->setForeground(2, statusColor);
        }
    }

    tree->expandAll();
}
