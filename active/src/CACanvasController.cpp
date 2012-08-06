/*
 * This file is part of the KDE project
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Boudewijn Rempt <boud@kogmbh.com>
 * Copyright (C) 2010-2011 Jarosław Staniek <staniek@kde.org>
 * Copyright (C) 2011 Shantanu Tushar <shaan7in@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#include "CACanvasController.h"
#include "CACanvasItem.h"

#include <KoCanvasBase.h>
#include <KoShape.h>
#include <KoZoomController.h>
#include <KoZoomHandler.h>

#include <KDE/KActionCollection>
#include <KDebug>

#include <QPoint>
#include <QSize>
#include <QGraphicsWidget>

CACanvasController::CACanvasController (QDeclarativeItem* parent)
    : QDeclarativeItem (parent), KoCanvasController (0), m_zoomHandler (0), m_zoomController (0),
      m_canvas (0), m_currentPoint (QPoint (0, 0)), m_documentSize (QSizeF (0, 0))
{
    setFlag (QGraphicsItem::ItemHasNoContents, false);
    setClip (true);
}

void CACanvasController::setVastScrolling (qreal factor)
{
    //kDebug() << factor;
}

void CACanvasController::setZoomWithWheel (bool zoom)
{
    //kDebug() << zoom;
}

void CACanvasController::updateDocumentSize (const QSize& sz, bool recalculateCenter)
{
    m_caCanvasItem->updateDocumentSize(sz, recalculateCenter);
    KoCanvasController::setDocumentSize(sz);

    return;
    m_documentSize = sz;
    emit docHeightChanged();
    emit docWidthChanged();
}

void CACanvasController::setScrollBarValue (const QPoint& value)
{
    //kDebug() << value;
}

QPoint CACanvasController::scrollBarValue() const
{
    return QPoint();
}

void CACanvasController::pan (const QPoint& distance)
{
    //kDebug() << distance;
}

QPoint CACanvasController::preferredCenter() const
{
    return QPoint();
}

void CACanvasController::setPreferredCenter (const QPoint& viewPoint)
{
    //kDebug() << viewPoint;
}

void CACanvasController::recenterPreferred()
{
}

void CACanvasController::zoomTo (const QRect& rect)
{
    //kDebug() << rect;
}

void CACanvasController::zoomBy (const QPoint& center, qreal zoom)
{
    proxyObject->emitZoomBy(zoom);
    canvas()->canvasItem()->update();
}

void CACanvasController::zoomOut (const QPoint& center)
{
    //kDebug() << center;
}

void CACanvasController::zoomIn (const QPoint& center)
{
    //kDebug() << center;
}

void CACanvasController::ensureVisible (KoShape* shape)
{
    ensureVisible(shape->boundingRect(), true);
}

void CACanvasController::ensureVisible (const QRectF& rect, bool smooth)
{
    setCameraX(rect.center().x());
    setCameraY(rect.center().y());
}

int CACanvasController::canvasOffsetY() const
{
    return 0;
}

int CACanvasController::canvasOffsetX() const
{
    return 0;
}

int CACanvasController::visibleWidth() const
{
    return 0;
}

int CACanvasController::visibleHeight() const
{
    return 0;
}

KoCanvasBase* CACanvasController::canvas() const
{
    return m_caCanvasItem->koCanvas();
}

KoCanvasControllerProxyObject* CACanvasController::canvasControllerProxyObject()
{
    return proxyObject;
}

QObject* CACanvasController::caCanvasItem()
{
    return dynamic_cast<QObject*>(m_caCanvasItem);
}

void CACanvasController::setCACanvasItem(QObject* caCanvas)
{
    m_caCanvasItem = static_cast<CACanvasItem*>(caCanvas);
}

void CACanvasController::setCanvas (KoCanvasBase* canvas)
{
//     m_canvas = canvas;
    canvas->setCanvasController(this);
    m_caCanvasItem->setKoCanvas(canvas);
    emit caCanvasItemChanged();
//     QGraphicsWidget* widget = canvas->canvasItem();
//     widget->setParentItem (this);
//     canvas->setCanvasController (this);
//     widget->setVisible (true);
//     m_canvas = canvas;
// 
//     zoomToFit();
}

void CACanvasController::setDrawShadow (bool drawShadow)
{
    //kDebug() << drawShadow;
}

QSize CACanvasController::viewportSize() const
{
    return QSize();
}

void CACanvasController::scrollContentsBy (int dx, int dy)
{
    //kDebug() << dx << dy;
}

qreal CACanvasController::docHeight() const
{
    return m_documentSize.height();
}

qreal CACanvasController::docWidth() const
{
    return m_documentSize.width();
}

int CACanvasController::cameraX() const
{
    return m_currentPoint.x();
}

int CACanvasController::cameraY() const
{
    return m_currentPoint.y();
}

void CACanvasController::setCameraX (int cameraX)
{
    m_currentPoint.setX (cameraX);
    emit cameraXChanged();
    centerToCamera();
}

void CACanvasController::setCameraY (int cameraY)
{
    m_currentPoint.setY (cameraY);
    emit cameraYChanged();
    centerToCamera();
}

void CACanvasController::centerToCamera()
{
    return;
    if (proxyObject) {
        proxyObject->emitMoveDocumentOffset (m_currentPoint);
    }
    updateCanvas();
}

CACanvasController::~CACanvasController()
{
}

void CACanvasController::zoomToFit()
{
    return;
    emit needsCanvasResize(QSizeF(width(), height()));
    emit docHeightChanged();
    emit docWidthChanged();
}

void CACanvasController::updateCanvas()
{
    return;
    emit needCanvasUpdate();
}

void CACanvasController::geometryChanged (const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QDeclarativeItem::geometryChanged (newGeometry, oldGeometry);
    return;
    if (m_canvas) {
        QGraphicsWidget* widget = m_canvas->canvasItem();
        widget->setParentItem (this);
        widget->setVisible (true);
        widget->setGeometry (newGeometry);

        zoomToFit();
    }
}

KoZoomController* CACanvasController::zoomController()
{
    return m_zoomController;
}

KoZoomHandler* CACanvasController::zoomHandler()
{
    return m_zoomHandler;
}

void CACanvasController::setZoomHandler (KoZoomHandler* zoomHandler)
{
    if (!m_zoomController) {
        m_zoomHandler = zoomHandler;
        m_zoomController = new KoZoomController(this, zoomHandler, new KActionCollection(this));
    }
}

void CACanvasController::setZoom(qreal zoom)
{
    m_zoomController->setZoom(KoZoomMode::ZOOM_CONSTANT, zoom);
    emit zoomChanged();
}

qreal CACanvasController::zoom() const
{
    return m_zoom;
}

void CACanvasController::updateZoomValue(KoZoomMode::Mode mode, qreal zoom)
{
    m_zoom = zoom;
}

#include "CACanvasController.moc"

