// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QWidget>

class AdminController;
class QLabel;

class ConfigurePage : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurePage(AdminController *controller, QWidget *parent = nullptr);

signals:
    void navigateNext();
    void navigateTo(int index);

protected:
    void showEvent(QShowEvent *event) override;

private:
    AdminController *m_controller;
    QLabel          *m_hostPortLabel;
    QLabel          *m_superuserLabel;
};
