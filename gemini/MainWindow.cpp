/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 * Copyright (C) 2012 KO GmbH. Contact: Boudewijn Rempt <boud@kogmbh.com>
 * Copyright (C) 2013 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

#include "MainWindow.h"
#include "desktopviewproxy.h"
#include "ViewModeSwitchEvent.h"

#include <QApplication>
#include <QResizeEvent>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QGraphicsObject>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QToolButton>
#include <QFileInfo>
#include <QGLWidget>

#include <kcmdlineargs.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kactioncollection.h>
#include <kaboutdata.h>
#include <ktoolbar.h>
#include <kmessagebox.h>
#include <kmenubar.h>
#include <KConfigGroup>

#include <KoCanvasBase.h>
#include <KoToolManager.h>
#include <KoMainWindow.h>
#include <KoGlobal.h>
#include <KoDocumentInfo.h>
#include <KoView.h>
#include <KoPart.h>
#include <part/KWFactory.h>
#include <stage/part/KPrDocument.h>
#include <stage/part/KPrFactory.h>
#include <KoAbstractGradient.h>
#include <KoZoomController.h>

#include "TouchDeclarativeView.h"
#include "RecentFileManager.h"
#include "DocumentManager.h"
#include "QmlGlobalEngine.h"
#include "Settings.h"
#include "Theme.h"
#include "DocumentListModel.h"
#include "Constants.h"
#include "SimpleTouchArea.h"
#include "ToolManager.h"
#include "ParagraphStylesModel.h"
#include "KeyboardModel.h"
#include "ScribbleArea.h"
#include "RecentImageImageProvider.h"

#ifdef Q_OS_WIN
// Slate mode/docked detection stuff
#include <shellapi.h>
#define SM_CONVERTIBLESLATEMODE 0x2003
#define SM_SYSTEMDOCKED         0x2004
#endif

class MainWindow::Private
{
public:
    Private(MainWindow* qq)
        : q(qq)
        , allowClose(true)
        , touchView(0)
        , desktopView(0)
        , currentView(0)
        , settings(0)
        , slateMode(false)
        , docked(false)
        , touchKoView(0)
        , desktopKoView(0)
        , desktopViewProxy(0)
        , forceDesktop(false)
        , forceTouch(false)
        , temporaryFile(false)
        , syncObject(0)
        , toDesktop(0)
        , toTouch(0)
        , switcher(0)
    {
#ifdef Q_OS_WIN
//         slateMode = (GetSystemMetrics(SM_CONVERTIBLESLATEMODE) == 0);
//         docked = (GetSystemMetrics(SM_SYSTEMDOCKED) != 0);
#endif
        centerer = new QTimer(q);
        centerer->setInterval(10);
        centerer->setSingleShot(true);
        connect(centerer, SIGNAL(timeout()), q, SLOT(adjustZoomOnDocumentChangedAndStuff()));
}
    MainWindow* q;
    bool allowClose;
    TouchDeclarativeView* touchView;
    KoMainWindow* desktopView;
    QObject* currentView;
    Settings *settings;

    bool slateMode;
    bool docked;
    QString currentTouchPage;
    KoView* touchKoView;
    KoView* desktopKoView;
    DesktopViewProxy* desktopViewProxy;

    bool forceDesktop;
    bool forceTouch;
    bool temporaryFile;
    ViewModeSynchronisationObject* syncObject;
    QTimer* centerer;

    KAction* toDesktop;
    KAction* toTouch;
    QToolButton* switcher;

    void initTouchView(QObject* parent)
    {
        touchView = new TouchDeclarativeView();
        QmlGlobalEngine::instance()->setEngine(touchView->engine());
        touchView->engine()->addImageProvider(QLatin1String("recentimage"), new RecentImageImageProvider);
        touchView->engine()->rootContext()->setContextProperty("mainWindow", parent);

        settings = new Settings( q );
        DocumentManager::instance()->setSettingsManager( settings );
        touchView->engine()->rootContext()->setContextProperty("Settings", settings);
        touchView->engine()->rootContext()->setContextProperty("Constants", new Constants( q ));
        touchView->engine()->rootContext()->setContextProperty("WORDS_MIME_TYPE", QString(WORDS_MIME_TYPE));
        touchView->engine()->rootContext()->setContextProperty("STAGE_MIME_TYPE", QString(STAGE_MIME_TYPE));

#ifdef Q_OS_WIN
        QDir appdir(qApp->applicationDirPath());

        // Corrects for mismatched case errors in path (qtdeclarative fails to load)
        wchar_t buffer[1024];
        QString absolute = appdir.absolutePath();
        DWORD rv = ::GetShortPathName((wchar_t*)absolute.utf16(), buffer, 1024);
        rv = ::GetLongPathName(buffer, buffer, 1024);
        QString correctedPath((QChar *)buffer);
        appdir.setPath(correctedPath);

        // for now, the app in bin/ and we still use the env.bat script
        appdir.cdUp();

        touchView->engine()->addImportPath(appdir.canonicalPath() + "/lib/calligra/imports");
        touchView->engine()->addImportPath(appdir.canonicalPath() + "/lib64/calligra/imports");
        QString mainqml = appdir.canonicalPath() + "/share/apps/calligragemini/calligragemini.qml";
#else
        touchView->engine()->addImportPath(KGlobal::dirs()->findDirs("lib", "calligra/imports").value(0));
        QString mainqml = KGlobal::dirs()->findResource("data", "calligragemini/calligragemini.qml");
#endif

        Q_ASSERT(QFile::exists(mainqml));
        if (!QFile::exists(mainqml)) {
            QMessageBox::warning(0, "No QML found", mainqml + " doesn't exist.");
        }
        QFileInfo fi(mainqml);

        touchView->setSource(QUrl::fromLocalFile(fi.canonicalFilePath()));
        touchView->setResizeMode( QDeclarativeView::SizeRootObjectToView );

        toDesktop = new KAction(q);
        toDesktop->setEnabled(true);
        toDesktop->setText(tr("Switch to Desktop"));
        // useful for monkey-testing to crash...
        //toDesktop->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_D);
        //q->addAction(toDesktop);
        //connect(toDesktop, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), q, SLOT(switchDesktopForced()));
        connect(toDesktop, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), q, SLOT(switchToDesktop()));
        touchView->engine()->rootContext()->setContextProperty("switchToDesktopAction", toDesktop);
    }

    void initDesktopView()
    {
        // Tell the iconloader about share/apps/calligra/icons
        KIconLoader::global()->addAppDir("calligra");
        // Initialize all Calligra directories etc.
        KoGlobal::initialize();

        // The default theme is not what we want for Gemini
        KConfigGroup group(KGlobal::config(), "theme");
        if(group.readEntry("Theme", "no-theme-is-set") == QLatin1String("no-theme-is-set")) {
            group.writeEntry("Theme", "Krita-dark");
        }

        if(settings->currentFileClass() == WORDS_MIME_TYPE)
            desktopView = new KoMainWindow(WORDS_MIME_TYPE, KWFactory::componentData());
        else if(settings->currentFileClass() == STAGE_MIME_TYPE)
            desktopView = new KoMainWindow(STAGE_MIME_TYPE, KPrFactory::componentData());
        else {
            desktopView = 0;
            qDebug() << "Big trouble, things gonna break. desktopView is not created." << settings->currentFileClass();
            return;
        }

        toTouch = new KAction(desktopView);
        toTouch->setEnabled(false);
        toTouch->setText(tr("Switch to Touch"));
        toTouch->setIcon(QIcon::fromTheme("system-reboot"));
        toTouch->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
        //connect(toTouch, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), q, SLOT(switchTouchForced()));
        connect(toTouch, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), q, SLOT(switchToTouch()));
        desktopView->actionCollection()->addAction("SwitchToTouchView", toTouch);
        switcher = new QToolButton();
        switcher->setEnabled(false);
        switcher->setText(tr("Switch to Touch"));
        switcher->setIcon(QIcon::fromTheme("system-reboot"));
        switcher->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        //connect(switcher, SIGNAL(clicked(bool)), q, SLOT(switchDesktopForced()));
        connect(switcher, SIGNAL(clicked(bool)), q, SLOT(switchToTouch()));
        desktopView->menuBar()->setCornerWidget(switcher);

        // DesktopViewProxy connects itself up to everything appropriate on construction,
        // and destroys itself again when the view is removed
        desktopViewProxy = new DesktopViewProxy(q, desktopView);
        connect(desktopViewProxy, SIGNAL(documentSaved()), q, SIGNAL(documentSaved()));
        connect(desktopViewProxy, SIGNAL(documentSaved()), q, SLOT(resetWindowTitle()));
    }

    void notifySlateModeChange();
    void notifyDockingModeChange();
    bool queryClose();
};

MainWindow::MainWindow(QStringList fileNames, QWidget* parent, Qt::WindowFlags flags )
    : QMainWindow( parent, flags ), d( new Private(this) )
{
    qmlRegisterType<Theme>("org.calligra", 1, 0, "Theme");
    qmlRegisterType<DocumentListModel>("org.calligra", 1, 0, "DocumentListModel");
    qmlRegisterType<SimpleTouchArea>("org.calligra", 1, 0, "SimpleTouchArea");
    qmlRegisterType<ToolManager>("org.calligra", 1, 0, "ToolManager");
    qmlRegisterType<ParagraphStylesModel>("org.calligra", 1, 0, "ParagraphStylesModel");
    qmlRegisterType<KeyboardModel>("org.calligra", 1, 0, "KeyboardModel");
    qmlRegisterType<ScribbleArea>("org.calligra", 1, 0, "ScribbleArea");

    qApp->setActiveWindow( this );

    setWindowTitle(i18n("Calligra Gemini"));
    setWindowIcon(KIcon("calligragemini"));//gemini"));

    foreach(QString fileName, fileNames) {
        DocumentManager::instance()->recentFileManager()->addRecent( QDir::current().absoluteFilePath( fileName ) );
    }

    connect(DocumentManager::instance(), SIGNAL(documentChanged()), SLOT(documentChanged()));
    connect(DocumentManager::instance(), SIGNAL(documentChanged()), SLOT(resetWindowTitle()));
    connect(DocumentManager::instance(), SIGNAL(documentSaved()), SLOT(resetWindowTitle()));

    d->initTouchView(this);

    // Set the initial view to touch... because reasons.
    // Really, this allows us to show the pleasant welcome screen from Touch
    switchToTouch();

    if(!fileNames.isEmpty()) {
        //It feels a little hacky, but call a QML function to open files.
        //This saves a lot of hassle required to change state for loading dialogs etc.
        QMetaObject::invokeMethod(d->touchView->rootObject(), "openFile", Q_ARG(QVariant, fileNames.at(0)));
    }
}

void MainWindow::resetWindowTitle()
{
    KUrl url(DocumentManager::instance()->settingsManager()->currentFile());
    QString fileName = url.fileName();
    if(url.protocol() == "temp")
        fileName = i18n("Untitled");
    setWindowTitle(QString("%1 - %2").arg(fileName).arg(i18n("Calligra Gemini")));
}

void MainWindow::switchDesktopForced()
{
    if (d->slateMode)
        d->forceDesktop = true;
    d->forceTouch = false;
}

void MainWindow::switchTouchForced()
{
    if (!d->slateMode)
        d->forceTouch = true;
    d->forceDesktop = false;
}

void MainWindow::switchToTouch()
{
    QTime timer;
    timer.start();

    if (d->toTouch)
    {
        d->toTouch->setEnabled(false);
        d->switcher->setEnabled(false);
    }

    d->syncObject = new ViewModeSynchronisationObject;
    KoView* view = 0;

    if (d->desktopView && centralWidget() == d->desktopView) {
        view = d->desktopView->rootView();

        //Notify the view we are switching away from that we are about to switch away from it
        //giving it the possibility to set up the synchronisation object.
        ViewModeSwitchEvent aboutToSwitchEvent(ViewModeSwitchEvent::AboutToSwitchViewModeEvent, view, d->touchView, d->syncObject);
        QApplication::sendEvent(view, &aboutToSwitchEvent);

        d->desktopView->setParent(0);
    }

    setCentralWidget(d->touchView);
    emit switchedToTouch();

    if (d->slateMode) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        if (d->syncObject->initialized)
            QTimer::singleShot(50, this, SLOT(touchChange()));
    }
    else
        QTimer::singleShot(50, this, SLOT(touchChange()));

    //qDebug() << "milliseconds to switch to touch:" << timer.elapsed();
}

void MainWindow::touchChange()
{
    if (centralWidget() != d->touchView || !d->syncObject)
        return;

    if (d->desktopView)
    {
        //if (/*!d->touchKoView ||*/ !d->touchView->canvasWidget())
        //{
        //    QTimer::singleShot(100, this, SLOT(touchChange()));
        //    return;
        //}
        qApp->processEvents();
        KoView* view = d->desktopView->rootView();
        //Notify the new view that we just switched to it, passing our synchronisation object
        //so it can use those values to sync with the old view.
        ViewModeSwitchEvent switchedEvent(ViewModeSwitchEvent::SwitchedToTouchModeEvent, view, d->touchView, d->syncObject);
        QApplication::sendEvent(d->touchView, &switchedEvent);
        d->syncObject = 0;
        qApp->processEvents();
    }
    if (d->toDesktop)
    {
        qApp->processEvents();
        d->toDesktop->setEnabled(true);
    }
}

void MainWindow::switchToDesktop(bool justLoaded)
{
    QTime timer;
    timer.start();

    if (d->toDesktop)
        d->toDesktop->setEnabled(false);

    ViewModeSynchronisationObject* syncObject = new ViewModeSynchronisationObject;

    KoView* view = 0;
    if (d->desktopView) {
        view = d->desktopView->rootView();
    }

    //Notify the view we are switching away from that we are about to switch away from it
    //giving it the possibility to set up the synchronisation object.
    ViewModeSwitchEvent aboutToSwitchEvent(ViewModeSwitchEvent::AboutToSwitchViewModeEvent, d->touchView, view, syncObject);
    QApplication::sendEvent(d->touchView, &aboutToSwitchEvent);
    qApp->processEvents();

    if (d->currentTouchPage == "MainPage")
    {
        d->touchView->setParent(0);
        setCentralWidget(d->desktopView);
    }

    setWindowState(windowState() & ~Qt::WindowFullScreen);

    if (view) {
        //Notify the new view that we just switched to it, passing our synchronisation object
        //so it can use those values to sync with the old view.
        ViewModeSwitchEvent switchedEvent(ViewModeSwitchEvent::SwitchedToDesktopModeEvent, d->touchView, view, syncObject);
        QApplication::sendEvent(view, &switchedEvent);
    }

//    if (d->toTouch && !justLoaded)
//    {
        qApp->processEvents();
        d->toTouch->setEnabled(true);
        d->switcher->setEnabled(true);
//    }

    //qDebug() << "milliseconds to switch to desktop:" << timer.elapsed();
}

void MainWindow::adjustZoomOnDocumentChangedAndStuff()
{
    if (d->desktopView && centralWidget() == d->desktopView) {
        KoView* view = d->desktopView->rootView();
        // We have to set the focus on the view here, otherwise the toolmanager is unaware of which
        // canvas should be handled.
        d->desktopView->rootView()->setFocus();
        QPoint center = view->rect().center();
        //view->canvasControllerWidget()->zoomRelativeToPoint(center, 0.9);
        qApp->processEvents();
        d->toTouch->setEnabled(true);
        d->switcher->setEnabled(true);
    }
    else if (d->touchKoView && centralWidget() == d->touchView) {
        qApp->processEvents();
        d->touchKoView->zoomController()->setZoom(KoZoomMode::ZOOM_PAGE, 1.0);
        qApp->processEvents();
        QPoint center = d->touchKoView->rect().center();
        //d->touchKoView->canvasControllerWidget()->zoomRelativeToPoint(center, 0.9);
        qApp->processEvents();
        d->toDesktop->setEnabled(true);
    }
    KoToolManager::instance()->switchToolRequested("TextToolFactory_ID");
}

void MainWindow::setDocAndPart(QObject* document, QObject* part)
{
    DocumentManager::instance()->setDocAndPart(qobject_cast<KoDocument*>(document), qobject_cast<KoPart*>(part));
}

void MainWindow::documentChanged()
{
    if (d->desktopView) {
        d->desktopView->deleteLater();
        d->desktopView = 0;
    }
    d->initDesktopView();
    d->desktopView->setRootDocument(DocumentManager::instance()->document(), DocumentManager::instance()->part(), false);
    qApp->processEvents();
    d->desktopKoView = d->desktopView->rootView();
//    d->desktopKoView->setQtMainWindow(d->desktopView);
//    connect(d->desktopKoView, SIGNAL(sigLoadingFinished()), d->centerer, SLOT(start()));
//    connect(d->desktopKoView, SIGNAL(sigSavingFinished()), this, SLOT(resetWindowTitle()));
//    KWView* wordsview = qobject_cast<KWView*>(d->desktopView->rootView());
//    if(wordsview) {
//        connect(wordsview->canvasBase()->resourceManager(), SIGNAL(canvasResourceChanged(int, const QVariant&)),
//                this, SLOT(resourceChanged(int, const QVariant&)));
//    }
    if (!d->forceTouch && !d->slateMode)
        switchToDesktop(true);
}

bool MainWindow::allowClose() const
{
    return d->allowClose;
}

void MainWindow::setAllowClose(bool allow)
{
    d->allowClose = allow;
}

bool MainWindow::slateMode() const
{
    return d->slateMode;
}

QString MainWindow::currentTouchPage() const
{
    return d->currentTouchPage;
}

void MainWindow::setCurrentTouchPage(QString newPage)
{
    d->currentTouchPage = newPage;
    emit currentTouchPageChanged();

    if (newPage == "MainPage")
    {
        if (!d->forceTouch && !d->slateMode)
        {
            // Just loaded to desktop, do nothing
        }
        else
        {
            //QTimer::singleShot(3000, this, SLOT(adjustZoomOnDocumentChangedAndStuff()));
        }
    }
}

bool MainWindow::temporaryFile() const
{
    return d->temporaryFile;
}

void MainWindow::setTemporaryFile(bool newValue)
{
    d->temporaryFile = newValue;
    emit temporaryFileChanged();
}

void MainWindow::resourceChanged(int key, const QVariant& v)
{
    if(centralWidget() == d->touchView)
        return;
}

void MainWindow::resourceChangedTouch(int key, const QVariant& v)
{
    if(centralWidget() == d->desktopView)
        return;
}

void MainWindow::minimize()
{
    setWindowState(windowState() ^ Qt::WindowMinimized);
}

void MainWindow::closeWindow()
{
    d->desktopView->setNoCleanup(true);
    //For some reason, close() does not work even if setAllowClose(true) was called just before this method.
    //So instead just completely quit the application, since we are using a single window anyway.
    DocumentManager::instance()->closeDocument();
    DocumentManager::instance()->part()->deleteLater();
    qApp->processEvents();
    QApplication::instance()->quit();
}

bool MainWindow::Private::queryClose()
{
    desktopView->setNoCleanup(true);
    if (DocumentManager::instance()->document() == 0)
        return true;

    // main doc + internally stored child documents
    if (DocumentManager::instance()->document()->isModified()) {
        QString name;
        if (DocumentManager::instance()->document()->documentInfo()) {
            name = DocumentManager::instance()->document()->documentInfo()->aboutInfo("title");
        }
        if (name.isEmpty())
            name = DocumentManager::instance()->document()->url().fileName();

        if (name.isEmpty())
            name = i18n("Untitled");

        int res = KMessageBox::warningYesNoCancel(q,
                  i18n("<p>The document <b>'%1'</b> has been modified.</p><p>Do you want to save it?</p>", name),
                  QString(),
                  KStandardGuiItem::save(),
                  KStandardGuiItem::discard());

        switch (res) {
        case KMessageBox::Yes : {
            if (DocumentManager::instance()->isTemporaryFile() && !desktopViewProxy->fileSaveAs())
                return false;
            if (!DocumentManager::instance()->save())
                return false;
            break;
        }
        case KMessageBox::No :
            DocumentManager::instance()->document()->removeAutoSaveFiles();
            DocumentManager::instance()->document()->setModified(false);   // Now when queryClose() is called by closeEvent it won't do anything.
            break;
        default : // case KMessageBox::Cancel :
            return false;
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (centralWidget() == d->desktopView)
    {
        if (DocumentManager::instance()->document()->isLoading()) {
            event->ignore();
            return;
        }
        d->allowClose = d->queryClose();
    }

    if (d->allowClose)
    {
        if (d->desktopView)
        {
            d->desktopView->setNoCleanup(true);
            d->desktopView->close();
        }
        event->accept();
    }
    else
    {
        event->ignore();
        emit closeRequested();
    }
}

MainWindow::~MainWindow()
{
    delete d;
}

#ifdef Q_OS_WIN
bool MainWindow::winEvent( MSG * message, long * result )
{
    if (message && message->message == WM_SETTINGCHANGE && message->lParam)
    {
        if (wcscmp(TEXT("ConvertibleSlateMode"), (TCHAR *) message->lParam) == 0)
            d->notifySlateModeChange();
        else if (wcscmp(TEXT("SystemDockMode"), (TCHAR *) message->lParam) == 0)
            d->notifyDockingModeChange();
        *result = 0;
        return true;
    }
    return false;
}
#endif

void MainWindow::Private::notifySlateModeChange()
{
#ifdef Q_OS_WIN
    bool bSlateMode = (GetSystemMetrics(SM_CONVERTIBLESLATEMODE) == 0);

    if (slateMode != bSlateMode)
    {
        slateMode = bSlateMode;
        emit q->slateModeChanged();
        if (forceTouch || (slateMode && !forceDesktop))
        {
            if (!toTouch || (toTouch && toTouch->isEnabled()))
                q->switchToTouch();
        }
        else
        {
                q->switchToDesktop();
        }
        //qDebug() << "Slate mode is now" << slateMode;
    } 
#endif
}

void MainWindow::Private::notifyDockingModeChange()
{
#ifdef Q_OS_WIN
    bool bDocked = (GetSystemMetrics(SM_SYSTEMDOCKED) != 0);

    if (docked != bDocked)
    {
        docked = bDocked;
        //qDebug() << "Docking mode is now" << docked;
    }
#endif
}

#include "MainWindow.moc"
