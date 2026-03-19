#pragma once

#include <QMainWindow>
#include <QVariantList>

class InvoiceController;
class QLabel;
class QProgressBar;
class QPushButton;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void populateTree(QTreeWidget *tree, const QVariantList &records);

    InvoiceController *m_controller;

    QPushButton  *m_settingsBtn;
    QPushButton  *m_runTestBtn;
    QProgressBar *m_progressBar;
    QLabel       *m_statusLabel;
    QTreeWidget  *m_sourceTree;
    QTreeWidget  *m_destTree;
};
