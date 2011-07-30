/*
 *  kis_layer_box.cc - part of Krita aka Krayon aka KimageShop
 *
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (C) 2006 Gábor Lehel <illissius@gmail.com>
 *  Copyright (C) 2007 Thomas Zander <zander@kde.org>
 *  Copyright (C) 2007 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2011 José Luis Vergara <pentalis@gmail.com>
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

#include "kis_layer_box.h"


#include <QToolButton>
#include <QLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QPixmap>
#include <QList>
#include <QVector>

#include <kis_debug.h>
#include <kglobal.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <klocale.h>
#include <khbox.h>
#include <kicon.h>
#include <kaction.h>

#include <KoDocumentSectionView.h>
#include <KoColorSpace.h>
#include <KoCompositeOp.h>

#include <kis_types.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_layer.h>
#include <kis_group_layer.h>
#include <kis_mask.h>
#include <kis_node.h>
#include <kis_composite_ops_model.h>

#include "widgets/kis_cmb_composite.h"
#include "widgets/kis_slider_spin_box.h"
#include "kis_view2.h"
#include "kis_node_manager.h"
#include "kis_node_model.h"
#include "canvas/kis_canvas2.h"
#include "kis_doc2.h"

#include "ui_wdglayerbox.h"


KisLayerBox::KisLayerBox()
        : QDockWidget(i18n("Layers"))
        , m_canvas(0)
        , m_wdgLayerBox(new Ui_WdgLayerBox)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* mainWidget = new QWidget(this);
    setWidget(mainWidget);
    m_delayTimer.setSingleShot(true);

    m_wdgLayerBox->setupUi(mainWidget);

    setMinimumSize(mainWidget->minimumSizeHint());

    m_wdgLayerBox->listLayers->setDragDropMode(QAbstractItemView::InternalMove);
    m_wdgLayerBox->listLayers->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

    connect(m_wdgLayerBox->listLayers, SIGNAL(contextMenuRequested(const QPoint&, const QModelIndex&)),
            this, SLOT(slotContextMenuRequested(const QPoint&, const QModelIndex&)));

    m_viewModeMenu = new KMenu(this);
    QActionGroup *group = new QActionGroup(this);
    QList<QAction*> actions;

    actions << m_viewModeMenu->addAction(KIcon("view-list-text"),
                                         i18n("Minimal View"), this, SLOT(slotMinimalView()));
    actions << m_viewModeMenu->addAction(KIcon("view-list-details"),
                                         i18n("Detailed View"), this, SLOT(slotDetailedView()));
    actions << m_viewModeMenu->addAction(KIcon("view-preview"),
                                         i18n("Thumbnail View"), this, SLOT(slotThumbnailView()));

    for (int i = 0, n = actions.count(); i < n; ++i) {
        actions[i]->setCheckable(true);
        actions[i]->setActionGroup(group);
    }
    actions[1]->trigger(); //TODO save/load previous state

    m_wdgLayerBox->bnViewMode->setMenu(m_viewModeMenu);
    m_wdgLayerBox->bnViewMode->setPopupMode(QToolButton::InstantPopup);
    m_wdgLayerBox->bnViewMode->setIcon(KIcon("view-choose"));
    m_wdgLayerBox->bnViewMode->setText(i18n("View mode"));

    m_wdgLayerBox->bnDelete->setIcon(BarIcon("list-remove"));
    m_wdgLayerBox->bnDelete->setIconSize(QSize(22, 22));

    m_wdgLayerBox->bnRaise->setEnabled(false);
    m_wdgLayerBox->bnRaise->setIcon(BarIcon("go-up"));
    m_wdgLayerBox->bnRaise->setIconSize(QSize(22, 22));
    
    m_wdgLayerBox->bnLower->setEnabled(false);
    m_wdgLayerBox->bnLower->setIcon(BarIcon("go-down"));
    m_wdgLayerBox->bnLower->setIconSize(QSize(22, 22));
    
    m_wdgLayerBox->bnLeft->setEnabled(true);
    m_wdgLayerBox->bnLeft->setIcon(BarIcon("arrow-left"));
    m_wdgLayerBox->bnLeft->setIconSize(QSize(22, 22));
    
    m_wdgLayerBox->bnRight->setEnabled(true);
    m_wdgLayerBox->bnRight->setIcon(BarIcon("arrow-right"));
    m_wdgLayerBox->bnRight->setIconSize(QSize(22, 22));

    m_wdgLayerBox->bnProperties->setIcon(BarIcon("document-properties"));
    m_wdgLayerBox->bnProperties->setIconSize(QSize(22, 22));

    m_wdgLayerBox->bnDuplicate->setIcon(BarIcon("edit-copy"));
    m_wdgLayerBox->bnDuplicate->setIconSize(QSize(22, 22));

    connect(m_wdgLayerBox->bnDelete, SIGNAL(clicked()), SLOT(slotRmClicked()));
    // NOTE: this is _not_ a mistake. The layerbox shows the layers in the reverse order
    connect(m_wdgLayerBox->bnRaise, SIGNAL(clicked()), SLOT(slotLowerClicked()));
    connect(m_wdgLayerBox->bnLower, SIGNAL(clicked()), SLOT(slotRaiseClicked()));
    // END NOTE
    connect(m_wdgLayerBox->bnLeft, SIGNAL(clicked()), SLOT(slotLeftClicked()));
    connect(m_wdgLayerBox->bnRight, SIGNAL(clicked()), SLOT(slotRightClicked()));

    connect(m_wdgLayerBox->bnProperties, SIGNAL(clicked()), SLOT(slotPropertiesClicked()));
    connect(m_wdgLayerBox->bnDuplicate, SIGNAL(clicked()), SLOT(slotDuplicateClicked()));

    connect(m_wdgLayerBox->doubleOpacity, SIGNAL(valueChanged(qreal)), SLOT(slotOpacitySliderMoved(qreal)));
    connect(&m_delayTimer, SIGNAL(timeout()), SLOT(slotOpacityChanged()));

    connect(m_wdgLayerBox->cmbComposite, SIGNAL(activated(int)), SLOT(slotCompositeOpChanged(int)));

    m_wdgLayerBox->bnAddPaintLayer->setIcon(BarIcon("document-new"));
    connect(m_wdgLayerBox->bnAddPaintLayer, SIGNAL(clicked()), SLOT(slotNewPaintLayer()));
    
    m_wdgLayerBox->bnAddGroupLayer->setIcon(BarIcon("folder-new"));
    connect(m_wdgLayerBox->bnAddGroupLayer, SIGNAL(clicked()), SLOT(slotNewGroupLayer()));
    
    m_wdgLayerBox->bnAddCloneLayer->setIcon(BarIcon("edit-copy"));
    connect(m_wdgLayerBox->bnAddCloneLayer, SIGNAL(clicked()), SLOT(slotNewCloneLayer()));
    
    m_wdgLayerBox->bnAddShapeLayer->setIcon(BarIcon("bookmaer-new"));
    connect(m_wdgLayerBox->bnAddShapeLayer, SIGNAL(clicked()), SLOT(slotNewShapeLayer()));
    
    m_wdgLayerBox->bnAddFilterLayer->setIcon(BarIcon("view-filter"));
    connect(m_wdgLayerBox->bnAddFilterLayer, SIGNAL(clicked()), SLOT(slotNewAdjustmentLayer()));
    
    m_wdgLayerBox->bnAddGeneratedLayer->setIcon(BarIcon("view-filter"));
    connect(m_wdgLayerBox->bnAddGeneratedLayer, SIGNAL(clicked()), SLOT(slotNewGeneratorLayer()));
    
    m_wdgLayerBox->bnAddTransparencyMask->setIcon(BarIcon("edit-copy"));
    connect(m_wdgLayerBox->bnAddTransparencyMask, SIGNAL(clicked()), SLOT(slotNewTransparencyMask()));
    
    m_wdgLayerBox->bnAddFilterMask->setIcon(BarIcon("bookmarks"));
    connect(m_wdgLayerBox->bnAddFilterMask, SIGNAL(clicked()), SLOT(slotNewEffectMask()));
    
    m_wdgLayerBox->bnAddLocalSelectionMask->setIcon(BarIcon("edit-paste"));
    connect(m_wdgLayerBox->bnAddLocalSelectionMask, SIGNAL(clicked()), SLOT(slotNewSelectionMask()));
    
    m_nodeModel = new KisNodeModel(this);

    // connect model updateUI() to enable/disable controls
    // connect(m_nodeModel, SIGNAL(nodeActivated(KisNodeSP)), SLOT(updateUI()));      NOTE: commented for temporary bug fix
    connect(m_nodeModel, SIGNAL(nodeActivated(KisNodeSP)), SLOT(setCurrentNode(KisNodeSP)));  // NOTE: temporary bug fix - Pentalis
    connect(m_nodeModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(updateUI()));
    connect(m_nodeModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(updateUI()));
    connect(m_nodeModel, SIGNAL(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)), SLOT(updateUI()));
    connect(m_nodeModel, SIGNAL(modelReset()), SLOT(updateUI()));

    m_wdgLayerBox->listLayers->setModel(m_nodeModel);
}

KisLayerBox::~KisLayerBox()
{
    delete m_wdgLayerBox;
}

void KisLayerBox::setCanvas(KoCanvasBase *canvas)
{
    if (m_canvas) {
       m_canvas->disconnectCanvasObserver(this);
    }

    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
    if (m_canvas) {
        connect(m_canvas, SIGNAL(imageChanged(KisImageWSP)), SLOT(setImage(KisImageWSP)));
        setImage(m_canvas->view()->image());
    }
}    


void KisLayerBox::unsetCanvas()
{
    m_canvas = 0;
}

void KisLayerBox::setImage(KisImageWSP image)
{
    if(m_image && m_canvas && m_canvas->view())
    {
        disconnect(m_image, SIGNAL(sigAboutToBeDeleted()), this, SLOT(notifyImageDeleted()));
        disconnect(m_nodeManager, SIGNAL(sigUiNeedChangeActiveNode(KisNodeSP)), this, SLOT(setCurrentNode(KisNodeSP)));
        disconnect(m_nodeModel, SIGNAL(nodeActivated(KisNodeSP)), m_nodeManager, SLOT(slotUiActivatedNode(KisNodeSP)));

        disconnect(m_nodeModel, SIGNAL(requestAddNode(KisNodeSP, KisNodeSP)), m_nodeManager, SLOT(addNode(KisNodeSP, KisNodeSP)));
        disconnect(m_nodeModel, SIGNAL(requestAddNode(KisNodeSP, KisNodeSP, int)), m_nodeManager, SLOT(insertNode(KisNodeSP, KisNodeSP, int)));
        disconnect(m_nodeModel, SIGNAL(requestMoveNode(KisNodeSP, KisNodeSP)), m_nodeManager, SLOT(moveNode(KisNodeSP, KisNodeSP)));
        disconnect(m_nodeModel, SIGNAL(requestMoveNode(KisNodeSP, KisNodeSP, int)), m_nodeManager, SLOT(moveNodeAt(KisNodeSP, KisNodeSP, int)));
    }
    
    m_image = image;

    if (m_image && m_canvas && m_canvas->view()) {

        if (m_nodeManager) {
            m_nodeManager->disconnect(this);
        }
        m_nodeManager = m_canvas->view()->nodeManager();
        m_nodeModel->setImage(m_image);
        connect(m_image, SIGNAL(sigAboutToBeDeleted()), SLOT(notifyImageDeleted()));

        // cold start
        setCurrentNode(m_nodeManager->activeNode());
        connect(m_nodeManager, SIGNAL(sigUiNeedChangeActiveNode(KisNodeSP)), this, SLOT(setCurrentNode(KisNodeSP)));
        connect(m_nodeModel, SIGNAL(nodeActivated(KisNodeSP)), m_nodeManager, SLOT(slotUiActivatedNode(KisNodeSP)));

        connect(m_nodeModel, SIGNAL(requestAddNode(KisNodeSP, KisNodeSP)), m_nodeManager, SLOT(addNode(KisNodeSP, KisNodeSP)));
        connect(m_nodeModel, SIGNAL(requestAddNode(KisNodeSP, KisNodeSP, int)), m_nodeManager, SLOT(insertNode(KisNodeSP, KisNodeSP, int)));
        connect(m_nodeModel, SIGNAL(requestMoveNode(KisNodeSP, KisNodeSP)), m_nodeManager, SLOT(moveNode(KisNodeSP, KisNodeSP)));
        connect(m_nodeModel, SIGNAL(requestMoveNode(KisNodeSP, KisNodeSP, int)), m_nodeManager, SLOT(moveNodeAt(KisNodeSP, KisNodeSP, int)));
    }
    else {
        m_nodeModel->setImage(m_image);
    }

    m_wdgLayerBox->listLayers->expandAll();
    m_wdgLayerBox->listLayers->scrollToBottom();
}

void KisLayerBox::notifyImageDeleted()
{
    setImage(0);
}

void KisLayerBox::updateUI()
{
    KisNodeSP active = m_image ? m_nodeManager->activeNode() : 0;

    m_wdgLayerBox->bnDelete->setEnabled(active);
    m_wdgLayerBox->bnRaise->setEnabled(active && (active->nextSibling()
                                       || (active->parent() && active->parent() != m_image->root())));
    m_wdgLayerBox->bnLower->setEnabled(active && (active->prevSibling()
                                       || (active->parent() && active->parent() != m_image->root())));
    m_wdgLayerBox->bnDuplicate->setEnabled(active);
    m_wdgLayerBox->bnProperties->setEnabled(active);

    m_wdgLayerBox->doubleOpacity->setEnabled(active);
    m_wdgLayerBox->doubleOpacity->setRange(0, 100, 0);

    m_wdgLayerBox->cmbComposite->setEnabled(active);

    if (active) {
        if (m_nodeManager->activePaintDevice())
            slotFillCompositeOps(m_nodeManager->activeColorSpace());
        else
            slotFillCompositeOps(m_image->colorSpace());
        if (active->inherits("KisMask")) {
            active = active->parent(); // We need a layer to set opacity and composite op, which masks don't have
        }
        if (active->inherits("KisLayer")) {
            KisLayerSP l = qobject_cast<KisLayer*>(active.data());
            slotSetOpacity(l->opacity() * 100.0 / 255);
            slotSetCompositeOp(l->compositeOp());
        }
    }
    m_wdgLayerBox->bnAddTransparencyMask->setEnabled(active);
    m_wdgLayerBox->bnAddFilterMask->setEnabled(active);
    m_wdgLayerBox->bnAddLocalSelectionMask->setEnabled(active);
    m_wdgLayerBox->bnAddCloneLayer->setEnabled(active && !active->inherits("KisGroupLayer"));
}

void KisLayerBox::setCurrentNode(KisNodeSP node)
{
    if (node) {
        m_wdgLayerBox->listLayers->setCurrentIndex(m_nodeModel->indexFromNode(node));
        m_nodeManager->activateNode(node);   // NOTE: temporary bug fix - Pentalis
        updateUI();
    }
}

void KisLayerBox::slotSetCompositeOp(const KoCompositeOp* compositeOp)
{
    KoID cmpOp = KoCompositeOpRegistry::instance().getKoID(compositeOp->id());
    int  index = m_wdgLayerBox->cmbComposite->indexOf(cmpOp);
    
    m_wdgLayerBox->cmbComposite->blockSignals(true);
    m_wdgLayerBox->cmbComposite->setCurrentIndex(index);
    m_wdgLayerBox->cmbComposite->blockSignals(false);
}

void KisLayerBox::slotFillCompositeOps(const KoColorSpace* colorSpace)
{
    m_wdgLayerBox->cmbComposite->getModel()->validateCompositeOps(colorSpace);
}

// range: 0-100
void KisLayerBox::slotSetOpacity(double opacity)
{
    Q_ASSERT(opacity >= 0 && opacity <= 100);
    m_wdgLayerBox->doubleOpacity->blockSignals(true);
    m_wdgLayerBox->doubleOpacity->setValue(opacity);
    m_wdgLayerBox->doubleOpacity->blockSignals(false);
}

void KisLayerBox::slotContextMenuRequested(const QPoint &pos, const QModelIndex &index)
{
    QMenu menu;

    if (index.isValid()) {
        menu.addAction(KIcon("document-properties"), i18n("&Properties..."), this, SLOT(slotPropertiesClicked()));
        menu.addSeparator();
        menu.addAction(KIcon("edit-delete"), i18n("&Remove Layer"), this, SLOT(slotRmClicked()));
        menu.addAction(KIcon("edit-duplicate"), i18n("&Duplicate Layer or Mask"), this, SLOT(slotDuplicateClicked()));
        QAction* mergeLayerDown = menu.addAction(KIcon("edit-merge"), i18n("&Merge with Layer Below"), this, SLOT(slotMergeLayer()));
        if (!index.sibling(index.row() + 1, 0).isValid()) mergeLayerDown->setEnabled(false);
        menu.addSeparator();
    }

    menu.exec(pos);
}

void KisLayerBox::slotMergeLayer()      
{
    m_nodeManager->mergeLayerDown();
}

void KisLayerBox::slotMinimalView()
{
    m_wdgLayerBox->listLayers->setDisplayMode(KoDocumentSectionView::MinimalMode);
}

void KisLayerBox::slotDetailedView()
{
    m_wdgLayerBox->listLayers->setDisplayMode(KoDocumentSectionView::DetailedMode);
}

void KisLayerBox::slotThumbnailView()
{
    m_wdgLayerBox->listLayers->setDisplayMode(KoDocumentSectionView::ThumbnailMode);
}

void KisLayerBox::slotNewPaintLayer()
{
    m_nodeManager->createNode("KisPaintLayer");
}

void KisLayerBox::slotNewGroupLayer()
{
    m_nodeManager->createNode("KisGroupLayer");
}

void KisLayerBox::slotNewCloneLayer()
{
    m_nodeManager->createNode("KisCloneLayer");
}


void KisLayerBox::slotNewShapeLayer()
{
    m_nodeManager->createNode("KisShapeLayer");
}


void KisLayerBox::slotNewAdjustmentLayer()
{
    m_nodeManager->createNode("KisAdjustmentLayer");
}

void KisLayerBox::slotNewGeneratorLayer()
{
    m_nodeManager->createNode("KisGeneratorLayer");
}

void KisLayerBox::slotNewTransparencyMask()
{
    m_nodeManager->createNode("KisTransparencyMask");
}

void KisLayerBox::slotNewEffectMask()
{
    m_nodeManager->createNode("KisFilterMask");
}


void KisLayerBox::slotNewSelectionMask()
{
    m_nodeManager->createNode("KisSelectionMask");
}


void KisLayerBox::slotRmClicked()
{
    QModelIndexList l = m_wdgLayerBox->listLayers->selectionModel()->selectedIndexes();
    if (l.count() < 2 && m_nodeManager->activeNode() && !l.contains(m_wdgLayerBox->listLayers->currentIndex())) {
        l.clear();
        l.append(m_wdgLayerBox->listLayers->currentIndex());
    }

    for (int i = 0, n = l.count(); i < n; ++i) {
        KisNodeSP node = m_nodeModel->nodeFromIndex(l.at(i));
        if (!node->systemLocked()) {
            m_nodeManager->removeNode(node);
        }
    }
    if (m_canvas && m_canvas->view()) {
        KisView2* view = m_canvas->view();
        view->updateGUI();
    }
}

void KisLayerBox::slotRaiseClicked()
{
    KisNodeSP node = m_nodeManager->activeNode();
    KisNodeSP parent = node->parent();
    KisNodeSP grandParent = parent->parent();

    if (!m_nodeManager->activeNode()->prevSibling()) {
        if (!grandParent) return;  
        if (!grandParent->parent() && node->inherits("KisMask")) return;
        m_nodeManager->moveNodeAt(node, grandParent, grandParent->index(parent));
    } else {
        m_nodeManager->raiseNode();
    }
}

void KisLayerBox::slotLowerClicked()
{
    KisNodeSP node = m_nodeManager->activeNode();
    KisNodeSP parent = node->parent();
    KisNodeSP grandParent = parent->parent();
    
    if (!m_nodeManager->activeNode()->nextSibling()) {
        if (!grandParent) return;  
        if (!grandParent->parent() && node->inherits("KisMask")) return;
        m_nodeManager->moveNodeAt(node, grandParent, grandParent->index(parent) + 1);
    } else {
        m_nodeManager->lowerNode();
    }
}

void KisLayerBox::slotLeftClicked()
{
    KisNodeSP node = m_nodeManager->activeNode();
    KisNodeSP parent = node->parent();
    KisNodeSP grandParent = parent->parent();
    quint16 nodeIndex = parent->index(node);
    
    if (!grandParent) return;  
    if (!grandParent->parent() && node->inherits("KisMask")) return;

    if (nodeIndex <= parent->childCount() / 2) {
        m_nodeManager->moveNodeAt(node, grandParent, grandParent->index(parent));
    } else {
        m_nodeManager->moveNodeAt(node, grandParent, grandParent->index(parent) + 1);
    }
}

void KisLayerBox::slotRightClicked()
{
    KisNodeSP node = m_nodeManager->activeNode();
    KisNodeSP parent = m_nodeManager->activeNode()->parent();
    KisNodeSP newParent;
    int nodeIndex = parent->index(node);
    int indexAbove = nodeIndex + 1;
    int indexBelow = nodeIndex - 1;

    if (parent->at(indexBelow) && parent->at(indexBelow)->allowAsChild(node)) {
        newParent = parent->at(indexBelow);
        m_nodeManager->moveNodeAt(node, newParent, newParent->childCount());
    } else if (parent->at(indexAbove) && parent->at(indexAbove)->allowAsChild(node)) {
        newParent = parent->at(indexAbove);
        m_nodeManager->moveNodeAt(node, newParent, 0);
    } else {
        return;
    }
}

void KisLayerBox::slotPropertiesClicked()
{
    if (KisNodeSP active = m_nodeManager->activeNode()) {
        m_nodeManager->nodeProperties(active);
    }
}

void KisLayerBox::slotDuplicateClicked()
{
    m_nodeManager->duplicateActiveNode();
}

void KisLayerBox::slotCompositeOpChanged(int index)
{
    KoID compositeOp;
    
    if(m_wdgLayerBox->cmbComposite->entryAt(compositeOp, index))
        m_nodeManager->nodeCompositeOpChanged(m_nodeManager->activeColorSpace()->compositeOp(compositeOp.id()));
}

void KisLayerBox::slotOpacityChanged()
{
    m_nodeManager->nodeOpacityChanged(m_newOpacity, true);
}

void KisLayerBox::slotOpacitySliderMoved(qreal opacity)
{
    m_newOpacity = opacity;
    m_delayTimer.start(200);
}



#include "kis_layer_box.moc"
