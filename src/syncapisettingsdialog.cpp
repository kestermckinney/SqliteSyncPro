#include "syncapisettingsdialog.h"
#include "sqlitesyncpro.h"
#include "ui_syncapisettingsdialog.h"

// ---------------------------------------------------------------------------
// SyncAPISettingsDialog
// ---------------------------------------------------------------------------

SyncAPISettingsDialog::SyncAPISettingsDialog(SqliteSyncPro *api, QWidget *parent)
    : QDialog(parent)
    , m_api(api)
    , ui(new Ui::SyncAPISettingsDialog)
{
    ui->setupUi(this);

    connect(ui->comboBoxSyncHostType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SyncAPISettingsDialog::onHostTypeChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveToApi();
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Disconnect the .ui-file direct accepted() connection to avoid double-trigger.
    disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    loadFromApi();
    onHostTypeChanged(ui->comboBoxSyncHostType->currentIndex());
}

SyncAPISettingsDialog::~SyncAPISettingsDialog()
{
    delete ui;
}

void SyncAPISettingsDialog::onHostTypeChanged(int index)
{
    const bool isSupabase = (index == 1);
    ui->labelAuthenticationKey->setVisible(isSupabase);
    ui->lineEditAuthenticationKey->setVisible(isSupabase);
}

void SyncAPISettingsDialog::loadFromApi()
{
    if (!m_api)
        return;

    ui->comboBoxSyncHostType->setCurrentIndex(m_api->syncHostType());
    ui->lineEditPostgrestURL->setText(m_api->postgrestUrl());
    ui->lineEditUsername->setText(m_api->email());
    ui->lineEditPassword->setText(m_api->password());
    ui->lineEditEncryptionPhrase->setText(m_api->encryptionPhrase());
    ui->lineEditAuthenticationKey->setText(m_api->supabaseKey());
}

void SyncAPISettingsDialog::saveToApi()
{
    if (!m_api)
        return;

    m_api->setSyncHostType(ui->comboBoxSyncHostType->currentIndex());
    m_api->setPostgrestUrl(ui->lineEditPostgrestURL->text().trimmed());
    m_api->setEmail(ui->lineEditUsername->text().trimmed());
    m_api->setPassword(ui->lineEditPassword->text());
    m_api->setEncryptionPhrase(ui->lineEditEncryptionPhrase->text());
    m_api->setSupabaseKey(ui->lineEditAuthenticationKey->text().trimmed());
}
