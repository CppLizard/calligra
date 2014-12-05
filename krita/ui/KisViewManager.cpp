/*
 *  This file is part of KimageShop^WKrayon^WKrita
 *
 *  Copyright (c) 1999 Matthias Elter  <me@kde.org>
 *                1999 Michael Koch    <koch@kde.org>
 *                1999 Carsten Pfeiffer <pfeiffer@kde.org>
 *                2002 Patrick Julien <freak@codepimps.org>
 *                2003-2011 Boudewijn Rempt <boud@valdyas.org>
 *                2004 Clarence Dang <dang@kde.org>
 *                2011 José Luis Vergara <pentalis@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>

#include "KisViewManager.h"
#include <QPrinter>

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QGridLayout>
#include <QRect>
#include <QWidget>
#include <QToolBar>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QApplication>
#include <QPrintDialog>
#include <QObject>
#include <QByteArray>
#include <QBuffer>
#include <QScrollBar>
#include <QMainWindow>
#include <QPoint>

#include <kactioncollection.h>
#include <kaction.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <KoServiceLocator.h>
#include <kservice.h>
#include <kstandardaction.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <ktoggleaction.h>
#include <kurl.h>
#include <kxmlguifactory.h>

#include <KoCanvasController.h>
#include <KoCompositeOp.h>
#include <KoDockRegistry.h>
#include <KoDockWidgetTitleBar.h>
#include <KoProperties.h>
#include <KoResourceItemChooserSync.h>
#include <KoResourceServerProvider.h>
#include <KoSelection.h>
#include <KoStore.h>
#include <KoToolBoxFactory.h>
#include <KoToolManager.h>
#include <KoToolRegistry.h>
#include <KoViewConverter.h>
#include <KoZoomHandler.h>

#include "canvas/kis_canvas2.h"
#include "canvas/kis_canvas_controller.h"
#include "canvas/kis_grid_manager.h"
#include "canvas/kis_perspective_grid_manager.h"
#include "dialogs/kis_dlg_blacklist_cleanup.h"
#include "input/kis_input_profile_manager.h"
#include "kis_action_manager.h"
#include "kis_canvas_controls_manager.h"
#include "kis_canvas_resource_provider.h"
#include "kis_composite_progress_proxy.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_control_frame.h"
#include "kis_coordinates_converter.h"
#include <KisDockerManager.h>
#include <KisDocumentEntry.h>
#include "KisDocument.h"
#include "kis_factory2.h"
#include "kis_favorite_resource_manager.h"
#include "kis_filter_manager.h"
#include "kis_group_layer.h"
#include <kis_image.h>
#include "kis_image_manager.h"
#include <kis_layer.h>
#include "KisMainWindow.h"
#include "kis_mainwindow_observer.h"
#include "kis_mask_manager.h"
#include "kis_mimedata.h"
#include "kis_mirror_manager.h"
#include "kis_node_commands_adapter.h"
#include "kis_node.h"
#include "kis_node_manager.h"
#include "kis_painting_assistants_manager.h"
#include <kis_paint_layer.h>
#include "kis_paintop_box.h"
#include <kis_paintop_preset.h>
#include "KisPart.h"
#include "KisPrintJob.h"
#include "kis_progress_widget.h"
#include "kis_resource_server_provider.h"
#include "kis_selection.h"
#include "kis_selection_manager.h"
#include "kis_shape_controller.h"
#include "kis_shape_layer.h"
#include <kis_signal_compressor.h>
#include "kis_statusbar.h"
#include <KisTemplateCreateDia.h>
#include <kis_tool_freehand.h>
#include "kis_tooltip_manager.h"
#include <kis_undo_adapter.h>
#include "KisView.h"
#include "kis_zoom_manager.h"
#include "kra/kis_kra_loader.h"
#include "widgets/kis_floating_message.h"

class StatusBarItem
{
public:
    StatusBarItem() // for QValueList
        : m_widget(0),
          m_connected(false),
          m_hidden(false) {}

    StatusBarItem(QWidget * widget, int stretch, bool permanent)
        : m_widget(widget),
          m_stretch(stretch),
          m_permanent(permanent),
          m_connected(false),
          m_hidden(false) {}

    bool operator==(const StatusBarItem& rhs) {
        return m_widget == rhs.m_widget;
    }

    bool operator!=(const StatusBarItem& rhs) {
        return m_widget != rhs.m_widget;
    }

    QWidget * widget() const {
        return m_widget;
    }

    void ensureItemShown(KStatusBar * sb) {
        Q_ASSERT(m_widget);
        if (!m_connected) {
            if (m_permanent)
                sb->addPermanentWidget(m_widget, m_stretch);
            else
                sb->addWidget(m_widget, m_stretch);

            if(!m_hidden)
                m_widget->show();

            m_connected = true;
        }
    }
    void ensureItemHidden(KStatusBar * sb) {
        if (m_connected) {
            m_hidden = m_widget->isHidden();
            sb->removeWidget(m_widget);
            m_widget->hide();
            m_connected = false;
        }
    }
private:
    QWidget * m_widget;
    int m_stretch;
    bool m_permanent;
    bool m_connected;
    bool m_hidden;
};

class BlockingUserInputEventFilter : public QObject
{
    bool eventFilter(QObject *watched, QEvent *event)
    {
        Q_UNUSED(watched);
        if(dynamic_cast<QWheelEvent*>(event)
                || dynamic_cast<QKeyEvent*>(event)
                || dynamic_cast<QMouseEvent*>(event)) {
            return true;
        }
        else {
            return false;
        }
    }
};

class KisViewManager::KisViewManagerPrivate
{

public:

    KisViewManagerPrivate()
        : filterManager(0)
        , statusBar(0)
        , selectionManager(0)
        , controlFrame(0)
        , nodeManager(0)
        , imageManager(0)
        , gridManager(0)
        , perspectiveGridManager(0)
        , paintingAssistantsManager(0)
        , actionManager(0)
        , mainWindow(0)
        , showFloatingMessage(true)
        , currentImageView(0)
        , canvasResourceProvider(0)
        , canvasResourceManager(0)
        , guiUpdateCompressor(0)
        , actionCollection(0)
        , mirrorManager(0)
    {
    }

    ~KisViewManagerPrivate() {
        delete filterManager;
        delete selectionManager;
        delete nodeManager;
        delete imageManager;
        delete gridManager;
        delete perspectiveGridManager;
        delete paintingAssistantsManager;
        delete statusBar;
        delete actionManager;
        delete canvasControlsManager;
        delete canvasResourceProvider;
        delete canvasResourceManager;
        delete mirrorManager;
    }

public:
    KisFilterManager *filterManager;
    KisStatusBar *statusBar;
    KAction *totalRefresh;
    KAction *createTemplate;
    KAction *saveIncremental;
    KAction *saveIncrementalBackup;
    KAction *openResourcesDirectory;
    KAction *rotateCanvasRight;
    KAction *rotateCanvasLeft;
    KToggleAction *wrapAroundAction;
    KisSelectionManager *selectionManager;
    KisControlFrame *controlFrame;
    KisNodeManager *nodeManager;
    KisImageManager *imageManager;
    KisGridManager *gridManager;
    KisCanvasControlsManager *canvasControlsManager;
    KisPerspectiveGridManager * perspectiveGridManager;
    KisPaintingAssistantsManager *paintingAssistantsManager;
    BlockingUserInputEventFilter blockingEventFilter;
    KisActionManager* actionManager;
    QMainWindow* mainWindow;
    QPointer<KisFloatingMessage> savedFloatingMessage;
    bool showFloatingMessage;
    QPointer<KisView> currentImageView;
    KisCanvasResourceProvider* canvasResourceProvider;
    KoCanvasResourceManager* canvasResourceManager;
    QList<StatusBarItem> statusBarItems;
    KisSignalCompressor* guiUpdateCompressor;
    KActionCollection *actionCollection;
    KisMirrorManager *mirrorManager;
};


KisViewManager::KisViewManager(QWidget * parent, KActionCollection *_actionCollection)
    : d(new KisViewManagerPrivate())
{
    d->actionCollection = _actionCollection;
    d->mainWindow = dynamic_cast<QMainWindow*>(parent);

    d->canvasResourceProvider = new KisCanvasResourceProvider(this);
    d->canvasResourceManager = new KoCanvasResourceManager();
    d->canvasResourceProvider->setResourceManager(d->canvasResourceManager);

    d->guiUpdateCompressor = new KisSignalCompressor(30, KisSignalCompressor::POSTPONE, this);
    connect(d->guiUpdateCompressor, SIGNAL(timeout()), this, SLOT(guiUpdateTimeout()));

    createActions();
    createManagers();

    d->controlFrame = new KisControlFrame(this, mainWindow());

    //Check to draw scrollbars after "Canvas only mode" toggle is created.
    this->showHideScrollbars();


    //Workaround, by default has the same shortcut as hide/show dockers
    if (mainWindow()) {

        mainWindow()->setDockNestingEnabled(true);
        actionCollection()->addAction(KStandardAction::KeyBindings, "keybindings", mainWindow()->guiFactory(), SLOT(configureShortcuts()));

        connect(mainWindow(), SIGNAL(documentSaved()), this, SLOT(slotDocumentSaved()));
        KAction *action = dynamic_cast<KAction*>(mainWindow()->actionCollection()->action("view_toggledockers"));
        if (action) {
            action->setShortcut(QKeySequence(), KAction::DefaultShortcut);
            action->setShortcut(QKeySequence(), KAction::ActiveShortcut);
        }

        KoCanvasController *dummy = new KoDummyCanvasController(actionCollection());
        KoToolManager::instance()->registerTools(actionCollection(), dummy);

        KoToolBoxFactory toolBoxFactory;
        QDockWidget* toolbox = mainWindow()->createDockWidget(&toolBoxFactory);
        toolbox->setMinimumWidth(60);

        mainWindow()->dockerManager()->setIcons(false);
    }

    d->statusBar = new KisStatusBar(this);
    QTimer::singleShot(0, this, SLOT(makeStatusBarVisible()));

    connect(d->nodeManager, SIGNAL(sigNodeActivated(KisNodeSP)),
            d->controlFrame->paintopBox(), SLOT(slotCurrentNodeChanged(KisNodeSP)));

    connect(KoToolManager::instance(), SIGNAL(inputDeviceChanged(KoInputDevice)),
            d->controlFrame->paintopBox(), SLOT(slotInputDeviceChanged(KoInputDevice)));

    connect(KoToolManager::instance(), SIGNAL(changedTool(KoCanvasController*,int)),
            d->controlFrame->paintopBox(), SLOT(slotToolChanged(KoCanvasController*,int)));

    connect(d->nodeManager, SIGNAL(sigNodeActivated(KisNodeSP)),
            resourceProvider(), SLOT(slotNodeActivated(KisNodeSP)));

    connect(resourceProvider()->resourceManager(), SIGNAL(canvasResourceChanged(int,QVariant)),
            d->controlFrame->paintopBox(), SLOT(slotCanvasResourceChanged(int,QVariant)));

    loadPlugins();

    KisInputProfileManager::instance()->loadProfiles();

    KisPaintOpPresetResourceServer * rserver = KisResourceServerProvider::instance()->paintOpPresetServer();
    if (rserver->resources().isEmpty()) {
        KMessageBox::error(mainWindow(), i18n("Krita cannot find any brush presets and will close now. Please check your installation.", i18n("Critical Error")));
        exit(0);
    }


    foreach(const QString & docker, KoDockRegistry::instance()->keys()) {
        KoDockFactoryBase *factory = KoDockRegistry::instance()->value(docker);
        if (mainWindow())
            mainWindow()->createDockWidget(factory);
    }
    foreach(KoCanvasObserverBase* observer, mainWindow()->canvasObservers()) {
        KisMainwindowObserver* mainwindowObserver = dynamic_cast<KisMainwindowObserver*>(observer);
        if (mainwindowObserver) {
            mainwindowObserver->setMainWindow(this);
        }
    }

    d->actionManager->updateGUI();

    connect(mainWindow(), SIGNAL(themeChanged()), this, SLOT(updateIcons()));
    updateIcons();
}


KisViewManager::~KisViewManager()
{
    KisConfig cfg;
    if (resourceProvider() && resourceProvider()->currentPreset()) {
        cfg.writeEntry("LastPreset", resourceProvider()->currentPreset()->name());
    }
    cfg.writeEntry("baseLength", KoResourceItemChooserSync::instance()->baseLength());

    if (d->filterManager->isStrokeRunning()) {
        d->filterManager->cancel();
    }

    // The reason for this is to ensure the shortcuts are saved at the right time,
    // and only the right shortcuts. Gemini has two views at all times, and shortcuts
    // must be handled by the desktopview, but if we use the logic as below, we
    // overwrite the desktop view's settings with the sketch view's
    if(qApp->applicationName() == QLatin1String("kritagemini")) {
        KConfigGroup group(KGlobal::config(), "krita/shortcuts");
        foreach(KActionCollection *collection, KActionCollection::allCollections()) {
            const QObject* obj = dynamic_cast<const QObject*>(collection->parentGUIClient());
            if(obj && qobject_cast<const KisViewManager*>(obj) && !obj->objectName().startsWith("view_0"))
                break;
            collection->setConfigGroup("krita/shortcuts");
            collection->writeSettings(&group);
        }
    }
    else {
        KConfigGroup group(KGlobal::config(), "krita/shortcuts");
        foreach(KActionCollection *collection, KActionCollection::allCollections()) {
            collection->setConfigGroup("krita/shortcuts");
            collection->writeSettings(&group);
        }
    }
    delete d;
}

KActionCollection *KisViewManager::actionCollection() const
{
    return d->actionCollection;
}

void KisViewManager::setCurrentView(KisView *view)
{
    bool first = true;
    if (d->currentImageView) {
        first = false;
        KisDocument* doc = d->currentImageView->document();
        if (doc) {
            doc->disconnect(this);
        }
        d->currentImageView->canvasController()->proxyObject->disconnect(d->statusBar);
        d->nodeManager->disconnect(doc->image());
        doc->image()->disconnect(d->controlFrame->paintopBox(), SLOT(slotColorSpaceChanged(const KoColorSpace*)));

        d->rotateCanvasRight->disconnect();
        d->rotateCanvasLeft->disconnect();
        d->wrapAroundAction->disconnect();
        d->currentImageView->canvasController()->disconnect(SIGNAL(toolOptionWidgetsChanged(QList<QPointer<QWidget> >)), mainWindow()->dockerManager());
        resourceProvider()->disconnect(d->currentImageView->canvasBase());
    }

    QPointer<KisView>imageView = qobject_cast<KisView*>(view);

    if (imageView) {

        imageView->setViewManager(this);

        connect(resourceProvider(), SIGNAL(sigDisplayProfileChanged(const KoColorProfile*)), imageView->canvasBase(), SLOT(slotSetDisplayProfile(const KoColorProfile*)));
        resourceProvider()->resetDisplayProfile(QApplication::desktop()->screenNumber(mainWindow()));

        // Wait for the async image to have loaded
        KisDocument* doc = view->document();
        //        connect(canvasController()->proxyObject, SIGNAL(documentMousePositionChanged(QPointF)), d->statusBar, SLOT(documentMousePositionChanged(QPointF)));

        d->currentImageView = imageView;
        imageView->canvasBase()->setSharedResourceManager(d->canvasResourceManager);

        connect(d->nodeManager, SIGNAL(sigNodeActivated(KisNodeSP)), doc->image(), SLOT(requestStrokeEnd()));
        connect(d->rotateCanvasRight, SIGNAL(triggered()), dynamic_cast<KisCanvasController*>(d->currentImageView->canvasController()), SLOT(rotateCanvasRight15()));
        connect(d->rotateCanvasLeft, SIGNAL(triggered()),dynamic_cast<KisCanvasController*>(d->currentImageView->canvasController()), SLOT(rotateCanvasLeft15()));
        connect(d->wrapAroundAction, SIGNAL(toggled(bool)), dynamic_cast<KisCanvasController*>(d->currentImageView->canvasController()), SLOT(slotToggleWrapAroundMode(bool)));
        connect(d->currentImageView->canvasController(), SIGNAL(toolOptionWidgetsChanged(QList<QPointer<QWidget> >)), mainWindow()->dockerManager(), SLOT(newOptionWidgets(QList<QPointer<QWidget> >)));
        connect(d->currentImageView->image(), SIGNAL(sigColorSpaceChanged(const KoColorSpace*)), d->controlFrame->paintopBox(), SLOT(slotColorSpaceChanged(const KoColorSpace*)));
    }

    d->filterManager->setView(imageView);
    d->selectionManager->setView(imageView);
    d->nodeManager->setView(imageView);
    d->imageManager->setView(imageView);
    d->canvasControlsManager->setView(imageView);
    d->actionManager->setView(imageView);
    d->gridManager->setView(imageView);
    d->statusBar->setView(imageView);
    d->paintingAssistantsManager->setView(imageView);
    d->perspectiveGridManager->setView(imageView);
    d->mirrorManager->setView(imageView);

    if (d->currentImageView) {
        d->currentImageView->canvasController()->activate();
        d->currentImageView->canvasController()->setFocus();
    }
    actionManager()->updateGUI();

    // Restore the last used brush preset
    if (first) {
        KisConfig cfg;
        KisPaintOpPresetResourceServer * rserver = KisResourceServerProvider::instance()->paintOpPresetServer();
        QString lastPreset = cfg.readEntry("LastPreset", QString("Basic_tip_default"));
        KisPaintOpPresetSP preset = rserver->resourceByName(lastPreset);
        if (!preset) {
            preset = rserver->resourceByName("Basic_tip_default");
        }

        if (!preset) {
            preset = rserver->resources().first();
        }
        if (preset) {
            paintOpBox()->resourceSelected(preset.data());
        }

    }
}


KoZoomController *KisViewManager::zoomController() const
{
    if (d->currentImageView) {
        return d->currentImageView->zoomController();
    }
    return 0;
}

KisImageWSP KisViewManager::image() const
{
    if (document()) {
        return document()->image();
    }
    return 0;
}

KisCanvasResourceProvider * KisViewManager::resourceProvider()
{
    return d->canvasResourceProvider;
}

KisCanvas2 * KisViewManager::canvasBase() const
{
    if (d && d->currentImageView) {
        return d->currentImageView->canvasBase();
    }
    return 0;
}

QWidget* KisViewManager::canvas() const
{
    if (d && d->currentImageView && d->currentImageView->canvasBase()->canvasWidget()) {
        return d->currentImageView->canvasBase()->canvasWidget();
    }
    return 0;
}

KisStatusBar * KisViewManager::statusBar() const
{
    return d->statusBar;
}

void KisViewManager::addStatusBarItem(QWidget * widget, int stretch, bool permanent)
{
    StatusBarItem item(widget, stretch, permanent);
    KStatusBar * sb = mainWindow()->statusBar();
    if (sb) {
        item.ensureItemShown(sb);
    }
    d->statusBarItems.append(item);
}

void KisViewManager::removeStatusBarItem(QWidget * widget)
{
    KStatusBar *sb = mainWindow()->statusBar();

    int itemCount = d->statusBarItems.count();
    for (int i = itemCount-1; i >= 0; --i) {
        StatusBarItem &sbItem = d->statusBarItems[i];
        if (sbItem.widget() == widget) {
            if (sb) {
                sbItem.ensureItemHidden(sb);
            }
            d->statusBarItems.removeOne(sbItem);
            break;
        }
    }
}

KisPaintopBox* KisViewManager::paintOpBox() const
{
    return d->controlFrame->paintopBox();
}

KoProgressUpdater* KisViewManager::createProgressUpdater(KoProgressUpdater::Mode mode)
{
    return new KisProgressUpdater(d->statusBar->progress(), document()->progressProxy(), mode);
}

KisSelectionManager * KisViewManager::selectionManager()
{
    return d->selectionManager;
}

KisNodeSP KisViewManager::activeNode()
{
    if (d->nodeManager)
        return d->nodeManager->activeNode();
    else
        return 0;
}

KisLayerSP KisViewManager::activeLayer()
{
    if (d->nodeManager)
        return d->nodeManager->activeLayer();
    else
        return 0;
}

KisPaintDeviceSP KisViewManager::activeDevice()
{
    if (d->nodeManager)
        return d->nodeManager->activePaintDevice();
    else
        return 0;
}

KisZoomManager * KisViewManager::zoomManager()
{
    if (d->currentImageView) {
        return d->currentImageView->zoomManager();
    }
    return 0;
}

KisFilterManager * KisViewManager::filterManager()
{
    return d->filterManager;
}

KisImageManager * KisViewManager::imageManager()
{
    return d->imageManager;
}

KisSelectionSP KisViewManager::selection()
{
    if (d->currentImageView) {
        return d->currentImageView->selection();
    }
    return 0;

}

bool KisViewManager::selectionEditable()
{
    KisLayerSP layer = activeLayer();
    if (layer) {
        KoProperties properties;
        QList<KisNodeSP> masks = layer->childNodes(QStringList("KisSelectionMask"), properties);
        if (masks.size() == 1) {
            return masks[0]->isEditable();
        }
    }
    // global selection is always editable
    return true;
}

KisUndoAdapter * KisViewManager::undoAdapter()
{
    if (!document()) return 0;

    KisImageWSP image = document()->image();
    Q_ASSERT(image);

    return image->undoAdapter();
}

void KisViewManager::createActions()
{
    d->saveIncremental = new KAction(i18n("Save Incremental &Version"), this);
    d->saveIncremental->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S));
    actionCollection()->addAction("save_incremental_version", d->saveIncremental);
    connect(d->saveIncremental, SIGNAL(triggered()), this, SLOT(slotSaveIncremental()));

    d->saveIncrementalBackup = new KAction(i18n("Save Incremental Backup"), this);
    d->saveIncrementalBackup->setShortcut(Qt::Key_F4);
    actionCollection()->addAction("save_incremental_backup", d->saveIncrementalBackup);
    connect(d->saveIncrementalBackup, SIGNAL(triggered()), this, SLOT(slotSaveIncrementalBackup()));

    connect(mainWindow(), SIGNAL(documentSaved()), this, SLOT(slotDocumentSaved()));

    d->saveIncremental->setEnabled(false);
    d->saveIncrementalBackup->setEnabled(false);

    KAction *tabletDebugger = new KAction(i18n("Toggle Tablet Debugger"), this);
    actionCollection()->addAction("tablet_debugger", tabletDebugger );
    tabletDebugger->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    connect(tabletDebugger, SIGNAL(triggered()), this, SLOT(toggleTabletLogger()));

    d->createTemplate = new KAction( i18n( "&Create Template From Image..." ), this);
    actionCollection()->addAction("createTemplate", d->createTemplate);
    connect(d->createTemplate, SIGNAL(triggered()), this, SLOT(slotCreateTemplate()));

    d->openResourcesDirectory = new KAction(i18n("Open Resources Folder"), this);
    d->openResourcesDirectory->setToolTip(i18n("Opens a file browser at the location Krita saves resources such as brushes to."));
    d->openResourcesDirectory->setWhatsThis(i18n("Opens a file browser at the location Krita saves resources such as brushes to."));
    actionCollection()->addAction("open_resources_directory", d->openResourcesDirectory);
    connect(d->openResourcesDirectory, SIGNAL(triggered()), SLOT(openResourcesDirectory()));

    d->rotateCanvasRight = new KAction(i18n("Rotate Canvas Right"), this);
    actionCollection()->addAction("rotate_canvas_right", d->rotateCanvasRight);
    d->rotateCanvasRight->setShortcut(QKeySequence("Ctrl+]"));

    d->rotateCanvasLeft = new KAction(i18n("Rotate Canvas Left"), this);
    actionCollection()->addAction("rotate_canvas_left", d->rotateCanvasLeft);
    d->rotateCanvasLeft->setShortcut(QKeySequence("Ctrl+["));

    d->wrapAroundAction = new KToggleAction(i18n("Wrap Around Mode"), this);
    actionCollection()->addAction("wrap_around_mode", d->wrapAroundAction);
    d->wrapAroundAction->setShortcut(QKeySequence(Qt::Key_W));


    KToggleAction *tAction = new KToggleAction(i18n("Show Status Bar"), this);
    tAction->setCheckedState(KGuiItem(i18n("Hide Status Bar")));
    tAction->setChecked(true);
    tAction->setToolTip(i18n("Shows or hides the status bar"));
    actionCollection()->addAction("showStatusBar", tAction);
    connect(tAction, SIGNAL(toggled(bool)), this, SLOT(showStatusBar(bool)));


    tAction = new KToggleAction(i18n("Show Canvas Only"), this);
    tAction->setCheckedState(KGuiItem(i18n("Return to Window")));
    tAction->setToolTip(i18n("Shows just the canvas or the whole window"));
    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(Qt::Key_Tab);
    tAction->setShortcuts(shortcuts);
    tAction->setChecked(false);
    actionCollection()->addAction("view_show_just_the_canvas", tAction);
    connect(tAction, SIGNAL(toggled(bool)), this, SLOT(showJustTheCanvas(bool)));

    //Workaround, by default has the same shortcut as mirrorCanvas
    KAction *a = dynamic_cast<KAction*>(actionCollection()->action("format_italic"));
    if (a) {
        a->setShortcut(QKeySequence(), KAction::DefaultShortcut);
        a->setShortcut(QKeySequence(), KAction::ActiveShortcut);
    }

    a = new KAction(i18n("Cleanup removed files..."), this);
    actionCollection()->addAction("edit_blacklist_cleanup", a);
    connect(a, SIGNAL(triggered()), this, SLOT(slotBlacklistCleanup()));

}



void KisViewManager::createManagers()
{
    // Create the managers for filters, selections, layers etc.
    // XXX: When the currentlayer changes, call updateGUI on all
    // managers

    d->actionManager = new KisActionManager(this);

    d->filterManager = new KisFilterManager(this);
    d->filterManager->setup(actionCollection(), actionManager());

    d->selectionManager = new KisSelectionManager(this);
    d->selectionManager->setup(actionCollection(), actionManager());

    d->nodeManager = new KisNodeManager(this);
    d->nodeManager->setup(actionCollection(), actionManager());

    d->imageManager = new KisImageManager(this);
    d->imageManager->setup(actionCollection(), actionManager());

    d->gridManager = new KisGridManager(this);
    d->gridManager->setup(actionCollection());

    d->perspectiveGridManager = new KisPerspectiveGridManager(this);
    d->perspectiveGridManager->setup(actionCollection());

    d->paintingAssistantsManager = new KisPaintingAssistantsManager(this);
    d->paintingAssistantsManager->setup(actionCollection());

    d->canvasControlsManager = new KisCanvasControlsManager(this);
    d->canvasControlsManager->setup(actionCollection(), actionManager());

    d->mirrorManager = new KisMirrorManager(this);
    d->mirrorManager->setup(actionCollection());

}

void KisViewManager::updateGUI()
{
    d->guiUpdateCompressor->start();
}

void KisViewManager::slotBlacklistCleanup()
{
    KisDlgBlacklistCleanup dialog;
    dialog.exec();
}

void KisViewManager::loadPlugins()
{
    // Load all plugins
    const KService::List offers = KoServiceLocator::instance()->entries("Krita/ViewPlugin");
    KService::List::ConstIterator iter;
    for (iter = offers.constBegin(); iter != offers.constEnd(); ++iter) {
        KService::Ptr service = *iter;
        dbgUI << "Load plugin " << service->name();
        QString error;

        KXMLGUIClient* plugin =
                dynamic_cast<KXMLGUIClient*>(service->createInstance<QObject>(this, QVariantList(), &error));
        if (plugin) {
            mainWindow()->insertChildClient(plugin);
        } else {
            errKrita << "Fail to create an instance for " << service->name() << " " << error;
        }
    }
}


KisNodeManager * KisViewManager::nodeManager() const
{
    return d->nodeManager;
}

KisActionManager* KisViewManager::actionManager() const
{
    return d->actionManager;
}

KisPerspectiveGridManager* KisViewManager::perspectiveGridManager() const
{
    return d->perspectiveGridManager;
}

KisGridManager * KisViewManager::gridManager() const
{
    return d->gridManager;
}

KisPaintingAssistantsManager* KisViewManager::paintingAssistantsManager() const
{
    return d->paintingAssistantsManager;
}

KisDocument *KisViewManager::document() const
{
    if (d->currentImageView && d->currentImageView->document()) {
        return d->currentImageView->document();
    }
    return 0;
}

void KisViewManager::slotCreateTemplate()
{
    if (!document()) return;
    KisTemplateCreateDia::createTemplate("krita_template", ".kra",
                                        KisFactory2::componentData(), document(), mainWindow());
}

QMainWindow* KisViewManager::qtMainWindow() const
{
    if (d->mainWindow)
        return d->mainWindow;

    //Fallback for when we have not yet set the main window.
    QMainWindow* w = qobject_cast<QMainWindow*>(qApp->activeWindow());
    if(w)
        return w;

    return mainWindow();
}

void KisViewManager::setQtMainWindow(QMainWindow* newMainWindow)
{
    d->mainWindow = newMainWindow;
}


void KisViewManager::slotDocumentSaved()
{
    d->saveIncremental->setEnabled(true);
    d->saveIncrementalBackup->setEnabled(true);
}

void KisViewManager::slotSaveIncremental()
{
    if (!document()) return;

    bool foundVersion;
    bool fileAlreadyExists;
    bool isBackup;
    QString version = "000";
    QString newVersion;
    QString letter;
    QString fileName = document()->localFilePath();

    // Find current version filenames
    // v v Regexp to find incremental versions in the filename, taking our backup scheme into account as well
    // Considering our incremental version and backup scheme, format is filename_001~001.ext
    QRegExp regex("_\\d{1,4}[.]|_\\d{1,4}[a-z][.]|_\\d{1,4}[~]|_\\d{1,4}[a-z][~]");
    regex.indexIn(fileName);     //  Perform the search
    QStringList matches = regex.capturedTexts();
    foundVersion = matches.at(0).isEmpty() ? false : true;

    // Ensure compatibility with Save Incremental Backup
    // If this regex is not kept separate, the entire algorithm needs modification;
    // It's simpler to just add this.
    QRegExp regexAux("_\\d{1,4}[~]|_\\d{1,4}[a-z][~]");
    regexAux.indexIn(fileName);     //  Perform the search
    QStringList matchesAux = regexAux.capturedTexts();
    isBackup = matchesAux.at(0).isEmpty() ? false : true;

    // If the filename has a version, prepare it for incrementation
    if (foundVersion) {
        version = matches.at(matches.count() - 1);     //  Look at the last index, we don't care about other matches
        if (version.contains(QRegExp("[a-z]"))) {
            version.chop(1);             //  Trim "."
            letter = version.right(1);   //  Save letter
            version.chop(1);             //  Trim letter
        } else {
            version.chop(1);             //  Trim "."
        }
        version.remove(0, 1);            //  Trim "_"
    } else {
        // ...else, simply add a version to it so the next loop works
        QRegExp regex2("[.][a-z]{2,4}$");  //  Heuristic to find file extension
        regex2.indexIn(fileName);
        QStringList matches2 = regex2.capturedTexts();
        QString extensionPlusVersion = matches2.at(0);
        extensionPlusVersion.prepend(version);
        extensionPlusVersion.prepend("_");
        fileName.replace(regex2, extensionPlusVersion);
    }

    // Prepare the base for new version filename
    int intVersion = version.toInt(0);
    ++intVersion;
    QString baseNewVersion = QString::number(intVersion);
    while (baseNewVersion.length() < version.length()) {
        baseNewVersion.prepend("0");
    }

    // Check if the file exists under the new name and search until options are exhausted (test appending a to z)
    do {
        newVersion = baseNewVersion;
        newVersion.prepend("_");
        if (!letter.isNull()) newVersion.append(letter);
        if (isBackup) {
            newVersion.append("~");
        } else {
            newVersion.append(".");
        }
        fileName.replace(regex, newVersion);
        fileAlreadyExists = KIO::NetAccess::exists(fileName, KIO::NetAccess::DestinationSide, mainWindow());
        if (fileAlreadyExists) {
            if (!letter.isNull()) {
                char letterCh = letter.at(0).toLatin1();
                ++letterCh;
                letter = QString(QChar(letterCh));
            } else {
                letter = 'a';
            }
        }
    } while (fileAlreadyExists && letter != "{");  // x, y, z, {...

    if (letter == "{") {
        KMessageBox::error(mainWindow(), "Alternative names exhausted, try manually saving with a higher number", "Couldn't save incremental version");
        return;
    }
    document()->setSaveInBatchMode(true);
    document()->saveAs(fileName);
    document()->setSaveInBatchMode(false);

    if (mainWindow()) {
        mainWindow()->updateCaption();
    }

}

void KisViewManager::slotSaveIncrementalBackup()
{
    if (!document()) return;

    bool workingOnBackup;
    bool fileAlreadyExists;
    QString version = "000";
    QString newVersion;
    QString letter;
    QString fileName = document()->localFilePath();

    // First, discover if working on a backup file, or a normal file
    QRegExp regex("~\\d{1,4}[.]|~\\d{1,4}[a-z][.]");
    regex.indexIn(fileName);     //  Perform the search
    QStringList matches = regex.capturedTexts();
    workingOnBackup = matches.at(0).isEmpty() ? false : true;

    if (workingOnBackup) {
        // Try to save incremental version (of backup), use letter for alt versions
        version = matches.at(matches.count() - 1);     //  Look at the last index, we don't care about other matches
        if (version.contains(QRegExp("[a-z]"))) {
            version.chop(1);             //  Trim "."
            letter = version.right(1);   //  Save letter
            version.chop(1);             //  Trim letter
        } else {
            version.chop(1);             //  Trim "."
        }
        version.remove(0, 1);            //  Trim "~"

        // Prepare the base for new version filename
        int intVersion = version.toInt(0);
        ++intVersion;
        QString baseNewVersion = QString::number(intVersion);
        QString backupFileName = document()->localFilePath();
        while (baseNewVersion.length() < version.length()) {
            baseNewVersion.prepend("0");
        }

        // Check if the file exists under the new name and search until options are exhausted (test appending a to z)
        do {
            newVersion = baseNewVersion;
            newVersion.prepend("~");
            if (!letter.isNull()) newVersion.append(letter);
            newVersion.append(".");
            backupFileName.replace(regex, newVersion);
            fileAlreadyExists = KIO::NetAccess::exists(backupFileName, KIO::NetAccess::DestinationSide, mainWindow());
            if (fileAlreadyExists) {
                if (!letter.isNull()) {
                    char letterCh = letter.at(0).toLatin1();
                    ++letterCh;
                    letter = QString(QChar(letterCh));
                } else {
                    letter = 'a';
                }
            }
        } while (fileAlreadyExists && letter != "{");  // x, y, z, {...

        if (letter == "{") {
            KMessageBox::error(mainWindow(), "Alternative names exhausted, try manually saving with a higher number", "Couldn't save incremental backup");
            return;
        }
        QFile::copy(fileName, backupFileName);
        document()->saveAs(fileName);

        if (mainWindow()) mainWindow()->updateCaption();
    }
    else { // if NOT working on a backup...
        // Navigate directory searching for latest backup version, ignore letters
        const quint8 HARDCODED_DIGIT_COUNT = 3;
        QString baseNewVersion = "000";
        QString backupFileName = document()->localFilePath();
        QRegExp regex2("[.][a-z]{2,4}$");  //  Heuristic to find file extension
        regex2.indexIn(backupFileName);
        QStringList matches2 = regex2.capturedTexts();
        QString extensionPlusVersion = matches2.at(0);
        extensionPlusVersion.prepend(baseNewVersion);
        extensionPlusVersion.prepend("~");
        backupFileName.replace(regex2, extensionPlusVersion);

        // Save version with 1 number higher than the highest version found ignoring letters
        do {
            newVersion = baseNewVersion;
            newVersion.prepend("~");
            newVersion.append(".");
            backupFileName.replace(regex, newVersion);
            fileAlreadyExists = KIO::NetAccess::exists(backupFileName, KIO::NetAccess::DestinationSide, mainWindow());
            if (fileAlreadyExists) {
                // Prepare the base for new version filename, increment by 1
                int intVersion = baseNewVersion.toInt(0);
                ++intVersion;
                baseNewVersion = QString::number(intVersion);
                while (baseNewVersion.length() < HARDCODED_DIGIT_COUNT) {
                    baseNewVersion.prepend("0");
                }
            }
        } while (fileAlreadyExists);

        // Save both as backup and on current file for interapplication workflow
        document()->setSaveInBatchMode(true);
        QFile::copy(fileName, backupFileName);
        document()->saveAs(fileName);
        document()->setSaveInBatchMode(false);

        if (mainWindow()) mainWindow()->updateCaption();
    }
}

void KisViewManager::disableControls()
{
    // prevents possible crashes, if somebody changes the paintop during dragging by using the mousewheel
    // this is for Bug 250944
    // the solution blocks all wheel, mouse and key event, while dragging with the freehand tool
    // see KisToolFreehand::initPaint() and endPaint()
    d->controlFrame->paintopBox()->installEventFilter(&d->blockingEventFilter);
    foreach(QObject* child, d->controlFrame->paintopBox()->children()) {
        child->installEventFilter(&d->blockingEventFilter);
    }
}

void KisViewManager::enableControls()
{
    d->controlFrame->paintopBox()->removeEventFilter(&d->blockingEventFilter);
    foreach(QObject* child, d->controlFrame->paintopBox()->children()) {
        child->removeEventFilter(&d->blockingEventFilter);
    }
}

void KisViewManager::showStatusBar(bool toggled)
{
    if (d->currentImageView && d->currentImageView->statusBar()) {
        d->currentImageView->statusBar()->setVisible(toggled);
    }
}

#if defined HAVE_OPENGL && defined Q_OS_WIN32
#include <QGLContext>
#endif

void KisViewManager::showJustTheCanvas(bool toggled)
{
    KisConfig cfg;

    /**
 * Workaround for a broken Intel video driver on Windows :(
 * See bug 330040
 */
#if defined HAVE_OPENGL && defined Q_OS_WIN32

    if (toggled && cfg.useOpenGL()) {
        QString renderer((const char*)glGetString(GL_RENDERER));
        bool failingDriver = renderer.startsWith("Intel(R) HD Graphics");

        if (failingDriver &&
                cfg.hideStatusbarFullscreen() &&
                cfg.hideDockersFullscreen() &&
                cfg.hideTitlebarFullscreen() &&
                cfg.hideMenuFullscreen() &&
                cfg.hideToolbarFullscreen() &&
                cfg.hideScrollbarsFullscreen()) {

            int result =
                    KMessageBox::warningYesNo(0,
                                              "Intel(R) HD Graphics video adapters "
                                              "are known to have problems with running "
                                              "Krita in pure canvas only mode. At least "
                                              "one UI control must be shown to "
                                              "workaround it.\n\nShow the scroll bars?",
                                              "Failing video adapter",
                                              KStandardGuiItem::yes(),
                                              KStandardGuiItem::no(),
                                              "messagebox_WorkaroundIntelVideoOnWindows");

            if (result == KMessageBox::Yes) {
                cfg.setHideScrollbarsFullscreen(false);
            }
        }
    }

#endif /* defined HAVE_OPENGL && defined Q_OS_WIN32 */

    KisMainWindow* main = mainWindow();

    if(!main) {
        dbgUI << "Unable to switch to canvas-only mode, main window not found";
        return;
    }

    if (cfg.hideStatusbarFullscreen()) {
        if(main->statusBar() && main->statusBar()->isVisible() == toggled) {
            main->statusBar()->setVisible(!toggled);
        }
    }

    if (cfg.hideDockersFullscreen()) {
        KToggleAction* action = qobject_cast<KToggleAction*>(main->actionCollection()->action("view_toggledockers"));
        if (action && action->isChecked() == toggled) {
            action->setChecked(!toggled);
        }
    }

    if (cfg.hideTitlebarFullscreen()) {
        if(toggled) {
            main->setWindowState( main->windowState() | Qt::WindowFullScreen);
        } else {
            main->setWindowState( main->windowState() & ~Qt::WindowFullScreen);
        }
    }

    if (cfg.hideMenuFullscreen()) {
        if (main->menuBar()->isVisible() == toggled) {
            main->menuBar()->setVisible(!toggled);
        }
    }

    if (cfg.hideToolbarFullscreen()) {
        QList<QToolBar*> toolBars = main->findChildren<QToolBar*>();
        foreach(QToolBar* toolbar, toolBars) {
            if (toolbar->isVisible() == toggled) {
                toolbar->setVisible(!toggled);
            }
        }
    }

    showHideScrollbars();

    if (toggled) {
        // show a fading heads-up display about the shortcut to go back

        showFloatingMessage(i18n("Going into Canvas-Only mode.\nPress %1 to go back.",
                                 actionCollection()->action("view_show_just_the_canvas")->shortcut().toString()), QIcon());
    }
}

void KisViewManager::toggleTabletLogger()
{
    if (d->currentImageView) {
        d->currentImageView->canvasBase()->toggleTabletLogger();
    }
}

void KisViewManager::openResourcesDirectory()
{
    QString dir = KStandardDirs::locateLocal("data", "krita");
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void KisViewManager::updateIcons()
{
#if QT_VERSION >= 0x040700
    QColor background = mainWindow()->palette().background().color();

    bool useDarkIcons = background.value() > 100;
    QString prefix = useDarkIcons ? QString("dark_") : QString("light_");

    QStringList whitelist;
    whitelist << "ToolBox" << "KisLayerBox";

    QStringList blacklistedIcons;
    blacklistedIcons << "editpath" << "artistictext-tool" << "view-choose";

    if (mainWindow()) {
        QList<QDockWidget*> dockers = mainWindow()->dockWidgets();
        foreach(QDockWidget* dock, dockers) {
            kDebug() << "name " << dock->objectName();
            KoDockWidgetTitleBar* titlebar = dynamic_cast<KoDockWidgetTitleBar*>(dock->titleBarWidget());
            if (titlebar) {
                titlebar->updateIcons();
            }
            if (!whitelist.contains(dock->objectName())) {
                continue;
            }

            QObjectList objects;
            objects.append(dock);
            while (!objects.isEmpty()) {
                QObject* object = objects.takeFirst();
                objects.append(object->children());

                QAbstractButton* button = dynamic_cast<QAbstractButton*>(object);
                if (button && !button->icon().name().isEmpty()) {
                    QString name = button->icon().name(); name = name.remove("dark_").remove("light_");

                    if (!blacklistedIcons.contains(name)) {
                        QString iconName = prefix + name;
                        KIcon icon = koIcon(iconName.toLatin1());
                        button->setIcon(icon);
                    }
                }
            }
        }
    }
#endif
}
void KisViewManager::makeStatusBarVisible()
{
    d->mainWindow->statusBar()->setVisible(true);
}

void KisViewManager::guiUpdateTimeout()
{
    d->nodeManager->updateGUI();
    d->selectionManager->updateGUI();
    d->filterManager->updateGUI();
    if (zoomManager()) {
        zoomManager()->updateGUI();
    }
    d->gridManager->updateGUI();
    d->perspectiveGridManager->updateGUI();
    d->actionManager->updateGUI();
}

void KisViewManager::showFloatingMessage(const QString message, const QIcon& icon, int timeout, KisFloatingMessage::Priority priority, int alignment)
{
    if(d->showFloatingMessage && qtMainWindow()) {
        if (d->savedFloatingMessage) {
            d->savedFloatingMessage->tryOverrideMessage(message, icon, timeout, priority, alignment);
        } else {
            d->savedFloatingMessage = new KisFloatingMessage(message, qtMainWindow()->centralWidget(), false, timeout, priority, alignment);
            d->savedFloatingMessage->setShowOverParent(true);
            d->savedFloatingMessage->setIcon(icon);
            d->savedFloatingMessage->showMessage();
        }
    }
#if QT_VERSION >= 0x040700
    emit floatingMessageRequested(message, icon.name());
#endif
}

KisMainWindow *KisViewManager::mainWindow() const
{
    return qobject_cast<KisMainWindow*>(d->mainWindow);
}


void KisViewManager::showHideScrollbars()
{
    if (!d->currentImageView) return;
    if (!d->currentImageView->canvasController()) return;

    KisConfig cfg;
    bool toggled = actionCollection()->action("view_show_just_the_canvas")->isChecked();

    if ( (toggled && cfg.hideScrollbarsFullscreen()) || (!toggled && cfg.hideScrollbars()) ) {
        d->currentImageView->canvasController()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        d->currentImageView->canvasController()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        d->currentImageView->canvasController()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        d->currentImageView->canvasController()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    }
}

void KisViewManager::setShowFloatingMessage(bool show)
{
    d->showFloatingMessage = show;
}


#include "KisViewManager.moc"
