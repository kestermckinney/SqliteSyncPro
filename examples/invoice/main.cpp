// Copyright (C) 2026 Paul McKinney
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("ProjectNotes"));
    app.setApplicationName(QStringLiteral("AppSettings"));

    MainWindow window;
    window.show();

    return app.exec();
}
