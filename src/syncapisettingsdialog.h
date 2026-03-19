#ifndef SYNCAPISETTINGSDIALOG_H
#define SYNCAPISETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SyncAPISettingsDialog;
}

/**
 * SyncAPISettingsDialog – settings dialog for SqliteSyncPro.
 *
 * Lets the user configure the sync host type (Self-Hosted or Supabase),
 * server URL, credentials, and optional encryption passphrase.
 *
 * Settings are persisted in QSettings("SqliteSyncPro", "SQLSyncAdmin")
 * under the group "sync_api", in the same location used by SQLSync Administrator.
 *
 * Sensitive values (password, encryption phrase, Supabase anon key) are
 * obfuscated with a machine-derived XOR key before storage.
 *
 * The Supabase Anon Key row is visible only when Supabase is selected.
 */
class SyncAPISettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SyncAPISettingsDialog(QWidget *parent = nullptr);
    ~SyncAPISettingsDialog() override;

    // Convenience: load settings, exec dialog, save on accept.
    // Returns QDialog::Accepted or QDialog::Rejected.
    int execWithSettings();

private slots:
    void onHostTypeChanged(int index);

private:
    void loadSettings();
    void saveSettings();

    Ui::SyncAPISettingsDialog *ui;
};

#endif // SYNCAPISETTINGSDIALOG_H
