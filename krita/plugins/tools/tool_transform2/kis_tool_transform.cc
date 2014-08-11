/*
 *  kis_tool_transform.cc -- part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2005 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2010 Marc Pegon <pe.marc@free.fr>
 *  Copyright (c) 2013 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_tool_transform.h"


#include <math.h>
#include <limits>

#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QObject>
#include <QLabel>
#include <QComboBox>
#include <QApplication>
#include <QMatrix4x4>

#include <kis_debug.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <klocale.h>
#include <knuminput.h>

#include <KoPointerEvent.h>
#include <KoID.h>
#include <KoCanvasBase.h>
#include <KoViewConverter.h>
#include <KoSelection.h>
#include <KoCompositeOp.h>
#include <KoShapeManager.h>
#include <KoProgressUpdater.h>


#include <kis_global.h>
#include <canvas/kis_canvas2.h>
#include <kis_view2.h>
#include <kis_painter.h>
#include <kis_cursor.h>
#include <kis_image.h>
#include <kis_undo_adapter.h>
#include <kis_transaction.h>
#include <kis_selection.h>
#include <kis_filter_strategy.h>
#include <widgets/kis_cmb_idlist.h>
#include <kis_statusbar.h>
#include <kis_transform_worker.h>
#include <kis_perspectivetransform_worker.h>
#include <kis_warptransform_worker.h>
#include <kis_pixel_selection.h>
#include <kis_shape_selection.h>
#include <kis_selection_manager.h>
#include <kis_system_locker.h>
#include <krita_utils.h>
#include <kis_resources_snapshot.h>

#include <KoShapeTransformCommand.h>

#include "widgets/kis_progress_widget.h"

#include "kis_warp_transform_strategy.h"
#include "kis_free_transform_strategy.h"

#include <Eigen/Geometry>
using namespace Eigen;

#include "strokes/transform_stroke_strategy.h"

KisToolTransform::KisToolTransform(KoCanvasBase * canvas)
    : KisTool(canvas, KisCursor::rotateCursor())
    , m_workRecursively(true)
    , m_isActive(false)
    , m_changesTracker(&m_transaction)
    , m_warpStrategy(
        new KisWarpTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            m_currentArgs, m_transaction))
    , m_freeStrategy(
        new KisFreeTransformStrategy(
            dynamic_cast<KisCanvas2*>(canvas)->coordinatesConverter(),
            m_currentArgs, m_transaction, m_transform))
{
    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
    Q_ASSERT(m_canvas);

    setObjectName("tool_transform");
    useCursor(KisCursor::selectCursor());
    m_optionsWidget = 0;

    connect(m_warpStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_freeStrategy.data(), SIGNAL(requestCanvasUpdate()), SLOT(canvasUpdateRequested()));
    connect(m_freeStrategy.data(), SIGNAL(requestResetRotationCenterButtons()), SLOT(resetRotationCenterButtonsRequested()));
    connect(m_freeStrategy.data(), SIGNAL(requestShowImageTooBig(bool)), SLOT(imageTooBigRequested(bool)));

    connect(&m_changesTracker, SIGNAL(sigConfigChanged()),
            this, SLOT(slotTrackerChangedConfig()));
}

KisToolTransform::~KisToolTransform()
{
    cancelStroke();
}

void KisToolTransform::outlineChanged()
{
    emit freeTransformChanged();
    m_canvas->updateCanvas();
}

void KisToolTransform::canvasUpdateRequested()
{
    m_canvas->updateCanvas();
}

void KisToolTransform::resetRotationCenterButtonsRequested()
{
    if (!m_optionsWidget) return;
    m_optionsWidget->resetRotationCenterButtons();
}

void KisToolTransform::imageTooBigRequested(bool value)
{
    if (!m_optionsWidget) return;
    m_optionsWidget->setTooBigLabelVisible(value);
}

void KisToolTransform::paint(QPainter& gc, const KoViewConverter &converter)
{
    Q_UNUSED(converter);

    if (!m_strokeData.strokeId()) return;

    gc.save();
    if (m_optionsWidget && m_optionsWidget->showDecorations()) {
        gc.setOpacity(0.3);
        gc.fillPath(m_selectionPath, Qt::black);
    }
    gc.restore();

    if (m_currentArgs.mode() == ToolTransformArgs::FREE_TRANSFORM) {
        m_freeStrategy->paint(gc);
    } else if (m_currentArgs.mode() == ToolTransformArgs::WARP) {
        m_warpStrategy->paint(gc);
    }
}

void KisToolTransform::setFunctionalCursor()
{
    if (!m_strokeData.strokeId()) {
        useCursor(KisCursor::pointingHandCursor());
    } else if (m_currentArgs.mode() == ToolTransformArgs::WARP) {
        useCursor(m_warpStrategy->getCurrentCursor());
    }
    else {
        useCursor(m_freeStrategy->getCurrentCursor());
    }
}

void KisToolTransform::mousePressEvent(KoPointerEvent *event)
{
    if (!PRESS_CONDITION_OM(event, KisTool::HOVER_MODE, Qt::LeftButton, Qt::ControlModifier | Qt::ShiftModifier)) {

        KisTool::mousePressEvent(event);
        return;
    }

    KisImageWSP kisimage = image();

    if (!currentNode())
        return;

    setMode(KisTool::PAINT_MODE);
    if (kisimage && event->button() == Qt::LeftButton) {
        QPointF mousePos = m_canvas->coordinatesConverter()->documentToImage(event->point);
        if (!m_strokeData.strokeId()) {
            startStroke(m_currentArgs.mode());
            setMode(KisTool::HOVER_MODE);
        } else if (m_currentArgs.mode() == ToolTransformArgs::WARP) {
            if (!m_warpStrategy->beginPrimaryAction(mousePos)) {
                setMode(KisTool::HOVER_MODE);
            }
        } else if (m_currentArgs.mode() == ToolTransformArgs::FREE_TRANSFORM) {
            if (!m_freeStrategy->beginPrimaryAction(mousePos)) {
                setMode(KisTool::HOVER_MODE);
            }
        }

        m_actuallyMoveWhileSelected = false;
    }

    outlineChanged();
}

void KisToolTransform::touchEvent( QTouchEvent* event )
{
    //Count all moving touch points
    int touchCount = 0;
    foreach( QTouchEvent::TouchPoint tp, event->touchPoints() ) {
        if( tp.state() == Qt::TouchPointMoved ) {
            touchCount++;
        }
    }

    //Use the touch point count to determine the gesture
    switch( touchCount ) {
    case 1: { //Panning
        QTouchEvent::TouchPoint tp = event->touchPoints().at( 0 );
        QPointF diff = tp.screenPos() - tp.lastScreenPos();

        m_currentArgs.setTransformedCenter( m_currentArgs.transformedCenter() + diff );
        outlineChanged();
        break;
    }
    case 2: { //Scaling
        QTouchEvent::TouchPoint tp1 = event->touchPoints().at( 0 );
        QTouchEvent::TouchPoint tp2 = event->touchPoints().at( 1 );

        float lastZoom = (tp1.lastScreenPos() - tp2.lastScreenPos()).manhattanLength();
        float newZoom = (tp1.screenPos() - tp2.screenPos()).manhattanLength();

        float diff = (newZoom - lastZoom) / 100;

        m_currentArgs.setScaleX( m_currentArgs.scaleX() + diff );
        m_currentArgs.setScaleY( m_currentArgs.scaleY() + diff );

        outlineChanged();
        break;
    }
    case 3: { //Rotation

/* TODO: implement touch-based rotation.

            Vector2f center;
            foreach( const QTouchEvent::TouchPoint &tp, event->touchPoints() ) {
                if( tp.state() == Qt::TouchPointMoved ) {
                    center += Vector2f( tp.screenPos().x(), tp.screenPos().y() );
                }
            }
            center /= touchCount;

            QTouchEvent::TouchPoint tp = event->touchPoints().at(0);

            Vector2f oldPosition = (Vector2f( tp.lastScreenPos().x(), tp.lastScreenPos().y() ) - center).normalized();
            Vector2f newPosition = (Vector2f( tp.screenPos().x(), tp.screenPos().y() ) - center).normalized();

            float oldAngle = qAcos( oldPosition.dot( Vector2f( 0.0f, 0.0f ) ) );
            float newAngle = qAcos( newPosition.dot( Vector2f( 0.0f, 0.0f ) ) );

            float diff = newAngle - oldAngle;

            m_currentArgs.setAZ( m_currentArgs.aZ() + diff );

            outlineChanged();
*/
        break;
    }
    }
}

void KisToolTransform::applyTransform()
{
    slotApplyTransform();
}

bool KisToolTransform::isActive() const
{
    return m_isActive;
}

KisToolTransform::TransformToolMode KisToolTransform::transformMode() const
{
    return m_currentArgs.mode() == ToolTransformArgs::FREE_TRANSFORM ? FreeTransformMode : WarpTransformMode;
}

double KisToolTransform::translateX() const
{
    return m_currentArgs.transformedCenter().x();
}

double KisToolTransform::translateY() const
{
    return m_currentArgs.transformedCenter().y();
}

double KisToolTransform::rotateX() const
{
    return m_currentArgs.aX();
}

double KisToolTransform::rotateY() const
{
    return m_currentArgs.aY();
}

double KisToolTransform::rotateZ() const
{
    return m_currentArgs.aZ();
}

double KisToolTransform::scaleX() const
{
    return m_currentArgs.scaleX();
}

double KisToolTransform::scaleY() const
{
    return m_currentArgs.scaleY();
}

double KisToolTransform::shearX() const
{
    return m_currentArgs.shearX();
}

double KisToolTransform::shearY() const
{
    return m_currentArgs.shearY();
}

KisToolTransform::WarpType KisToolTransform::warpType() const
{
    switch(m_currentArgs.warpType()) {
    case KisWarpTransformWorker::AFFINE_TRANSFORM:
        return AffineWarpType;
    case KisWarpTransformWorker::RIGID_TRANSFORM:
        return RigidWarpType;
    case KisWarpTransformWorker::SIMILITUDE_TRANSFORM:
        return SimilitudeWarpType;
    default:
        return RigidWarpType;
    }
}

double KisToolTransform::warpFlexibility() const
{
    return m_currentArgs.alpha();
}

int KisToolTransform::warpPointDensity() const
{
    return m_currentArgs.numPoints();
}

void KisToolTransform::setTransformMode(KisToolTransform::TransformToolMode newMode)
{
    ToolTransformArgs::TransformMode mode = newMode == FreeTransformMode ? ToolTransformArgs::FREE_TRANSFORM : ToolTransformArgs::WARP;
    if( mode != m_currentArgs.mode() ) {
        if( newMode == FreeTransformMode ) {
            m_optionsWidget->slotSetFreeTransformModeButtonClicked( true );
        } else {
            m_optionsWidget->slotSetWrapModeButtonClicked( true );
        }
        emit transformModeChanged();
    }
}

void KisToolTransform::setRotateX( double rotation )
{
    m_currentArgs.setAX( normalizeAngle(rotation) );
}

void KisToolTransform::setRotateY( double rotation )
{
    m_currentArgs.setAY( normalizeAngle(rotation) );
}

void KisToolTransform::setRotateZ( double rotation )
{
    m_currentArgs.setAZ( normalizeAngle(rotation) );
}

void KisToolTransform::setWarpType( KisToolTransform::WarpType type )
{
    switch( type ) {
    case RigidWarpType:
        m_currentArgs.setWarpType(KisWarpTransformWorker::RIGID_TRANSFORM);
        break;
    case AffineWarpType:
        m_currentArgs.setWarpType(KisWarpTransformWorker::AFFINE_TRANSFORM);
        break;
    case SimilitudeWarpType:
        m_currentArgs.setWarpType(KisWarpTransformWorker::SIMILITUDE_TRANSFORM);
        break;
    default:
        break;
    }
}

void KisToolTransform::setWarpFlexibility( double flexibility )
{
    m_currentArgs.setAlpha( flexibility );
}

void KisToolTransform::setWarpPointDensity( int density )
{
    m_optionsWidget->slotSetWarpDensity(density);
}

void KisToolTransform::mouseMoveEvent(KoPointerEvent *event)
{
    QPointF mousePos = m_canvas->coordinatesConverter()->documentToImage(event->point);

    if (!MOVE_CONDITION(event, KisTool::PAINT_MODE)) {
        m_warpStrategy->setTransformFunction(mousePos);
        m_freeStrategy->setTransformFunction(mousePos, event->modifiers() & Qt::ControlModifier);
        setFunctionalCursor();
        KisTool::mouseMoveEvent(event);
        return;
    }

    m_actuallyMoveWhileSelected = true;

    if (m_currentArgs.mode() == ToolTransformArgs::WARP) {
        m_warpStrategy->continuePrimaryAction(mousePos);
    }
    else {
        m_freeStrategy->continuePrimaryAction(mousePos, event->modifiers() & Qt::ShiftModifier);
        updateOptionWidget();
    }

    outlineChanged();
}

void KisToolTransform::mouseReleaseEvent(KoPointerEvent *event)
{
    if (!RELEASE_CONDITION(event, KisTool::PAINT_MODE, Qt::LeftButton)) {
        KisTool::mouseReleaseEvent(event);
        return;
    }

    setMode(KisTool::HOVER_MODE);

    if (m_actuallyMoveWhileSelected) {
        if (m_currentArgs.mode() == ToolTransformArgs::WARP) {
            if (m_warpStrategy->endPrimaryAction()) {
                commitChanges();
            }
        }
        else {
            if (m_freeStrategy->endPrimaryAction()) {
                commitChanges();
            } else {
                outlineChanged();
            }
        }
    }
    updateApplyResetAvailability();
}

void KisToolTransform::initTransformMode(ToolTransformArgs::TransformMode mode)
{
    // NOTE: we are requesting an old value of m_currentArgs variable
    //       here, which is global, don't forget about this on higher
    //       levels.
    QString filterId = m_currentArgs.filterId();

    if (mode == ToolTransformArgs::FREE_TRANSFORM) {
        m_currentArgs = ToolTransformArgs(ToolTransformArgs::FREE_TRANSFORM, m_transaction.originalCenter(), m_transaction.originalCenter(), QPointF(),0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, KisWarpTransformWorker::RIGID_TRANSFORM, 1.0, true, filterId);
        m_freeStrategy->externalConfigChanged();
    } else /* if (mode == ToolTransformArgs::WARP) */ {
        m_currentArgs = ToolTransformArgs(ToolTransformArgs::WARP, m_transaction.originalCenter(), m_transaction.originalCenter(), QPointF(0, 0), 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, KisWarpTransformWorker::RIGID_TRANSFORM, 1.0, true, filterId);
        m_optionsWidget->setDefaultWarpPoints();
        m_warpStrategy->externalConfigChanged();
    }

    outlineChanged();
    updateOptionWidget();
    updateApplyResetAvailability();
}

void KisToolTransform::updateSelectionPath()
{
    m_selectionPath = QPainterPath();

    KisResourcesSnapshotSP resources =
        new KisResourcesSnapshot(image(), 0, this->canvas()->resourceManager());

    QPainterPath selectionOutline;
    KisSelectionSP selection = resources->activeSelection();

    if (selection && selection->outlineCacheValid()) {
        selectionOutline = selection->outlineCache();
    } else {
        selectionOutline.addRect(m_selectedPortionCache->exactBounds());
    }

    const KisCoordinatesConverter *converter = m_canvas->coordinatesConverter();
    QTransform i2f = converter->imageToDocumentTransform() * converter->documentToFlakeTransform();

    m_selectionPath = i2f.map(selectionOutline);
}

void KisToolTransform::initThumbnailImage(KisPaintDeviceSP previewDevice)
{
    m_transform = QTransform();
    QImage origImg;
    m_selectedPortionCache = previewDevice;

    QTransform thumbToImageTransform;

    const int maxSize = 2000;

    QRect srcRect(m_transaction.originalRect().toAlignedRect());
    int x, y, w, h;
    srcRect.getRect(&x, &y, &w, &h);

    if (w > maxSize || h > maxSize) {
        qreal scale = qreal(maxSize) / (w > h ? w : h);
        QTransform scaleTransform = QTransform::fromScale(scale, scale);

        QRect thumbRect = scaleTransform.mapRect(m_transaction.originalRect()).toAlignedRect();

        origImg = m_selectedPortionCache->
            createThumbnail(thumbRect.width(),
                            thumbRect.height(),
                            srcRect,
                            KoColorConversionTransformation::InternalRenderingIntent,
                            KoColorConversionTransformation::InternalConversionFlags);
        thumbToImageTransform = scaleTransform.inverted();

    } else {
        origImg = m_selectedPortionCache->convertToQImage(0, x, y, w, h,
                                                            KoColorConversionTransformation::InternalRenderingIntent,
                                                            KoColorConversionTransformation::InternalConversionFlags);
        thumbToImageTransform = QTransform();
    }

    m_warpStrategy->setThumbnailImage(origImg, thumbToImageTransform);
    m_freeStrategy->setThumbnailImage(origImg, thumbToImageTransform);
}

void KisToolTransform::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    KisTool::activate(toolActivation, shapes);

    if (currentNode()) {
        m_transaction = TransformTransactionProperties(QRectF(), &m_currentArgs, currentNode());
    }

    m_isActive = true;
    emit isActiveChanged();
    startStroke(ToolTransformArgs::FREE_TRANSFORM);
}

void KisToolTransform::deactivate()
{
    endStroke();
    m_canvas->updateCanvas();
    m_isActive = false;
    emit isActiveChanged();

    KisTool::deactivate();
}

void KisToolTransform::requestUndoDuringStroke()
{
    if (!m_strokeData.strokeId()) return;

    m_changesTracker.requestUndo();
}

void KisToolTransform::requestStrokeEnd()
{
    endStroke();
}

void KisToolTransform::requestStrokeCancellation()
{
    cancelStroke();
}

void KisToolTransform::startStroke(ToolTransformArgs::TransformMode mode)
{
    Q_ASSERT(!m_strokeData.strokeId());

    KisPaintDeviceSP dev;

    KisResourcesSnapshotSP resources =
        new KisResourcesSnapshot(image(), 0, this->canvas()->resourceManager());

    KisNodeSP currentNode = resources->currentNode();

    if (!currentNode || !currentNode->isEditable()) {
        return;
    }

    /**
     * FIXME: The transform tool is not completely asynchronous, it
     * needs the content of the layer for creation of the stroke
     * strategy. It means that we cannot start a new stroke until the
     * previous one is finished. Ideally, we should create the
     * m_selectedPortionCache and m_selectionPath somewhere in the
     * stroke and pass it to the tool somehow. But currently, we will
     * just disable starting a new stroke asynchronously
     */
    if (image()->tryBarrierLock()) {
        image()->unlock();
    } else {
        return;
    }

    if (m_optionsWidget) {
        m_workRecursively = m_optionsWidget->workRecursively() ||
            !currentNode->paintDevice();
    }

    TransformStrokeStrategy *strategy = new TransformStrokeStrategy(currentNode, resources->activeSelection(), image()->postExecutionUndoAdapter());
    KisPaintDeviceSP previewDevice = strategy->previewDevice();

    KisSelectionSP selection = resources->activeSelection();
    QRect srcRect = selection ? selection->selectedExactRect() : previewDevice->exactBounds();

    m_transaction = TransformTransactionProperties(srcRect, &m_currentArgs, currentNode);

    initThumbnailImage(previewDevice);
    updateSelectionPath();

    initTransformMode(mode);

    m_strokeData = StrokeData(image()->startStroke(strategy));
    clearDevices(m_transaction.rootNode(), m_workRecursively);

    Q_ASSERT(m_changesTracker.isEmpty());
    commitChanges();
}

void KisToolTransform::endStroke()
{
    if (!m_strokeData.strokeId()) return;

    if (!m_currentArgs.isIdentity()) {
        transformDevices(m_transaction.rootNode(), m_workRecursively);

        image()->addJob(m_strokeData.strokeId(),
                        new TransformStrokeStrategy::TransformData(
                            TransformStrokeStrategy::TransformData::SELECTION,
                            m_currentArgs,
                            m_transaction.rootNode()));

        image()->endStroke(m_strokeData.strokeId());
    } else {
        image()->cancelStroke(m_strokeData.strokeId());
    }

    m_strokeData.clear();
    m_changesTracker.reset();
}

void KisToolTransform::cancelStroke()
{
    if (!m_strokeData.strokeId()) return;

    image()->cancelStroke(m_strokeData.strokeId());
    m_strokeData.clear();
    m_changesTracker.reset();
}

void KisToolTransform::commitChanges()
{
    if (!m_strokeData.strokeId()) return;

    m_changesTracker.commitConfig(m_currentArgs);
}

void KisToolTransform::slotTrackerChangedConfig()
{
    slotUiChangedConfig();
    updateOptionWidget();
}

void KisToolTransform::clearDevices(KisNodeSP node, bool recursive)
{
    if (!node->isEditable()) return;

    if (recursive) {
        // simple tail-recursive iteration
        KisNodeSP prevNode = node->lastChild();
        while(prevNode) {
            clearDevices(prevNode, recursive);
            prevNode = prevNode->prevSibling();
        }
    }

    image()->addJob(m_strokeData.strokeId(),
                    new TransformStrokeStrategy::ClearSelectionData(node));

    /**
     * It might happen that the editablity state of the node would
     * change during the stroke, so we need to save the set of
     * applicable nodes right in the beginning of the processing
     */
    m_strokeData.addClearedNode(node);
}

void KisToolTransform::transformDevices(KisNodeSP node, bool recursive)
{
    if (!node->isEditable()) return;

    KIS_ASSERT_RECOVER_RETURN(recursive ||
                              (m_strokeData.clearedNodes().size() == 1 &&
                               KisNodeSP(m_strokeData.clearedNodes().first()) == node));

    foreach (KisNodeSP currentNode, m_strokeData.clearedNodes()) {
        KIS_ASSERT_RECOVER_RETURN(currentNode);

        image()->addJob(m_strokeData.strokeId(),
                        new TransformStrokeStrategy::TransformData(
                            TransformStrokeStrategy::TransformData::PAINT_DEVICE,
                            m_currentArgs,
                            currentNode));
    }
}

QWidget* KisToolTransform::createOptionWidget() {
    m_optionsWidget = new KisToolTransformConfigWidget(&m_transaction, m_canvas, m_workRecursively, 0);
    Q_CHECK_PTR(m_optionsWidget);
    m_optionsWidget->setObjectName(toolId() + " option widget");

    // See https://bugs.kde.org/show_bug.cgi?id=316896
    QWidget *specialSpacer = new QWidget(m_optionsWidget);
    specialSpacer->setObjectName("SpecialSpacer");
    specialSpacer->setFixedSize(0, 0);
    m_optionsWidget->layout()->addWidget(specialSpacer);


    connect(m_optionsWidget, SIGNAL(sigConfigChanged()),
            this, SLOT(slotUiChangedConfig()));

    connect(m_optionsWidget, SIGNAL(sigApplyTransform()),
            this, SLOT(slotApplyTransform()));

    connect(m_optionsWidget, SIGNAL(sigResetTransform()),
            this, SLOT(slotResetTransform()));

    connect(m_optionsWidget, SIGNAL(sigRestartTransform()),
            this, SLOT(slotRestartTransform()));

    connect(m_optionsWidget, SIGNAL(sigEditingFinished()),
            this, SLOT(slotEditingFinished()));

    updateOptionWidget();

    return m_optionsWidget;
}

void KisToolTransform::updateOptionWidget()
{
    if (!m_optionsWidget) return;

    if (!currentNode()) {
        m_optionsWidget->setEnabled(false);
        return;
    }
    else {
        m_optionsWidget->setEnabled(true);
        m_optionsWidget->updateConfig(m_currentArgs);
    }
}

void KisToolTransform::updateApplyResetAvailability()
{
    if (m_optionsWidget) {
        m_optionsWidget->setApplyResetDisabled(m_currentArgs.isIdentity());
    }
}

void KisToolTransform::slotUiChangedConfig()
{
    if (mode() == KisTool::PAINT_MODE) return;

    m_warpStrategy->externalConfigChanged();
    m_freeStrategy->externalConfigChanged();

    outlineChanged();
    updateApplyResetAvailability();
}

void KisToolTransform::slotApplyTransform()
{
    QApplication::setOverrideCursor(KisCursor::waitCursor());
    endStroke();
    QApplication::restoreOverrideCursor();
}

void KisToolTransform::slotResetTransform()
{
    initTransformMode(m_currentArgs.mode());
    slotEditingFinished();
}

void KisToolTransform::slotRestartTransform()
{
    if (!m_strokeData.strokeId()) return;

    ToolTransformArgs savedArgs(m_currentArgs);
    cancelStroke();
    image()->waitForDone();
    startStroke(savedArgs.mode());
}

void KisToolTransform::slotEditingFinished()
{
    commitChanges();
}

void KisToolTransform::setShearY(double shear)
{
    m_optionsWidget->slotSetShearY(shear);
}

void KisToolTransform::setShearX(double shear)
{
    m_optionsWidget->slotSetShearX(shear);
}

void KisToolTransform::setScaleY(double scale)
{
    m_optionsWidget->slotSetScaleY(scale);
}

void KisToolTransform::setScaleX(double scale)
{
    m_optionsWidget->slotSetScaleX(scale);
}

void KisToolTransform::setTranslateY(double translation)
{
    m_optionsWidget->slotSetTranslateY(translation);
}

void KisToolTransform::setTranslateX(double translation)
{
    m_optionsWidget->slotSetTranslateX(translation);
}

#include "kis_tool_transform.moc"
