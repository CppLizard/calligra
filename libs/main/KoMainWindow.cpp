/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000-2006 David Faure <faure@kde.org>
   Copyright (C) 2007, 2009 Thomas zander <zander@kde.org>
   Copyright (C) 2010 Benjamin Port <port.benjamin@gmail.com>

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

#include "KoMainWindow.h"

#ifdef __APPLE__
#include "MacSupport.h"
#endif

#include "KoView.h"
#include "KoDocument.h"
#include "KoFilterManager.h"
#include "KoDocumentInfo.h"
#include "KoDocumentInfoDlg.h"
#include "KoFileDialogHelper.h"
#include "KoVersionDialog.h"
#include "KoDockFactoryBase.h"
#include "KoDockWidgetTitleBar.h"
#include "KoPrintJob.h"
#include "KoDockerManager.h"
#include "KoPart.h"
#include <KoPageLayoutDialog.h>
#include "KoApplication.h"
#include <KoPageLayoutWidget.h>
#include <KoIcon.h>
#include <KoConfig.h>

#include <kdeversion.h>
#if KDE_IS_VERSION(4,6,0)
#include <krecentdirs.h>
#endif
#include <khelpmenu.h>
#include <krecentfilesaction.h>
#include <kaboutdata.h>
#include <ktoggleaction.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <kedittoolbar.h>
#include <ktemporaryfile.h>
#include <krecentdocument.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <kglobalsettings.h>
#include <ktoolinvocation.h>
#include <kxmlguifactory.h>
#include <kfileitem.h>
#include <ktoolbar.h>
#include <kdebug.h>
#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kurlcombobox.h>
#include <kdiroperator.h>
#include <kmenubar.h>
#include <kmimetype.h>

#ifdef HAVE_KACTIVITIES
#include <KActivities/ResourceInstance>
#endif

//   // qt includes
#include <QDockWidget>
#include <QApplication>
#include <QLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTabBar>
#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QPrintPreviewDialog>
#include <QCloseEvent>
#include <QPointer>

#include "thememanager.h"

#include "calligraversion.h"

class KoMainWindowPrivate
{
public:
    KoMainWindowPrivate(KoPart *_part, KoMainWindow *w)
    {
        part = _part;
        parent = w;
        mainWindowGuiIsBuilt = false;
        forQuit = false;
        activeView = 0;
        firstTime = true;
        progress = 0;
        showDocumentInfo = 0;
        saveAction = 0;
        saveActionAs = 0;
        printAction = 0;
        printActionPreview = 0;
        statusBarLabel = 0;
        sendFileAction = 0;
        exportPdf = 0;
        closeFile = 0;
        reloadFile = 0;
        showFileVersions = 0;
        importFile = 0;
        exportFile = 0;
        encryptDocument = 0;
#ifndef NDEBUG
        uncompressToDir = 0;
#endif
        isImporting = false;
        isExporting = false;
        windowSizeDirty = false;
        lastExportSpecialOutputFlag = 0;
        readOnly = false;
        dockWidgetMenu = 0;
        dockerManager = 0;
        deferredClosingEvent = 0;
#ifdef HAVE_KACTIVITIES
        activityResource = 0;
#endif
        themeManager = 0;
        m_helpMenu = 0;
        m_activeWidget = 0;

        noCleanup = false;
    }

    ~KoMainWindowPrivate() {
        qDeleteAll(toolbarList);
    }

    void applyDefaultSettings(QPrinter &printer) {

        if (!activeView) return;

        QString title = activeView->document()->documentInfo()->aboutInfo("title");
        if (title.isEmpty()) {
            title = activeView->document()->url().fileName();
            // strip off the native extension (I don't want foobar.kwd.ps when printing into a file)
            KMimeType::Ptr mime = KMimeType::mimeType(activeView->document()->outputMimeType());
            if (mime) {
                QString extension = mime->property("X-KDE-NativeExtension").toString();

                if (title.endsWith(extension))
                    title.chop(extension.length());
            }
        }

        if (title.isEmpty()) {
            // #139905
            const QString programName = parent->componentData().aboutData() ?
                        parent->componentData().aboutData()->programName() : parent->componentData().componentName();
            title = i18n("%1 unsaved document (%2)", programName,
                         KGlobal::locale()->formatDate(QDate::currentDate(), KLocale::ShortDate));
        }
        printer.setDocName(title);
    }

    // PartManager
    QPointer<KoPart> part;

    KoMainWindow *parent;

    QList<QPointer<KoView> > views;
    QPointer<KoView> activeView;

    QWidget *m_activeWidget;

    QLabel * statusBarLabel;
    QProgressBar *progress;

    QList<QAction *> toolbarList;

    bool mainWindowGuiIsBuilt;
    bool forQuit;
    bool firstTime;
    bool windowSizeDirty;
    bool readOnly;

    KAction *showDocumentInfo;
    KAction *saveAction;
    KAction *saveActionAs;
    KAction *printAction;
    KAction *printActionPreview;
    KAction *sendFileAction;
    KAction *exportPdf;
    KAction *closeFile;
    KAction *reloadFile;
    KAction *showFileVersions;
    KAction *importFile;
    KAction *exportFile;
    KAction *encryptDocument;
#ifndef NDEBUG
    KAction *uncompressToDir;
#endif
    KToggleAction *toggleDockers;
    KRecentFilesAction *recent;

    bool isImporting;
    bool isExporting;

    KUrl lastExportUrl;
    QByteArray lastExportedFormat;
    int lastExportSpecialOutputFlag;

    QMap<QString, QDockWidget *> dockWidgetsMap;
    KActionMenu *dockWidgetMenu;
    QMap<QDockWidget *, bool> dockWidgetVisibilityMap;
    KoDockerManager *dockerManager;
    QList<QDockWidget *> dockWidgets;
    QByteArray m_dockerStateBeforeHiding;

    QCloseEvent *deferredClosingEvent;

#ifdef HAVE_KACTIVITIES
    KActivities::ResourceInstance *activityResource;
#endif

    Digikam::ThemeManager *themeManager;

    KHelpMenu *m_helpMenu;

    bool noCleanup;

};

KoMainWindow::KoMainWindow(KoPart *part, const KComponentData &componentData)
    : KXmlGuiWindow()
    , d(new KoMainWindowPrivate(part, this))
{
#ifdef __APPLE__
    //setUnifiedTitleAndToolBarOnMac(true);
    MacSupport::addFullscreen(this);
#endif
    setStandardToolBarMenuEnabled(true);
    Q_ASSERT(componentData.isValid());

    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    connect(this, SIGNAL(restoringDone()), this, SLOT(forceDockTabFonts()));

    if (componentData.isValid()) {
        setComponentData(componentData);   // don't load plugins! we don't want
        // the part's plugins with this main window, even though we are using the
        // part's componentData! (Simon)
        KGlobal::setActiveComponent(componentData);
    }

    QString doc;
    QStringList allFiles = KGlobal::dirs()->findAllResources("data", "calligra/calligra_shell.rc");
    setXMLFile(findMostRecentXMLFile(allFiles, doc));
    setLocalXMLFile(KStandardDirs::locateLocal("data", "calligra/calligra_shell.rc"));

    actionCollection()->addAction(KStandardAction::New, "file_new", this, SLOT(slotFileNew()));
    actionCollection()->addAction(KStandardAction::Open, "file_open", this, SLOT(slotFileOpen()));
    d->recent = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(const KUrl&)), actionCollection());
    connect(d->recent, SIGNAL(recentListCleared()), this, SLOT(saveRecentFiles()));
    d->saveAction = actionCollection()->addAction(KStandardAction::Save,  "file_save", this, SLOT(slotFileSave()));
    d->saveActionAs = actionCollection()->addAction(KStandardAction::SaveAs,  "file_save_as", this, SLOT(slotFileSaveAs()));
    d->printAction = actionCollection()->addAction(KStandardAction::Print,  "file_print", this, SLOT(slotFilePrint()));
    d->printActionPreview = actionCollection()->addAction(KStandardAction::PrintPreview,  "file_print_preview", this, SLOT(slotFilePrintPreview()));

    d->exportPdf  = new KAction(i18n("Export as PDF..."), this);
    d->exportPdf->setIcon(koIcon("application-pdf"));
    actionCollection()->addAction("file_export_pdf", d->exportPdf);
    connect(d->exportPdf, SIGNAL(triggered()), this, SLOT(exportToPdf()));

    d->sendFileAction = actionCollection()->addAction(KStandardAction::Mail,  "file_send_file", this, SLOT(slotEmailFile()));

    d->closeFile = actionCollection()->addAction(KStandardAction::Close,  "file_close", this, SLOT(slotFileClose()));
    actionCollection()->addAction(KStandardAction::Quit,  "file_quit", this, SLOT(slotFileQuit()));

    d->reloadFile  = new KAction(i18n("Reload"), this);
    actionCollection()->addAction("file_reload_file", d->reloadFile);
    connect(d->reloadFile, SIGNAL(triggered(bool)), this, SLOT(slotReloadFile()));

    d->showFileVersions  = new KAction(i18n("Versions..."), this);
    actionCollection()->addAction("file_versions_file", d->showFileVersions);
    connect(d->showFileVersions, SIGNAL(triggered(bool)), this, SLOT(slotVersionsFile()));

    d->importFile  = new KAction(koIcon("document-import"), i18n("Open ex&isting Document as Untitled Document..."), this);
    actionCollection()->addAction("file_import_file", d->importFile);
    connect(d->importFile, SIGNAL(triggered(bool)), this, SLOT(slotImportFile()));

    d->exportFile  = new KAction(koIcon("document-export"), i18n("E&xport..."), this);
    actionCollection()->addAction("file_export_file", d->exportFile);
    connect(d->exportFile, SIGNAL(triggered(bool)), this, SLOT(slotExportFile()));

    d->encryptDocument = new KAction(i18n("En&crypt Document"), this);
    actionCollection()->addAction("file_encrypt_doc", d->encryptDocument);
    connect(d->encryptDocument, SIGNAL(triggered(bool)), this, SLOT(slotEncryptDocument()));

#ifndef NDEBUG
    d->uncompressToDir = new KAction(i18n("&Uncompress to Directory"), this);
    actionCollection()->addAction("file_uncompress_doc", d->uncompressToDir);
    connect(d->uncompressToDir, SIGNAL(triggered(bool)), this, SLOT(slotUncompressToDir()));
#endif

    KAction *actionNewView  = new KAction(koIcon("window-new"), i18n("&Open Current Document in New Window"), this);
    actionCollection()->addAction("view_newview", actionNewView);
    connect(actionNewView, SIGNAL(triggered(bool)), this, SLOT(newWindow()));

    /* The following entry opens the document information dialog.  Since the action is named so it
        intends to show data this entry should not have a trailing ellipses (...).  */
    d->showDocumentInfo  = new KAction(koIcon("document-properties"), i18n("Document Information"), this);
    actionCollection()->addAction("file_documentinfo", d->showDocumentInfo);
    connect(d->showDocumentInfo, SIGNAL(triggered(bool)), this, SLOT(slotDocumentInfo()));

    KStandardAction::keyBindings(this, SLOT(slotConfigureKeys()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(slotConfigureToolbars()), actionCollection());

    d->showDocumentInfo->setEnabled(false);
    d->saveActionAs->setEnabled(false);
    d->reloadFile->setEnabled(false);
    d->showFileVersions->setEnabled(false);
    d->importFile->setEnabled(true);    // always enabled like File --> Open
    d->exportFile->setEnabled(false);
    d->saveAction->setEnabled(false);
    d->printAction->setEnabled(false);
    d->printActionPreview->setEnabled(false);
    d->sendFileAction->setEnabled(false);
    d->exportPdf->setEnabled(false);
    d->closeFile->setEnabled(false);
    d->encryptDocument->setEnabled(false);
#ifndef NDEBUG
    d->uncompressToDir->setEnabled(false);
#endif

    // populate theme menu
    d->themeManager = new Digikam::ThemeManager(this);
    KConfigGroup group(KGlobal::config(), "theme");
    d->themeManager->setThemeMenuAction(new KActionMenu(i18n("&Themes"), this));
    d->themeManager->registerThemeActions(actionCollection());
    d->themeManager->setCurrentTheme(group.readEntry("Theme",
                                                     d->themeManager->defaultThemeName()));

    KToggleAction *fullscreenAction  = new KToggleAction(koIcon("view-fullscreen"), i18n("Full Screen Mode"), this);
    actionCollection()->addAction("view_fullscreen", fullscreenAction);
    fullscreenAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F));
    connect(fullscreenAction, SIGNAL(toggled(bool)), this, SLOT(viewFullscreen(bool)));

    d->toggleDockers = new KToggleAction(i18n("Show Dockers"), this);
    d->toggleDockers->setChecked(true);
    actionCollection()->addAction("view_toggledockers", d->toggleDockers);

    connect(d->toggleDockers, SIGNAL(toggled(bool)), SLOT(toggleDockersVisibility(bool)));

    d->dockWidgetMenu  = new KActionMenu(i18n("Dockers"), this);
    actionCollection()->addAction("settings_dockers_menu", d->dockWidgetMenu);
    d->dockWidgetMenu->setVisible(false);
    d->dockWidgetMenu->setDelayed(false);

    // Load list of recent files
    KSharedConfigPtr configPtr = componentData.isValid() ? componentData.config() : KGlobal::config();
    d->recent->loadEntries(configPtr->group("RecentFiles"));


    createMainwindowGUI();
    d->mainWindowGuiIsBuilt = true;
#ifndef Q_OS_WIN
    // if the user didn's specify the geometry on the command line (does anyone do that still?),
    // we first figure out some good default size and restore the x,y position. See bug 285804Z.
    if (!initialGeometrySet()) {

        const int scnum = QApplication::desktop()->screenNumber(parentWidget());
        QRect desk = QApplication::desktop()->availableGeometry(scnum);
        // if the desktop is virtual then use virtual screen size
        if (QApplication::desktop()->isVirtualDesktop()) {
            desk = QApplication::desktop()->availableGeometry(QApplication::desktop()->screen());
            desk = QApplication::desktop()->availableGeometry(QApplication::desktop()->screen(scnum));
        }

        quint32 x = desk.x();
        quint32 y = desk.y();
        quint32 w = 0;
        quint32 h = 0;

        // Default size -- maximize on small screens, something useful on big screens
        const int deskWidth = desk.width();
        if (deskWidth > 1024) {
            // a nice width, and slightly less than total available
            // height to componensate for the window decs
            w = ( deskWidth / 3 ) * 2;
            h = desk.height();
        }
        else {
            w = desk.width();
            h = desk.height();
        }
        // KDE doesn't restore the x,y position, so let's do that ourselves
        KConfigGroup cfg(KGlobal::config(), "MainWindow");
        x = cfg.readEntry("ko_x", x);
        y = cfg.readEntry("ko_y", y);
        setGeometry(x, y, w, h);
    }
#endif

    // Now ask kde to restore the size of the window; this could probably be replaced by
    // QWidget::saveGeometry and QWidget::restoreGeometry, but let's stay with the KDE
    // way of doing things.
    KConfigGroup config(KGlobal::config(), "MainWindow");
    restoreWindowSize( config );

    d->dockerManager = new KoDockerManager(this);
}

void KoMainWindow::setNoCleanup(bool noCleanup)
{
    d->noCleanup = noCleanup;
}

KoMainWindow::~KoMainWindow()
{
    KConfigGroup cfg(KGlobal::config(), "MainWindow");
    cfg.writeEntry("ko_x", frameGeometry().x());
    cfg.writeEntry("ko_y", frameGeometry().y());
    {
        KConfigGroup group(KGlobal::config(), "theme");
        group.writeEntry("Theme", d->themeManager->currentThemeName());
    }

    // Explicitly delete the docker manager to ensure that it is deleted before the dockers
    delete d->dockerManager;
    d->dockerManager = 0;

    // The doc and view might still exist (this is the case when closing the window)
    if (d->part) {
        d->part->removeMainWindow(this);
    }

    // safety first ;)
    setActiveView(0);

    if(d->noCleanup)
        return;

    delete d;
}

void KoMainWindow::addView(KoView *view)
{
    if (d->activeView == view) return;

    if (d->activeView) {
        KoDocument *activeDocument = d->activeView->document();

        if (activeDocument) {
            activeDocument->disconnect(this);

            if (dockerManager()) {
                dockerManager()->resetToolDockerWidgets();
            }

            // Hide all dockwidgets and remember their old state
            d->dockWidgetVisibilityMap.clear();

            foreach(QDockWidget* dockWidget, d->dockWidgetsMap) {
                d->dockWidgetVisibilityMap.insert(dockWidget, dockWidget->isVisible());
                dockWidget->setVisible(false);
            }

            d->dockWidgetMenu->setVisible(false);
        }
    }

    d->dockWidgetMenu->setVisible(true);

    d->views.append(view);
    // XXX: This should change -- with multiple views per main window, the view should
    // no longer be a kxmlguiclient.
    guiFactory()->addClient(view);
    showView(view);

    bool viewHasDocument = d->activeView->document() != 0 ? true : false;

    d->showDocumentInfo->setEnabled(viewHasDocument);
    d->saveAction->setEnabled(viewHasDocument);
    d->saveActionAs->setEnabled(viewHasDocument);
    d->importFile->setEnabled(viewHasDocument);
    d->exportFile->setEnabled(viewHasDocument);
    d->encryptDocument->setEnabled(viewHasDocument);
#ifndef NDEBUG
    d->uncompressToDir->setEnabled(viewHasDocument);
#endif
    d->printAction->setEnabled(viewHasDocument);
    d->printActionPreview->setEnabled(viewHasDocument);
    d->sendFileAction->setEnabled(viewHasDocument);
    d->exportPdf->setEnabled(viewHasDocument);
    d->closeFile->setEnabled(viewHasDocument);
    statusBar()->setVisible(viewHasDocument);

    updateCaption();

    emit restoringDone();

    if (viewHasDocument && !d->dockWidgetVisibilityMap.isEmpty()) {
        foreach(QDockWidget* dockWidget, d->dockWidgetsMap) {
            dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    }

    if (viewHasDocument) {
        connect(d->activeView->document(), SIGNAL(titleModified(QString,bool)), SLOT(slotDocumentTitleModified(QString,bool)));
    }
}

void KoMainWindow::showView(KoView *view)
{
    Q_ASSERT(d->views.contains(view));
    d->activeView = view;
    setCentralWidget(view);
    view->show();
    view->setFocus();
}

void KoMainWindow::updateReloadFileAction(KoDocument *doc)
{
    d->reloadFile->setEnabled(doc && !doc->url().isEmpty());
}

void KoMainWindow::updateVersionsFileAction(KoDocument *doc)
{
    //TODO activate it just when we save it in oasis file format
    d->showFileVersions->setEnabled(doc && !doc->url().isEmpty() && (doc->outputMimeType() == doc->nativeOasisMimeType() || doc->outputMimeType() == doc->nativeOasisMimeType() + "-template"));
}

void KoMainWindow::setReadWrite(bool readwrite)
{
    d->saveAction->setEnabled(readwrite);
    d->importFile->setEnabled(readwrite);
    d->readOnly =  !readwrite;
    updateCaption();
}

void KoMainWindow::addRecentURL(const KUrl& url)
{
    kDebug(30003) << "KoMainWindow::addRecentURL url=" << url.prettyUrl();
    // Add entry to recent documents list
    // (call coming from KoDocument because it must work with cmd line, template dlg, file/open, etc.)
    if (!url.isEmpty()) {
        bool ok = true;
        if (url.isLocalFile()) {
            QString path = url.toLocalFile(KUrl::RemoveTrailingSlash);
            const QStringList tmpDirs = KGlobal::dirs()->resourceDirs("tmp");
            for (QStringList::ConstIterator it = tmpDirs.begin() ; ok && it != tmpDirs.end() ; ++it)
                if (path.contains(*it))
                    ok = false; // it's in the tmp resource
            if (ok) {
                KRecentDocument::add(path);
#if KDE_IS_VERSION(4,6,0)
                KRecentDirs::add(":OpenDialog", QFileInfo(path).dir().canonicalPath());
#endif
            }
        } else {
            KRecentDocument::add(url.url(KUrl::RemoveTrailingSlash), true);
        }
        if (ok) {
            d->recent->addUrl(url);
        }
        saveRecentFiles();

#ifdef HAVE_KACTIVITIES
        if (!d->activityResource) {
            d->activityResource = new KActivities::ResourceInstance(winId(), this);
        }
        d->activityResource->setUri(url);
#endif
    }
}

void KoMainWindow::saveRecentFiles()
{
    // Save list of recent files
    KSharedConfigPtr config = componentData().isValid() ? componentData().config() : KGlobal::config();
    kDebug(30003) << this << " Saving recent files list into config. componentData()=" << componentData().componentName();
    d->recent->saveEntries(config->group("RecentFiles"));
    config->sync();

    // Tell all windows to reload their list, after saving
    // Doesn't work multi-process, but it's a start
    foreach(KMainWindow* window, KMainWindow::memberList())
        static_cast<KoMainWindow *>(window)->reloadRecentFileList();
}

void KoMainWindow::reloadRecentFileList()
{
    KSharedConfigPtr config = componentData().isValid() ? componentData().config() : KGlobal::config();
    d->recent->loadEntries(config->group("RecentFiles"));
}

void KoMainWindow::updateCaption()
{
    kDebug(30003) << "KoMainWindow::updateCaption()";
    if (!d->activeView->document()) {
        updateCaption(QString(), false);
    }
    else {
        QString caption( d->activeView->document()->caption() );
        if (d->readOnly) {
            caption += ' ' + i18n("(write protected)");
        }

        updateCaption(caption, d->activeView->document()->isModified());
        if (!d->activeView->document()->url().fileName(KUrl::ObeyTrailingSlash).isEmpty())
            d->saveAction->setToolTip(i18n("Save as %1", d->activeView->document()->url().fileName(KUrl::ObeyTrailingSlash)));
        else
            d->saveAction->setToolTip(i18n("Save"));
    }
}

void KoMainWindow::updateCaption(const QString & caption, bool mod)
{
    kDebug(30003) << "KoMainWindow::updateCaption(" << caption << "," << mod << ")";
#ifdef CALLIGRA_ALPHA
    setCaption(QString("ALPHA %1: %2").arg(CALLIGRA_ALPHA).arg(caption), mod);
    return;
#endif
#ifdef CALLIGRA_BETA
    setCaption(QString("BETA %1: %2").arg(CALLIGRA_BETA).arg(caption), mod);
    return;
#endif
#ifdef CALLIGRA_RC
    setCaption(QString("RELEASE CANDIDATE %1: %2").arg(CALLIGRA_RC).arg(caption), mod);
    return;
#endif

    setCaption(caption, mod);
}


KoView *KoMainWindow::activeView() const
{
    if (d->activeView) {
        return d->activeView;
    }
    else if (!d->views.isEmpty()) {
        return d->views.first();
    }
    return 0;
}

bool KoMainWindow::openDocument(const KUrl & url)
{
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, 0)) {
        KMessageBox::error(0, i18n("The file %1 does not exist.", url.url()));
        d->recent->removeUrl(url); //remove the file from the recent-opened-file-list
        saveRecentFiles();
        return false;
    }
    return openDocumentInternal(url);
}

KoDocument *KoMainWindow::createDocumentFromUrl(const KUrl & url)
{
    KoDocument *newdoc = d->part->createDocument();
    KoView *view = d->part->createView(newdoc);
    d->part->addDocument(newdoc);

    // For remote documents
#if 0 // XXX: seems broken for now
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, 0)) {
        newdoc->initEmpty(); //create an empty document
        addView(view);
        newdoc->setUrl(url);
        QString mime = KMimeType::findByUrl(url)->name();
        if (mime.isEmpty() || mime == KMimeType::defaultMimeType())
            mime = newdoc->nativeFormatMimeType();
        newdoc->setMimeTypeAfterLoading(mime);
        updateCaption();
        return newdoc;
    }
#endif
    if (openDocumentInternal(url, newdoc)) {
        addView(view);
    }
    return newdoc;
}

bool KoMainWindow::openDocumentInternal(const KUrl & url, KoDocument *newdoc)
{
    kDebug(30003) <<"KoMainWindow::openDocument" << url.url();

    if (!newdoc) {
        newdoc = d->part->createDocument();
        d->part->addDocument(newdoc);
    }

    d->firstTime = true;
    connect(newdoc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    connect(newdoc, SIGNAL(completed()), this, SLOT(slotLoadCompleted()));
    connect(newdoc, SIGNAL(canceled(const QString &)), this, SLOT(slotLoadCanceled(const QString &)));
    bool openRet = (!isImporting()) ? newdoc->openUrl(url) : newdoc->importDocument(url);
    if (!openRet) {
        d->part->removeMainWindow(this);
        delete newdoc;
        return false;
    }
    updateReloadFileAction(newdoc);
    updateVersionsFileAction(newdoc);

    KFileItem file(url, newdoc->mimeType(), KFileItem::Unknown);
    if (!file.isWritable()) {
        setReadWrite(false);
    }
    return true;
}

// Separate from openDocument to handle async loading (remote URLs)
void KoMainWindow::slotLoadCompleted()
{
    kDebug(30003) << "KoMainWindow::slotLoadCompleted";
    KoDocument *newdoc = qobject_cast<KoDocument*>(sender());
    KoView *view = newdoc->documentPart()->createView(newdoc);
    addView(view);

    disconnect(newdoc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    disconnect(newdoc, SIGNAL(completed()), this, SLOT(slotLoadCompleted()));
    disconnect(newdoc, SIGNAL(canceled(const QString &)), this, SLOT(slotLoadCanceled(const QString &)));
    emit loadCompleted();
}

void KoMainWindow::slotLoadCanceled(const QString & errMsg)
{
    kDebug(30003) << "KoMainWindow::slotLoadCanceled";
    if (!errMsg.isEmpty())   // empty when canceled by user
        KMessageBox::error(this, errMsg);
    // ... can't delete the document, it's the one who emitted the signal...

    KoDocument* doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    disconnect(doc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    disconnect(doc, SIGNAL(completed()), this, SLOT(slotLoadCompleted()));
    disconnect(doc, SIGNAL(canceled(const QString &)), this, SLOT(slotLoadCanceled(const QString &)));
}

void KoMainWindow::slotSaveCanceled(const QString &errMsg)
{
    kDebug(30003) << "KoMainWindow::slotSaveCanceled";
    if (!errMsg.isEmpty())   // empty when canceled by user
        KMessageBox::error(this, errMsg);
    slotSaveCompleted();
}

void KoMainWindow::slotSaveCompleted()
{
    kDebug(30003) << "KoMainWindow::slotSaveCompleted";
    KoDocument* doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    disconnect(doc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    disconnect(doc, SIGNAL(completed()), this, SLOT(slotSaveCompleted()));
    disconnect(doc, SIGNAL(canceled(const QString &)), this, SLOT(slotSaveCanceled(const QString &)));

    if (d->deferredClosingEvent) {
        KXmlGuiWindow::closeEvent(d->deferredClosingEvent);
    }
}

// returns true if we should save, false otherwise.
bool KoMainWindow::exportConfirmation(const QByteArray &outputFormat)
{
    KConfigGroup group = KGlobal::config()->group(d->part->componentData().componentName());
    if (!group.readEntry("WantExportConfirmation", true)) {
        return true;
    }

    KMimeType::Ptr mime = KMimeType::mimeType(outputFormat);
    QString comment = mime ? mime->comment() : i18n("%1 (unknown file type)", QString::fromLatin1(outputFormat));

    // Warn the user
    int ret;
    if (!isExporting()) { // File --> Save
        ret = KMessageBox::warningContinueCancel
                (
                    this,
                    i18n("<qt>Saving as a %1 may result in some loss of formatting."
                         "<p>Do you still want to save in this format?</qt>",
                         QString("<b>%1</b>").arg(comment)),      // in case we want to remove the bold later
                    i18n("Confirm Save"),
                    KStandardGuiItem::save(),
                    KStandardGuiItem::cancel(),
                    "NonNativeSaveConfirmation"
                    );
    } else { // File --> Export
        ret = KMessageBox::warningContinueCancel
                (
                    this,
                    i18n("<qt>Exporting as a %1 may result in some loss of formatting."
                         "<p>Do you still want to export to this format?</qt>",
                         QString("<b>%1</b>").arg(comment)),      // in case we want to remove the bold later
                    i18n("Confirm Export"),
                    KGuiItem(i18n("Export")),
                    KStandardGuiItem::cancel(),
                    "NonNativeExportConfirmation" // different to the one used for Save (above)
                    );
    }

    return (ret == KMessageBox::Continue);
}

bool KoMainWindow::saveDocument(bool saveas, bool silent, int specialOutputFlag)
{
    if (!d->activeView->document() || !d->part) {
        return true;
    }

    bool reset_url;

    if (d->activeView->document()->url().isEmpty()) {
        reset_url = true;
        saveas = true;
    } else {
        reset_url = false;
    }

    connect(d->activeView->document(), SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    connect(d->activeView->document(), SIGNAL(completed()), this, SLOT(slotSaveCompleted()));
    connect(d->activeView->document(), SIGNAL(canceled(const QString &)), this, SLOT(slotSaveCanceled(const QString &)));

    KUrl oldURL = d->activeView->document()->url();
    QString oldFile = d->activeView->document()->localFilePath();

    QByteArray _native_format = d->activeView->document()->nativeFormatMimeType();
    QByteArray oldOutputFormat = d->activeView->document()->outputMimeType();

    int oldSpecialOutputFlag = d->activeView->document()->specialOutputFlag();

    KUrl suggestedURL = d->activeView->document()->url();

    QStringList mimeFilter;
    KMimeType::Ptr mime = KMimeType::mimeType(_native_format);
    if (! mime)
        mime = KMimeType::defaultMimeTypePtr();
    if (specialOutputFlag)
        mimeFilter = mime->patterns();
    else
        mimeFilter = KoFilterManager::mimeFilter(_native_format,
                                                 KoFilterManager::Export,
                                                 d->activeView->document()->extraNativeMimeTypes());


    if (!mimeFilter.contains(oldOutputFormat) && !isExporting()) {
        kDebug(30003) << "KoMainWindow::saveDocument no export filter for" << oldOutputFormat;

        // --- don't setOutputMimeType in case the user cancels the Save As
        // dialog and then tries to just plain Save ---

        // suggest a different filename extension (yes, we fortunately don't all live in a world of magic :))
        QString suggestedFilename = suggestedURL.fileName();
        if (!suggestedFilename.isEmpty()) {  // ".kra" looks strange for a name
            int c = suggestedFilename.lastIndexOf('.');

            QString ext = mime->property("X-KDE-NativeExtension").toString();
            if (!ext.isEmpty()) {
                if (c < 0)
                    suggestedFilename += ext;
                else
                    suggestedFilename = suggestedFilename.left(c) + ext;
            } else { // current filename extension wrong anyway
                if (c > 0) {
                    // this assumes that a . signifies an extension, not just a .
                    suggestedFilename = suggestedFilename.left(c);
                }
            }

            suggestedURL.setFileName(suggestedFilename);
        }

        // force the user to choose outputMimeType
        saveas = true;
    }

    bool ret = false;

    if (d->activeView->document()->url().isEmpty() || saveas) {
        // if you're just File/Save As'ing to change filter options you
        // don't want to be reminded about overwriting files etc.
        bool justChangingFilterOptions = false;

        KUrl newURL(KoFileDialogHelper::getSaveFileName(
                        this,
                        i18n("untitled"),
                        (isExporting() && !d->lastExportUrl.isEmpty()) ?
                            d->lastExportUrl.url() : suggestedURL.url(),
                        mimeFilter));

        QByteArray outputFormat = _native_format;
        if (!specialOutputFlag) {
            KMimeType::Ptr mime = KMimeType::findByUrl(newURL);
            QString outputFormatString = mime->name();
            outputFormat = outputFormatString.toLatin1();
        }

        if (!isExporting())
            justChangingFilterOptions = (newURL == d->activeView->document()->url()) &&
                    (outputFormat == d->activeView->document()->mimeType()) &&
                    (specialOutputFlag == oldSpecialOutputFlag);
        else
            justChangingFilterOptions = (newURL == d->lastExportUrl) &&
                    (outputFormat == d->lastExportedFormat) &&
                    (specialOutputFlag == d->lastExportSpecialOutputFlag);


        bool bOk = true;
        if (newURL.isEmpty()) {
            bOk = false;
        }

        // adjust URL before doing checks on whether the file exists.
        if (specialOutputFlag) {
            QString fileName = newURL.fileName();
            if ( specialOutputFlag== KoDocument::SaveAsDirectoryStore) {
                qDebug() << "save to directory: " << newURL.url();
            }
            else if (specialOutputFlag == KoDocument::SaveEncrypted) {
                int dot = fileName.lastIndexOf('.');
                qDebug() << dot;
                QString ext = mime->mainExtension();
                if (!ext.isEmpty()) {
                    if (dot < 0) fileName += ext;
                    else fileName = fileName.left(dot) + ext;
                } else { // current filename extension wrong anyway
                    if (dot > 0) fileName = fileName.left(dot);
                }
                newURL.setFileName(fileName);
            }
        }

        if (bOk) {
            bool wantToSave = true;

            // don't change this line unless you know what you're doing :)
            if (!justChangingFilterOptions || d->activeView->document()->confirmNonNativeSave(isExporting())) {
                if (!d->activeView->document()->isNativeFormat(outputFormat))
                    wantToSave = exportConfirmation(outputFormat);
            }

            if (wantToSave) {
                //
                // Note:
                // If the user is stupid enough to Export to the current URL,
                // we do _not_ change this operation into a Save As.  Reasons
                // follow:
                //
                // 1. A check like "isExporting() && oldURL == newURL"
                //    doesn't _always_ work on case-insensitive filesystems
                //    and inconsistent behaviour is bad.
                // 2. It is probably not a good idea to change d->activeView->document()->mimeType
                //    and friends because the next time the user File/Save's,
                //    (not Save As) they won't be expecting that they are
                //    using their File/Export settings
                //
                // As a bad side-effect of this, the modified flag will not
                // be updated and it is possible that what is currently on
                // their screen is not what is stored on disk (through loss
                // of formatting).  But if you are dumb enough to change
                // mimetype but not the filename, then arguably, _you_ are
                // the "bug" :)
                //
                // - Clarence
                //


                d->activeView->document()->setOutputMimeType(outputFormat, specialOutputFlag);
                if (!isExporting()) {  // Save As
                    ret = d->activeView->document()->saveAs(newURL);

                    if (ret) {
                        kDebug(30003) << "Successful Save As!";
                        addRecentURL(newURL);
                        setReadWrite(true);
                    } else {
                        kDebug(30003) << "Failed Save As!";
                        d->activeView->document()->setUrl(oldURL);
                        d->activeView->document()->setLocalFilePath(oldFile);
                        d->activeView->document()->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                    }
                } else { // Export
                    ret = d->activeView->document()->exportDocument(newURL);

                    if (ret) {
                        // a few file dialog convenience things
                        d->lastExportUrl = newURL;
                        d->lastExportedFormat = outputFormat;
                        d->lastExportSpecialOutputFlag = specialOutputFlag;
                    }

                    // always restore output format
                    d->activeView->document()->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                }

                if (silent) // don't let the document change the window caption
                    d->activeView->document()->setTitleModified();
            }   // if (wantToSave)  {
            else
                ret = false;
        }   // if (bOk) {
        else
            ret = false;
    } else { // saving

        bool needConfirm = d->activeView->document()->confirmNonNativeSave(false) && !d->activeView->document()->isNativeFormat(oldOutputFormat);

        if (!needConfirm ||
                (needConfirm && exportConfirmation(oldOutputFormat /* not so old :) */))
                ) {
            // be sure d->activeView->document() has the correct outputMimeType!
            if (isExporting() || d->activeView->document()->isModified()) {
                ret = d->activeView->document()->save();
            }

            if (!ret) {
                kDebug(30003) << "Failed Save!";
                d->activeView->document()->setUrl(oldURL);
                d->activeView->document()->setLocalFilePath(oldFile);
            }
        } else
            ret = false;
    }


    if (!ret && reset_url)
        d->activeView->document()->resetURL(); //clean the suggested filename as the save dialog was rejected

    updateReloadFileAction(d->activeView->document());
    updateCaption();

    return ret;
}

void KoMainWindow::closeEvent(QCloseEvent *e)
{
    if(d->activeView && d->activeView->document() && d->activeView->document()->isLoading()) {
        e->setAccepted(false);
        return;
    }
    if (queryClose()) {
        d->deferredClosingEvent = e;

        if (!d->m_dockerStateBeforeHiding.isEmpty()) {
            restoreState(d->m_dockerStateBeforeHiding);
        }
        statusBar()->setVisible(true);
        menuBar()->setVisible(true);

        saveWindowSettings();

        if (d->noCleanup)
            return;

        foreach(KoView *view, d->views) {
            d->part->removeView(view);
        }

        if (!d->dockWidgetVisibilityMap.isEmpty()) { // re-enable dockers for persistency
            foreach(QDockWidget* dockWidget, d->dockWidgetsMap)
                dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    } else {
        e->setAccepted(false);
    }
}

void KoMainWindow::saveWindowSettings()
{
    KSharedConfigPtr config = componentData().config();

    if (d->windowSizeDirty ) {

        // Save window size into the config file of our componentData
        kDebug(30003) << "KoMainWindow::saveWindowSettings";
        saveWindowSize(config->group("MainWindow"));
        config->sync();
        d->windowSizeDirty = false;
    }

    if (!d->activeView || d->activeView->document()) {

        // Save toolbar position into the config file of the app, under the doc's component name
        KConfigGroup group = KGlobal::config()->group(d->part->componentData().componentName());
        saveMainWindowSettings(group);

        // Save collapsable state of dock widgets
        for (QMap<QString, QDockWidget*>::const_iterator i = d->dockWidgetsMap.constBegin();
             i != d->dockWidgetsMap.constEnd(); ++i) {
            if (i.value()->widget()) {
                KConfigGroup dockGroup = group.group(QString("DockWidget ") + i.key());
                dockGroup.writeEntry("Collapsed", i.value()->widget()->isHidden());
                dockGroup.writeEntry("DockArea", (int) dockWidgetArea(i.value()));
            }
        }

    }

    KGlobal::config()->sync();
    resetAutoSaveSettings(); // Don't let KMainWindow override the good stuff we wrote down

}

void KoMainWindow::resizeEvent(QResizeEvent * e)
{
    d->windowSizeDirty = true;
    KXmlGuiWindow::resizeEvent(e);
}

bool KoMainWindow::queryClose()
{
    if (!d->activeView || d->activeView->document() == 0)
        return true;

    //kDebug(30003) <<"KoMainWindow::queryClose() viewcount=" << d->activeView->document()->viewCount()
    //               << " mainWindowCount=" << d->activeView->document()->mainWindowCount() << endl;
    if (!d->forQuit && d->part->mainwindowCount() > 1)
        // there are more open, and we are closing just one, so no problem for closing
        return true;

    // main doc + internally stored child documents
    if (d->activeView->document()->isModified()) {
        QString name;
        if (d->activeView->document()->documentInfo()) {
            name = d->activeView->document()->documentInfo()->aboutInfo("title");
        }
        if (name.isEmpty())
            name = d->activeView->document()->url().fileName();

        if (name.isEmpty())
            name = i18n("Untitled");

        int res = KMessageBox::warningYesNoCancel(this,
                                                  i18n("<p>The document <b>'%1'</b> has been modified.</p><p>Do you want to save it?</p>", name),
                                                  QString(),
                                                  KStandardGuiItem::save(),
                                                  KStandardGuiItem::discard());

        switch (res) {
        case KMessageBox::Yes : {
            bool isNative = (d->activeView->document()->outputMimeType() == d->activeView->document()->nativeFormatMimeType());
            if (!saveDocument(!isNative))
                return false;
            break;
        }
        case KMessageBox::No :
            d->activeView->document()->removeAutoSaveFiles();
            d->activeView->document()->setModified(false);   // Now when queryClose() is called by closeEvent it won't do anything.
            break;
        default : // case KMessageBox::Cancel :
            return false;
        }
    }

    return true;
}

void KoMainWindow::slotFileNew()
{
    KoMainWindow *s = d->part->createMainWindow();
    s->show();
    d->part->showStartUpWidget(s, true /*Always show widget*/);
}

void KoMainWindow::slotFileOpen()
{

    const QStringList mimeFilter = koApp->mimeFilter(KoFilterManager::Import);
    //KoFilterManager::mimeFilter(KoServiceProvider::readNativeFormatMimeType(),
    //                                       KoFilterManager::Import,
    //                                       KoServiceProvider::readExtraNativeMimeTypes());

    KConfigGroup group = KGlobal::config()->group("File Dialogs");
    QString defaultDir = group.readEntry("OpenDialog");
    if (defaultDir.isEmpty())
        defaultDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    QString url;
    if (!isImporting()) {
        url = KoFileDialogHelper::getOpenFileName(this,
                                                  i18n("Open Document"),
                                                  defaultDir,
                                                  mimeFilter);
    } else {
        url = KoFileDialogHelper::getImportFileName(this,
                                                    i18n("Import Document"),
                                                    defaultDir,
                                                    mimeFilter);
    }

    if (url.isEmpty())
        return;
    group.writeEntry("OpenDialog", url);

    (void) openDocument(KUrl(url));
}

void KoMainWindow::slotFileOpenRecent(const KUrl & url)
{
    // Create a copy, because the original KUrl in the map of recent files in
    // KRecentFilesAction may get deleted.
    (void) openDocument(KUrl(url));
}

void KoMainWindow::slotFileSave()
{
    if (saveDocument())
        emit documentSaved();
}

void KoMainWindow::slotFileSaveAs()
{
    if (saveDocument(true))
        emit documentSaved();
}

void KoMainWindow::slotEncryptDocument()
{
    if (saveDocument(false, false, KoDocument::SaveEncrypted))
        emit documentSaved();
}

void KoMainWindow::slotUncompressToDir()
{
    if (saveDocument(true, false, KoDocument::SaveAsDirectoryStore))
        emit documentSaved();
}

void KoMainWindow::slotDocumentInfo()
{
    if (!d->activeView->document())
        return;

    KoDocumentInfo *docInfo = d->activeView->document()->documentInfo();

    if (!docInfo)
        return;

    KoDocumentInfoDlg *dlg = d->activeView->document()->createDocumentInfoDialog(this, docInfo);

    if (dlg->exec()) {
        if (dlg->isDocumentSaved()) {
            d->activeView->document()->setModified(false);
        } else {
            d->activeView->document()->setModified(true);
        }
        d->activeView->document()->setTitleModified();
    }

    delete dlg;
}

void KoMainWindow::slotFileClose()
{
    if (queryClose()) {
        saveWindowSettings();

        KoView *activeView = d->activeView;
        d->views.removeAll(activeView);
        if (activeView->document()) {
            activeView->document()->clearUndoHistory();
        }
        d->part->removeView(activeView);

        if (d->views.size() > 0) {
            showView(d->views.first());
        }
        else {
            d->activeView = 0;
        }
        activeView->close();
        delete activeView;
        d->part->showStartUpWidget(this, true /*Always show widget*/);
    }
}

void KoMainWindow::slotFileQuit()
{
    close();
}

void KoMainWindow::slotFilePrint()
{
    if (!activeView())
        return;
    KoPrintJob *printJob = activeView()->createPrintJob();
    if (printJob == 0)
        return;
    d->applyDefaultSettings(printJob->printer());
    QPrintDialog *printDialog = activeView()->createPrintDialog( printJob, this );
    if (printDialog && printDialog->exec() == QDialog::Accepted)
        printJob->startPrinting(KoPrintJob::DeleteWhenDone);
    else
        delete printJob;
    delete printDialog;
}

void KoMainWindow::slotFilePrintPreview()
{
    if (!activeView())
        return;
    KoPrintJob *printJob = activeView()->createPrintJob();
    if (printJob == 0)
        return;

    /* Sets the startPrinting() slot to be blocking.
     The Qt print-preview dialog requires the printing to be completely blocking
     and only return when the full document has been printed.
     By default the KoPrintingDialog is non-blocking and
     multithreading, setting blocking to true will allow it to be used in the preview dialog */
    printJob->setProperty("blocking", true);
    QPrintPreviewDialog *preview = new QPrintPreviewDialog(&printJob->printer(), this);
    printJob->setParent(preview); // will take care of deleting the job
    connect(preview, SIGNAL(paintRequested(QPrinter*)), printJob, SLOT(startPrinting()));
    preview->exec();
    delete preview;
}

KoPrintJob* KoMainWindow::exportToPdf(const QString &pdfFileName)
{
    if (!activeView())
        return 0;
    KoPageLayout pageLayout;
    pageLayout = activeView()->pageLayout();
    return exportToPdf(pageLayout, pdfFileName);
}

KoPrintJob* KoMainWindow::exportToPdf(KoPageLayout pageLayout, QString pdfFileName)
{
    if (!activeView())
        return 0;
    if (pdfFileName.isEmpty()) {
        KConfigGroup group = KGlobal::config()->group("File Dialogs");
        QString defaultDir = group.readEntry("SavePdfDialog");
        if (defaultDir.isEmpty())
            defaultDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
        KUrl startUrl = KUrl(defaultDir);
        KoDocument* pDoc = d->activeView->document();
        /** if document has a file name, take file name and replace extension with .pdf */
        if (pDoc && pDoc->url().isValid()) {
            startUrl = pDoc->url();
            QString fileName = startUrl.fileName();
            fileName = fileName.replace( QRegExp( "\\.\\w{2,5}$", Qt::CaseInsensitive ), ".pdf" );
            startUrl.setFileName( fileName );
        }

        QPointer<KoPageLayoutDialog> layoutDlg(new KoPageLayoutDialog(this, pageLayout));
        layoutDlg->setWindowModality(Qt::WindowModal);
        if (layoutDlg->exec() != QDialog::Accepted || !layoutDlg) {
            delete layoutDlg;
            return 0;
        }
        pageLayout = layoutDlg->pageLayout();
        delete layoutDlg;

        KUrl url(KoFileDialogHelper::getSaveFileName(
                     this,
                     i18n("Export as PDF"),
                     startUrl.toLocalFile(),
                     QStringList() << "application/pdf"));

        pdfFileName = url.toLocalFile();
        if (pdfFileName.isEmpty())
            return 0;
    }

    KoPrintJob *printJob = activeView()->createPdfPrintJob();
    if (printJob == 0)
        return 0;
    if (isHidden()) {
        printJob->setProperty("noprogressdialog", true);
    }

    d->applyDefaultSettings(printJob->printer());
    // TODO for remote files we have to first save locally and then upload.
    printJob->printer().setOutputFileName(pdfFileName);
    printJob->printer().setColorMode(QPrinter::Color);

    if (pageLayout.format == KoPageFormat::CustomSize) {
        printJob->printer().setPaperSize(QSizeF(pageLayout.width, pageLayout.height), QPrinter::Millimeter);
    } else {
        printJob->printer().setPaperSize(KoPageFormat::printerPageSize(pageLayout.format));
    }

    switch (pageLayout.orientation) {
    case KoPageFormat::Portrait: printJob->printer().setOrientation(QPrinter::Portrait); break;
    case KoPageFormat::Landscape: printJob->printer().setOrientation(QPrinter::Landscape); break;
    }

    printJob->printer().setPageMargins(pageLayout.leftMargin, pageLayout.topMargin, pageLayout.rightMargin, pageLayout.bottomMargin, QPrinter::Millimeter);

    //before printing check if the printer can handle printing
    if (!printJob->canPrint()) {
        KMessageBox::error(this, i18n("Cannot export to the specified file"));
    }

    printJob->startPrinting(KoPrintJob::DeleteWhenDone);
    return printJob;
}

void KoMainWindow::slotConfigureKeys()
{
    QAction* undoAction=0;
    QAction* redoAction=0;
    QString oldUndoText;
    QString oldRedoText;
    if(activeView()) {
        //The undo/redo action text is "undo" + command, replace by simple text while inside editor
        undoAction = activeView()->actionCollection()->action("edit_undo");
        redoAction = activeView()->actionCollection()->action("edit_redo");
        oldUndoText = undoAction->text();
        oldRedoText = redoAction->text();
        undoAction->setText(i18n("Undo"));
        redoAction->setText(i18n("Redo"));
    }

    guiFactory()->configureShortcuts();

    if(activeView()) {
        undoAction->setText(oldUndoText);
        redoAction->setText(oldRedoText);
    }
}

void KoMainWindow::slotConfigureToolbars()
{
    if (d->activeView->document())
        saveMainWindowSettings(KGlobal::config()->group(d->part->componentData().componentName()));
    KEditToolBar edit(factory(), this);
    connect(&edit, SIGNAL(newToolBarConfig()), this, SLOT(slotNewToolbarConfig()));
    (void) edit.exec();
}

void KoMainWindow::slotNewToolbarConfig()
{
    if (d->activeView->document()) {
        applyMainWindowSettings(KGlobal::config()->group(d->part->componentData().componentName()));
    }

    KXMLGUIFactory *factory = guiFactory();
    Q_UNUSED(factory);

    // Check if there's an active view
    if (!d->activeView)
        return;

    plugActionList("toolbarlist", d->toolbarList);
}

void KoMainWindow::slotToolbarToggled(bool toggle)
{
    //kDebug(30003) <<"KoMainWindow::slotToolbarToggled" << sender()->name() <<" toggle=" << true;
    // The action (sender) and the toolbar have the same name
    KToolBar * bar = toolBar(sender()->objectName());
    if (bar) {
        if (toggle)
            bar->show();
        else
            bar->hide();

        if (d->activeView->document())
            saveMainWindowSettings(KGlobal::config()->group(d->part->componentData().componentName()));
    } else
        kWarning(30003) << "slotToolbarToggled : Toolbar " << sender()->objectName() << " not found!";
}

bool KoMainWindow::toolbarIsVisible(const char *tbName)
{
    QWidget *tb = toolBar(tbName);
    return !tb->isHidden();
}

void KoMainWindow::showToolbar(const char * tbName, bool shown)
{
    QWidget * tb = toolBar(tbName);
    if (!tb) {
        kWarning(30003) << "KoMainWindow: toolbar " << tbName << " not found.";
        return;
    }
    if (shown)
        tb->show();
    else
        tb->hide();

    // Update the action appropriately
    foreach(QAction* action, d->toolbarList) {
        if (action->objectName() != tbName) {
            //kDebug(30003) <<"KoMainWindow::showToolbar setChecked" << shown;
            static_cast<KToggleAction *>(action)->setChecked(shown);
            break;
        }
    }
}

void KoMainWindow::viewFullscreen(bool fullScreen)
{
    if (fullScreen) {
        setWindowState(windowState() | Qt::WindowFullScreen);   // set
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);   // reset
    }
}

void KoMainWindow::slotProgress(int value)
{
    kDebug(30003) << "KoMainWindow::slotProgress" << value;
    if (value <= -1) {
        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = 0;
        }
        d->firstTime = true;
        return;
    }
    if (d->firstTime || !d->progress) {
        // The statusbar might not even be created yet.
        // So check for that first, and create it if necessary
        QStatusBar *bar = findChild<QStatusBar *>();
        if (!bar) {
            statusBar()->show();
            QApplication::sendPostedEvents(this, QEvent::ChildAdded);
        }

        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = 0;
        }

        d->progress = new QProgressBar(statusBar());
        d->progress->setMaximumHeight(statusBar()->fontMetrics().height());
        d->progress->setRange(0, 100);
        statusBar()->addPermanentWidget(d->progress);
        d->progress->show();
        d->firstTime = false;
    }
    d->progress->setValue(value);
    qApp->processEvents();
}

QLabel * KoMainWindow::statusBarLabel()
{
    if (!d->statusBarLabel) {
        d->statusBarLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(d->statusBarLabel, 1);
    }
    return d->statusBarLabel;
}

void KoMainWindow::setMaxRecentItems(uint _number)
{
    d->recent->setMaxItems(_number);
}

void KoMainWindow::slotEmailFile()
{
    if (!d->activeView->document())
        return;

    // Subject = Document file name
    // Attachment = The current file
    // Message Body = The current document in HTML export? <-- This may be an option.
    QString theSubject;
    QStringList urls;
    QString fileURL;
    if (d->activeView->document()->url().isEmpty() ||
            d->activeView->document()->isModified()) {
        //Save the file as a temporary file
        bool const tmp_modified = d->activeView->document()->isModified();
        KUrl const tmp_url = d->activeView->document()->url();
        QByteArray const tmp_mimetype = d->activeView->document()->outputMimeType();

        // a little open, close, delete dance to make sure we have a nice filename
        // to use, but won't block windows from creating a new file with this name.
        KTemporaryFile *tmpfile = new KTemporaryFile();
        tmpfile->open();
        QString fileName = tmpfile->fileName();
        tmpfile->close();
        delete tmpfile;

        KUrl u;
        u.setPath(fileName);
        d->activeView->document()->setUrl(u);
        d->activeView->document()->setModified(true);
        d->activeView->document()->setOutputMimeType(d->activeView->document()->nativeFormatMimeType());

        saveDocument(false, true);

        fileURL = fileName;
        theSubject = i18n("Document");
        urls.append(fileURL);

        d->activeView->document()->setUrl(tmp_url);
        d->activeView->document()->setModified(tmp_modified);
        d->activeView->document()->setOutputMimeType(tmp_mimetype);
    } else {
        fileURL = d->activeView->document()->url().url();
        theSubject = i18n("Document - %1", d->activeView->document()->url().fileName(KUrl::ObeyTrailingSlash));
        urls.append(fileURL);
    }

    kDebug(30003) << "(" << fileURL << ")";

    if (!fileURL.isEmpty()) {
        KToolInvocation::invokeMailer(QString(), QString(), QString(), theSubject,
                                      QString(), //body
                                      QString(),
                                      urls); // attachments
    }
}

void KoMainWindow::slotVersionsFile()
{
    if (!d->activeView->document())
        return;
    KoVersionDialog *dlg = new KoVersionDialog(this, d->activeView->document());
    dlg->exec();
    delete dlg;
}

void KoMainWindow::slotReloadFile()
{
    KoDocument* document = d->activeView->document();
    if (!document || document->url().isEmpty())
        return;

    if (document->isModified()) {
        bool ok = KMessageBox::questionYesNo(this,
                                          i18n("You will lose all changes made since your last save\n"
                                               "Do you want to continue?"),
                                          i18n("Warning")) == KMessageBox::Yes;
        if (!ok)
            return;
    }

    KUrl url = document->url();

    if (!document->reload()) {
        KMessageBox::error(this, i18n("Could not reload this document", i18n("Warning")));
    }

    return;

}

void KoMainWindow::slotImportFile()
{
    kDebug(30003) << "slotImportFile()";

    d->isImporting = true;
    slotFileOpen();
    d->isImporting = false;
}

void KoMainWindow::slotExportFile()
{
    kDebug(30003) << "slotExportFile()";

    d->isExporting = true;
    slotFileSaveAs();
    d->isExporting = false;
}

bool KoMainWindow::isImporting() const
{
    return d->isImporting;
}

bool KoMainWindow::isExporting() const
{
    return d->isExporting;
}

QDockWidget* KoMainWindow::createDockWidget(KoDockFactoryBase* factory)
{
    QDockWidget* dockWidget = 0;

    if (!d->dockWidgetsMap.contains(factory->id())) {
        dockWidget = factory->createDockWidget();

        // It is quite possible that a dock factory cannot create the dock; don't
        // do anything in that case.
        if (!dockWidget) return 0;
        d->dockWidgets.push_back(dockWidget);

        KoDockWidgetTitleBar *titleBar = 0;
        // Check if the dock widget is supposed to be collapsable
        if (!dockWidget->titleBarWidget()) {
            titleBar = new KoDockWidgetTitleBar(dockWidget);
            dockWidget->setTitleBarWidget(titleBar);
            titleBar->setCollapsable(factory->isCollapsable());
        }

        dockWidget->setObjectName(factory->id());
        dockWidget->setParent(this);

        if (dockWidget->widget() && dockWidget->widget()->layout())
            dockWidget->widget()->layout()->setContentsMargins(1, 1, 1, 1);

        Qt::DockWidgetArea side = Qt::RightDockWidgetArea;
        bool visible = true;

        switch (factory->defaultDockPosition()) {
        case KoDockFactoryBase::DockTornOff:
            dockWidget->setFloating(true); // position nicely?
            break;
        case KoDockFactoryBase::DockTop:
            side = Qt::TopDockWidgetArea; break;
        case KoDockFactoryBase::DockLeft:
            side = Qt::LeftDockWidgetArea; break;
        case KoDockFactoryBase::DockBottom:
            side = Qt::BottomDockWidgetArea; break;
        case KoDockFactoryBase::DockRight:
            side = Qt::RightDockWidgetArea; break;
        case KoDockFactoryBase::DockMinimized:
        default:
            side = Qt::RightDockWidgetArea;
            visible = false;
        }

        KConfigGroup group = KGlobal::config()->group(d->part->componentData().componentName()).group("DockWidget " + factory->id());
        side = static_cast<Qt::DockWidgetArea>(group.readEntry("DockArea", static_cast<int>(side)));
        if (side == Qt::NoDockWidgetArea) side = Qt::RightDockWidgetArea;

        addDockWidget(side, dockWidget);
        if (dockWidget->features() & QDockWidget::DockWidgetClosable) {
            d->dockWidgetMenu->addAction(dockWidget->toggleViewAction());
            if (!visible)
                dockWidget->hide();
        }

        bool collapsed = factory->defaultCollapsed();
        collapsed = group.readEntry("Collapsed", collapsed);

        if (titleBar && collapsed)
            titleBar->setCollapsed(true);
        d->dockWidgetsMap.insert(factory->id(), dockWidget);
    } else {
        dockWidget = d->dockWidgetsMap[ factory->id()];
    }

    KConfigGroup group(KGlobal::config(), "GUI");
    QFont dockWidgetFont  = KGlobalSettings::generalFont();
    qreal pointSize = group.readEntry("palettefontsize", dockWidgetFont.pointSize() * 0.75);
    pointSize = qMax(pointSize, KGlobalSettings::smallestReadableFont().pointSizeF());
    dockWidgetFont.setPointSizeF(pointSize);
#ifdef Q_WS_MAC
    dockWidget->setAttribute(Qt::WA_MacSmallSize, true);
#endif
    dockWidget->setFont(dockWidgetFont);

    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(forceDockTabFonts()));

    return dockWidget;
}

void KoMainWindow::forceDockTabFonts()
{
    QObjectList chis = children();
    for (int i = 0; i < chis.size(); ++i) {
        if (chis.at(i)->inherits("QTabBar")) {
            QFont dockWidgetFont  = KGlobalSettings::generalFont();
            qreal pointSize = KGlobalSettings::smallestReadableFont().pointSizeF();
            dockWidgetFont.setPointSizeF(pointSize);
            ((QTabBar *)chis.at(i))->setFont(dockWidgetFont);
        }
    }
}

QList<QDockWidget*> KoMainWindow::dockWidgets()
{
    return d->dockWidgetsMap.values();
}

QList<KoCanvasObserverBase*> KoMainWindow::canvasObservers()
{

    QList<KoCanvasObserverBase*> observers;

    foreach(QDockWidget *docker, dockWidgets()) {
        KoCanvasObserverBase *observer = dynamic_cast<KoCanvasObserverBase*>(docker);
        if (observer) {
            observers << observer;
        }
    }
    return observers;
}


KoDockerManager * KoMainWindow::dockerManager() const
{
    return d->dockerManager;
}

void KoMainWindow::toggleDockersVisibility(bool visible)
{
    if (!visible) {
        d->m_dockerStateBeforeHiding = saveState();

        foreach(QObject* widget, children()) {
            if (widget->inherits("QDockWidget")) {
                QDockWidget* dw = static_cast<QDockWidget*>(widget);
                if (dw->isVisible()) {
                    dw->hide();
                }
            }
        }
    }
    else {
        restoreState(d->m_dockerStateBeforeHiding);
    }
}

KRecentFilesAction *KoMainWindow::recentAction() const
{
    return d->recent;
}


void KoMainWindow::slotSetStatusBarText( const QString & text )
{
    statusBar()->showMessage( text );
}

void KoMainWindow::newWindow()
{
    Q_ASSERT((d != 0 && d->activeView && d->part && d->activeView->document()));

    KoMainWindow *mainWindow = d->part->createMainWindow();
    KoView *view = d->part->createView(d->activeView->document());
    mainWindow->addView(view);
    mainWindow->show();
}

void KoMainWindow::createMainwindowGUI()
{
    if ( isHelpMenuEnabled() && !d->m_helpMenu )
        d->m_helpMenu = new KHelpMenu( this, componentData().aboutData(), true, actionCollection() );

    QString f = xmlFile();
    setXMLFile( KStandardDirs::locate( "config", "ui/ui_standards.rc", componentData() ) );
    if ( !f.isEmpty() )
        setXMLFile( f, true );
    else
    {
        QString auto_file( componentData().componentName() + "ui.rc" );
        setXMLFile( auto_file, true );
    }

    guiFactory()->addClient( this );

}

// PartManager

void KoMainWindow::setActiveView(QWidget *widget)
{
    // don't activate twice
    if (!widget || d->m_activeWidget == widget)
        return;

    QWidget *oldActiveWidget = d->m_activeWidget;

    d->m_activeWidget = widget;

    if (oldActiveWidget) {
        QWidget *savedActiveWidget = widget;
        disconnect( oldActiveWidget, SIGNAL(destroyed()), this, SLOT(slotWidgetDestroyed()) );
        d->m_activeWidget = savedActiveWidget;
    }

    if (d->m_activeWidget ) {
        connect( d->m_activeWidget, SIGNAL(destroyed()), this, SLOT(slotWidgetDestroyed()) );
    }
    // Set the new active instance in KGlobal
    KGlobal::setActiveComponent(d->part ? d->part->componentData() : KGlobal::mainComponent());

    KXMLGUIFactory *factory = guiFactory();

    if (d->activeView) {

        factory->removeClient(d->activeView);

        unplugActionList("toolbarlist");
        qDeleteAll(d->toolbarList);
        d->toolbarList.clear();
    }

    if (!d->mainWindowGuiIsBuilt) {
        createMainwindowGUI();
    }

    if (d->part && d->m_activeWidget && d->m_activeWidget->inherits("KoView")) {
        d->activeView = qobject_cast<KoView *>(d->m_activeWidget);
        factory->addClient(d->activeView);

        // Position and show toolbars according to user's preference
        setAutoSaveSettings(d->part->componentData().componentName(), false);

        foreach (QDockWidget *wdg, d->dockWidgets) {
            if ((wdg->features() & QDockWidget::DockWidgetClosable) == 0) {
                wdg->setVisible(true);
            }
        }

        // Create and plug toolbar list for Settings menu
        foreach(QWidget* it, factory->containers("ToolBar")) {
            KToolBar * toolBar = ::qobject_cast<KToolBar *>(it);
            if (toolBar) {
                KToggleAction * act = new KToggleAction(i18n("Show %1 Toolbar", toolBar->windowTitle()), this);
                actionCollection()->addAction(toolBar->objectName().toUtf8(), act);
                act->setCheckedState(KGuiItem(i18n("Hide %1 Toolbar", toolBar->windowTitle())));
                connect(act, SIGNAL(toggled(bool)), this, SLOT(slotToolbarToggled(bool)));
                act->setChecked(!toolBar->isHidden());
                d->toolbarList.append(act);
            } else
                kWarning(30003) << "Toolbar list contains a " << it->metaObject()->className() << " which is not a toolbar!";
        }
        plugActionList("toolbarlist", d->toolbarList);

    }
    else {
        d->activeView = 0;
    }

    if (d->activeView) {
        d->activeView->guiActivateEvent(true);
    }
}

void KoMainWindow::slotWidgetDestroyed()
{
    kDebug(1000);
    if (static_cast<const QWidget *>( sender() ) == d->m_activeWidget)
        setActiveView(0);
}

void KoMainWindow::slotDocumentTitleModified(const QString &caption, bool mod)
{
    updateCaption(caption, mod);
    updateReloadFileAction(d->activeView->document());
    updateVersionsFileAction(d->activeView->document());

}

#include <KoMainWindow.moc>
