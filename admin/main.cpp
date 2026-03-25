#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SQLSync Administrator"));
    app.setOrganizationName(QStringLiteral("SqliteSyncPro"));

    // Build a multi-resolution icon so Qt picks the sharpest size at every
    // DPI (title bar, taskbar, Alt+Tab, high-DPI screens).
    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_16.png"),  QSize(16,  16));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_24.png"),  QSize(24,  24));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_32.png"),  QSize(32,  32));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_48.png"),  QSize(48,  48));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_64.png"),  QSize(64,  64));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_128.png"), QSize(128, 128));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_256.png"), QSize(256, 256));
    icon.addFile(QStringLiteral(":/icons/sqlsyncadmin_512.png"), QSize(512, 512));
    QApplication::setWindowIcon(icon);

    MainWindow w;
    w.show();

    return app.exec();
}


