/* This file is part of the KDE project
*
* Copyright (C) 2010 Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
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

#include "KPrViewModeSlidesSorter.h"
#include "KPrSlidesSorterDocumentModel.h"
#include "KPrFactory.h"
#include "KPrSlidesManagerView.h"
#include "KPrSelectionManager.h"
#include "KPrCustomSlideShowsModel.h"
#include "KPrDocument.h"
#include "KPrCustomSlideShows.h"
#include "KPrSlidesSorterItemDelegate.h"
#include "KPrView.h"

//Qt Headers
#include <QMenu>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <QPushButton>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QPropertyAnimation>
#include <QLineEdit>

//Calligra Headers
#include <KoResourceManager.h>
#include <KoText.h>
#include <KoZoomController.h>
#include <KoPACanvas.h>
#include <KoPADocument.h>
#include <KoPAPageBase.h>
#include <KoPAMasterPage.h>
#include <KoPAView.h>
#include <KoCanvasController.h>
#include <KoCopyController.h>
#include <KoCutController.h>

//KDE Headers
#include <klocale.h>
#include <KDebug>
#include <kconfiggroup.h>
#include <KIconLoader>
#include <KGlobalSettings>
#include <KMessageBox>
#include <KActionCollection>

KPrViewModeSlidesSorter::KPrViewModeSlidesSorter(KoPAView *view, KoPACanvas *canvas)
    : KoPAViewMode(view, canvas)
    , m_slidesSorterView(new KPrSlidesManagerView())
    , m_customSlideShowView(new KPrSlidesManagerView())
    , m_slidesSorterModel(new KPrSlidesSorterDocumentModel(this, view->parentWidget()))
    , m_centralWidget(new QWidget(view->parentWidget()))
    , m_customSlideShowModel(new KPrCustomSlideShowsModel(static_cast<KPrDocument *>(view->kopaDocument()), view->parentWidget()))
    , m_iconSize(QSize(200, 200))
    , m_editCustomSlideShow(false)
{
    //Create customSlideShow GUI
    QWidget *m_customShowsToolBar = new QWidget();

    QHBoxLayout *toolBarLayout = new QHBoxLayout(m_customShowsToolBar);
    QVBoxLayout *centralWidgetLayout = new QVBoxLayout(m_centralWidget);

    QLabel *slideShowsLabel = new QLabel(i18n("Slide Show: "));
    m_customSlideShowsList = new QComboBox;
    m_customSlideShowsList->setEditable(false);
    m_customSlideShowsList->setInsertPolicy(QComboBox::NoInsert);
    m_customSlideShowsList->setMinimumContentsLength(30);
    slideShowsLabel->setBuddy(m_customSlideShowsList);

    m_buttonAddCustomSlideShow = new QToolButton();
    m_buttonAddCustomSlideShow->setIcon(SmallIcon("list-add"));
    m_buttonAddCustomSlideShow->setToolTip(i18n("Add a new custom slide show"));

    m_buttonDelCustomSlideShow = new QToolButton();
    m_buttonDelCustomSlideShow->setIcon(SmallIcon("list-remove"));
    m_buttonDelCustomSlideShow->setEnabled(false);
    m_buttonDelCustomSlideShow->setToolTip(i18n("Delete current custom slide show"));

    m_buttonAddSlideToCurrentShow = new QToolButton();
    m_buttonAddSlideToCurrentShow->setIcon(SmallIcon("arrow-down"));
    m_buttonAddSlideToCurrentShow->setToolTip(i18n("Add slides to current custom slide show"));
    m_buttonAddSlideToCurrentShow->setEnabled(false);

    m_buttonDelSlideFromCurrentShow = new QToolButton();
    m_buttonDelSlideFromCurrentShow->setIcon(SmallIcon("arrow-up"));
    m_buttonDelSlideFromCurrentShow->setToolTip(i18n("Remove slides from current custom slide show"));
    m_buttonDelSlideFromCurrentShow->setEnabled(false);

    QSplitter *viewsSplitter = new QSplitter(Qt::Vertical);

    //hide Custom Shows View
    m_customSlideShowView->setMaximumHeight(0);

    //Layout Widgets
    toolBarLayout->addWidget(slideShowsLabel);
    toolBarLayout->addWidget(m_customSlideShowsList);
    toolBarLayout->addWidget(m_buttonAddCustomSlideShow);
    toolBarLayout->addWidget(m_buttonDelCustomSlideShow);
    toolBarLayout->addStretch();
    toolBarLayout->addWidget(m_buttonAddSlideToCurrentShow);
    toolBarLayout->addWidget(m_buttonDelSlideFromCurrentShow);

    viewsSplitter->addWidget(m_slidesSorterView);
    viewsSplitter->addWidget(m_customSlideShowView);
    centralWidgetLayout->addWidget(viewsSplitter);
    centralWidgetLayout->addWidget(m_customShowsToolBar);

    //initialize widgets
    m_centralWidget->hide();
    m_slidesSorterView->setIconSize(m_iconSize);
    m_slidesSorterView->setAutoScroll(true);
    m_customSlideShowView->setIconSize(m_iconSize);
    m_customSlideShowView->setAutoScroll(true);

    //Populate ComboBox
    customShowChanged(0);
    updateCustomSlideShowsList();

    //Setup customSlideShows view
    m_customSlideShowView->setModel(m_customSlideShowModel);
    m_customSlideShowView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_customSlideShowView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_customSlideShowView->setDragDropMode(QAbstractItemView::InternalMove);

    //Setup slides sorter view
    m_slidesSorterModel->setDocument(m_view->kopaDocument());
    m_slidesSorterView->setModel(m_slidesSorterModel);
    m_slidesSorterView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_slidesSorterView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_slidesSorterView->setDragDropMode(QAbstractItemView::InternalMove);

    //setup signals
    connect(m_slidesSorterView, SIGNAL(requestContextMenu(QContextMenuEvent*)), this, SLOT(slidesSorterContextMenu(QContextMenuEvent*)));
    connect(m_customSlideShowView, SIGNAL(requestContextMenu(QContextMenuEvent*)), this, SLOT(customSlideShowsContextMenu(QContextMenuEvent*)));
    connect(m_slidesSorterView, SIGNAL(slideDblClick()), this, SLOT(activateNormalViewMode()));
    connect(m_buttonAddCustomSlideShow, SIGNAL(clicked()), this, SLOT(addCustomSlideShow()));
    connect(m_buttonDelCustomSlideShow, SIGNAL(clicked()), this, SLOT(removeCustomSlideShow()));
    connect(m_buttonAddSlideToCurrentShow, SIGNAL(clicked()), this, SLOT(addSlideToCustomShow()));
    connect(m_buttonDelSlideFromCurrentShow, SIGNAL(clicked()), this, SLOT(deleteSlidesFromCustomShow()));
    connect(m_customSlideShowModel, SIGNAL(customSlideShowsChanged()), this, SLOT(updateCustomSlideShowsList()));

    //setup signals for manage edit actions
    connect(view->copyController(), SIGNAL(copyRequested()), this, SLOT(editCopy()));
    connect(view->cutController(), SIGNAL(copyRequested()), this, SLOT(editCut()));
    connect(view, SIGNAL(selectAllRequested()), m_slidesSorterView, SLOT(selectAll()));
    connect(view, SIGNAL(deselectAllRequested()), m_slidesSorterView, SLOT(clearSelection()));
    connect(m_slidesSorterView, SIGNAL(selectionCleared()), this, SLOT(disableEditActions()));
    connect(m_slidesSorterView, SIGNAL(itemSelected()), this, SLOT(enableEditActions()));
    connect(m_slidesSorterView, SIGNAL(focusLost()), SLOT(disableEditActions()));
    connect(m_slidesSorterView, SIGNAL(focusGot()), SLOT(manageAddRemoveSlidesButtons()));
    connect(m_customSlideShowView, SIGNAL(focusGot()), SLOT(disableEditActions()));
    connect(m_customSlideShowView, SIGNAL(focusGot()), SLOT(manageAddRemoveSlidesButtons()));

    //install selection manager for Slides Sorter View and Custom Shows View
    new KPrSelectionManager(m_slidesSorterView);
    new KPrSelectionManager(m_customSlideShowView);

    //install delegate for Slides Sorter View
    KPrSlidesSorterItemDelegate *slidesSorterDelegate = new KPrSlidesSorterItemDelegate(m_slidesSorterView);
    m_slidesSorterView->setItemDelegate(slidesSorterDelegate);
}

KPrViewModeSlidesSorter::~KPrViewModeSlidesSorter()
{
    //save zoom value
    saveZoomConfig(zoom());
    delete m_centralWidget;
}

void KPrViewModeSlidesSorter::paint(KoPACanvasBase* /*canvas*/, QPainter& /*painter*/, const QRectF &/*paintRect*/)
{
}

void KPrViewModeSlidesSorter::paintEvent( KoPACanvas *canvas, QPaintEvent *event )
{
    Q_UNUSED(canvas);
    Q_UNUSED(event);
    Q_ASSERT( m_canvas == canvas );
}

void KPrViewModeSlidesSorter::tabletEvent(QTabletEvent *event, const QPointF &point)
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeSlidesSorter::mousePressEvent(QMouseEvent *event, const QPointF &point)
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeSlidesSorter::mouseDoubleClickEvent(QMouseEvent *event, const QPointF &point)
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeSlidesSorter::mouseMoveEvent(QMouseEvent *event, const QPointF &point)
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeSlidesSorter::mouseReleaseEvent(QMouseEvent *event, const QPointF &point)
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeSlidesSorter::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}

void KPrViewModeSlidesSorter::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}

void KPrViewModeSlidesSorter::wheelEvent(QWheelEvent *event, const QPointF &point)
{
    Q_UNUSED(event);
    Q_UNUSED(point);
}

void KPrViewModeSlidesSorter::activate(KoPAViewMode *previousViewMode)
{
    Q_UNUSED(previousViewMode);
    populate();
    KoPAView *view = dynamic_cast<KoPAView *>(m_view);
    if (view) {
        view->hide();
    }
    m_centralWidget->show();
    m_slidesSorterView->setFocus(Qt::ActiveWindowFocusReason);
    updateToActivePageIndex();

    //setup signals
    connect(m_slidesSorterView,SIGNAL(indexChanged(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
    connect(m_slidesSorterView, SIGNAL(pressed(QModelIndex)), this, SLOT(itemClicked(const QModelIndex)));
    connect(m_view->proxyObject, SIGNAL(activePageChanged()), this, SLOT(updateToActivePageIndex()));
    connect(m_view->kopaDocument(),SIGNAL(pageAdded(KoPAPageBase*)),this, SLOT(updateSlidesSorterDocumentModel()));
    connect(m_view->kopaDocument(),SIGNAL(pageRemoved(KoPAPageBase*)),this, SLOT(updateSlidesSorterDocumentModel()));

    //change zoom saving slot
    connect(m_view->zoomController(), SIGNAL(zoomChanged(KoZoomMode::Mode, qreal)), this, SLOT(updateZoom(KoZoomMode::Mode, qreal)));

    KPrView *kPrview = dynamic_cast<KPrView *>(m_view);
    if (kPrview) {
        disconnect(kPrview->zoomController(), SIGNAL(zoomChanged(KoZoomMode::Mode, qreal)), kPrview, SLOT(zoomChanged(KoZoomMode::Mode, qreal)));
        m_view->zoomController()->zoomAction()->setZoomModes(KoZoomMode::ZOOM_CONSTANT);
        loadZoomConfig();
        disconnect(kPrview->deleteSelectionAction(), SIGNAL(triggered()), kPrview, SLOT(editDeleteSelection()));
        connect(kPrview->deleteSelectionAction(), SIGNAL(triggered()), this, SLOT(deleteSlide()));
    }
}

void KPrViewModeSlidesSorter::deactivate()
{
    m_centralWidget->hide();
    // Give the ressources back to the canvas
    m_canvas->resourceManager()->setResource(KoText::ShowTextFrames, 0);
    // Active the view as a basic but active one
    m_view->setActionEnabled(KoPAView::AllActions, true);
    m_view->doUpdateActivePage(m_view->activePage());
    KoPAView *view = dynamic_cast<KoPAView *>(m_view);
    if (view) {
        view->show();
    }

    //save zoom value
    saveZoomConfig(zoom());

    //change zoom saving slot and restore normal view zoom values
    disconnect(m_view->zoomController(), SIGNAL(zoomChanged(KoZoomMode::Mode, qreal)), this, SLOT(updateZoom(KoZoomMode::Mode, qreal)));
    m_view->zoomController()->zoomAction()->setZoomModes(KoZoomMode::ZOOM_PAGE | KoZoomMode::ZOOM_WIDTH);
    m_view->setActivePage(m_view->kopaDocument()->pageByIndex(m_slidesSorterView->currentIndex().row(), false));

    KPrView *kPrview = dynamic_cast<KPrView *>(m_view);
    if (kPrview) {
        kPrview->restoreZoomConfig();
        connect(kPrview->zoomController(), SIGNAL(zoomChanged(KoZoomMode::Mode, qreal)), kPrview, SLOT(zoomChanged(KoZoomMode::Mode, qreal)));
        connect(kPrview->deleteSelectionAction(), SIGNAL(triggered()), kPrview, SLOT(editDeleteSelection()));
        disconnect(kPrview->deleteSelectionAction(), SIGNAL(triggered()), this, SLOT(deleteSlide()));
    }
    disableEditActions();
}

void KPrViewModeSlidesSorter::updateSlidesSorterDocumentModel()
{
    m_slidesSorterModel->update();
    updateToActivePageIndex();
}

void KPrViewModeSlidesSorter::updateActivePage( KoPAPageBase *page )
{
    if (m_view->activePage() != page) {
        m_view->setActivePage(page);
    }
    updateToActivePageIndex();
}

void KPrViewModeSlidesSorter::updateToActivePageIndex()
{
    int row = m_view->kopaDocument()->pageIndex(m_view->activePage());
    QModelIndex index = m_slidesSorterModel->index(row, 0, QModelIndex());
    m_slidesSorterView->setCurrentIndex(index);
}

void KPrViewModeSlidesSorter::updateActivePageToCurrentIndex()
{
    QModelIndex c_index = m_slidesSorterView->currentIndex();
    m_view->setActivePage(m_view->kopaDocument()->pageByIndex(c_index.row(), false));
}

void KPrViewModeSlidesSorter::addShape( KoShape *shape )
{
    Q_UNUSED(shape);
}

void KPrViewModeSlidesSorter::removeShape( KoShape *shape )
{
    Q_UNUSED(shape);
}

void KPrViewModeSlidesSorter::populate()
{
    //Init m_slidesSorter view
    QModelIndex item = m_slidesSorterModel->index(0, 0, QModelIndex());
    m_slidesSorterView->setItemSize(m_slidesSorterView->visualRect(item));

    //Init m_customSlidesShowsView
    m_customSlideShowView->setItemSize(m_slidesSorterView->visualRect(item));
}

QSize KPrViewModeSlidesSorter::iconSize() const
{
    return m_iconSize;
}

void KPrViewModeSlidesSorter::selectSlides(const QList<KoPAPageBase *> &slides)
{
    if (slides.isEmpty()) {
        return;
    }

    m_slidesSorterView->clearSelection();

    foreach (KoPAPageBase *slide, slides) {
        int row = m_view->kopaDocument()->pageIndex(slide);
        QModelIndex index = m_slidesSorterModel->index(row, 0, QModelIndex());
        if (index.isValid()) {
            m_slidesSorterView->selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }
}

void KPrViewModeSlidesSorter::activateNormalViewMode()
{
    KPrView *view = static_cast<KPrView *>(m_view);
    view->showNormal();
}

void KPrViewModeSlidesSorter::itemClicked(const QModelIndex index)
{
    if (!index.isValid()) {
        return;
    }

    //Avoid deselect slides when dragging
    if (m_slidesSorterView->selectionModel()->selectedIndexes().length () > 1) {
        return;
    }

    KoPAPageBase *page = m_view->kopaDocument()->pageByIndex(index.row(), false);
    if (page) {
        m_view->setActivePage(page);
    }
    enableEditActions();
}

QList<KoPAPageBase *> KPrViewModeSlidesSorter::extractSelectedSlides()
{
    QList<KoPAPageBase *> slides;
    QModelIndexList selectedItems = m_slidesSorterView->selectionModel()->selectedIndexes();
    if (selectedItems.count() == 0) {
        return slides;
    }

    foreach (const QModelIndex &index, selectedItems) {
        KoPAPageBase *page = m_view->kopaDocument()->pageByIndex(index.row (), false);
        if (page) {
            slides.append(page);
        }
    }

    //order slides
    QMap<int, KoPAPageBase*> map;
    foreach (KoPAPageBase *slide, slides)
        map.insert(m_view->kopaDocument()->pages(false).indexOf(slide), slide);
    slides = map.values();

    return slides;
}

void KPrViewModeSlidesSorter::deleteSlide()
{
    if (m_slidesSorterView->hasFocus()) {
        // create a list with all selected slides
        QList<KoPAPageBase*> selectedSlides = extractSelectedSlides();
        m_slidesSorterModel->removeSlides(selectedSlides);
    }
    else if (m_customSlideShowView->hasFocus()) {
        deleteSlidesFromCustomShow();
    }
}

void KPrViewModeSlidesSorter::addSlide()
{
    m_slidesSorterModel->addNewSlide();
}

void KPrViewModeSlidesSorter::renameCurrentSlide()
{
    QModelIndexList selectedItems = m_slidesSorterView->selectionModel()->selectedIndexes();
    m_slidesSorterView->edit(selectedItems.first());
}

void KPrViewModeSlidesSorter::editCut()
{
    editCopy();
    deleteSlide();
}

void KPrViewModeSlidesSorter::editCopy()
{
    // separate selected layers and selected shapes
    QList<KoPAPageBase*> slides = extractSelectedSlides();
    m_slidesSorterModel->copySlides(slides);
}

void KPrViewModeSlidesSorter::editPaste()
{
    m_slidesSorterModel->pasteSlides();
}

void KPrViewModeSlidesSorter::updateZoom(KoZoomMode::Mode mode, qreal zoom)
{
    Q_UNUSED(mode);
    //at zoom 100%, iconSize is set in 200 x 200
    //KPrSlidesSorterDocumentModel uses iconSize function in decorate Role.
    setIconSize(QSize(qRound(zoom*200),qRound(zoom*200)));
    m_slidesSorterView->setIconSize(iconSize());
<<<<<<< HEAD
=======
    m_customSlideShowModel->setIconSize(iconSize());
>>>>>>> master
    m_customSlideShowView->setIconSize(iconSize());

    //update item size
    QModelIndex item = m_slidesSorterModel->index(0, 0, QModelIndex());
    m_slidesSorterView->setItemSize(m_slidesSorterView->visualRect(item));
    m_customSlideShowView->setItemSize(m_slidesSorterView->visualRect(item));

    setZoom(qRound(zoom * 100.));
}

void KPrViewModeSlidesSorter::setIconSize(QSize size)
{
    if (size != m_iconSize) {
        m_iconSize = size;
    }
}

void KPrViewModeSlidesSorter::loadZoomConfig()
{
    KSharedConfigPtr config = KPrFactory::componentData().config();
    int s_zoom = 100;

    if (config->hasGroup("Interface")) {
        const KConfigGroup interface = config->group("Interface");
        s_zoom = interface.readEntry("ZoomSlidesSorter", s_zoom);
    }
    m_view->zoomController()->setZoom(KoZoomMode::ZOOM_CONSTANT, s_zoom/100.);
}

void KPrViewModeSlidesSorter::saveZoomConfig(int zoom)
{
    KSharedConfigPtr config = KPrFactory::componentData().config();
    KConfigGroup interface = config->group("Interface");
    interface.writeEntry("ZoomSlidesSorter", zoom);
}

void KPrViewModeSlidesSorter::setZoom(int zoom)
{
    m_zoom = zoom;
}

int KPrViewModeSlidesSorter::zoom()
{
    return m_zoom;
}

void KPrViewModeSlidesSorter::slidesSorterContextMenu(QContextMenuEvent *event)
{
<<<<<<< HEAD
    QMenu menu(m_slidesSorterView);   
=======
    QMenu menu(m_slidesSorterView);
>>>>>>> master
    menu.addAction(KIcon("document-new"), i18n("Add a new slide"), this, SLOT(addSlide()));
    menu.addAction(KIcon("edit-delete"), i18n("Delete selected slides"), this, SLOT(deleteSlide()));

    QModelIndexList selectedItems = m_slidesSorterView->selectionModel()->selectedIndexes();
    if (selectedItems.count() == 1 && selectedItems.first().isValid()) {
        menu.addAction(KIcon("edit-rename"), i18n("Rename"), this, SLOT(renameCurrentSlide()));
    }

    menu.addSeparator();
    menu.addAction(KIcon("edit-cut"), i18n("Cut"), this,  SLOT(editCut()));
    menu.addAction(KIcon("edit-copy"), i18n("Copy"), this,  SLOT(editCopy()));
    menu.addAction(KIcon("edit-paste"), i18n("Paste"), this, SLOT(editPaste()));
    menu.exec(event->globalPos());
    enableEditActions();
}

void KPrViewModeSlidesSorter::customSlideShowsContextMenu(QContextMenuEvent *event)
{
    QMenu menu(m_customSlideShowView);
    menu.addAction(KIcon("edit-delete"), i18n("Delete selected slides"), this, SLOT(deleteSlidesFromCustomShow()));
    menu.exec(event->globalPos());
}

void KPrViewModeSlidesSorter::enableEditActions()
{
    KActionCollection *ac = canvas()->canvasController()->actionCollection();
    ac->action("edit_copy")->setEnabled(true);
    ac->action("edit_cut")->setEnabled(true);
    ac->action("edit_delete")->setEnabled(true);
}

void KPrViewModeSlidesSorter::disableEditActions()
{
    KActionCollection *ac = canvas()->canvasController()->actionCollection();
    ac->action("edit_copy")->setEnabled(false);
    ac->action("edit_cut")->setEnabled(false);
    ac->action("edit_delete")->setEnabled(false);
}

void KPrViewModeSlidesSorter::customShowChanged(int showNumber)
{
    QString name = m_customSlideShowsList->itemText(showNumber);

    bool panelVisible = true;
    if (showNumber < 1) {
        panelVisible = false;
        name = QString();
    }

    //Change document current custom slide show
    KPrDocument *doc = static_cast<KPrDocument *>(m_view->kopaDocument());
    doc->setActiveCustomSlideShow(name);

    //Decide show or hide Custom Slide Shows View
    if (panelVisible != m_editCustomSlideShow) {
        const bool animate = KGlobalSettings::graphicEffectsLevel() && KGlobalSettings::SimpleAnimationEffects;
        const int duration = animate ? 250 : 1;
<<<<<<< HEAD
        QPropertyAnimation *animation = new QPropertyAnimation (m_customSlideShowView, "maximumHeight");
=======
        QPropertyAnimation *animation = new QPropertyAnimation(m_customSlideShowView, "maximumHeight");
>>>>>>> master

        if (!panelVisible) {
            animation->setDuration(duration);
            animation->setStartValue(m_customSlideShowView->maximumHeight());
            animation->setEndValue(0);
            //Deactivate tool buttons and edition
            disableEditCustomShowButtons();
            m_slidesSorterView->setAutoScroll(true);
        }
        else {
            animation->setDuration(duration);
            animation->setStartValue(0);
<<<<<<< HEAD
            animation->setEndValue(m_slidesSorterView->height()/2);
=======
            animation->setEndValue(m_slidesSorterView->height() / 2);
>>>>>>> master
            //Activate tool buttons and edition
            enableEditCustomShowButtons();
            m_slidesSorterView->setAutoScroll(false);
        }
        animation->start();
    }

    m_editCustomSlideShow = panelVisible;

    //Populate Custom Slide Shows View if visible
    if (panelVisible) {
        m_customSlideShowModel->setActiveSlideShow(showNumber - 1);
    }
}

void KPrViewModeSlidesSorter::deleteSlidesFromCustomShow()
{
    QModelIndexList selectedItems = m_customSlideShowView->selectionModel()->selectedIndexes();
    if (selectedItems.count() == 0) {
        return;
    }
    m_customSlideShowModel->removeSlidesByIndexes(selectedItems);
}

void KPrViewModeSlidesSorter::addSlideToCustomShow()
{
    // create a list with all selected slides
    QList<KoPAPageBase*> selectedSlides = extractSelectedSlides();
    int row = (m_customSlideShowView->currentIndex().row() >= 0) ? m_customSlideShowView->currentIndex().row() + 1 : 0;
    m_customSlideShowModel->addSlides(selectedSlides, row);
}

void KPrViewModeSlidesSorter::addCustomSlideShow()
{
    //We create a different default name for every SlideShow:
    static int newSlideShowsCount = 1;
    while(m_customSlideShowModel->customShowsNamesList().contains(i18n("Slide Show %1", newSlideShowsCount)))
    {
        ++newSlideShowsCount;
    }

    m_customSlideShowModel->addNewCustomShow(i18n("Slide Show %1", newSlideShowsCount));
}


void KPrViewModeSlidesSorter::removeCustomSlideShow()
{
    m_customSlideShowModel->removeCustomShow(m_customSlideShowsList->currentText());
}

void KPrViewModeSlidesSorter::updateCustomSlideShowsList()
{
    disconnect(m_customSlideShowsList, SIGNAL(currentIndexChanged(int)), this, SLOT(customShowChanged(int)));

    QStringList slideShows;
    slideShows << i18n("All slides") << (m_customSlideShowModel->customShowsNamesList());
    m_customSlideShowsList->clear();
    m_customSlideShowsList->addItems(slideShows);
    int index = slideShows.indexOf(m_customSlideShowModel->activeCustomSlideShow());
    m_customSlideShowsList->setCurrentIndex(index >= 0 ? index : 0);
    customShowChanged(m_customSlideShowsList->currentIndex());

    connect(m_customSlideShowsList, SIGNAL(currentIndexChanged(int)), this, SLOT(customShowChanged(int)));
}

void KPrViewModeSlidesSorter::renameCustomSlideShow()
{
    QString newName = m_customSlideShowsList->currentText();

    if (newName == m_customSlideShowModel->activeCustomSlideShow()) {
        return;
    }

    // Empty string is not allowed as a name, if the name is empty, revert back to previous name
    if (newName.isEmpty()) {
        updateCustomSlideShowsList();
    }
    //If the name is not already in use, use it, otherwise let the user know
    else if (!m_customSlideShowModel->customShowsNamesList().contains(newName)) {
       m_customSlideShowModel->renameCustomShow(m_customSlideShowModel->activeCustomSlideShow(), newName);
       updateCustomSlideShowsList();
    }
    else {
        KMessageBox Message;
        Message.sorry(m_customSlideShowView, i18n("There cannot be two slideshows with the same name."), i18n("Error"),
                      KMessageBox::Notify);
        updateCustomSlideShowsList();
    }
}

void KPrViewModeSlidesSorter::enableEditCustomShowButtons()
{
    m_customSlideShowsList->setEditable(true);
    connect(m_customSlideShowsList->lineEdit(), SIGNAL(editingFinished()), this, SLOT(renameCustomSlideShow()));
    m_buttonDelCustomSlideShow->setEnabled(true);
}

void KPrViewModeSlidesSorter::disableEditCustomShowButtons()
{
    m_customSlideShowsList->setEditable(false);
    m_buttonDelCustomSlideShow->setEnabled(false);
    m_buttonAddSlideToCurrentShow->setEnabled(false);
    m_buttonDelSlideFromCurrentShow->setEnabled(false);
}

void KPrViewModeSlidesSorter::manageAddRemoveSlidesButtons()
{
    m_buttonAddSlideToCurrentShow->setEnabled(m_slidesSorterView->hasFocus());
    m_buttonDelSlideFromCurrentShow->setEnabled(m_customSlideShowView->hasFocus());
    KActionCollection *ac = canvas()->canvasController()->actionCollection();
    ac->action("edit_delete")->setEnabled(m_customSlideShowView->hasFocus() ||
                                          !m_slidesSorterView->selectionModel()->selectedIndexes().isEmpty());
}

void KPrViewModeSlidesSorter::setActiveCustomSlideShow(int index)
{
    disconnect(m_customSlideShowsList, SIGNAL(currentIndexChanged(int)), this, SLOT(customShowChanged(int)));

    m_customSlideShowsList->setCurrentIndex(index >= 0 && index < m_customSlideShowsList->count() ? index : 0);
    customShowChanged(m_customSlideShowsList->currentIndex());

    connect(m_customSlideShowsList, SIGNAL(currentIndexChanged(int)), this, SLOT(customShowChanged(int)));
}
