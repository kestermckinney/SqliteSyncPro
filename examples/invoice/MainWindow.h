#pragma once

#include <QMainWindow>
#include <QVariantList>

class InvoiceController;
class QCloseEvent;
class QLabel;
class QProgressBar;
class QPushButton;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void populateTree(QTreeWidget *tree, const QVariantList &records);

    InvoiceController *m_controller;

    QPushButton  *m_settingsBtn;
    QPushButton  *m_startSyncBtn;
    QPushButton  *m_stopSyncBtn;
    QProgressBar *m_progressBar;
    QLabel       *m_statusLabel;
    QTreeWidget  *m_clientATree;
    QTreeWidget  *m_clientBTree;

    bool m_closing = false;
};
