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

#include "KisMainWindow.h"

#if defined (Q_OS_MAC) && QT_VERSION < 0x050000
#include "MacSupport.h"
#endif
// qt includes
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QPrintDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QProgressBar>
#include <QSignalMapper>
#include <QTabBar>
#include <QDebug>
#include <QTime>

#include <kdeversion.h>
#if KDE_IS_VERSION(4,6,0)
#include <krecentdirs.h>
#endif
#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kactionmenu.h>
#include <kdebug.h>
#include <kdiroperator.h>
#include <kedittoolbar.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <khelpmenu.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmenu.h>
#include <QMessageBox>
#include <kmimetype.h>
#include <krecentdocument.h>
#include <krecentfilesaction.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <ktemporaryfile.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <ktoolinvocation.h>
#include <kurlcombobox.h>
#include <kurl.h>
#include <kmainwindow.h>
#include <kxmlguiwindow.h>
#include <kxmlguifactory.h>
#include <kxmlguiclient.h>

#include <KoConfig.h>
#include "KoDockFactoryBase.h"
#include "KoDockWidgetTitleBar.h"
#include "KoDocumentInfoDlg.h"
#include "KoDocumentInfo.h"
#include "KoFileDialog.h"
#include <KoIcon.h>
#include <KoPageLayoutDialog.h>
#include <KoPageLayoutWidget.h>
#include <KoToolManager.h>
#include <KoZoomController.h>
#include "KoToolDocker.h"
#include <KoToolBoxFactory.h>
#include <KoDockRegistry.h>
#include <KoPluginLoader.h>
#include <KoColorSpaceEngine.h>

#include "KisView.h"
#include "KisDocument.h"
#include "KisImportExportManager.h"
#include "KisPrintJob.h"
#include "KisPart.h"
#include "KisApplication.h"
#include "kis_factory2.h"

#include "kis_action.h"
#include "kis_canvas_controller.h"
#include "kis_canvas2.h"
#include "KisViewManager.h"
#include "KisDocument.h"
#include "KisView.h"
#include "dialogs/kis_dlg_preferences.h"
#include "kis_config_notifier.h"
#include "kis_canvas_resource_provider.h"
#include "kis_node.h"
#include "kis_image.h"
#include "kis_group_layer.h"
#include "kis_paintop_settings.h"
#include "kis_paintop_box.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "dialogs/kis_about_application.h"
#include "kis_mainwindow_observer.h"
#include "kis_action_manager.h"
#include "thememanager.h"
#include "kis_resource_server_provider.h"

#include "calligraversion.h"

class ToolDockerFactory : public KoDockFactoryBase
{
public:
    ToolDockerFactory() : KoDockFactoryBase() { }

    QString id() const {
        return "sharedtooldocker";
    }

    QDockWidget* createDockWidget() {
        KoToolDocker* dockWidget = new KoToolDocker();
        dockWidget->setTabEnabled(false);
        return dockWidget;
    }

    DockPosition defaultDockPosition() const {
        return DockRight;
    }
};

class KisMainWindow::Private
{
public:
    Private(KisMainWindow *parent)
        : viewManager(0)
        , firstTime(true)
        , windowSizeDirty(false)
        , readOnly(false)
        , isImporting(false)
        , isExporting(false)
        , noCleanup(false)
        , showDocumentInfo(0)
        , saveAction(0)
        , saveActionAs(0)
        , printAction(0)
        , printActionPreview(0)
        , exportPdf(0)
        , closeAll(0)
//        , reloadFile(0)
        , importFile(0)
        , exportFile(0)
        , undo(0)
        , redo(0)
        , newWindow(0)
        , close(0)
        , mdiCascade(0)
        , mdiTile(0)
        , mdiNextWindow(0)
        , mdiPreviousWindow(0)
        , toggleDockers(0)
        , toggleDockerTitleBars(0)
        , dockWidgetMenu(new KActionMenu(i18nc("@action:inmenu", "&Dockers"), parent))
        , windowMenu(new KActionMenu(i18nc("@action:inmenu", "&Window"), parent))
        , documentMenu(new KActionMenu(i18nc("@action:inmenu", "New &View"), parent))
        , helpMenu(0)
        , brushesAndStuff(0)
        , recentFiles(0)
        , toolOptionsDocker(0)
        , deferredClosingEvent(0)
        , themeManager(0)
        , mdiArea(new QMdiArea(parent))
        , activeSubWindow(0)
        , windowMapper(new QSignalMapper(parent))
        , documentMapper(new QSignalMapper(parent))
        , lastExportSpecialOutputFlag(0)
    {
    }

    ~Private() {
        qDeleteAll(toolbarList);
    }

    KisViewManager *viewManager;

    QPointer<KisView> activeView;

    QPointer<QProgressBar> progress;
    QMutex progressMutex;

    QList<QAction *> toolbarList;

    bool firstTime;
    bool windowSizeDirty;
    bool readOnly;
    bool isImporting;
    bool isExporting;
    bool noCleanup;

    KisAction *showDocumentInfo;
    KisAction *saveAction;
    KisAction *saveActionAs;
    KisAction *printAction;
    KisAction *printActionPreview;
    KisAction *exportPdf;
    KisAction *closeAll;
//    KisAction *reloadFile;
    KisAction *importFile;
    KisAction *exportFile;
    KisAction *undo;
    KisAction *redo;
    KisAction *newWindow;
    KisAction *close;
    KisAction *mdiCascade;
    KisAction *mdiTile;
    KisAction *mdiNextWindow;
    KisAction *mdiPreviousWindow;
    KisAction *toggleDockers;
    KisAction *toggleDockerTitleBars;

    KisAction *expandingSpacers[2];

    KActionMenu *dockWidgetMenu;
    KActionMenu *windowMenu;
    KActionMenu *documentMenu;

    KHelpMenu *helpMenu;

    KToolBar *brushesAndStuff;

    KRecentFilesAction *recentFiles;

    KUrl lastExportUrl;

    QMap<QString, QDockWidget *> dockWidgetsMap;
    QMap<QDockWidget *, bool> dockWidgetVisibilityMap;
    QByteArray dockerStateBeforeHiding;
    KoToolDocker *toolOptionsDocker;

    QCloseEvent *deferredClosingEvent;

    Digikam::ThemeManager *themeManager;

    QMdiArea *mdiArea;
    QMdiSubWindow *activeSubWindow;
    QSignalMapper *windowMapper;
    QSignalMapper *documentMapper;

    QByteArray lastExportedFormat;
    int lastExportSpecialOutputFlag;
};

KisMainWindow::KisMainWindow()
    : KXmlGuiWindow()
    , d(new Private(this))
{
    QTime t;
    t.start();

    setComponentData(KisFactory::componentData());
    KGlobal::setActiveComponent(KisFactory::componentData());

    qDebug() << "KisMainWindow() 1" << t.elapsed();

    d->viewManager = new KisViewManager(this, actionCollection());

    qDebug() << "KisMainWindow() 2" << t.elapsed();

    d->themeManager = new Digikam::ThemeManager(this);

    qDebug() << "KisMainWindow() 3" << t.elapsed();

    setAcceptDrops(true);
    setStandardToolBarMenuEnabled(true);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    setDockNestingEnabled(true);

    qApp->setStartDragDistance(25);     // 25 px is a distance that works well for Tablet and Mouse events

#ifdef Q_OS_MAC
#if QT_VERSION < 0x050000
    MacSupport::addFullscreen(this);
#endif
#if QT_VERSION >= 0x050201
    setUnifiedTitleAndToolBarOnMac(true);
#endif
#endif

    qDebug() << "KisMainWindow() 4" << t.elapsed();

    connect(this, SIGNAL(restoringDone()), this, SLOT(forceDockTabFonts()));
    connect(this, SIGNAL(documentSaved()), d->viewManager, SLOT(slotDocumentSaved()));
    connect(this, SIGNAL(themeChanged()), d->viewManager, SLOT(updateIcons()));
    connect(KisPart::instance(), SIGNAL(documentClosed(QString)), SLOT(updateWindowMenu()));
    connect(KisPart::instance(), SIGNAL(documentOpened(QString)), SLOT(updateWindowMenu()));
    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), this, SLOT(configChanged()));

    qDebug() << "KisMainWindow() 5" << t.elapsed();

    actionCollection()->addAssociatedWidget(this);

    QMetaObject::invokeMethod(this, "initializeGeometry", Qt::QueuedConnection);

    ToolDockerFactory toolDockerFactory;
    d->toolOptionsDocker = qobject_cast<KoToolDocker*>(createDockWidget(&toolDockerFactory));

    qDebug() << "KisMainWindow() 6" << t.elapsed();

    KoToolBoxFactory toolBoxFactory;
    createDockWidget(&toolBoxFactory);

    qDebug() << "KisMainWindow() 7" << t.elapsed();

    foreach(const QString & docker, KoDockRegistry::instance()->keys()) {

        qDebug() << "KisMainWindow() 8" << docker << t.elapsed();

        KoDockFactoryBase *factory = KoDockRegistry::instance()->value(docker);
        createDockWidget(factory);
    }

    qDebug() << "KisMainWindow() 9" << t.elapsed();

    foreach (QDockWidget *wdg, dockWidgets()) {
        if ((wdg->features() & QDockWidget::DockWidgetClosable) == 0) {
            wdg->setVisible(true);
        }
    }

    qDebug() << "KisMainWindow() 10" << t.elapsed();

    foreach(KoCanvasObserverBase* observer, canvasObservers()) {
        observer->setObservedCanvas(0);
        KisMainwindowObserver* mainwindowObserver = dynamic_cast<KisMainwindowObserver*>(observer);
        if (mainwindowObserver) {
            mainwindowObserver->setMainWindow(d->viewManager);
        }
    }

    qDebug() << "KisMainWindow() 11" << t.elapsed();

    d->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->mdiArea->setTabPosition(QTabWidget::North);

#if QT_VERSION >= 0x040800
    d->mdiArea->setTabsClosable(true);
#endif /* QT_VERSION >= 0x040800 */

    qDebug() << "KisMainWindow() 12" << t.elapsed();

    setCentralWidget(d->mdiArea);
    d->mdiArea->show();

    qDebug() << "KisMainWindow() 13" << t.elapsed();

    connect(d->mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(subWindowActivated()));
    connect(d->windowMapper, SIGNAL(mapped(QWidget*)), this, SLOT(setActiveSubWindow(QWidget*)));
    connect(d->documentMapper, SIGNAL(mapped(QObject*)), this, SLOT(newView(QObject*)));

    qDebug() << "KisMainWindow() 14" << t.elapsed();

    createActions();

    qDebug() << "KisMainWindow() 15" << t.elapsed();

    setAutoSaveSettings(KisFactory::componentName(), false);

    qDebug() << "KisMainWindow() 16" << t.elapsed();

    KoPluginLoader::instance()->load("Krita/ViewPlugin", "Type == 'Service' and ([X-Krita-Version] == 28)", KoPluginLoader::PluginsConfig(), d->viewManager);

    qDebug() << "KisMainWindow() 17" << t.elapsed();

    subWindowActivated();

    qDebug() << "KisMainWindow() 18" << t.elapsed();

    updateWindowMenu();

    qDebug() << "KisMainWindow() 19" << t.elapsed();

    if (isHelpMenuEnabled() && !d->helpMenu) {
        d->helpMenu = new KHelpMenu( this, KisFactory::aboutData(), false, actionCollection() );
        connect(d->helpMenu, SIGNAL(showAboutApplication()), SLOT(showAboutApplication()));
    }


#if 0
    //check for colliding shortcuts
    QSet<QKeySequence> existingShortcuts;
    foreach(QAction* action, actionCollection()->actions()) {
        if(action->shortcut() == QKeySequence(0)) {
            continue;
        }
        qDebug() << "shortcut " << action->text() << " " << action->shortcut();
        Q_ASSERT(!existingShortcuts.contains(action->shortcut()));
        existingShortcuts.insert(action->shortcut());
    }
#endif

    qDebug() << "KisMainWindow() 20" << t.elapsed();

    configChanged();

    qDebug() << "KisMainWindow() 21" << t.elapsed();

    QString doc;
    QStringList allFiles = KGlobal::dirs()->findAllResources("data", "krita/krita.rc");
    KIS_ASSERT(allFiles.size() > 0); // We need at least one krita.rc file!

    setXMLFile(findMostRecentXMLFile(allFiles, doc));
    setLocalXMLFile(KStandardDirs::locateLocal("data", "krita/krita.rc"));

    qDebug() << "KisMainWindow() 22" << t.elapsed();

    guiFactory()->addClient(this);

    qDebug() << "KisMainWindow() 23" << t.elapsed();

    // Create and plug toolbar list for Settings menu
    QList<QAction *> toolbarList;
    foreach(QWidget* it, guiFactory()->containers("ToolBar")) {
        KToolBar * toolBar = ::qobject_cast<KToolBar *>(it);

        if (toolBar) {
            if (toolBar->objectName() == "BrushesAndStuff") {
                toolBar->setEnabled(false);
            }

            KToggleAction* act = new KToggleAction(i18n("Show %1 Toolbar", toolBar->windowTitle()), this);
            actionCollection()->addAction(toolBar->objectName().toUtf8(), act);
            act->setCheckedState(KGuiItem(i18n("Hide %1 Toolbar", toolBar->windowTitle())));
            connect(act, SIGNAL(toggled(bool)), this, SLOT(slotToolbarToggled(bool)));
            act->setChecked(!toolBar->isHidden());
            toolbarList.append(act);
        } else
            kWarning(30003) << "Toolbar list contains a " << it->metaObject()->className() << " which is not a toolbar!";
    }

    qDebug() << "KisMainWindow() 24" << t.elapsed();

    plugActionList("toolbarlist", toolbarList);
    setToolbarList(toolbarList);

    qDebug() << "KisMainWindow() 25" << t.elapsed();

    applyToolBarLayout();

    d->viewManager->updateGUI();
    d->viewManager->updateIcons();

    QTimer::singleShot(1000, this, SLOT(checkSanity()));

    qDebug() << "KisMainWindow() 26" << t.elapsed();
}

void KisMainWindow::setNoCleanup(bool noCleanup)
{
    d->noCleanup = noCleanup;
}

KisMainWindow::~KisMainWindow()
{
//    foreach(QAction *ac, actionCollection()->actions()) {
//        KAction *action = qobject_cast<KAction*>(ac);
//        if (action) {
//        qDebug() << "<Action"
//                 << "name=" << action->objectName()
//                 << "icon=" << action->icon().name()
//                 << "text="  << action->text().replace("&", "&amp;")
//                 << "whatsThis="  << action->whatsThis()
//                 << "toolTip="  << action->toolTip().replace("<html>", "").replace("</html>", "")
//                 << "iconText="  << action->iconText().replace("&", "&amp;")
//                 << "shortcut="  << action->shortcut(KAction::ActiveShortcut).toString()
//                 << "defaultShortcut="  << action->shortcut(KAction::DefaultShortcut).toString()
//                 << "isCheckable="  << QString((action->isChecked() ? "true" : "false"))
//                 << "statusTip=" << action->statusTip()
//                 << "/>"   ;
//        }
//        else {
//            qDebug() << "Got a QAction:" << ac->objectName();
//        }

//    }

    KConfigGroup cfg(KGlobal::config(), "MainWindow");
    cfg.writeEntry("ko_geometry", saveGeometry().toBase64());
    cfg.writeEntry("ko_windowstate", saveState().toBase64());

    {
        KConfigGroup group(KGlobal::config(), "theme");
        group.writeEntry("Theme", d->themeManager->currentThemeName());
    }

    // The doc and view might still exist (this is the case when closing the window)
    KisPart::instance()->removeMainWindow(this);

    if (d->noCleanup)
        return;

    delete d->viewManager;
    delete d;

}

void KisMainWindow::addView(KisView *view)
{
    //qDebug() << "KisMainWindow::addView" << view;
    if (d->activeView == view) return;

    if (d->activeView) {
        d->activeView->disconnect(this);
    }

    showView(view);
    updateCaption();
    emit restoringDone();

    if (d->activeView) {
        connect(d->activeView, SIGNAL(titleModified(QString,bool)), SLOT(slotDocumentTitleModified(QString,bool)));
    }

}

void KisMainWindow::showView(KisView *imageView)
{
    if (imageView && activeView() != imageView) {
        // XXX: find a better way to initialize this!
        imageView->setViewManager(d->viewManager);
        imageView->canvasBase()->setFavoriteResourceManager(d->viewManager->paintOpBox()->favoriteResourcesManager());
        imageView->slotLoadingFinished();

        QMdiSubWindow *subwin = d->mdiArea->addSubWindow(imageView);
        KisConfig cfg;
        subwin->setOption(QMdiSubWindow::RubberBandMove, cfg.readEntry<int>("mdi_rubberband", cfg.useOpenGL()));
        subwin->setOption(QMdiSubWindow::RubberBandResize, cfg.readEntry<int>("mdi_rubberband", cfg.useOpenGL()));
        subwin->setWindowIcon(qApp->windowIcon());

        if (d->mdiArea->subWindowList().size() == 1) {
            imageView->showMaximized();
        }
        else {
            imageView->show();
        }

        setActiveView(imageView);
        updateWindowMenu();
        updateCaption();
    }
}

void KisMainWindow::slotPreferences()
{
    if (KisDlgPreferences::editPreferences()) {
        KisConfigNotifier::instance()->notifyConfigChanged();

        // XXX: should this be changed for the views in other windows as well?
        foreach(QPointer<KisView> koview, KisPart::instance()->views()) {
            KisViewManager *view = qobject_cast<KisViewManager*>(koview);
            if (view) {
                view->resourceProvider()->resetDisplayProfile(QApplication::desktop()->screenNumber(this));

                // Update the settings for all nodes -- they don't query
                // KisConfig directly because they need the settings during
                // compositing, and they don't connect to the config notifier
                // because nodes are not QObjects (because only one base class
                // can be a QObject).
                KisNode* node = dynamic_cast<KisNode*>(view->image()->rootLayer().data());
                node->updateSettings();
            }

        }

        d->viewManager->showHideScrollbars();
    }
}

void KisMainWindow::updateReloadFileAction(KisDocument */*doc*/)
{
//    d->reloadFile->setEnabled(doc && !doc->url().isEmpty());
}

void KisMainWindow::setReadWrite(bool readwrite)
{
    d->saveAction->setEnabled(readwrite);
    d->importFile->setEnabled(readwrite);
    d->readOnly = !readwrite;
    updateCaption();
}

void KisMainWindow::addRecentURL(const KUrl& url)
{
    kDebug(30003) << "KisMainWindow::addRecentURL url=" << url.prettyUrl();
    // Add entry to recent documents list
    // (call coming from KisDocument because it must work with cmd line, template dlg, file/open, etc.)
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
            d->recentFiles->addUrl(url);
        }
        saveRecentFiles();

    }
}

void KisMainWindow::saveRecentFiles()
{
    // Save list of recent files
    KSharedConfigPtr config = KisFactory::componentData().config();
    d->recentFiles->saveEntries(config->group("RecentFiles"));
    config->sync();

    // Tell all windows to reload their list, after saving
    // Doesn't work multi-process, but it's a start
    foreach(KMainWindow* window, KMainWindow::memberList())
        static_cast<KisMainWindow *>(window)->reloadRecentFileList();
}

void KisMainWindow::reloadRecentFileList()
{
    KSharedConfigPtr config = KisFactory::componentData().config();
    d->recentFiles->loadEntries(config->group("RecentFiles"));
}

void KisMainWindow::updateCaption()
{
    if (!d->mdiArea->activeSubWindow()) {
        updateCaption(QString(), false);
    }
    else {
        QString caption( d->activeView->document()->caption() );
        if (d->readOnly) {
            caption += ' ' + i18n("(write protected)");
        }

        d->activeView->setWindowTitle(caption);

        updateCaption(caption, d->activeView->document()->isModified());

        if (!d->activeView->document()->url().fileName(KUrl::ObeyTrailingSlash).isEmpty())
            d->saveAction->setToolTip(i18n("Save as %1", d->activeView->document()->url().fileName(KUrl::ObeyTrailingSlash)));
        else
            d->saveAction->setToolTip(i18n("Save"));
    }
}

void KisMainWindow::updateCaption(const QString & caption, bool mod)
{
    kDebug(30003) << "KisMainWindow::updateCaption(" << caption << "," << mod << ")";
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


KisView *KisMainWindow::activeView() const
{
    if (d->activeView) {
        return d->activeView;
    }
    return 0;
}

bool KisMainWindow::openDocument(const KUrl & url)
{
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, 0)) {
        QMessageBox::critical(0, i18nc("@title:window", "Krita"), i18n("The file %1 does not exist.", url.url()));
        d->recentFiles->removeUrl(url); //remove the file from the recent-opened-file-list
        saveRecentFiles();
        return false;
    }
    return openDocumentInternal(url);
}

bool KisMainWindow::openDocumentInternal(const KUrl & url, KisDocument *newdoc)
{
    if (!newdoc) {
        newdoc = KisPart::instance()->createDocument();
        KisPart::instance()->addDocument(newdoc);
    }

    d->firstTime = true;
    connect(newdoc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    connect(newdoc, SIGNAL(completed()), this, SLOT(slotLoadCompleted()));
    connect(newdoc, SIGNAL(canceled(const QString &)), this, SLOT(slotLoadCanceled(const QString &)));
    bool openRet = (!isImporting()) ? newdoc->openUrl(url) : newdoc->importDocument(url);
    if (!openRet) {
        delete newdoc;
        return false;
    }
    updateReloadFileAction(newdoc);

    KFileItem file(url, newdoc->mimeType(), KFileItem::Unknown);
    if (!file.isWritable()) {
        setReadWrite(false);
    }
    return true;
}

// Separate from openDocument to handle async loading (remote URLs)
void KisMainWindow::slotLoadCompleted()
{
    KisDocument *newdoc = qobject_cast<KisDocument*>(sender());

    KisView *view = KisPart::instance()->createView(newdoc, resourceManager(), actionCollection(), this);
    addView(view);

    disconnect(newdoc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    disconnect(newdoc, SIGNAL(completed()), this, SLOT(slotLoadCompleted()));
    disconnect(newdoc, SIGNAL(canceled(const QString &)), this, SLOT(slotLoadCanceled(const QString &)));
    emit loadCompleted();
}

void KisMainWindow::slotLoadCanceled(const QString & errMsg)
{
    kDebug(30003) << "KisMainWindow::slotLoadCanceled";
    if (!errMsg.isEmpty())   // empty when canceled by user
        QMessageBox::critical(this, i18nc("@title:window", "Krita"), errMsg);
    // ... can't delete the document, it's the one who emitted the signal...

    KisDocument* doc = qobject_cast<KisDocument*>(sender());
    Q_ASSERT(doc);
    disconnect(doc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    disconnect(doc, SIGNAL(completed()), this, SLOT(slotLoadCompleted()));
    disconnect(doc, SIGNAL(canceled(const QString &)), this, SLOT(slotLoadCanceled(const QString &)));
}

void KisMainWindow::slotSaveCanceled(const QString &errMsg)
{
    kDebug(30003) << "KisMainWindow::slotSaveCanceled";
    if (!errMsg.isEmpty())   // empty when canceled by user
        QMessageBox::critical(this, i18nc("@title:window", "Krita"), errMsg);
    slotSaveCompleted();
}

void KisMainWindow::slotSaveCompleted()
{
    kDebug(30003) << "KisMainWindow::slotSaveCompleted";
    KisDocument* doc = qobject_cast<KisDocument*>(sender());
    Q_ASSERT(doc);
    disconnect(doc, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    disconnect(doc, SIGNAL(completed()), this, SLOT(slotSaveCompleted()));
    disconnect(doc, SIGNAL(canceled(const QString &)), this, SLOT(slotSaveCanceled(const QString &)));

    if (d->deferredClosingEvent) {
        KXmlGuiWindow::closeEvent(d->deferredClosingEvent);
    }
}

bool KisMainWindow::saveDocument(KisDocument *document, bool saveas, bool silent, int specialOutputFlag)
{
    if (!document) {
        return true;
    }

    bool reset_url;

    if (document->url().isEmpty()) {
        reset_url = true;
        saveas = true;
    } else {
        reset_url = false;
    }

    connect(document, SIGNAL(sigProgress(int)), this, SLOT(slotProgress(int)));
    connect(document, SIGNAL(completed()), this, SLOT(slotSaveCompleted()));
    connect(document, SIGNAL(canceled(const QString &)), this, SLOT(slotSaveCanceled(const QString &)));

    KUrl oldURL = document->url();
    QString oldFile = document->localFilePath();

    QByteArray _native_format = document->nativeFormatMimeType();
    QByteArray oldOutputFormat = document->outputMimeType();

    int oldSpecialOutputFlag = document->specialOutputFlag();

    KUrl suggestedURL = document->url();

    QStringList mimeFilter;
    KMimeType::Ptr mime = KMimeType::mimeType(_native_format);
    if (! mime)
        mime = KMimeType::defaultMimeTypePtr();
    if (specialOutputFlag)
        mimeFilter = mime->patterns();
    else
        mimeFilter = KisImportExportManager::mimeFilter(_native_format,
                                                 KisImportExportManager::Export,
                                                 document->extraNativeMimeTypes());


    if (!mimeFilter.contains(oldOutputFormat) && !isExporting()) {
        kDebug(30003) << "KisMainWindow::saveDocument no export filter for" << oldOutputFormat;

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

    if (document->url().isEmpty() || saveas) {
        // if you're just File/Save As'ing to change filter options you
        // don't want to be reminded about overwriting files etc.
        bool justChangingFilterOptions = false;

        KoFileDialog dialog(this, KoFileDialog::SaveFile, "SaveDocument");
        dialog.setCaption(i18n("untitled"));
        if (isExporting() && !d->lastExportUrl.isEmpty()) {
            dialog.setDefaultDir(d->lastExportUrl.toLocalFile(), true);
        }
        else {
            dialog.setDefaultDir(suggestedURL.toLocalFile(), true);
        }
        dialog.setMimeTypeFilters(mimeFilter, KIS_MIME_TYPE);
        KUrl newURL = dialog.url();

        if (newURL.isLocalFile()) {
            QString fn = newURL.toLocalFile();
            if (QFileInfo(fn).completeSuffix().isEmpty()) {
                KMimeType::Ptr mime = KMimeType::mimeType(_native_format);
                fn.append(mime->mainExtension());
                newURL = KUrl::fromPath(fn);
            }
        }

        if (document->documentInfo()->aboutInfo("title") == i18n("Unnamed")) {
            QString fn = newURL.toLocalFile();
            QFileInfo info(fn);
            document->documentInfo()->setAboutInfo("title", info.baseName());
        }

        QByteArray outputFormat = _native_format;

        if (!specialOutputFlag) {
            KMimeType::Ptr mime = KMimeType::findByUrl(newURL);
            QString outputFormatString = mime->name();
            outputFormat = outputFormatString.toLatin1();
        }

        if (!isExporting())
            justChangingFilterOptions = (newURL == document->url()) &&
                    (outputFormat == document->mimeType()) &&
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
            if ( specialOutputFlag== KisDocument::SaveAsDirectoryStore) {
                //qDebug() << "save to directory: " << newURL.url();
            }
            else if (specialOutputFlag == KisDocument::SaveEncrypted) {
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
            if (!justChangingFilterOptions || document->confirmNonNativeSave(isExporting())) {
                if (!document->isNativeFormat(outputFormat))
                    wantToSave = true;
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
                // 2. It is probably not a good idea to change document->mimeType
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
                document->setOutputMimeType(outputFormat, specialOutputFlag);
                if (!isExporting()) {  // Save As
                    ret = document->saveAs(newURL);

                    if (ret) {
                        kDebug(30003) << "Successful Save As!";
                        addRecentURL(newURL);
                        setReadWrite(true);
                    } else {
                        kDebug(30003) << "Failed Save As!";
                        document->setUrl(oldURL);
                        document->setLocalFilePath(oldFile);
                        document->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                    }
                } else { // Export
                    ret = document->exportDocument(newURL);

                    if (ret) {
                        // a few file dialog convenience things
                        d->lastExportUrl = newURL;
                        d->lastExportedFormat = outputFormat;
                        d->lastExportSpecialOutputFlag = specialOutputFlag;
                    }

                    // always restore output format
                    document->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                }

                if (silent) // don't let the document change the window caption
                    document->setTitleModified();
            }   // if (wantToSave)  {
            else
                ret = false;
        }   // if (bOk) {
        else
            ret = false;
    } else { // saving

        bool needConfirm = document->confirmNonNativeSave(false) && !document->isNativeFormat(oldOutputFormat);

        if (!needConfirm ||
                (needConfirm && exportConfirmation(oldOutputFormat /* not so old :) */))
                ) {
            // be sure document has the correct outputMimeType!
            if (isExporting() || document->isModified()) {
                ret = document->save();
            }

            if (!ret) {
                kDebug(30003) << "Failed Save!";
                document->setUrl(oldURL);
                document->setLocalFilePath(oldFile);
            }
        } else
            ret = false;
    }


    if (!ret && reset_url)
        document->resetURL(); //clean the suggested filename as the save dialog was rejected

    updateReloadFileAction(document);
    updateCaption();

    return ret;
}

bool KisMainWindow::exportConfirmation(const QByteArray &/*outputFormat*/)
{
    return true;
}

void KisMainWindow::undo()
{
    if (activeView()) {
        activeView()->undoAction()->trigger();
        d->undo->setText(activeView()->undoAction()->text());
    }
}

void KisMainWindow::redo()
{
    if (activeView()) {
        activeView()->redoAction()->trigger();
        d->redo->setText(activeView()->redoAction()->text());
    }
}

void KisMainWindow::closeEvent(QCloseEvent *e)
{
    d->mdiArea->closeAllSubWindows();

    if(d->activeView && d->activeView->document() && d->activeView->document()->isLoading()) {
        e->setAccepted(false);
        return;
    }

    QList<QMdiSubWindow*> childrenList = d->mdiArea->subWindowList();

    if (childrenList.isEmpty()) {
        d->deferredClosingEvent = e;

        if (!d->dockerStateBeforeHiding.isEmpty()) {
            restoreState(d->dockerStateBeforeHiding);
        }
        statusBar()->setVisible(true);
        menuBar()->setVisible(true);

        saveWindowSettings();

        if (d->noCleanup)
            return;

        foreach(QMdiSubWindow *subwin, d->mdiArea->subWindowList()) {
            KisView *view = dynamic_cast<KisView*>(subwin);
            if (view) {
                KisPart::instance()->removeView(view);
            }
        }

        if (!d->dockWidgetVisibilityMap.isEmpty()) { // re-enable dockers for persistency
            foreach(QDockWidget* dockWidget, d->dockWidgetsMap)
                dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    } else {
        e->setAccepted(false);
    }
}

void KisMainWindow::saveWindowSettings()
{
    KSharedConfigPtr config = KisFactory::componentData().config();

    if (d->windowSizeDirty ) {

        // Save window size into the config file of our componentData
        kDebug(30003) << "KisMainWindow::saveWindowSettings";
        saveWindowSize(config->group("MainWindow"));
        config->sync();
        d->windowSizeDirty = false;
    }

    if (!d->activeView || d->activeView->document()) {

        // Save toolbar position into the config file of the app, under the doc's component name
        KConfigGroup group = KGlobal::config()->group(KisFactory::componentName());
        saveMainWindowSettings(group);

        // Save collapsable state of dock widgets
        for (QMap<QString, QDockWidget*>::const_iterator i = d->dockWidgetsMap.constBegin();
             i != d->dockWidgetsMap.constEnd(); ++i) {
            if (i.value()->widget()) {
                KConfigGroup dockGroup = group.group(QString("DockWidget ") + i.key());
                dockGroup.writeEntry("Collapsed", i.value()->widget()->isHidden());
                dockGroup.writeEntry("Locked", i.value()->property("Locked").toBool());
                dockGroup.writeEntry("DockArea", (int) dockWidgetArea(i.value()));
            }
        }

    }

    KGlobal::config()->sync();
    resetAutoSaveSettings(); // Don't let KMainWindow override the good stuff we wrote down

}

void KisMainWindow::resizeEvent(QResizeEvent * e)
{
    d->windowSizeDirty = true;
    KXmlGuiWindow::resizeEvent(e);
}

void KisMainWindow::setActiveView(KisView* view)
{
    d->activeView = view;
    updateCaption();
    actionCollection()->action("edit_undo")->setText(activeView()->undoAction()->text());
    actionCollection()->action("edit_redo")->setText(activeView()->redoAction()->text());
}

void KisMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->accept();
    }
}

void KisMainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() > 0) {
        foreach(const QUrl &url, event->mimeData()->urls()) {
            openDocument(url);
        }
    }
}

void KisMainWindow::slotFileNew()
{
    KisPart::instance()->showStartUpWidget(this, true /*Always show widget*/);
}

void KisMainWindow::slotFileOpen()
{
    QStringList urls;
    KoFileDialog dialog(this, KoFileDialog::ImportFiles, "OpenDocument");
    dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
    dialog.setMimeTypeFilters(koApp->mimeFilter(KisImportExportManager::Import));
    dialog.setHideNameFilterDetailsOption();
    dialog.setCaption(isImporting() ? i18n("Import Images") : i18n("Open Images"));

    urls = dialog.urls();

    if (urls.isEmpty())
        return;

    foreach(const QString& url, urls) {

        if (!url.isEmpty()) {
            bool res = openDocument(KUrl::fromLocalFile(url));
            if (!res) {
                qWarning() << "Loading" << url << "failed";
            }
        }
    }
}

void KisMainWindow::slotFileOpenRecent(const KUrl & url)
{
    // Create a copy, because the original KUrl in the map of recent files in
    // KRecentFilesAction may get deleted.
    (void) openDocument(KUrl(url));
}

void KisMainWindow::slotFileSave()
{
    if (saveDocument(d->activeView->document()))
        emit documentSaved();
}

void KisMainWindow::slotFileSaveAs()
{
    if (saveDocument(d->activeView->document(), true))
        emit documentSaved();
}

KoCanvasResourceManager *KisMainWindow::resourceManager() const
{
    return d->viewManager->resourceProvider()->resourceManager();
}

int KisMainWindow::viewCount() const
{
    return d->mdiArea->subWindowList().size();
}

bool KisMainWindow::restoreWorkspace(const QByteArray &state)
{
    QByteArray oldState = saveState();
    const bool showTitlebars = KisConfig().showDockerTitleBars();

    // needed because otherwise the layout isn't correctly restored in some situations
    Q_FOREACH (QDockWidget *dock, dockWidgets()) {
        dock->hide();
        dock->titleBarWidget()->setVisible(showTitlebars);
    }

    bool success = KXmlGuiWindow::restoreState(state);

    if (!success) {
        KXmlGuiWindow::restoreState(oldState);
        Q_FOREACH (QDockWidget *dock, dockWidgets()) {
            if (dock->titleBarWidget()) {
                dock->titleBarWidget()->setVisible(showTitlebars || dock->isFloating());
            }
        }
        return false;
    }


    Q_FOREACH (QDockWidget *dock, dockWidgets()) {
        if (dock->titleBarWidget()) {
            const bool isCollapsed = (dock->widget() && dock->widget()->isHidden()) || !dock->widget();
            dock->titleBarWidget()->setVisible(showTitlebars || (dock->isFloating() && isCollapsed));
        }
    }

    return success;
}

KisViewManager *KisMainWindow::viewManager() const
{
    return d->viewManager;
}

void KisMainWindow::slotDocumentInfo()
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

void KisMainWindow::slotFileCloseAll()
{
    foreach(QMdiSubWindow *subwin, d->mdiArea->subWindowList()) {
        if (subwin) {
            subwin->close();
        }
    }
    d->mdiArea->closeAllSubWindows();
    updateCaption();
}

void KisMainWindow::slotFileQuit()
{
    foreach(QPointer<KisMainWindow> mainWin, KisPart::instance()->mainWindows()) {
        if (mainWin != this) {
            mainWin->slotFileCloseAll();
            close();
        }
    }

    slotFileCloseAll();
    close();
}

void KisMainWindow::slotFilePrint()
{
    if (!activeView())
        return;
    KisPrintJob *printJob = activeView()->createPrintJob();
    if (printJob == 0)
        return;
    applyDefaultSettings(printJob->printer());
    QPrintDialog *printDialog = activeView()->createPrintDialog( printJob, this );
    if (printDialog && printDialog->exec() == QDialog::Accepted) {
        printJob->printer().setPageMargins(0.0, 0.0, 0.0, 0.0, QPrinter::Point);
        printJob->printer().setPaperSize(QSizeF(activeView()->image()->width() / (72.0 * activeView()->image()->xRes()),
                                                activeView()->image()->height()/ (72.0 * activeView()->image()->yRes())),
                                         QPrinter::Inch);
        printJob->startPrinting(KisPrintJob::DeleteWhenDone);
    }
    else {
        delete printJob;
    }
    delete printDialog;
}

void KisMainWindow::slotFilePrintPreview()
{
    if (!activeView())
        return;
    KisPrintJob *printJob = activeView()->createPrintJob();
    if (printJob == 0)
        return;

    /* Sets the startPrinting() slot to be blocking.
     The Qt print-preview dialog requires the printing to be completely blocking
     and only return when the full document has been printed.
     By default the KisPrintingDialog is non-blocking and
     multithreading, setting blocking to true will allow it to be used in the preview dialog */
    printJob->setProperty("blocking", true);
    QPrintPreviewDialog *preview = new QPrintPreviewDialog(&printJob->printer(), this);
    printJob->setParent(preview); // will take care of deleting the job
    connect(preview, SIGNAL(paintRequested(QPrinter*)), printJob, SLOT(startPrinting()));
    preview->exec();
    delete preview;
}

KisPrintJob* KisMainWindow::exportToPdf(const QString &pdfFileName)
{
    if (!activeView())
        return 0;
    KoPageLayout pageLayout;
    pageLayout = activeView()->pageLayout();
    return exportToPdf(pageLayout, pdfFileName);
}

KisPrintJob* KisMainWindow::exportToPdf(KoPageLayout pageLayout, QString pdfFileName)
{
    if (!activeView())
        return 0;
    if (!activeView()->document())
        return 0;

    if (pdfFileName.isEmpty()) {
        KConfigGroup group = KGlobal::config()->group("File Dialogs");
        QString defaultDir = group.readEntry("SavePdfDialog");
        if (defaultDir.isEmpty())
            defaultDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
        KUrl startUrl = KUrl(defaultDir);
        KisDocument* pDoc = d->activeView->document();
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

        KoFileDialog dialog(this, KoFileDialog::SaveFile, "SaveDocument");
        dialog.setCaption(i18n("Export as PDF"));
        dialog.setDefaultDir(startUrl.toLocalFile());
        dialog.setMimeTypeFilters(QStringList() << "application/pdf");
        KUrl url = dialog.url();

        pdfFileName = url.toLocalFile();
        if (pdfFileName.isEmpty())
            return 0;
    }

    KisPrintJob *printJob = activeView()->createPrintJob();
    if (printJob == 0)
        return 0;
    if (isHidden()) {
        printJob->setProperty("noprogressdialog", true);
    }

    applyDefaultSettings(printJob->printer());
    // TODO for remote files we have to first save locally and then upload.
    printJob->printer().setOutputFileName(pdfFileName);
    printJob->printer().setDocName(pdfFileName);
    printJob->printer().setColorMode(QPrinter::Color);

    if (pageLayout.format == KoPageFormat::CustomSize) {
        printJob->printer().setPaperSize(QSizeF(pageLayout.width, pageLayout.height), QPrinter::Millimeter);
    } else {
        printJob->printer().setPaperSize(KoPageFormat::printerPageSize(pageLayout.format));
    }

    printJob->printer().setPageMargins(pageLayout.leftMargin, pageLayout.topMargin, pageLayout.rightMargin, pageLayout.bottomMargin, QPrinter::Millimeter);

    switch (pageLayout.orientation) {
    case KoPageFormat::Portrait:
        printJob->printer().setOrientation(QPrinter::Portrait);
        break;
    case KoPageFormat::Landscape:
        printJob->printer().setOrientation(QPrinter::Landscape);
        break;
    }

    //before printing check if the printer can handle printing
    if (!printJob->canPrint()) {
        QMessageBox::critical(this, i18nc("@title:window", "Krita"), i18n("Cannot export to the specified file"));
    }

    printJob->startPrinting(KisPrintJob::DeleteWhenDone);
    return printJob;
}

void KisMainWindow::slotConfigureKeys()
{
    KisPart::instance()->configureShortcuts();
    emit keyBindingsChanged();
}

void KisMainWindow::slotConfigureToolbars()
{
    saveMainWindowSettings(KGlobal::config()->group(KisFactory::componentName()));
    KEditToolBar edit(factory(), this);
    connect(&edit, SIGNAL(newToolBarConfig()), this, SLOT(slotNewToolbarConfig()));
    (void) edit.exec();
    applyToolBarLayout();
}

void KisMainWindow::slotNewToolbarConfig()
{
    applyMainWindowSettings(KGlobal::config()->group(KisFactory::componentName()));

    KXMLGUIFactory *factory = guiFactory();
    Q_UNUSED(factory);

    // Check if there's an active view
    if (!d->activeView)
        return;

    plugActionList("toolbarlist", d->toolbarList);
    applyToolBarLayout();
}

void KisMainWindow::slotToolbarToggled(bool toggle)
{
    //kDebug(30003) <<"KisMainWindow::slotToolbarToggled" << sender()->name() <<" toggle=" << true;
    // The action (sender) and the toolbar have the same name
    KToolBar * bar = toolBar(sender()->objectName());
    if (bar) {
        if (toggle) {
            bar->show();
        }
        else {
            bar->hide();
        }

        if (d->activeView && d->activeView->document()) {
            saveMainWindowSettings(KGlobal::config()->group(KisFactory::componentName()));
        }
    } else
        kWarning(30003) << "slotToolbarToggled : Toolbar " << sender()->objectName() << " not found!";
}

void KisMainWindow::viewFullscreen(bool fullScreen)
{
    if (fullScreen) {
        setWindowState(windowState() | Qt::WindowFullScreen);   // set
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);   // reset
    }
}

void KisMainWindow::slotProgress(int value)
{
    qApp->processEvents();

    if (!d->progressMutex.tryLock()) return;

    kDebug(30003) << "KisMainWindow::slotProgress" << value;
    if (value <= -1 || value >= 100) {
        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = 0;
        }
        d->firstTime = true;
        d->progressMutex.unlock();
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
    if (!d->progress.isNull()) {
        d->progress->setValue(value);
    }
    qApp->processEvents();

    d->progressMutex.unlock();
}

void KisMainWindow::setMaxRecentItems(uint _number)
{
    d->recentFiles->setMaxItems(_number);
}

void KisMainWindow::slotEmailFile()
{
    if (!d->activeView || !d->activeView->document())
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

        saveDocument(d->activeView->document(), false, true);

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

void KisMainWindow::slotReloadFile()
{
    KisDocument* document = d->activeView->document();
    if (!document || document->url().isEmpty())
        return;

    if (document->isModified()) {
        bool ok = QMessageBox::question(this,
                                        i18nc("@title:window", "Krita"),
                                        i18n("You will lose all changes made since your last save\n"
                                             "Do you want to continue?"),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes;
        if (!ok)
            return;
    }

    KUrl url = document->url();

    saveWindowSettings();
    if (!document->reload()) {
        QMessageBox::critical(this, i18nc("@title:window", "Krita"), i18n("Error: Could not reload this document"));
    }

    return;

}

void KisMainWindow::slotImportFile()
{
    kDebug(30003) << "slotImportFile()";

    d->isImporting = true;
    slotFileOpen();
    d->isImporting = false;
}

void KisMainWindow::slotExportFile()
{
    kDebug(30003) << "slotExportFile()";

    d->isExporting = true;
    slotFileSaveAs();
    d->isExporting = false;
}

bool KisMainWindow::isImporting() const
{
    return d->isImporting;
}

bool KisMainWindow::isExporting() const
{
    return d->isExporting;
}

QDockWidget* KisMainWindow::createDockWidget(KoDockFactoryBase* factory)
{
    QDockWidget* dockWidget = 0;

    if (!d->dockWidgetsMap.contains(factory->id())) {
        dockWidget = factory->createDockWidget();

        // It is quite possible that a dock factory cannot create the dock; don't
        // do anything in that case.
        if (!dockWidget) {
            qWarning() << "Could not create docker for" << factory->id();
            return 0;
        }

        KoDockWidgetTitleBar *titleBar = dynamic_cast<KoDockWidgetTitleBar*>(dockWidget->titleBarWidget());
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

        KConfigGroup group = KGlobal::config()->group(KisFactory::componentName()).group("DockWidget " + factory->id());
        side = static_cast<Qt::DockWidgetArea>(group.readEntry("DockArea", static_cast<int>(side)));
        if (side == Qt::NoDockWidgetArea) side = Qt::RightDockWidgetArea;

        addDockWidget(side, dockWidget);
        if (dockWidget->features() & QDockWidget::DockWidgetClosable) {
            d->dockWidgetMenu->addAction(dockWidget->toggleViewAction());
            if (!visible)
                dockWidget->hide();
        }

        bool collapsed = factory->defaultCollapsed();

        bool locked = false;
        group = KGlobal::config()->group(KisFactory::componentName()).group("DockWidget " + factory->id());
        collapsed = group.readEntry("Collapsed", collapsed);
        locked = group.readEntry("Locked", locked);

        //qDebug() << "docker" << factory->id() << dockWidget << "collapsed" << collapsed << "locked" << locked << "titlebar" << titleBar;

        if (titleBar && collapsed)
            titleBar->setCollapsed(true);
        if (titleBar && locked)
            titleBar->setLocked(true);

        d->dockWidgetsMap.insert(factory->id(), dockWidget);
    } else {
        dockWidget = d->dockWidgetsMap[factory->id()];
    }

    KConfigGroup group(KGlobal::config(), "GUI");
    QFont dockWidgetFont  = KGlobalSettings::generalFont();
    qreal pointSize = group.readEntry("palettefontsize", dockWidgetFont.pointSize() * 0.75);
    dockWidgetFont.setPointSizeF(qMax(pointSize, KGlobalSettings::smallestReadableFont().pointSizeF()));
#ifdef Q_OS_MAC
    dockWidget->setAttribute(Qt::WA_MacSmallSize, true);
#endif
    dockWidget->setFont(dockWidgetFont);

    connect(dockWidget, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(forceDockTabFonts()));

    return dockWidget;
}

void KisMainWindow::forceDockTabFonts()
{
    foreach(QObject *child, children()) {
        if (child->inherits("QTabBar")) {
            KConfigGroup group(KGlobal::config(), "GUI");
            QFont dockWidgetFont  = KGlobalSettings::generalFont();
            qreal pointSize = group.readEntry("palettefontsize", dockWidgetFont.pointSize() * 0.75);
            dockWidgetFont.setPointSizeF(qMax(pointSize, KGlobalSettings::smallestReadableFont().pointSizeF()));
            ((QTabBar *)child)->setFont(dockWidgetFont);
        }
    }
}

QList<QDockWidget*> KisMainWindow::dockWidgets()
{
    return d->dockWidgetsMap.values();
}

QList<KoCanvasObserverBase*> KisMainWindow::canvasObservers()
{
    QList<KoCanvasObserverBase*> observers;

    foreach(QDockWidget *docker, dockWidgets()) {
        KoCanvasObserverBase *observer = dynamic_cast<KoCanvasObserverBase*>(docker);
        if (observer) {
            observers << observer;
        }
        else {
            qWarning() << docker << "is not a canvas observer";
        }
    }
    return observers;
}


void KisMainWindow::toggleDockersVisibility(bool visible)
{
    if (!visible) {
        d->dockerStateBeforeHiding = saveState();

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
        restoreState(d->dockerStateBeforeHiding);
    }
}

void KisMainWindow::setToolbarList(QList<QAction *> toolbarList)
{
    qDeleteAll(d->toolbarList);
    d->toolbarList = toolbarList;
}

void KisMainWindow::slotDocumentTitleModified(const QString &caption, bool mod)
{
    updateCaption(caption, mod);
    updateReloadFileAction(d->activeView ? d->activeView->document() : 0);
}


void KisMainWindow::subWindowActivated()
{
    bool enabled = (activeKisView() != 0);

    d->mdiCascade->setEnabled(enabled);
    d->mdiNextWindow->setEnabled(enabled);
    d->mdiPreviousWindow->setEnabled(enabled);
    d->mdiTile->setEnabled(enabled);
    d->close->setEnabled(enabled);
    d->closeAll->setEnabled(enabled);

    setActiveSubWindow(d->mdiArea->activeSubWindow());
    foreach(QToolBar *tb, toolBars()) {
        if (tb->objectName() == "BrushesAndStuff") {
            tb->setEnabled(enabled);
        }
    }

    updateCaption();
    d->viewManager->actionManager()->updateGUI();
}

void KisMainWindow::updateWindowMenu()
{
    KMenu *menu = d->windowMenu->menu();
    menu->clear();

    menu->addAction(d->newWindow);
    menu->addAction(d->documentMenu);

    KMenu *docMenu = d->documentMenu->menu();
    docMenu->clear();

    foreach (QPointer<KisDocument> doc, KisPart::instance()->documents()) {
        if (doc) {
            QAction *action = docMenu->addAction(doc->url().prettyUrl());
            action->setIcon(qApp->windowIcon());
            connect(action, SIGNAL(triggered()), d->documentMapper, SLOT(map()));
            d->documentMapper->setMapping(action, doc);
        }
    }

    menu->addSeparator();
    menu->addAction(d->close);
    menu->addAction(d->closeAll);
    if (d->mdiArea->viewMode() == QMdiArea::SubWindowView) {
        menu->addSeparator();
        menu->addAction(d->mdiTile);
        menu->addAction(d->mdiCascade);
    }
    menu->addSeparator();
    menu->addAction(d->mdiNextWindow);
    menu->addAction(d->mdiPreviousWindow);
    menu->addSeparator();

    QList<QMdiSubWindow *> windows = d->mdiArea->subWindowList();
    for (int i = 0; i < windows.size(); ++i) {
        QPointer<KisView>child = qobject_cast<KisView*>(windows.at(i)->widget());
        if (child) {
            QString text;
            if (i < 9) {
                text = i18n("&%1 %2").arg(i + 1)
                        .arg(child->document()->url().prettyUrl());
            }
            else {
                text = i18n("%1 %2").arg(i + 1)
                        .arg(child->document()->url().prettyUrl());
            }

            QAction *action  = menu->addAction(text);
            action->setIcon(qApp->windowIcon());
            action->setCheckable(true);
            action->setChecked(child == activeKisView());
            connect(action, SIGNAL(triggered()), d->windowMapper, SLOT(map()));
            d->windowMapper->setMapping(action, windows.at(i));
        }
    }

    updateCaption();
}

void KisMainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window) return;
    QMdiSubWindow *subwin = qobject_cast<QMdiSubWindow *>(window);
    //qDebug() << "setActiveSubWindow();" << subwin << d->activeSubWindow;

    if (subwin && subwin != d->activeSubWindow) {
        KisView *view = qobject_cast<KisView *>(subwin->widget());
        //qDebug() << "\t" << view << activeView();
        if (view && view != activeView()) {
            d->viewManager->setCurrentView(view);
            setActiveView(view);
        }
        d->activeSubWindow = subwin;
    }
    updateWindowMenu();
    d->viewManager->actionManager()->updateGUI();
}

void KisMainWindow::configChanged()
{
    KisConfig cfg;
    QMdiArea::ViewMode viewMode = (QMdiArea::ViewMode)cfg.readEntry<int>("mdi_viewmode", (int)QMdiArea::TabbedView);
    d->mdiArea->setViewMode(viewMode);
    foreach(QMdiSubWindow *subwin, d->mdiArea->subWindowList()) {
        subwin->setOption(QMdiSubWindow::RubberBandMove, cfg.readEntry<int>("mdi_rubberband", cfg.useOpenGL()));
        subwin->setOption(QMdiSubWindow::RubberBandResize, cfg.readEntry<int>("mdi_rubberband", cfg.useOpenGL()));
    }

    KConfigGroup group(KGlobal::config(), "theme");
    d->themeManager->setCurrentTheme(group.readEntry("Theme", "Krita dark"));
    d->viewManager->actionManager()->updateGUI();

    QBrush brush(cfg.getMDIBackgroundColor());
    d->mdiArea->setBackground(brush);

    QString backgroundImage = cfg.getMDIBackgroundImage();
    if (backgroundImage != "") {
        QImage image(backgroundImage);
        QBrush brush(image);
        d->mdiArea->setBackground(brush);
    }

    d->mdiArea->update();
}

void KisMainWindow::newView(QObject *document)
{
    KisDocument *doc = qobject_cast<KisDocument*>(document);
    KisView *view = KisPart::instance()->createView(doc, resourceManager(), actionCollection(), this);
    addView(view);
    d->viewManager->actionManager()->updateGUI();
}

void KisMainWindow::newWindow()
{
    KisPart::instance()->createMainWindow()->show();
}

void KisMainWindow::closeCurrentWindow()
{
    d->mdiArea->currentSubWindow()->close();
    d->viewManager->actionManager()->updateGUI();
}

void KisMainWindow::checkSanity()
{
    // print error if the lcms engine is not available
    if (!KoColorSpaceEngineRegistry::instance()->contains("icc")) {
        // need to wait 1 event since exiting here would not work.
        m_errorMessage = i18n("The Calligra LittleCMS color management plugin is not installed. Krita will quit now.");
        m_dieOnError = true;
        QTimer::singleShot(0, this, SLOT(showErrorAndDie()));
        return;
    }

    KisPaintOpPresetResourceServer * rserver = KisResourceServerProvider::instance()->paintOpPresetServer();
    if (rserver->resources().isEmpty()) {
        m_errorMessage = i18n("Krita cannot find any brush presets! Krita will quit now.");
        m_dieOnError = true;
        QTimer::singleShot(0, this, SLOT(showErrorAndDie()));
        return;
    }
}

void KisMainWindow::showErrorAndDie()
{
    QMessageBox::critical(0, i18nc("@title:window", "Installation error"), m_errorMessage);
    if (m_dieOnError) {
        exit(10);
    }
}

void KisMainWindow::showAboutApplication()
{
    KisAboutApplication dlg(KisFactory::aboutData(), this);
    dlg.exec();
}

QPointer<KisView>KisMainWindow::activeKisView()
{
    if (!d->mdiArea) return 0;
    QMdiSubWindow *activeSubWindow = d->mdiArea->activeSubWindow();
    //qDebug() << "activeKisView" << activeSubWindow;
    if (!activeSubWindow) return 0;
    return qobject_cast<KisView*>(activeSubWindow->widget());
}


void KisMainWindow::newOptionWidgets(const QList<QPointer<QWidget> > &optionWidgetList)
{
    d->toolOptionsDocker->setOptionWidgets(optionWidgetList);

    KConfigGroup group(KGlobal::config(), "GUI");
    QFont dockWidgetFont  = KGlobalSettings::generalFont();
    qreal pointSize = group.readEntry("palettefontsize", dockWidgetFont.pointSize() * 0.75);
    pointSize = qMax(pointSize, KGlobalSettings::smallestReadableFont().pointSizeF());
    dockWidgetFont.setPointSizeF(pointSize);

    foreach(QWidget *w, optionWidgetList) {
#ifdef Q_OS_MAC
        w->setAttribute(Qt::WA_MacSmallSize, true);
#endif
        w->setFont(dockWidgetFont);
    }
}

void KisMainWindow::applyDefaultSettings(QPrinter &printer) {

    if (!d->activeView) return;

    QString title = d->activeView->document()->documentInfo()->aboutInfo("title");
    if (title.isEmpty()) {
        title = d->activeView->document()->url().fileName();
        // strip off the native extension (I don't want foobar.kwd.ps when printing into a file)
        KMimeType::Ptr mime = KMimeType::mimeType(d->activeView->document()->outputMimeType());
        if (mime) {
            QString extension = mime->property("X-KDE-NativeExtension").toString();

            if (title.endsWith(extension))
                title.chop(extension.length());
        }
    }

    if (title.isEmpty()) {
        // #139905
        title = i18n("%1 unsaved document (%2)", KisFactory::aboutData()->programName(),
                     KGlobal::locale()->formatDate(QDate::currentDate(), KLocale::ShortDate));
    }
    printer.setDocName(title);
}

void KisMainWindow::createActions()
{
    KisActionManager *actionManager = d->viewManager->actionManager();

    actionManager->createStandardAction(KStandardAction::New, this, SLOT(slotFileNew()));
    actionManager->createStandardAction(KStandardAction::Open, this, SLOT(slotFileOpen()));

    d->recentFiles = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(const KUrl&)), actionCollection());
    connect(d->recentFiles, SIGNAL(recentListCleared()), this, SLOT(saveRecentFiles()));
    KSharedConfigPtr configPtr = KisFactory::componentData().config();
    d->recentFiles->loadEntries(configPtr->group("RecentFiles"));

    d->saveAction = actionManager->createStandardAction(KStandardAction::Save, this, SLOT(slotFileSave()));
    d->saveAction->setActivationFlags(KisAction::ACTIVE_IMAGE);

    d->saveActionAs = actionManager->createStandardAction(KStandardAction::SaveAs, this, SLOT(slotFileSaveAs()));
    d->saveActionAs->setActivationFlags(KisAction::ACTIVE_IMAGE);

    d->printAction = actionManager->createStandardAction(KStandardAction::Print, this, SLOT(slotFilePrint()));
    d->printAction->setActivationFlags(KisAction::ACTIVE_IMAGE);

    d->printActionPreview = actionManager->createStandardAction(KStandardAction::PrintPreview, this, SLOT(slotFilePrintPreview()));
    d->printActionPreview->setActivationFlags(KisAction::ACTIVE_IMAGE);

    d->undo = actionManager->createStandardAction(KStandardAction::Undo, this, SLOT(undo()));
    d->undo ->setActivationFlags(KisAction::ACTIVE_IMAGE);

    d->redo = actionManager->createStandardAction(KStandardAction::Redo, this, SLOT(redo()));
    d->redo->setActivationFlags(KisAction::ACTIVE_IMAGE);

    d->exportPdf  = new KisAction(i18nc("@action:inmenu", "Export as PDF..."));
    d->exportPdf->setActivationFlags(KisAction::ACTIVE_IMAGE);
    d->exportPdf->setIcon(koIcon("application-pdf"));
    actionManager->addAction("file_export_pdf", d->exportPdf);
    connect(d->exportPdf, SIGNAL(triggered()), this, SLOT(exportToPdf()));

    actionManager->createStandardAction(KStandardAction::Quit, this, SLOT(slotFileQuit()));

    d->closeAll = new KisAction(i18nc("@action:inmenu", "Close All"));
    d->closeAll->setActivationFlags(KisAction::ACTIVE_IMAGE);
    d->closeAll->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    actionManager->addAction("file_close_all", d->closeAll);
    connect(d->closeAll, SIGNAL(triggered()), this, SLOT(slotFileCloseAll()));

//    d->reloadFile  = new KisAction(i18nc("@action:inmenu", "Reload"));
//    d->reloadFile->setActivationFlags(KisAction::CURRENT_IMAGE_MODIFIED);
//    actionManager->addAction("file_reload_file", d->reloadFile);
//    connect(d->reloadFile, SIGNAL(triggered(bool)), this, SLOT(slotReloadFile()));

    d->importFile  = new KisAction(koIcon("document-import"), i18nc("@action:inmenu", "Open ex&isting Document as Untitled Document..."));
    actionManager->addAction("file_import_file", d->importFile);
    connect(d->importFile, SIGNAL(triggered(bool)), this, SLOT(slotImportFile()));

    d->exportFile  = new KisAction(koIcon("document-export"), i18nc("@action:inmenu", "E&xport..."));
    d->exportFile->setActivationFlags(KisAction::ACTIVE_IMAGE);
    actionManager->addAction("file_export_file", d->exportFile);
    connect(d->exportFile, SIGNAL(triggered(bool)), this, SLOT(slotExportFile()));

    /* The following entry opens the document information dialog.  Since the action is named so it
        intends to show data this entry should not have a trailing ellipses (...).  */
    d->showDocumentInfo  = new KisAction(koIcon("document-properties"), i18nc("@action:inmenu", "Document Information"));
    d->showDocumentInfo->setActivationFlags(KisAction::ACTIVE_IMAGE);
    actionManager->addAction("file_documentinfo", d->showDocumentInfo);
    connect(d->showDocumentInfo, SIGNAL(triggered(bool)), this, SLOT(slotDocumentInfo()));

    actionManager->createStandardAction(KStandardAction::KeyBindings, this, SLOT(slotConfigureKeys()));
    actionManager->createStandardAction(KStandardAction::ConfigureToolbars, this, SLOT(slotConfigureToolbars()));

    d->themeManager->setThemeMenuAction(new KActionMenu(i18nc("@action:inmenu", "&Themes"), this));
    d->themeManager->registerThemeActions(actionCollection());
    connect(d->themeManager, SIGNAL(signalThemeChanged()), this, SIGNAL(themeChanged()));

    actionManager->createStandardAction(KStandardAction::FullScreen, this, SLOT(viewFullscreen(bool)));

    d->toggleDockers = new KisAction(i18nc("@action:inmenu", "Show Dockers"));
    d->toggleDockers->setCheckable(true);
    d->toggleDockers->setChecked(true);
    actionManager->addAction("view_toggledockers", d->toggleDockers);
    connect(d->toggleDockers, SIGNAL(toggled(bool)), SLOT(toggleDockersVisibility(bool)));

    d->toggleDockerTitleBars = new KisAction(i18nc("@action:inmenu", "Show Docker Titlebars"));
    d->toggleDockerTitleBars->setCheckable(true);
    KisConfig cfg;
    d->toggleDockerTitleBars->setChecked(cfg.showDockerTitleBars());
    actionManager->addAction("view_toggledockertitlebars", d->toggleDockerTitleBars);
    connect(d->toggleDockerTitleBars, SIGNAL(toggled(bool)), SLOT(showDockerTitleBars(bool)));

    actionCollection()->addAction("settings_dockers_menu", d->dockWidgetMenu);
    actionCollection()->addAction("window", d->windowMenu);

    d->mdiCascade = new KisAction(i18nc("@action:inmenu", "Cascade"));
    d->mdiCascade->setActivationFlags(KisAction::MULTIPLE_IMAGES);
    actionManager->addAction("windows_cascade", d->mdiCascade);
    connect(d->mdiCascade, SIGNAL(triggered()), d->mdiArea, SLOT(cascadeSubWindows()));

    d->mdiTile = new KisAction(i18nc("@action:inmenu", "Tile"));
    d->mdiTile->setActivationFlags(KisAction::MULTIPLE_IMAGES);
    actionManager->addAction("windows_tile", d->mdiTile);
    connect(d->mdiTile, SIGNAL(triggered()), d->mdiArea, SLOT(tileSubWindows()));

    d->mdiNextWindow = new KisAction(i18nc("@action:inmenu", "Next"));
    d->mdiNextWindow->setActivationFlags(KisAction::MULTIPLE_IMAGES);
    actionManager->addAction("windows_next", d->mdiNextWindow);
    connect(d->mdiNextWindow, SIGNAL(triggered()), d->mdiArea, SLOT(activateNextSubWindow()));

    d->mdiPreviousWindow = new KisAction(i18nc("@action:inmenu", "Previous"));
    d->mdiPreviousWindow->setActivationFlags(KisAction::MULTIPLE_IMAGES);
    actionCollection()->addAction("windows_previous", d->mdiPreviousWindow);
    connect(d->mdiPreviousWindow, SIGNAL(triggered()), d->mdiArea, SLOT(activatePreviousSubWindow()));

    d->newWindow = new KisAction(koIcon("window-new"), i18nc("@action:inmenu", "&New Window"));
    actionManager->addAction("view_newwindow", d->newWindow);
    connect(d->newWindow, SIGNAL(triggered(bool)), this, SLOT(newWindow()));

    d->close = new KisAction(i18nc("@action:inmenu", "Close"));
    d->close->setActivationFlags(KisAction::ACTIVE_IMAGE);
    connect(d->close, SIGNAL(triggered()), SLOT(closeCurrentWindow()));
    actionManager->addAction("file_close", d->close);

    actionManager->createStandardAction(KStandardAction::Preferences, this, SLOT(slotPreferences()));

    for (int i = 0; i < 2; i++) {
        d->expandingSpacers[i] = new KisAction(i18n("Expanding Spacer"));
        d->expandingSpacers[i]->setDefaultWidget(new QWidget(this));
        d->expandingSpacers[i]->defaultWidget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        actionManager->addAction(QString("expanding_spacer_%1").arg(i), d->expandingSpacers[i]);
    }
}

void KisMainWindow::applyToolBarLayout()
{
    const bool isPlastiqueStyle = style()->objectName() == "plastique";

    Q_FOREACH (KToolBar *toolBar, toolBars()) {
        toolBar->layout()->setSpacing(4);
        if (isPlastiqueStyle) {
            toolBar->setContentsMargins(0, 0, 0, 2);
        }
    }
}

void KisMainWindow::initializeGeometry()
{
    // if the user didn's specify the geometry on the command line (does anyone do that still?),
    // we first figure out some good default size and restore the x,y position. See bug 285804Z.
    KConfigGroup cfg(KGlobal::config(), "MainWindow");
    if (!initialGeometrySet()) {
        QByteArray geom = QByteArray::fromBase64(cfg.readEntry("ko_geometry", QByteArray()));
        if (!restoreGeometry(geom)) {
            const int scnum = QApplication::desktop()->screenNumber(parentWidget());
            QRect desk = QApplication::desktop()->availableGeometry(scnum);
            // if the desktop is virtual then use virtual screen size
            if (QApplication::desktop()->isVirtualDesktop()) {
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
                w = (deskWidth / 3) * 2;
                h = (desk.height() / 3) * 2;
            }
            else {
                w = desk.width();
                h = desk.height();
            }

            x += (desk.width() - w) / 2;
            y += (desk.height() - h) / 2;

            move(x,y);
            setGeometry(geometry().x(), geometry().y(), w, h);
        }
    }
    restoreWorkspace(QByteArray::fromBase64(cfg.readEntry("ko_windowstate", QByteArray())));
}

void KisMainWindow::showDockerTitleBars(bool show)
{
    foreach (QDockWidget *dock, dockWidgets()) {
        if (dock->titleBarWidget()) {
            const bool isCollapsed = (dock->widget() && dock->widget()->isHidden()) || !dock->widget();
            dock->titleBarWidget()->setVisible(show || (dock->isFloating() && isCollapsed));
        }
    }

    KisConfig cfg;
    cfg.setShowDockerTitleBars(show);
}


#include <KisMainWindow.moc>
