// Copyright (C) 2026 Paul McKinney
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/bin");
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/plugins");
    QCoreApplication::addLibraryPath("./site-packages/PyQt6/Qt6/plugins/bin");

    QApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("SQLSync Administrator"));

    // --developer-profile mirrors the same flag in ProjectNotes.  When set,
    // settings are stored under "ProjectNotes<PROFILE>/AppSettings" so the
    // admin tool reads/writes the same profile-specific file as ProjectNotes.
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption devProfileOption(
        QStringLiteral("developer-profile"),
        QStringLiteral("Use a separate settings profile (must match the ProjectNotes --developer-profile value)."),
        QStringLiteral("PROFILENAME"));
    parser.addOption(devProfileOption);
    parser.process(app);

    const QString profile = parser.isSet(devProfileOption)
                            ? parser.value(devProfileOption)
                            : QString();
    app.setOrganizationName(QStringLiteral("ProjectNotes") + profile);

#ifndef Q_OS_MACOS
    // On macOS the dock/title-bar icon comes from the .icns in the app bundle
    // (set via MACOSX_BUNDLE_ICON_FILE in CMakeLists.txt).  Overriding it here
    // with PNGs bypasses the native theming and squircle mask, so we skip it.
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
#endif

    MainWindow w;
    w.show();

    return app.exec();
}


