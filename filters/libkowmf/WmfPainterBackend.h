/* This file is part of the KDE libraries
 *
 * Copyright (c) 2003 thierry lorthiois (lorthioist@wanadoo.fr)
 *               2009-2011 Inge Wallin <inge@lysator.liu.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
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
#ifndef _KOWMFPAINT_H_
#define _KOWMFPAINT_H_

#include "kowmf_export.h"
#include "WmfAbstractBackend.h"

#include <QPainter>
#include <QTransform>

class QPolygon;

#define DEBUG_WMFPAINT 0

/**
   Namespace for Windows Metafile (WMF) classes
*/
namespace Libwmf
{

class WmfDeviceContext;

/**
 * WmfPainterBackend inherits the abstract class WmfAbstractbackend
 * and redirects WMF actions onto a QPaintDevice.
 * Uses relative or absolute coordinate.
 *
 * how to use:
 * <pre>
 *   QPixmap  pix(100, 100);
 *   QPainter painter(pix);
 *   WmfPainterBackend wmf(painter, pix.size());
 *   if (wmf.load("/home/test.wmf" )) {
 *      wmf.play(pix);
 *   }
 *   paint.drawPixmap(0, 0, pix);
 * </pre>
 *
 */

class LIBWMF_EXPORT WmfPainterBackend : public WmfAbstractBackend
{
public:
    WmfPainterBackend(QPainter *painter, const QSizeF &outputSize);
    ~WmfPainterBackend();

    using WmfAbstractBackend::play;

    /**
     * Play a WMF file on a QPaintDevice. Return true on success.
     */
    //bool play(QPaintDevice& target);
    //bool play(QPainter &painter);
    bool play();


private:
    // -------------------------------------------------------------------------
    // A virtual QPainter
    bool  begin(const QRect &boundingBox);
    bool  end();
    void  save();
    void  restore();

    /// Recalculate the world transform and then apply it to the painter
    /// This must be called at the end of every function that changes the transform.
    void recalculateWorldTransform();

    // Drawing tools
    void  setFont(const QFont& font, int rotation, int height);
    // the pen : the width of the pen is in logical coordinate
    void  setPen(const QPen& pen);
    void  setTextPen(const QPen& pen);
    const QPen& pen() const;
    void  setBrush(const QBrush& brush);

    // Drawing attributes/modes
    void  setBackgroundColor(const QColor& c);
    void  setBackgroundMode(Qt::BGMode mode);
    void  setCompositionMode(QPainter::CompositionMode mode);

    /**
     * Change logical Coordinate
     * some wmf files call those functions several times in the middle of a drawing
     * others wmf files doesn't call setWindow* at all
     * negative width and height are possible
     */
    void  setWindowOrg(int left, int top);
    void  setWindowExt(int width, int height);
    void  setViewportOrg(int left, int top);
    void  setViewportExt(int width, int height);

    // Clipping
    // the 'CoordinateMode' is ommitted : always CoordPainter in wmf
    // setClipRegion() is often used with save() and restore() => implement all or none
    void  setClipRegion(const QRegion &rec);
    QRegion clipRegion();

    // Graphics drawing functions
    void  moveTo(int x, int y);
    void  lineTo(int x, int y);
    void  drawRect(int x, int y, int w, int h);
    void  drawRoundRect(int x, int y, int w, int h, int = 25, int = 25);
    void  drawEllipse(int x, int y, int w, int h);
    void  drawArc(int x, int y, int w, int h, int a, int alen);
    void  drawPie(int x, int y, int w, int h, int a, int alen);
    void  drawChord(int x, int y, int w, int h, int a, int alen);
    void  drawPolyline(const QPolygon& pa);
    void  drawPolygon(const QPolygon& pa, bool winding = false);
    /**
     * drawPolyPolygon draw the XOR of a list of polygons
     * listPa : list of polygons
     */
    void  drawPolyPolygon(QList<QPolygon>& listPa, bool winding = false);
    void  drawImage(int x, int y, const QImage &, int sx = 0, int sy = 0, int sw = -1, int sh = -1);
    void  patBlt(int x, int y, int width, int height, quint32 rasterOperation);

    // Text drawing functions
    // rotation = the degrees of rotation in counterclockwise
    // not yet implemented in KWinMetaFile
    void  drawText(int x, int y, int w, int h, int flags, const QString &s, double rotation);

    // matrix transformation : only used in some bitmap manipulation
    void  setMatrix(const QMatrix &, bool combine = false);

 private:
    void updateFromGraphicscontext(WmfDeviceContext &context);

protected:
    bool  mIsInternalPainter;      // True if the painter wasn't externally provided.
    QPainter *mPainter;
    QSizeF    mOutputSize;
    int       mFontRotation;
    int       mFontHeight;
    QPen  mTextPen;
    QPaintDevice *mTarget;
    bool  mRelativeCoord;
    QPoint mLastPos;

    // Everything that has to do with window and viewport calculation
    QPoint        mWindowOrg;
    QSize         mWindowExt;
    QPoint        mViewportOrg;
    QSize         mViewportExt;
    bool          mWindowExtIsSet;
    bool          mViewportExtIsSet;
    QTransform    mOutputTransform;
    QTransform    mWorldTransform;
    
    int mSaveCount; //number of times Save() was called without Restore()
};


}

#endif
