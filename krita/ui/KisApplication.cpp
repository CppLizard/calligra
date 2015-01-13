/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2009 Thomas Zander <zander@kde.org>
   Copyright (C) 2012 Boudewijn Rempt <boud@valdyas.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KisApplication.h"

#ifndef QT_NO_DBUS
#include <QtDBus>
#endif

#include "KisPrintJob.h"
#include "KisDocumentEntry.h"
#include "KisDocument.h"
#include "KisMainWindow.h"
#include "KisAutoSaveRecoveryDialog.h"
#include <KoDpi.h>
#include "KisPart.h"
#include "KoGlobal.h"

#include <kdeversion.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kdesktopfile.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kconfiggroup.h>

#if KDE_IS_VERSION(4,6,0)
#include <krecentdirs.h>
#endif

#include <QFile>
#include <QWidget>
#include <QSysInfo>
#include <QStringList>
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QDir>

#include <stdlib.h>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tchar.h>
#endif

#include <QDesktopWidget>

KisApplication* KisApplication::KoApp = 0;

namespace {
const QTime appStartTime(QTime::currentTime());
}

class KisApplicationPrivate
{
public:
    KisApplicationPrivate()
        : part(0)
        , splashScreen(0)
    {}
    QByteArray nativeMimeType;
    KisPart *part;
    QWidget *splashScreen;
};

class KisApplication::ResetStarting
{
public:
    ResetStarting(QWidget *splash = 0)
        : m_splash(splash)
    {
    }

    ~ResetStarting()  {
        if (m_splash) {

            KConfigGroup cfg(KGlobal::config(), "SplashScreen");
            bool hideSplash = cfg.readEntry("HideSplashAfterStartup", false);

            if (hideSplash) {
                m_splash->hide();
            }
            else {
                m_splash->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
                QRect r(QPoint(), m_splash->size());
                m_splash->move(QApplication::desktop()->screenGeometry().center() - r.center());
                m_splash->setWindowTitle(qAppName());
                foreach(QObject *o, m_splash->children()) {
                    QWidget *w = qobject_cast<QWidget*>(o);
                    if (w && w->isHidden()) {
                        w->setVisible(true);
                    }
                }

                m_splash->show();
            }
        }
    }

    QWidget *m_splash;
};



KisApplication::KisApplication(const QByteArray &nativeMimeType)
    : KApplication(initHack())
    , d(new KisApplicationPrivate)
{
    KisApplication::KoApp = this;

    d->nativeMimeType = nativeMimeType;
    // Tell the iconloader about share/apps/calligra/icons
    KIconLoader::global()->addAppDir("calligra");

    // Initialize all Calligra directories etc.
    KoGlobal::initialize();

#ifdef Q_OS_MACX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
    setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

    if (applicationName() == "krita" && qgetenv("KDE_FULL_SESSION").isEmpty()) {
        // There are two themes that work for Krita, oxygen and plastique. Try to set plastique first, then oxygen
        setStyle("Plastique");
        setStyle("Oxygen");
    }

}

// This gets called before entering KApplication::KApplication
bool KisApplication::initHack()
{
    KCmdLineOptions options;
    options.add("print", ki18n("Only print and exit"));
    options.add("template", ki18n("Open a new document with a template"));
    options.add("dpi <dpiX,dpiY>", ki18n("Override display DPI"));
    options.add("export-pdf", ki18n("Only export to PDF and exit"));
    options.add("export-filename <filename>", ki18n("Filename for export-pdf"));
    options.add("profile-filename <filename>", ki18n("Filename to write profiling information into."));
    options.add("roundtrip-filename <filename>", ki18n("Load a file and save it as an ODF file. Meant for debugging."));
    KCmdLineArgs::addCmdLineOptions(options, ki18n("Calligra"), "calligra", "kde");
    return true;
}

#if defined(Q_OS_WIN) && defined(ENV32BIT)
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL isWow64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}
#endif

bool KisApplication::start()
{
#if defined(Q_OS_WIN)  || defined (Q_OS_MACX)
#ifdef ENV32BIT
    if (isWow64()) {
        KMessageBox::information(0,
                                 i18n("You are running a 32 bits build on a 64 bits Windows.\n"
                                      "This is not recommended.\n"
                                      "Please download and install the x64 build instead."),
                                 qApp->applicationName(),
                                 "calligra_32_on_64_warning");

    }
#endif
    QDir appdir(applicationDirPath());
    appdir.cdUp();

    KGlobal::dirs()->addXdgDataPrefix(appdir.absolutePath() + "/share");
    KGlobal::dirs()->addPrefix(appdir.absolutePath());

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // If there's no kdehome, set it and restart the process.
    if (!env.contains("KDEHOME")) {
        qputenv("KDEHOME", QFile::encodeName(QDesktopServices::storageLocation(QDesktopServices::DataLocation)));
    }
    if (!env.contains("XDG_DATA_DIRS")) {
        qputenv("XDG_DATA_DIRS", QFile::encodeName(appdir.absolutePath() + "/share"));
    }
    if (!env.contains("KDEDIR")) {
        qputenv("KDEDIR", QFile::encodeName(appdir.absolutePath()));
    }
    if (!env.contains("KDEDIRS")) {
        qputenv("KDEDIRS", QFile::encodeName(appdir.absolutePath()));
    }
    qputenv("PATH", QFile::encodeName(appdir.absolutePath() + "/bin" + ";"
                                      + appdir.absolutePath() + "/lib" + ";"
                                      + appdir.absolutePath() + "/lib/kde4" + ";"
                                      + appdir.absolutePath() + "/Frameworks" + ";"
                                      + appdir.absolutePath()));

#endif

    if (d->splashScreen) {
        d->splashScreen->show();
        d->splashScreen->repaint();
        processEvents();
    }

    ResetStarting resetStarting(d->splashScreen); // remove the splash when done
    Q_UNUSED(resetStarting);

    // Get the command line arguments which we have to parse
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    int argsCount = args->count();

    KCmdLineArgs *koargs = KCmdLineArgs::parsedArgs("calligra");
    QString dpiValues = koargs->getOption("dpi");
    if (!dpiValues.isEmpty()) {
        int sep = dpiValues.indexOf(QRegExp("[x, ]"));
        int dpiX;
        int dpiY = 0;
        bool ok = true;
        if (sep != -1) {
            dpiY = dpiValues.mid(sep + 1).toInt(&ok);
            dpiValues.truncate(sep);
        }
        if (ok) {
            dpiX = dpiValues.toInt(&ok);
            if (ok) {
                if (!dpiY) dpiY = dpiX;
                KoDpi::setDPI(dpiX, dpiY);
            }
        }
    }

    const bool doTemplate = koargs->isSet("template");
    const bool print = koargs->isSet("print");

    const bool exportAsPdf = koargs->isSet("export-pdf");
    const QString pdfFileName = koargs->getOption("export-filename");

    const QString roundtripFileName = koargs->getOption("roundtrip-filename");
    const QString profileFileName = koargs->getOption("profile-filename");


    // only show the mainWindow when no command-line mode option is passed
    const bool showmainWindow = (   !exportAsPdf
                                 && roundtripFileName.isEmpty());

    const bool batchRun = (   showmainWindow
                           && !print
                           && !profileFileName.isEmpty());

    // Figure out _which_ application we actually are
    KisDocumentEntry entry = KisDocumentEntry::queryByMimeType(d->nativeMimeType);
    if (entry.isEmpty()) {
        QMessageBox::critical(0, i18n("%1: Critical Error", applicationName()), i18n("Essential application components could not be found.\n"
                                                                                    "This might be an installation issue.\n"
                                                                                    "Try restarting or reinstalling."));
        return false;
    }

    // Create the global document, view, window factory
    // XXX: maybe the main() should create the factory
    //      and pass it to KisApplication?
    QString errorMsg;
    d->part = entry.createKisPart(&errorMsg);

    if (!d->part) {
        if (!errorMsg.isEmpty())
            KMessageBox::error(0, errorMsg);
        return false;
    }

    // show a mainWindow asap, if we want that
    KisMainWindow *mainWindow = d->part->createMainWindow();
    d->part->addMainWindow(mainWindow);
    if (showmainWindow) {
        mainWindow->show();
    }

    short int numberOfOpenDocuments = 0; // number of documents open

    // Check for autosave files that can be restored, if we're not running a batchrun (test, print, export to pdf)
    if (!batchRun) {
        numberOfOpenDocuments += checkAutosaveFiles(mainWindow);
    }

    if (argsCount > 0) {

        // remove all non-filename options
        koargs->clear();

        QTextStream profileoutput;
        QFile profileFile(profileFileName);
        if (!profileFileName.isEmpty()
                && profileFile.open(QFile::WriteOnly | QFile::Truncate)) {
            profileoutput.setDevice(&profileFile);
        }

        // Loop through arguments
        short int nPrinted = 0;
        for (int argNumber = 0; argNumber < argsCount; argNumber++) {

            // are we just trying to open a template?
            if (doTemplate) {
                QStringList paths;
                if (args->url(argNumber).isLocalFile() && QFile::exists(args->url(argNumber).toLocalFile())) {
                    paths << QString(args->url(argNumber).toLocalFile());
                    kDebug(30003) << "using full path...";
                }
                else {
                    QString desktopName(args->arg(argNumber));
                    QString appName = KGlobal::mainComponent().componentName();

                    paths = KGlobal::dirs()->findAllResources("data", appName + "/templates/*/" + desktopName);
                    if (paths.isEmpty()) {
                        paths = KGlobal::dirs()->findAllResources("data", appName + "/templates/" + desktopName);
                    }
                    if (paths.isEmpty()) {
                        KMessageBox::error(0, i18n("No template found for: %1", desktopName));
                        delete mainWindow;
                    }
                    else if (paths.count() > 1) {
                        KMessageBox::error(0, i18n("Too many templates found for: %1", desktopName));
                        delete mainWindow;
                    }
                }

                if (!paths.isEmpty()) {
                    KUrl templateBase;
                    templateBase.setPath(paths[0]);
                    KDesktopFile templateInfo(paths[0]);

                    QString templateName = templateInfo.readUrl();
                    KUrl templateURL;
                    templateURL.setPath(templateBase.directory() + '/' + templateName);
                    KisDocument *doc = mainWindow->createDocumentFromUrl(templateURL);
                    if (doc) {
                        doc->resetURL();
                        doc->setEmpty();
                        doc->setTitleModified();
                        kDebug(30003) << "Template loaded...";
                        numberOfOpenDocuments++;
                    }
                    else {
                        KMessageBox::error(0, i18n("Template %1 failed to load.", templateURL.prettyUrl()));

                    }
                }
                // now try to load
            }
            else if (mainWindow->createDocumentFromUrl(args->url(argNumber))) {

                if (print) {
                    mainWindow->slotFilePrint();
                    nPrinted++;
                }
                else if (exportAsPdf) {
                    KisPrintJob *job = mainWindow->exportToPdf(pdfFileName);
                    if (job)
                        connect (job, SIGNAL(destroyed(QObject*)), mainWindow,
                                 SLOT(slotFileQuit()), Qt::QueuedConnection);
                    nPrinted++;
                } else {
                    // Normal case, success
                    numberOfOpenDocuments++;
                }

            } else {
                // .... if failed
                // delete doc; done by openDocument
                // delete mainWindow; done by ~KisDocument
            }

        }
        if (print || exportAsPdf) {
            return nPrinted > 0;
        }
    }

    args->clear();
    // not calling this before since the program will quit there.
    return true;
}

KisApplication::~KisApplication()
{
    delete d;
}

void KisApplication::setSplashScreen(QWidget *splashScreen)
{
    d->splashScreen = splashScreen;
}

QStringList KisApplication::mimeFilter(KisImportExportManager::Direction direction) const
{
    KisDocumentEntry entry = KisDocumentEntry::queryByMimeType(d->nativeMimeType);
    KService::Ptr service = entry.service();
    return KisImportExportManager::mimeFilter(d->nativeMimeType,
                                       direction,
                                       service->property("X-KDE-ExtraNativeMimeTypes", QVariant::StringList).toStringList());
}


bool KisApplication::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (std::exception &e) {
        qWarning("Error %s sending event %i to object %s",
                 e.what(), event->type(), qPrintable(receiver->objectName()));
    } catch (...) {
        qWarning("Error <unknown> sending event %i to object %s",
                 event->type(), qPrintable(receiver->objectName()));
    }
    return false;

}

KisApplication *KisApplication::koApplication()
{
    return KoApp;
}


int KisApplication::checkAutosaveFiles(KisMainWindow *mainWindow)
{

    // Check for autosave files from a previous run. There can be several, and
    // we want to offer a restore for every one. Including a nice thumbnail!
    QStringList autoSaveFiles;

    // get all possible autosave files in the home dir, this is for unsaved document autosave files
    // Using the extension allows to avoid relying on the mime magic when opening
    KMimeType::Ptr mime = KMimeType::mimeType(d->nativeMimeType);
    if (!mime) {
        qFatal("It seems your installation is broken/incomplete because we failed to load the native mimetype \"%s\".", d->nativeMimeType.constData());
    }
    QString extension = mime->property("X-KDE-NativeExtension").toString();
    if (extension.isEmpty()) {
        extension = mime->mainExtension();
    }

    QStringList filters;
    filters << QString(".%1-%2-%3-autosave%4").arg(d->part->componentData().componentName()).arg("*").arg("*").arg(extension);

#ifdef Q_OS_WIN
        QDir dir = QDir::tempPath();
#else
        QDir dir = QDir::home();
#endif

    // all autosave files for our application
    autoSaveFiles = dir.entryList(filters, QDir::Files | QDir::Hidden);

    QStringList pids;
    QString ourPid;
    ourPid.setNum(kapp->applicationPid());

#ifndef QT_NO_DBUS
    // all running instances of our application -- bit hackish, but we cannot get at the dbus name here, for some reason
    QDBusReply<QStringList> reply = QDBusConnection::sessionBus().interface()->registeredServiceNames();

    foreach (const QString &name, reply.value()) {
        if (name.contains(d->part->componentData().componentName())) {
            // we got another instance of ourselves running, let's get the pid
            QString pid = name.split('-').last();
            if (pid != ourPid) {
                pids << pid;
            }
        }
    }
#endif

    // remove the autosave files that are saved for other, open instances of ourselves.
    foreach(const QString &autoSaveFileName, autoSaveFiles) {
        if (!QFile::exists(QDir::homePath() + "/" + autoSaveFileName)) {
            autoSaveFiles.removeAll(autoSaveFileName);
            continue;
        }
        QStringList split = autoSaveFileName.split('-');
        if (split.size() == 4) {
            if (pids.contains(split[1])) {
                // We've got an active, owned autosave file. Remove.
                autoSaveFiles.removeAll(autoSaveFileName);
            }
        }
    }

    // Allow the user to make their selection
    if (autoSaveFiles.size() > 0) {
        KisAutoSaveRecoveryDialog dlg(autoSaveFiles);
        if (dlg.exec() == QDialog::Accepted) {
            QStringList filesToRecover = dlg.recoverableFiles();
            foreach (const QString &autosaveFile, autoSaveFiles) {
                if (!filesToRecover.contains(autosaveFile)) {
                    // remove the files the user didn't want to recover
                    QFile::remove(QDir::homePath() + "/" + autosaveFile);
                }
            }
            autoSaveFiles = filesToRecover;
        }
        else {
            // don't recover any of the files, but don't delete them either
            autoSaveFiles.clear();
        }
    }

    if (autoSaveFiles.size() > 0) {
        short int numberOfOpenDocuments = 0; // number of documents open
        KUrl url;

        foreach(const QString &autoSaveFile, autoSaveFiles) {
            // For now create an empty document
            url.setPath(QDir::homePath() + "/" + autoSaveFile);
            KisDocument *doc = mainWindow->createDocumentFromUrl(url);
            if (doc) {
                doc->resetURL();
                doc->setModified(true);
                QFile::remove(url.toLocalFile());
                numberOfOpenDocuments++;
            }
        }
        return numberOfOpenDocuments;
    }

    return 0;
}

#include <KisApplication.moc>
