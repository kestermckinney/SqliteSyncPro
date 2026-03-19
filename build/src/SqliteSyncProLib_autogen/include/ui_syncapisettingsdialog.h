/********************************************************************************
** Form generated from reading UI file 'syncapisettingsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SYNCAPISETTINGSDIALOG_H
#define UI_SYNCAPISETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

QT_BEGIN_NAMESPACE

class Ui_SyncAPISettingsDialog
{
public:
    QFormLayout *formLayout;
    QLabel *labelSyncHostType;
    QComboBox *comboBoxSyncHostType;
    QLabel *labelPostgrestURL;
    QLineEdit *lineEditPostgrestURL;
    QLabel *labelUsername;
    QLineEdit *lineEditUsername;
    QLabel *labelPassword;
    QLineEdit *lineEditPassword;
    QLabel *labelEncryptionPhrase;
    QLineEdit *lineEditEncryptionPhrase;
    QLabel *labelAuthenticationKey;
    QLineEdit *lineEditAuthenticationKey;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SyncAPISettingsDialog)
    {
        if (SyncAPISettingsDialog->objectName().isEmpty())
            SyncAPISettingsDialog->setObjectName("SyncAPISettingsDialog");
        SyncAPISettingsDialog->resize(480, 260);
        formLayout = new QFormLayout(SyncAPISettingsDialog);
        formLayout->setObjectName("formLayout");
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        labelSyncHostType = new QLabel(SyncAPISettingsDialog);
        labelSyncHostType->setObjectName("labelSyncHostType");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, labelSyncHostType);

        comboBoxSyncHostType = new QComboBox(SyncAPISettingsDialog);
        comboBoxSyncHostType->addItem(QString());
        comboBoxSyncHostType->addItem(QString());
        comboBoxSyncHostType->setObjectName("comboBoxSyncHostType");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, comboBoxSyncHostType);

        labelPostgrestURL = new QLabel(SyncAPISettingsDialog);
        labelPostgrestURL->setObjectName("labelPostgrestURL");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, labelPostgrestURL);

        lineEditPostgrestURL = new QLineEdit(SyncAPISettingsDialog);
        lineEditPostgrestURL->setObjectName("lineEditPostgrestURL");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, lineEditPostgrestURL);

        labelUsername = new QLabel(SyncAPISettingsDialog);
        labelUsername->setObjectName("labelUsername");

        formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, labelUsername);

        lineEditUsername = new QLineEdit(SyncAPISettingsDialog);
        lineEditUsername->setObjectName("lineEditUsername");

        formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, lineEditUsername);

        labelPassword = new QLabel(SyncAPISettingsDialog);
        labelPassword->setObjectName("labelPassword");

        formLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, labelPassword);

        lineEditPassword = new QLineEdit(SyncAPISettingsDialog);
        lineEditPassword->setObjectName("lineEditPassword");
        lineEditPassword->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(3, QFormLayout::ItemRole::FieldRole, lineEditPassword);

        labelEncryptionPhrase = new QLabel(SyncAPISettingsDialog);
        labelEncryptionPhrase->setObjectName("labelEncryptionPhrase");

        formLayout->setWidget(4, QFormLayout::ItemRole::LabelRole, labelEncryptionPhrase);

        lineEditEncryptionPhrase = new QLineEdit(SyncAPISettingsDialog);
        lineEditEncryptionPhrase->setObjectName("lineEditEncryptionPhrase");
        lineEditEncryptionPhrase->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(4, QFormLayout::ItemRole::FieldRole, lineEditEncryptionPhrase);

        labelAuthenticationKey = new QLabel(SyncAPISettingsDialog);
        labelAuthenticationKey->setObjectName("labelAuthenticationKey");

        formLayout->setWidget(5, QFormLayout::ItemRole::LabelRole, labelAuthenticationKey);

        lineEditAuthenticationKey = new QLineEdit(SyncAPISettingsDialog);
        lineEditAuthenticationKey->setObjectName("lineEditAuthenticationKey");
        lineEditAuthenticationKey->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(5, QFormLayout::ItemRole::FieldRole, lineEditAuthenticationKey);

        buttonBox = new QDialogButtonBox(SyncAPISettingsDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        formLayout->setWidget(6, QFormLayout::ItemRole::SpanningRole, buttonBox);


        retranslateUi(SyncAPISettingsDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, SyncAPISettingsDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, SyncAPISettingsDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(SyncAPISettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *SyncAPISettingsDialog)
    {
        SyncAPISettingsDialog->setWindowTitle(QCoreApplication::translate("SyncAPISettingsDialog", "Sync Connection Settings", nullptr));
        labelSyncHostType->setText(QCoreApplication::translate("SyncAPISettingsDialog", "Sync Host Type:", nullptr));
        comboBoxSyncHostType->setItemText(0, QCoreApplication::translate("SyncAPISettingsDialog", "Self-Hosted", nullptr));
        comboBoxSyncHostType->setItemText(1, QCoreApplication::translate("SyncAPISettingsDialog", "Supabase", nullptr));

        labelPostgrestURL->setText(QCoreApplication::translate("SyncAPISettingsDialog", "Server URL:", nullptr));
        lineEditPostgrestURL->setPlaceholderText(QCoreApplication::translate("SyncAPISettingsDialog", "http://localhost:3000  or  https://xxxx.supabase.co", nullptr));
        labelUsername->setText(QCoreApplication::translate("SyncAPISettingsDialog", "Username (Email):", nullptr));
        lineEditUsername->setPlaceholderText(QCoreApplication::translate("SyncAPISettingsDialog", "user@example.com", nullptr));
        labelPassword->setText(QCoreApplication::translate("SyncAPISettingsDialog", "Password:", nullptr));
        labelEncryptionPhrase->setText(QCoreApplication::translate("SyncAPISettingsDialog", "Encryption Phrase:", nullptr));
        lineEditEncryptionPhrase->setPlaceholderText(QCoreApplication::translate("SyncAPISettingsDialog", "(optional \342\200\224 leave blank to disable encryption)", nullptr));
        labelAuthenticationKey->setText(QCoreApplication::translate("SyncAPISettingsDialog", "Supabase Anon Key:", nullptr));
        lineEditAuthenticationKey->setPlaceholderText(QCoreApplication::translate("SyncAPISettingsDialog", "eyJ\342\200\246 (anon / public key from Supabase Dashboard)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SyncAPISettingsDialog: public Ui_SyncAPISettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYNCAPISETTINGSDIALOG_H
