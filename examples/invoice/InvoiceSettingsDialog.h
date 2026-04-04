// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QDialog>
#include <QString>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class SqliteSyncPro;

/**
 * InvoiceSettingsDialog – settings dialog for the Invoice Sync Demo.
 *
 * Manages database paths for Client A and Client B plus the shared
 * PostgREST connection settings.  Settings are persisted in QSettings.
 *
 * Call applyToApis() to push the saved settings into both API instances.
 */
class InvoiceSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InvoiceSettingsDialog(QWidget *parent = nullptr);

    void loadSettings();
    void saveSettings();

    static void applyToApis(SqliteSyncPro *clientAApi, SqliteSyncPro *clientBApi);
    static bool settingsComplete();

    static QString savedClientAPath();
    static QString savedClientBPath();

private slots:
    void onHostTypeChanged(int index);
    void browseClientAPath();
    void browseClientBPath();

private:
    QComboBox        *m_hostTypeCombo;
    QLineEdit        *m_urlEdit;
    QLineEdit        *m_emailEdit;
    QLineEdit        *m_passwordEdit;
    QLabel           *m_supabaseKeyLabel;
    QLineEdit        *m_supabaseKeyEdit;
    QLineEdit        *m_encPhraseEdit;
    QLineEdit        *m_clientAPathEdit;
    QLineEdit        *m_clientBPathEdit;
    QPushButton      *m_clientABrowseBtn;
    QPushButton      *m_clientBBrowseBtn;
    QDialogButtonBox *m_buttonBox;
};
