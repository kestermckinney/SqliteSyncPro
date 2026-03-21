#ifndef SYNCAPISETTINGSDIALOG_H
#define SYNCAPISETTINGSDIALOG_H

#include <QDialog>

class SqliteSyncPro;

namespace Ui {
class SyncAPISettingsDialog;
}

/**
 * SyncAPISettingsDialog – settings dialog for SqliteSyncPro.
 *
 * Reads the current settings from the SqliteSyncPro instance passed to the
 * constructor and writes them back when the user clicks OK.
 *
 * The calling application is responsible for persisting the settings
 * (e.g. to QSettings) after the dialog is accepted.
 *
 * The Supabase Anon Key row is visible only when Supabase is selected.
 */
class SyncAPISettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SyncAPISettingsDialog(SqliteSyncPro *api, QWidget *parent = nullptr);
    ~SyncAPISettingsDialog() override;

private slots:
    void onHostTypeChanged(int index);

private:
    void loadFromApi();
    void saveToApi();

    SqliteSyncPro              *m_api;
    Ui::SyncAPISettingsDialog  *ui;
};

#endif // SYNCAPISETTINGSDIALOG_H
