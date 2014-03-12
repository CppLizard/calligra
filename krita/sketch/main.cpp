/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QDir>
#include <QMessageBox>

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kcomponentdata.h>
#include <kstandarddirs.h>
#include <kglobal.h>

#include "MainWindow.h"

#include "DocumentListModel.h"
#include "KisSketchView.h"
#include "SketchInputContext.h"

#if defined HAVE_STEAMWORKS
#include <unistd.h>
#include "steam/steam_api.h"
#endif

#if defined Q_OS_WIN
#include "stdlib.h"
#include <ui/input/wintab/kis_tablet_support_win.h>
#elif defined Q_WS_X11
#include <ui/input/wintab/kis_tablet_support_x11.h>
#endif


#if defined HAVE_STEAMWORKS
//-----------------------------------------------------------------------------
// Callback hook for debug text emitted from the Steam API
//-----------------------------------------------------------------------------
extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const char *pchDebugText )
{
    // if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
    // if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
    qDebug( pchDebugText );

    if ( nSeverity >= 1 )
    {
        // place to set a breakpoint for catching API errors
        int x = 3;
        x = x;
    }
}
#endif


int main( int argc, char** argv )
{
    int result;
    KAboutData aboutData("kritasketch",
                         0,
                         ki18n("Krita Sketch"),
                         "0.1",
                         ki18n("Krita Sketch: Painting on the Go for Artists"),
                         KAboutData::License_GPL,
                         ki18n("(c) 1999-2012 The Krita team and KO GmbH.\n"),
                         KLocalizedString(),
                         "http://www.krita.org",
                         "submit@bugs.kde.org");

    KCmdLineArgs::init (argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add( "+[files]", ki18n( "Images to open" ) );
    options.add( "novkb", ki18n( "Don't use the virtual keyboard" ) );
    options.add( "windowed", ki18n( "Open sketch in a window, otherwise defaults to full-screen" ) );
    KCmdLineArgs::addCmdLineOptions( options );

#if defined HAVE_STEAMWORKS
    qDebug("RestartSteam Test");
    if ( SteamAPI_RestartAppIfNecessary( k_uAppIdInvalid ) )
    {
        // if Steam is not running or the game wasn't started through Steam, SteamAPI_RestartAppIfNecessary starts the
        // local Steam client and also launches this game again.

        // Once you get a public Steam AppID assigned for this game, you need to replace k_uAppIdInvalid with it and
        // removed steam_appid.txt from the game depot.
        return 1;
    }

    qDebug("Initialising Steam");
    bool steamSuccess = SteamAPI_Init();
    if (steamSuccess) {
        qDebug("Steam initialised correctly!");

        // set our debug handler
        SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );

        // We are going to use the controller interface, initialize it, which is a seperate step as it
        // create a new thread in the game proc and we don't want to force that on games that don't
        // have native Steam controller implementations

        char rgchCWD[1024];
        getcwd( rgchCWD, sizeof( rgchCWD ) );

        char rgchFullPath[1024];
    #if defined(_WIN32)
        _snprintf( rgchFullPath, sizeof( rgchFullPath ), "%s\\%s", rgchCWD, "controller.vdf" );
    #else
        _snprintf( rgchFullPath, sizeof( rgchFullPath ), "%s/%s", rgchCWD, "controller.vdf" );
    #endif
        qDebug("Initialising Steam Controller");
        SteamController()->Init( rgchFullPath );
    } else {
        qDebug("Steam did not initialise!");
    }
#endif

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    QStringList fileNames;
    if (args->count() > 0) {
        for (int i = 0; i < args->count(); ++i) {
            QString fileName = args->arg(i);
            if (QFile::exists(fileName)) {
                fileNames << fileName;
            }
        }
    }

    KApplication app;
    app.setApplicationName("kritasketch");
    QDir appdir(app.applicationDirPath());
    appdir.cdUp();

#ifdef Q_OS_WIN
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // If there's no kdehome, set it and restart the process.
    //QMessageBox::information(0, "krita sketch", "KDEHOME: " + env.value("KDEHOME"));
    if (!env.contains("KDEHOME") ) {
        _putenv_s("KDEHOME", QDesktopServices::storageLocation(QDesktopServices::DataLocation).toLocal8Bit());
    }
    if (!env.contains("KDESYCOCA")) {
        _putenv_s("KDESYCOCA", QString(appdir.absolutePath() + "/sycoca").toLocal8Bit());
    }
    if (!env.contains("XDG_DATA_DIRS")) {
        _putenv_s("XDG_DATA_DIRS", QString(appdir.absolutePath() + "/share").toLocal8Bit());
    }
    if (!env.contains("KDEDIR")) {
        _putenv_s("KDEDIR", appdir.absolutePath().toLocal8Bit());
    }
    if (!env.contains("KDEDIRS")) {
        _putenv_s("KDEDIRS", appdir.absolutePath().toLocal8Bit());
    }
    _putenv_s("PATH", QString(appdir.absolutePath() + "/bin" + ";"
              + appdir.absolutePath() + "/lib" + ";"
              + appdir.absolutePath() + "/lib"  +  "/kde4" + ";"
              + appdir.absolutePath()).toLocal8Bit());

    app.addLibraryPath(appdir.absolutePath());
    app.addLibraryPath(appdir.absolutePath() + "/bin");
    app.addLibraryPath(appdir.absolutePath() + "/lib");
    app.addLibraryPath(appdir.absolutePath() + "/lib/kde4");
#endif

#if defined Q_OS_WIN
    KisTabletSupportWin::init();
    app.setEventFilter(&KisTabletSupportWin::eventFilter);
#elif defined Q_WS_X11
    KisTabletSupportX11::init();
    app.setEventFilter(&KisTabletSupportX11::eventFilter);
#endif

#if defined Q_WS_X11 && QT_VERSION >= 0x040800
    QApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

    QStringList fonts = KGlobal::dirs()->findAllResources( "appdata", "fonts/*.otf" );
    foreach( const QString &font, fonts ) {
        QFontDatabase::addApplicationFont( font );
    }

    QFontDatabase db;
    QApplication::setFont( db.font( "Source Sans Pro", "Regular", 12 ) );

    MainWindow window(fileNames);

    if (args->isSet("vkb")) {
        app.setInputContext(new SketchInputContext(&app));
    }

    if (args->isSet("windowed")) {
        window.show();
    } else {
        window.showFullScreen();
    }

    result = app.exec();

#if defined HAVE_STEAMWORKS
    if(steamSuccess) {
        SteamController()->Shutdown();
        SteamAPI_Shutdown();
    }
#endif
    return result;
}
