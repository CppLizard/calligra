/* This file is part of the KDE project
   Copyright 2010 Marijn Kruisselbrink <mkruisselbrink@kde.org>

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
   Boston, MA 02110-1301, USA.
*/

#include "PixmapCachingSheetView.h"

#include "CellView.h"

#include "../Sheet.h"
#include "../part/CanvasBase.h"

#include <QCache>
#include <QPainter>

#include <kdebug.h>

#ifdef CALLIGRA_TABLES_MT
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Weaver>
#endif

using namespace Calligra::Tables;

#define TILESIZE 256

class PixmapCachingSheetView::Private
{
public:
    Private(PixmapCachingSheetView* q) : q(q) {}
    PixmapCachingSheetView* q;
    QCache<int, QPixmap> tileCache;
    QPointF lastScale;

    QPixmap* getTile(const Sheet* sheet, int x, int y, CanvasBase* canvas);
};

#ifdef CALLIGRA_TABLES_MT
class TileDrawingJob : public ThreadWeaver::Job
{
public:
    TileDrawingJob(const Sheet* sheet, SheetView* sheetView, CanvasBase* canvas, const QPointF& scale, int x, int y);
    ~TileDrawingJob();
protected:
    virtual void run();
private:
    const Sheet* m_sheet;
    SheetView* m_sheetView;
public:
    CanvasBase* m_canvas;
    QPointF m_scale;
    int m_x;
    int m_y;
    QImage m_image;
};

TileDrawingJob::TileDrawingJob(const Sheet *sheet, SheetView* sheetView, CanvasBase* canvas, const QPointF& scale, int x, int y)
    : m_sheet(sheet), m_sheetView(sheetView), m_canvas(canvas), m_scale(scale), m_x(x), m_y(y)
    , m_image(TILESIZE, TILESIZE, QImage::Format_ARGB32)
{
    kDebug() << "new job for " << x << "," << y << " " << m_scale;
}

TileDrawingJob::~TileDrawingJob()
{
    kDebug() << "end job for " << m_x << "," << m_y << " " << m_scale;
}

void TileDrawingJob::run()
{
    kDebug() << "start draw for " << m_x << "," << m_y << " " << m_scale;
    m_image.fill(QColor(255, 255, 255, 0).rgba());
    QPainter pixmapPainter(&m_image);
    pixmapPainter.setClipRect(m_image.rect());
    pixmapPainter.scale(m_scale.x(), m_scale.y());

    QRect globalPixelRect(QPoint(m_x * TILESIZE, m_y * TILESIZE), QSize(TILESIZE, TILESIZE));
    QRectF docRect(
            globalPixelRect.x() / m_scale.x(),
            globalPixelRect.y() / m_scale.y(),
            globalPixelRect.width() / m_scale.x(),
            globalPixelRect.height() / m_scale.y()
    );

    pixmapPainter.translate(-docRect.x(), -docRect.y());

    double loffset, toffset;
    const int left = m_sheet->leftColumn(docRect.left(), loffset);
    const int right = m_sheet->rightColumn(docRect.right());
    const int top = m_sheet->topRow(docRect.top(), toffset);
    const int bottom = m_sheet->bottomRow(docRect.bottom());
    QRect cellRect(left, top, right - left + 1, bottom - top + 1);

    kDebug() << globalPixelRect << docRect;
    kDebug() << cellRect;

    // TODO
    m_sheetView->SheetView::paintCells(pixmapPainter, docRect, QPointF(loffset, toffset), 0, cellRect);

    //m_image.save(QString("/tmp/tile%1_%2.png").arg(m_x).arg(m_y));
    kDebug() << "end draw for " << m_x << "," << m_y << " " << m_scale;
}
#endif

PixmapCachingSheetView::PixmapCachingSheetView(const Sheet* sheet)
    : SheetView(sheet), d(new Private(this))
{
    d->tileCache.setMaxCost(128); // number of tiles to cache
}

PixmapCachingSheetView::~PixmapCachingSheetView()
{
    delete d;
}

void PixmapCachingSheetView::jobDone(ThreadWeaver::Job *tjob)
{
#ifdef CALLIGRA_TABLES_MT
    TileDrawingJob* job = static_cast<TileDrawingJob*>(tjob);
    if (job->m_scale == d->lastScale) {
        int idx = job->m_x << 16 | job->m_y;
        d->tileCache.insert(idx, new QPixmap(QPixmap::fromImage(job->m_image)));
        // TODO: figure out what area to repaint
        job->m_canvas->update();
    }
    job->deleteLater();
#endif
}

QPixmap* PixmapCachingSheetView::Private::getTile(const Sheet* sheet, int x, int y, CanvasBase* canvas)
{
    int idx = x << 16 | y;
    if (tileCache.contains(idx)) return tileCache.object(idx);

#ifdef CALLIGRA_TABLES_MT
    TileDrawingJob* job = new TileDrawingJob(sheet, q, canvas, lastScale, x, y);
    QObject::connect(job, SIGNAL(done(ThreadWeaver::Job*)), q, SLOT(jobDone(ThreadWeaver::Job*)), Qt::QueuedConnection);
    ThreadWeaver::Weaver::instance()->enqueue(job);
    QPixmap* pm = new QPixmap(TILESIZE, TILESIZE);
    pm->fill(QColor(255, 255, 255, 0));
    tileCache.insert(idx, pm);
    return pm;
#else
    QPixmap* pm = new QPixmap(TILESIZE, TILESIZE);
    pm->fill(QColor(255, 255, 255, 0));
    QPainter pixmapPainter(pm);
    pixmapPainter.setClipRect(pm->rect());
    pixmapPainter.scale(lastScale.x(), lastScale.y());

    QRect globalPixelRect(QPoint(x * TILESIZE, y * TILESIZE), QSize(TILESIZE, TILESIZE));
    QRectF docRect(
            globalPixelRect.x() / lastScale.x(),
            globalPixelRect.y() / lastScale.y(),
            globalPixelRect.width() / lastScale.x(),
            globalPixelRect.height() / lastScale.y()
    );

    pixmapPainter.translate(-docRect.x(), -docRect.y());

    double loffset, toffset;
    const int left = sheet->leftColumn(docRect.left(), loffset);
    const int right = sheet->rightColumn(docRect.right());
    const int top = sheet->topRow(docRect.top(), toffset);
    const int bottom = sheet->bottomRow(docRect.bottom());
    QRect cellRect(left, top, right - left + 1, bottom - top + 1);

    kDebug() << globalPixelRect << docRect;
    kDebug() << cellRect;

    q->SheetView::paintCells(pixmapPainter, docRect, QPointF(loffset, toffset), 0, cellRect);
    pm->save(QString("/tmp/tile%1.png").arg(idx));
    tileCache.insert(idx, pm);
    return pm;
#endif
}

void PixmapCachingSheetView::paintCells(QPainter& painter, const QRectF& paintRect, const QPointF& topLeft, CanvasBase* canvas, const QRect& visibleRect)
{
    if (!canvas) {
        SheetView::paintCells(painter, paintRect, topLeft, canvas, visibleRect);
        return;
    }
    // paintRect:   the canvas area, that should be painted; in document coordinates;
    //              no layout direction consideration; scrolling offset applied;
    //              independent from painter transformations
    // topLeft:     the document coordinate of the top left cell's top left corner;
    //              no layout direction consideration; independent from painter
    //              transformations

    QTransform t = painter.transform();

    // figure out scaling from the transformation... not really perfect, but should work as long as rotation is in 90 degree steps I think
    const qreal cos_sx = t.m11();
    const qreal sin_sx = t.m12();
    const qreal msin_sy = t.m21();
    const qreal cos_sy = t.m22();

    const qreal sx = sqrt(cos_sx*cos_sx + sin_sx*sin_sx);
    const qreal sy = sqrt(cos_sy*cos_sy + msin_sy*msin_sy);
    //const qreal cost = (sx > 1e-10 ? cos_sx / sx : cos_sy / sy);
    //const qreal ang = acos(cost);

    QPointF scale = QPointF(sx, sy);
    if (scale != d->lastScale) {
        d->tileCache.clear();
    }
    d->lastScale = scale;

    QRect tiles;
    const QRect visibleCells = paintCellRange();
    const Sheet * s = sheet();
    const QPointF bottomRight(s->columnPosition(visibleCells.right() + 1), s->rowPosition(visibleCells.bottom() + 1));
    tiles.setLeft(topLeft.x() * sx / TILESIZE);
    tiles.setTop(topLeft.y() * sy / TILESIZE);
    tiles.setRight((bottomRight.x() * sx + TILESIZE-1) / TILESIZE);
    tiles.setBottom((bottomRight.y() * sy + TILESIZE-1) / TILESIZE);

    for (int x = qMax(0, tiles.left()); x < tiles.right(); x++) {
        for (int y = qMax(0, tiles.top()); y < tiles.bottom(); y++) {
            QPixmap *p = d->getTile(s, x, y, canvas);
            QPointF pt(x * TILESIZE / scale.x(), y * TILESIZE / scale.y());
            QRectF r(pt, QSizeF(TILESIZE / sx, TILESIZE / sy));
            painter.drawPixmap(r, *p, p->rect());
        }
    }
}

void PixmapCachingSheetView::invalidateRegion(const Region &region)
{
    // TODO: figure out which tiles to invalidate
    d->tileCache.clear();

    SheetView::invalidateRegion(region);
}

void PixmapCachingSheetView::invalidate()
{
    d->tileCache.clear();

    SheetView::invalidate();
}

#include "PixmapCachingSheetView.moc"
