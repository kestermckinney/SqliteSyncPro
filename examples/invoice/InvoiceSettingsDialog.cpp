// Copyright (C) 2026 Paul McKinney
#include "InvoiceSettingsDialog.h"
#include "sqlitesyncpro.h"

#include <QByteArray>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSysInfo>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// Obfuscation helpers (XOR with machine-derived key + base64)
// ---------------------------------------------------------------------------

static QByteArray obfKey()
{
    QByteArray uid = QSysInfo::machineUniqueId();
    if (uid.isEmpty())
        uid = QByteArrayLiteral("sqlitesyncpro-invoice-fallback");
    return QCryptographicHash::hash(uid, QCryptographicHash::Sha256);
}

static QString obfuscate(const QString &plain)
{
    if (plain.isEmpty()) return {};
    QByteArray data = plain.toUtf8();
    const QByteArray key = obfKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromLatin1(data.toBase64());
}

static QString deobfuscate(const QString &stored)
{
    if (stored.isEmpty()) return {};
    QByteArray data = QByteArray::fromBase64(stored.toLatin1());
    const QByteArray key = obfKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromUtf8(data);
}

static constexpr char kOrg[]   = "SqliteSyncPro";
static constexpr char kApp[]   = "InvoiceDemo";
static constexpr char kGroup[] = "settings";

// ---------------------------------------------------------------------------
// InvoiceSettingsDialog
// ---------------------------------------------------------------------------

InvoiceSettingsDialog::InvoiceSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Invoice Demo Settings"));
    setMinimumWidth(520);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setSpacing(12);

    // ── Database paths ───────────────────────────────────────────────────────
    auto *dbGroup = new QGroupBox(tr("Database Paths"));
    auto *dbForm  = new QFormLayout(dbGroup);
    dbForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_clientAPathEdit  = new QLineEdit;
    m_clientABrowseBtn = new QPushButton(tr("Browse…"));
    m_clientABrowseBtn->setFixedWidth(72);
    auto *rowA = new QHBoxLayout;
    rowA->addWidget(m_clientAPathEdit);
    rowA->addWidget(m_clientABrowseBtn);
    dbForm->addRow(tr("Client A DB:"), rowA);

    m_clientBPathEdit  = new QLineEdit;
    m_clientBBrowseBtn = new QPushButton(tr("Browse…"));
    m_clientBBrowseBtn->setFixedWidth(72);
    auto *rowB = new QHBoxLayout;
    rowB->addWidget(m_clientBPathEdit);
    rowB->addWidget(m_clientBBrowseBtn);
    dbForm->addRow(tr("Client B DB:"), rowB);

    outerLayout->addWidget(dbGroup);

    // ── Connection settings ──────────────────────────────────────────────────
    auto *connGroup = new QGroupBox(tr("Sync Connection"));
    auto *connForm  = new QFormLayout(connGroup);
    connForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_hostTypeCombo = new QComboBox;
    m_hostTypeCombo->addItem(tr("Self-Hosted"));
    m_hostTypeCombo->addItem(tr("Supabase"));
    connForm->addRow(tr("Host Type:"), m_hostTypeCombo);

    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(tr("http://localhost:3000  or  https://xxxx.supabase.co"));
    connForm->addRow(tr("Server URL:"), m_urlEdit);

    m_emailEdit = new QLineEdit;
    m_emailEdit->setPlaceholderText(tr("user@example.com"));
    connForm->addRow(tr("Username (Email):"), m_emailEdit);

    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    connForm->addRow(tr("Password:"), m_passwordEdit);

    m_supabaseKeyLabel = new QLabel(tr("Supabase Anon Key:"));
    m_supabaseKeyEdit  = new QLineEdit;
    m_supabaseKeyEdit->setEchoMode(QLineEdit::Password);
    m_supabaseKeyEdit->setPlaceholderText(tr("eyJ… (anon key from Supabase Dashboard)"));
    connForm->addRow(m_supabaseKeyLabel, m_supabaseKeyEdit);

    m_encPhraseEdit = new QLineEdit;
    m_encPhraseEdit->setEchoMode(QLineEdit::Password);
    m_encPhraseEdit->setPlaceholderText(tr("(optional — leave blank to disable encryption)"));
    connForm->addRow(tr("Encryption Phrase:"), m_encPhraseEdit);

    outerLayout->addWidget(connGroup);

    // ── Buttons ──────────────────────────────────────────────────────────────
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    outerLayout->addWidget(m_buttonBox);

    connect(m_hostTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &InvoiceSettingsDialog::onHostTypeChanged);
    connect(m_clientABrowseBtn, &QPushButton::clicked,
            this, &InvoiceSettingsDialog::browseClientAPath);
    connect(m_clientBBrowseBtn, &QPushButton::clicked,
            this, &InvoiceSettingsDialog::browseClientBPath);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadSettings();
    onHostTypeChanged(m_hostTypeCombo->currentIndex());
}

void InvoiceSettingsDialog::onHostTypeChanged(int index)
{
    const bool isSupabase = (index == 1);
    m_supabaseKeyLabel->setVisible(isSupabase);
    m_supabaseKeyEdit->setVisible(isSupabase);
}

void InvoiceSettingsDialog::browseClientAPath()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Client A Database File"),
        m_clientAPathEdit->text().isEmpty()
            ? QDir::homePath() + QStringLiteral("/client_a.db")
            : m_clientAPathEdit->text(),
        tr("SQLite Database (*.db *.sqlite);;All Files (*)"));
    if (!path.isEmpty())
        m_clientAPathEdit->setText(path);
}

void InvoiceSettingsDialog::browseClientBPath()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Client B Database File"),
        m_clientBPathEdit->text().isEmpty()
            ? QDir::homePath() + QStringLiteral("/client_b.db")
            : m_clientBPathEdit->text(),
        tr("SQLite Database (*.db *.sqlite);;All Files (*)"));
    if (!path.isEmpty())
        m_clientBPathEdit->setText(path);
}

void InvoiceSettingsDialog::loadSettings()
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));

    const QString defA = QDir::tempPath() + QStringLiteral("/sqsp_invoice_client_a.db");
    const QString defB = QDir::tempPath() + QStringLiteral("/sqsp_invoice_client_b.db");

    m_clientAPathEdit->setText(s.value(QStringLiteral("clientAPath"), defA).toString());
    m_clientBPathEdit->setText(s.value(QStringLiteral("clientBPath"), defB).toString());
    m_hostTypeCombo->setCurrentIndex(s.value(QStringLiteral("syncHostType"), 0).toInt());
    m_urlEdit->setText(s.value(QStringLiteral("postgrestUrl")).toString());
    m_emailEdit->setText(s.value(QStringLiteral("email")).toString());
    m_passwordEdit->setText(deobfuscate(s.value(QStringLiteral("password")).toString()));
    m_supabaseKeyEdit->setText(deobfuscate(s.value(QStringLiteral("supabaseKey")).toString()));
    m_encPhraseEdit->setText(deobfuscate(s.value(QStringLiteral("encryptionPhrase")).toString()));

    s.endGroup();
}

void InvoiceSettingsDialog::saveSettings()
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));

    s.setValue(QStringLiteral("clientAPath"),       m_clientAPathEdit->text().trimmed());
    s.setValue(QStringLiteral("clientBPath"),       m_clientBPathEdit->text().trimmed());
    s.setValue(QStringLiteral("syncHostType"),      m_hostTypeCombo->currentIndex());
    s.setValue(QStringLiteral("postgrestUrl"),      m_urlEdit->text().trimmed());
    s.setValue(QStringLiteral("email"),             m_emailEdit->text().trimmed());
    s.setValue(QStringLiteral("password"),          obfuscate(m_passwordEdit->text()));
    s.setValue(QStringLiteral("supabaseKey"),       obfuscate(m_supabaseKeyEdit->text().trimmed()));
    s.setValue(QStringLiteral("encryptionPhrase"),  obfuscate(m_encPhraseEdit->text()));

    s.endGroup();
}

// static
void InvoiceSettingsDialog::applyToApis(SqliteSyncPro *clientAApi, SqliteSyncPro *clientBApi)
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));

    const int     hostType    = s.value(QStringLiteral("syncHostType"), 0).toInt();
    const QString url         = s.value(QStringLiteral("postgrestUrl")).toString().trimmed();
    const QString email       = s.value(QStringLiteral("email")).toString().trimmed();
    const QString password    = deobfuscate(s.value(QStringLiteral("password")).toString());
    const QString supabaseKey = deobfuscate(s.value(QStringLiteral("supabaseKey")).toString());
    const QString encPhrase   = deobfuscate(s.value(QStringLiteral("encryptionPhrase")).toString());
    const QString pathA       = s.value(QStringLiteral("clientAPath")).toString();
    const QString pathB       = s.value(QStringLiteral("clientBPath")).toString();

    s.endGroup();

    for (SqliteSyncPro *api : {clientAApi, clientBApi}) {
        api->setSyncHostType(hostType);
        api->setPostgrestUrl(url);
        api->setEmail(email);
        api->setPassword(password);
        api->setSupabaseKey(supabaseKey);
        api->setEncryptionPhrase(encPhrase);
    }

    clientAApi->setDatabasePath(pathA);
    clientBApi->setDatabasePath(pathB);
}

// static
bool InvoiceSettingsDialog::settingsComplete()
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));
    const bool ok = !s.value(QStringLiteral("postgrestUrl")).toString().isEmpty()
                 && !s.value(QStringLiteral("email")).toString().isEmpty()
                 && !s.value(QStringLiteral("clientAPath")).toString().isEmpty()
                 && !s.value(QStringLiteral("clientBPath")).toString().isEmpty();
    s.endGroup();
    return ok;
}

// static
QString InvoiceSettingsDialog::savedClientAPath()
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));
    const QString def  = QDir::tempPath() + QStringLiteral("/sqsp_invoice_client_a.db");
    const QString path = s.value(QStringLiteral("clientAPath"), def).toString();
    s.endGroup();
    return path;
}

// static
QString InvoiceSettingsDialog::savedClientBPath()
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));
    const QString def  = QDir::tempPath() + QStringLiteral("/sqsp_invoice_client_b.db");
    const QString path = s.value(QStringLiteral("clientBPath"), def).toString();
    s.endGroup();
    return path;
}
