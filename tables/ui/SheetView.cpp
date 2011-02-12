/* This file is part of the KDE project
   Copyright 2006 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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

// Local
#include "SheetView.h"

#include <QCache>
#include <QRect>
#include <QPainter>
#ifdef CALLIGRA_TABLES_MT
#include <QMutex>
#include <QMutexLocker>
#endif

#include <KoViewConverter.h>

#include "CellView.h"
#include "calligra_tables_limits.h"
#include "Region.h"
#include "RowColumnFormat.h"
#include "RowFormatStorage.h"
#include "Sheet.h"

using namespace Calligra::Tables;

struct CellPaintData
{
    CellPaintData(const CellView &cellView, const Cell &cell, const QPointF &coordinate)
        : cellView(cellView)
          , cell(cell)
          , coordinate(coordinate)
    {}
    CellView cellView;
    Cell cell;
    QPointF coordinate;
};

class SheetView::Private
{
public:
    Private()
#ifdef CALLIGRA_TABLES_MT
        : cacheMutex(QMutex::Recursive)
#endif
    {}
    const Sheet* sheet;
    const KoViewConverter* viewConverter;
    QRect visibleRect;
    QCache<QPoint, CellView> cache;
#ifdef CALLIGRA_TABLES_MT
    QMutex cacheMutex;
#endif
    QRegion cachedArea;
    CellView* defaultCellView;
    // The maximum accessed cell range used for the scrollbar ranges.
    QSize accessedCellRange;

public:
    Cell cellToProcess(int col, int row, QPointF& coordinate, QSet<Cell>& processedMergedCells, const QRect& visRect);
#ifdef CALLIGRA_TABLES_MT
    CellView cellViewToProcess(Cell& cell, QPointF& coordinate, QSet<Cell>& processedObscuredCells,
                               SheetView* sheetView, const QRect& visRect);
#else
    const CellView& cellViewToProcess(Cell& cell, QPointF& coordinate, QSet<Cell>& processedObscuredCells,
                               SheetView* sheetView, const QRect& visRect);
#endif
};

Cell SheetView::Private::cellToProcess(int col, int row, QPointF& coordinate,
                                       QSet<Cell>& processedMergedCells,
                                       const QRect& visRect)
{
    Cell cell(sheet, col, row);
    if (cell.isPartOfMerged()) {
        cell = cell.masterCell();
        // if the rect of visible cells contains this master cell, it was already painted
        if (visRect.contains(cell.cellPosition())) {
            coordinate.setY(coordinate.y() + sheet->rowFormats()->rowHeight(row));
            return Cell(); // next row
        }
        // if the out of bounds master cell was already painted, there's nothing more to do
        if (processedMergedCells.contains(cell)) {
            coordinate.setY(coordinate.y() + sheet->rowFormats()->rowHeight(row));
            return Cell(); // next row
        }
        processedMergedCells.insert(cell);
        // take the coordinate of the master cell
        for (int c = cell.column(); c < col; ++c)
            coordinate.setX(coordinate.x() - sheet->columnFormat(c)->width());
        for (int r = cell.row(); r < row; ++r)
            coordinate.setY(coordinate.y() - sheet->rowFormats()->rowHeight(r));
    }
    return cell;
}

#ifdef CALLIGRA_TABLES_MT
CellView SheetView::Private::cellViewToProcess(Cell& cell, QPointF& coordinate,
        QSet<Cell>& processedObscuredCells, SheetView* sheetView, const QRect& visRect)
#else
const CellView& SheetView::Private::cellViewToProcess(Cell& cell, QPointF& coordinate,
        QSet<Cell>& processedObscuredCells, SheetView* sheetView, const QRect& visRect)
#endif
{
    const int col = cell.column();
    const int row = cell.row();
#ifdef CALLIGRA_TABLES_MT
    CellView cellView = sheetView->cellView(col, row);
#else
    const CellView& cellView = sheetView->cellView(col, row);
#endif
    if (cellView.isObscured()) {
        // if the rect of visible cells contains the obscuring cell, it was already painted
        if (visRect.contains(cellView.obscuringCell())) {
            coordinate.setY(coordinate.y() + sheet->rowFormats()->rowHeight(row));
            cell = Cell();
            return cellView; // next row
        }
        cell = Cell(sheet, cellView.obscuringCell());
        if (processedObscuredCells.contains(cell)) {
            coordinate.setY(coordinate.y() + sheet->rowFormats()->rowHeight(row));
            cell = Cell();
            return cellView; // next row
        }
        processedObscuredCells.insert(cell);
        // take the coordinate of the obscuring cell
        for (int c = cell.column(); c < col; ++c)
            coordinate.setX(coordinate.x() - sheet->columnFormat(c)->width());
        for (int r = cell.row(); r < row; ++r)
            coordinate.setY(coordinate.y() - sheet->rowFormats()->rowHeight(r));
        // use the CellView of the obscuring cell
        return sheetView->cellView(cell.column(), cell.row());
    }
    return cellView;
}


SheetView::SheetView(const Sheet* sheet)
        : QObject(const_cast<Sheet*>(sheet))
        , d(new Private)
{
    d->sheet = sheet;
    d->viewConverter = 0;
    d->visibleRect = QRect(1, 1, 0, 0);
    d->cache.setMaxCost(10000);
    d->defaultCellView = createDefaultCellView();
    d->accessedCellRange =  sheet->usedArea().size().expandedTo(QSize(256, 256));
}

SheetView::~SheetView()
{
    delete d->defaultCellView;
    delete d;
}

const Sheet* SheetView::sheet() const
{
    return d->sheet;
}

void SheetView::setViewConverter(const KoViewConverter* viewConverter)
{
    Q_ASSERT(viewConverter);
    d->viewConverter = viewConverter;
}

const KoViewConverter* SheetView::viewConverter() const
{
    Q_ASSERT(d->viewConverter);
    return d->viewConverter;
}

#ifdef CALLIGRA_TABLES_MT
CellView SheetView::cellView(int col, int row)
#else
const CellView& SheetView::cellView(int col, int row)
#endif
{
    Q_ASSERT(1 <= col && col <= KS_colMax);
    Q_ASSERT(1 <= row && col <= KS_rowMax);
#ifdef CALLIGRA_TABLES_MT
    QMutexLocker ml(&d->cacheMutex);
#endif
    if (!d->cache.contains(QPoint(col, row))) {
        CellView *v = createCellView(col, row);
        d->cache.insert(QPoint(col, row), v);
        d->cachedArea += QRect(col, row, 1, 1);
    }
#ifdef CALLIGRA_TABLES_MT
    CellView v = *d->cache.object(QPoint(col, row));
    return v;
#else
    return *d->cache.object(QPoint(col, row));
#endif
}

void SheetView::setPaintCellRange(const QRect& rect)
{
#ifdef CALLIGRA_TABLES_MT
    QMutexLocker ml(&d->cacheMutex);
#endif
    d->visibleRect = rect & QRect(1, 1, KS_colMax, KS_rowMax);
    d->cache.setMaxCost(2 * rect.width() * rect.height());
}

QRect SheetView::paintCellRange() const
{
    return d->visibleRect;
}

void SheetView::invalidateRegion(const Region& region)
{
    QRegion qregion;
    Region::ConstIterator end(region.constEnd());
    for (Region::ConstIterator it(region.constBegin()); it != end; ++it) {
        qregion += (*it)->rect();
    }
    // reduce to the cached area
    qregion &= d->cachedArea;
    QVector<QRect> rects = qregion.rects();
    for (int i = 0; i < rects.count(); ++i)
        invalidateRange(rects[i]);
}

void SheetView::invalidate()
{
#ifdef CALLIGRA_TABLES_MT
    QMutexLocker ml(&d->cacheMutex);
#endif
    delete d->defaultCellView;
    d->defaultCellView = createDefaultCellView();
    d->cache.clear();
    d->cachedArea = QRegion();
}

void SheetView::paintCells(QPainter& painter, const QRectF& paintRect, const QPointF& topLeft, CanvasBase*, const QRect& visibleRect)
{
    const QRect& visRect = visibleRect.isValid() ? visibleRect : d->visibleRect;
    // paintRect:   the canvas area, that should be painted; in document coordinates;
    //              no layout direction consideration; scrolling offset applied;
    //              independent from painter transformations
    // topLeft:     the document coordinate of the top left cell's top left corner;
    //              no layout direction consideration; independent from painter
    //              transformations

    // NOTE Stefan: The painting is splitted into several steps. In each of these all cells in
    //              d->visibleRect are traversed. This may appear suboptimal at the first look, but
    //              ensures that the borders are not erased by the background of adjacent cells.

// kDebug() << "paintRect:" << paintRect;
// kDebug() << "topLeft:" << topLeft;

    QRegion clipRect(painter.clipRegion());
    // 0. Paint the sheet background
    if (!sheet()->backgroundImage().isNull()) {
        //TODO support all the different properties
        Sheet::BackgroundImageProperties properties = sheet()->backgroundImageProperties();
        if( properties.repeat == Sheet::BackgroundImageProperties::Repeat ) {
            const int firstCol = visRect.left();
            const int firstRow = visRect.top();
            const int firstColPosition = d->sheet->columnPosition(firstCol);
            const int firstRowPosition = d->sheet->rowPosition(firstRow);

            const int imageWidth = sheet()->backgroundImage().rect().width();
            const int imageHeight = sheet()->backgroundImage().rect().height();

            int xBackground = firstColPosition - (firstColPosition % imageWidth);
            int yBackground = firstRowPosition - (firstRowPosition % imageHeight);

            const int lastCol = visRect.right();
            const int lastRow = visRect.bottom();
            const int lastColPosition = d->sheet->columnPosition(lastCol);
            const int lastRowPosition = d->sheet->rowPosition(lastRow);

            while( xBackground < lastColPosition ) {
                int y = yBackground;
                while( y < lastRowPosition ) {
                    painter.drawImage(QRect(xBackground, y, imageWidth, imageHeight), sheet()->backgroundImage());
                    y += imageHeight;
                }
                xBackground += imageWidth;
            }
        }
    }

    // 1. Paint the cell background

    // Handle right-to-left layout.
    // In an RTL sheet the cells have to be painted at their opposite horizontal
    // location on the canvas, meaning that column A will be the rightmost column
    // on screen, column B will be to the left of it and so on. Here we change
    // the horizontal coordinate at which we start painting the cell in case the
    // sheet's direction is RTL.
    const bool rightToLeft = sheet()->layoutDirection() == Qt::RightToLeft;
    const QPointF startCoordinate(rightToLeft ? paintRect.width() - topLeft.x() : topLeft.x(), topLeft.y());
    QPointF coordinate(startCoordinate);
// kDebug() << "start coordinate:" << coordinate;
    QSet<Cell> processedMergedCells;
    QSet<Cell> processedObscuredCells;
    QList<CellPaintData> cached_cells;
    for (int col = visRect.left(); col <= visRect.right(); ++col) {
        if (d->sheet->columnFormat(col)->isHiddenOrFiltered())
            continue;
        if (rightToLeft)
            coordinate.setX(coordinate.x() - d->sheet->columnFormat(col)->width());
// kDebug() <<"coordinate:" << coordinate;
        for (int row = visRect.top(); row <= visRect.bottom(); ++row) {
            int lastHiddenRow;
            if (d->sheet->rowFormats()->isHiddenOrFiltered(row, &lastHiddenRow)) {
                row = lastHiddenRow;
                continue;
            }
            // save the coordinate
            const QPointF savedCoordinate = coordinate;
            // figure out, if any and which cell has to be painted (may be a master cell)
            Cell cell = d->cellToProcess(col, row, coordinate, processedMergedCells, visRect);
            if (!cell)
                continue;
            // figure out, which CellView to use (may be one for an obscuring cell)
            CellPaintData cpd(d->cellViewToProcess(cell, coordinate, processedObscuredCells, this, visRect), cell, coordinate);
            if (!cell)
                continue;
            cpd.cellView.paintCellBackground(painter, clipRect, coordinate);
            cached_cells.append(cpd);
            // restore coordinate
            coordinate = savedCoordinate;
            coordinate.setY(coordinate.y() + d->sheet->rowFormats()->rowHeight(row));
        }
        coordinate.setY(topLeft.y());
        if (!rightToLeft)
            coordinate.setX(coordinate.x() + d->sheet->columnFormat(col)->width());
    }

    // 2. Paint the cell content including markers (formula, comment, ...)
    for (QList<CellPaintData>::iterator it(cached_cells.begin()); it != cached_cells.end(); ++it) {
        it->cellView.paintCellContents(paintRect, painter, clipRect, it->coordinate, it->cell, this);
    }

    // 3. Paint the default borders
    coordinate = startCoordinate;
    for (int col = visRect.left(); col <= visRect.right(); ++col) {
        if (d->sheet->columnFormat(col)->isHiddenOrFiltered())
            continue;
        if (rightToLeft)
            coordinate.setX(coordinate.x() - d->sheet->columnFormat(col)->width());
        for (int row = visRect.top(); row <= visRect.bottom(); ++row) {
            int lastHiddenRow;
            if (d->sheet->rowFormats()->isHiddenOrFiltered(row, &lastHiddenRow)) {
                row = lastHiddenRow;
                continue;
            }
            // For borders even cells, that are merged in, need to be traversed.
            // Think of a merged cell with a set border and one its neighbours has a thicker border.
            const CellView cellView = this->cellView(col, row);
            cellView.paintDefaultBorders(painter, clipRect, paintRect, coordinate,
                                         CellView::LeftBorder | CellView::RightBorder |
                                         CellView::TopBorder | CellView::BottomBorder,
                                         visRect, Cell(d->sheet, col, row), this);
            coordinate.setY(coordinate.y() + d->sheet->rowFormats()->rowHeight(row));
        }
        coordinate.setY(topLeft.y());
        if (!rightToLeft)
            coordinate.setX(coordinate.x() + d->sheet->columnFormat(col)->width());
    }

    // 4. Paint the custom borders, diagonal lines and page borders
    coordinate = startCoordinate;
    for (int col = visRect.left(); col <= visRect.right(); ++col) {
        if (d->sheet->columnFormat(col)->isHiddenOrFiltered())
            continue;
        if (rightToLeft)
            coordinate.setX(coordinate.x() - d->sheet->columnFormat(col)->width());
        for (int row = visRect.top(); row <= visRect.bottom(); ++row) {
            int lastHiddenRow;
            if (d->sheet->rowFormats()->isHiddenOrFiltered(row, &lastHiddenRow)) {
                row = lastHiddenRow;
                continue;
            }
            // For borders even cells, that are merged in, need to be traversed.
            // Think of a merged cell with a set border and one its neighbours has a thicker border.
            const CellView cellView = this->cellView(col, row);
            cellView.paintCellBorders(paintRect, painter, clipRect, coordinate,
                                      visRect,
                                      Cell(sheet(), col, row), this);
            coordinate.setY(coordinate.y() + d->sheet->rowFormats()->rowHeight(row));
        }
        coordinate.setY(topLeft.y());
        if (!rightToLeft)
            coordinate.setX(coordinate.x() + d->sheet->columnFormat(col)->width());
    }
}

void SheetView::invalidateRange(const QRect& range)
{
#ifdef CALLIGRA_TABLES_MT
    QMutexLocker ml(&d->cacheMutex);
#endif
    const int right  = range.right();
    for (int col = range.left(); col <= right; ++col) {
        const int bottom = range.bottom();
        for (int row = range.top(); row <= bottom; ++row) {
            if (!d->cache.contains(QPoint(col, row)))
                continue;
            const CellView& cellView = this->cellView(col, row);
            if (cellView.obscuresCells()) {
                // First, delete the obscuring CellView; otherwise: recursion.
                d->cache.remove(QPoint(col, row));
                // Remove the obscured range.
                invalidateRange(QRect(range.topLeft(), cellView.obscuredRange() + QSize(1, 1)));
                continue; // already removed from cache
            } else if (cellView.isObscured())
                if (!range.contains(cellView.obscuringCell()))
                    invalidateRange(QRect(cellView.obscuringCell(), QSize(1, 1)));
            d->cache.remove(QPoint(col, row));
        }
    }
    d->cachedArea -= range;
}

void SheetView::obscureCells(const QRect& range, const QPoint& position)
{
#ifdef CALLIGRA_TABLES_MT
    QMutexLocker ml(&d->cacheMutex);
#endif
    const int right = range.right();
    const int bottom = range.bottom();
    for (int col = range.left(); col <= right; ++col) {
        for (int row = range.top(); row <= bottom; ++row) {
            // create the CellView, but do not use the returned CellView. It is shared!
            cellView(col, row);
            // alter the CellView directly instead
            d->cache.object(QPoint(col, row))->obscure(position.x(), position.y());
        }
    }
}

#ifdef CALLIGRA_TABLES_MT
CellView SheetView::defaultCellView() const
#else
const CellView& SheetView::defaultCellView() const
#endif
{
    return *d->defaultCellView;
}

void SheetView::updateAccessedCellRange(const QPoint& location)
{
    const QSize cellRange = d->accessedCellRange.expandedTo(QSize(location.x(), location.y()));
    if (d->accessedCellRange != cellRange || location.isNull()) {
        d->accessedCellRange = cellRange;
        const int col = qMin(KS_colMax, cellRange.width() + 10);
        const int row = qMin(KS_rowMax, cellRange.height() + 10);
        const double width = sheet()->columnPosition(col) + sheet()->columnFormat(col)->width();
        const double height = sheet()->rowPosition(row) + sheet()->rowFormats()->rowHeight(row);
        emit visibleSizeChanged(QSizeF(width, height));
    }
}

CellView* SheetView::createDefaultCellView()
{
    return new CellView(this);
}

CellView* SheetView::createCellView(int col, int row)
{
    return new CellView(this, col, row);
}

#include "SheetView.moc"
