#include "syncapisettingsdialog.h"
#include "ui_syncapisettingsdialog.h"

#include <QSettings>
#include <QSysInfo>
#include <QCryptographicHash>

// ---------------------------------------------------------------------------
// Key obfuscation — XOR with a key derived from the machine unique ID.
// Prevents plain-text storage of sensitive values in QSettings.
// Not a security boundary: someone who can read QSettings can read the machine
// ID too.  Purpose is to avoid accidental exposure (logs, backups, etc.).
// ---------------------------------------------------------------------------

static QByteArray obfuscationKey()
{
    QByteArray uid = QSysInfo::machineUniqueId();
    if (uid.isEmpty())
        uid = QByteArrayLiteral("sqlitesyncpro-fallback-id");
    return QCryptographicHash::hash(uid, QCryptographicHash::Sha256);
}

static QString obfuscate(const QString &plain)
{
    if (plain.isEmpty()) return {};
    QByteArray data = plain.toUtf8();
    const QByteArray key = obfuscationKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromLatin1(data.toBase64());
}

static QString deobfuscate(const QString &stored)
{
    if (stored.isEmpty()) return {};
    QByteArray data = QByteArray::fromBase64(stored.toLatin1());
    const QByteArray key = obfuscationKey();
    for (int i = 0; i < data.size(); ++i)
        data[i] ^= key[i % key.size()];
    return QString::fromUtf8(data);
}

// QSettings keys — same org/app as SQLSync Administrator
#define SETTINGS_ORG   "ProjectNotes"
#define SETTINGS_APP   "AppSettings"
#define SETTINGS_GROUP "sync_api"

// ---------------------------------------------------------------------------
// SyncAPISettingsDialog
// ---------------------------------------------------------------------------

SyncAPISettingsDialog::SyncAPISettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SyncAPISettingsDialog)
{
    ui->setupUi(this);

    connect(ui->comboBoxSyncHostType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SyncAPISettingsDialog::onHostTypeChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Disconnect the .ui-file connections so we don't double-trigger accept/reject
    // (the .ui connections were already set up via setupUi, but we re-routed
    // accepted() through saveSettings above — remove the direct ones).
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    loadSettings();
    onHostTypeChanged(ui->comboBoxSyncHostType->currentIndex());
}

SyncAPISettingsDialog::~SyncAPISettingsDialog()
{
    delete ui;
}

int SyncAPISettingsDialog::execWithSettings()
{
    loadSettings();
    onHostTypeChanged(ui->comboBoxSyncHostType->currentIndex());
    return exec();
}

void SyncAPISettingsDialog::onHostTypeChanged(int index)
{
    // index 0 = Self-Hosted, index 1 = Supabase
    const bool isSupabase = (index == 1);
    ui->labelAuthenticationKey->setVisible(isSupabase);
    ui->lineEditAuthenticationKey->setVisible(isSupabase);
}

void SyncAPISettingsDialog::loadSettings()
{
    QSettings s(QStringLiteral(SETTINGS_ORG), QStringLiteral(SETTINGS_APP));
    s.beginGroup(QStringLiteral(SETTINGS_GROUP));

    ui->comboBoxSyncHostType->setCurrentIndex(s.value(QStringLiteral("syncHostType"), 0).toInt());
    ui->lineEditPostgrestURL->setText(s.value(QStringLiteral("postgrestUrl")).toString());
    ui->lineEditUsername->setText(s.value(QStringLiteral("username")).toString());
    ui->lineEditPassword->setText(deobfuscate(s.value(QStringLiteral("password")).toString()));
    ui->lineEditEncryptionPhrase->setText(
        deobfuscate(s.value(QStringLiteral("encryptionPhrase")).toString()));
    ui->lineEditAuthenticationKey->setText(
        deobfuscate(s.value(QStringLiteral("authenticationKey")).toString()));

    s.endGroup();
}

void SyncAPISettingsDialog::saveSettings()
{
    QSettings s(QStringLiteral(SETTINGS_ORG), QStringLiteral(SETTINGS_APP));
    s.beginGroup(QStringLiteral(SETTINGS_GROUP));

    s.setValue(QStringLiteral("syncHostType"),
               ui->comboBoxSyncHostType->currentIndex());
    s.setValue(QStringLiteral("postgrestUrl"),
               ui->lineEditPostgrestURL->text().trimmed());
    s.setValue(QStringLiteral("username"),
               ui->lineEditUsername->text().trimmed());
    s.setValue(QStringLiteral("password"),
               obfuscate(ui->lineEditPassword->text()));
    s.setValue(QStringLiteral("encryptionPhrase"),
               obfuscate(ui->lineEditEncryptionPhrase->text()));
    s.setValue(QStringLiteral("authenticationKey"),
               obfuscate(ui->lineEditAuthenticationKey->text().trimmed()));

    s.endGroup();
}
