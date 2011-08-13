/*
  Copyright 2008      Brad Hards  <bradh@frogmouth.net>
  Copyright 2009-2010 Inge Wallin <inge@lysator.liu.se>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "EmfDebugBackend.h"

#include <math.h>

#include <KDebug>

#include "EmfDeviceContext.h"
#include "EmfObjects.h"

namespace Libemf
{




EmfDebugBackend::EmfDebugBackend()
{
}

EmfDebugBackend::~EmfDebugBackend()
{
}

void EmfDebugBackend::init( const Header *header )
{
    kDebug(33100) << "Initialising EmfDebugBackend";
    kDebug(33100) << "image size:" << header->bounds().size();
}

void EmfDebugBackend::cleanup( const Header *header )
{
    kDebug(33100) << "Cleanup EmfDebugBackend";
    kDebug(33100) << "image size:" << header->bounds().size();
}

void EmfDebugBackend::eof()
{
    kDebug(33100) << "EMR_EOF";
}

void EmfDebugBackend::setPixelV(EmfDeviceContext &context,
                                    QPoint &point, quint8 red, quint8 green, quint8 blue, quint8 reserved )
{
    Q_UNUSED( reserved );
    kDebug(33100) << "EMR_SETPIXELV:" << point << QColor( red, green, blue );
}

void EmfDebugBackend::beginPath(EmfDeviceContext &context)
{
    kDebug(33100) << "EMR_BEGINPATH";
}

void EmfDebugBackend::closeFigure(EmfDeviceContext &context)
{
    kDebug(33100) << "EMR_CLOSEFIGURE";
}

void EmfDebugBackend::endPath(EmfDeviceContext &context)
{
    kDebug(33100) << "EMR_ENDPATH";
}

void EmfDebugBackend::saveDC(EmfDeviceContext &context)
{
    kDebug(33100) << "EMR_SAVEDC";
}

void EmfDebugBackend::restoreDC(EmfDeviceContext &context, qint32 savedDC )
{
    kDebug(33100) << "EMR_RESTOREDC" << savedDC;
}

void EmfDebugBackend::setMetaRgn(EmfDeviceContext &context)
{
    kDebug(33100) << "EMR_SETMETARGN";
}

void EmfDebugBackend::setWindowOrgEx(EmfDeviceContext &context, const QPoint &origin )
{
    kDebug(33100) << "EMR_SETWINDOWORGEX" << origin;
}

void EmfDebugBackend::setWindowExtEx(EmfDeviceContext &context, const QSize &size )
{
    kDebug(33100) << "EMR_SETWINDOWEXTEX" << size;
}

void EmfDebugBackend::setViewportOrgEx(EmfDeviceContext &context, const QPoint &origin )
{
    kDebug(33100) << "EMR_SETVIEWPORTORGEX" << origin;
}

void EmfDebugBackend::setViewportExtEx(EmfDeviceContext &context, const QSize &size )
{
    kDebug(33100) << "EMR_SETVIEWPORTEXTEX" << size;
}

void EmfDebugBackend::deleteObject(EmfDeviceContext &context, const quint32 ihObject )
{
    kDebug(33100) << "EMR_DELETEOBJECT:" << ihObject;
}

void EmfDebugBackend::arc(EmfDeviceContext &context, const QRect &box, const QPoint &start, const QPoint &end )
{
    kDebug(33100) << "EMR_ARC" << box << start << end;
}

void EmfDebugBackend::chord(EmfDeviceContext &context, const QRect &box, const QPoint &start, const QPoint &end )
{
    kDebug(33100) << "EMR_CHORD" << box << start << end;
}

void EmfDebugBackend::pie(EmfDeviceContext &context, const QRect &box, const QPoint &start, const QPoint &end )
{
    kDebug(33100) << "EMR_PIE" << box << start << end;
}

void EmfDebugBackend::ellipse(EmfDeviceContext &context, const QRect &box )
{
    kDebug(33100) << "EMR_ELLIPSE:" << box;
}

void EmfDebugBackend::rectangle(EmfDeviceContext &context, const QRect &box )
{
    kDebug(33100) << "EMR_RECTANGLE:" << box;
}

void EmfDebugBackend::modifyWorldTransform(EmfDeviceContext &context, quint32 mode, float M11, float M12,
					float M21, float M22, float Dx, float Dy )
{
    kDebug(33100) << "EMR_MODIFYWORLDTRANSFORM:" << mode << QTransform ( M11, M12, M21, M22, Dx, Dy );
}

void EmfDebugBackend::setWorldTransform(EmfDeviceContext &context, float M11, float M12, float M21,
				     float M22, float Dx, float Dy )
{
    kDebug(33100) << "EMR_SETWORLDTRANSFORM:" << QTransform ( M11, M12, M21, M22, Dx, Dy );
}

void EmfDebugBackend::setMapMode(EmfDeviceContext &context, quint32 mapMode )
{
    QString modeAsText;
    switch ( mapMode ) {
    case MM_TEXT:
	modeAsText = QString( "map mode - text" );
	break;
    case MM_LOMETRIC:
	modeAsText = QString( "map mode - lometric" );
	break;
    case MM_HIMETRIC:
	modeAsText = QString( "map mode - himetric" );
	break;
    case MM_LOENGLISH:
	modeAsText = QString( "map mode - loenglish" );
	break;
    case MM_HIENGLISH:
	modeAsText = QString( "map mode - hienglish" );
	break;
    case MM_TWIPS:
	modeAsText = QString( "map mode - twips" );
	break;
    case MM_ISOTROPIC:
	modeAsText = QString( "map mode - isotropic" );
	break;
    case MM_ANISOTROPIC:
	modeAsText = QString( "map mode - anisotropic" );
	break;
    default:
	modeAsText = QString( "unexpected map mode: %1").arg( mapMode );
    }
    kDebug(33100) << "EMR_SETMAPMODE:" << modeAsText;

}

void EmfDebugBackend::setBkMode(EmfDeviceContext &context, const quint32 backgroundMode )
{
    if ( backgroundMode == TRANSPARENT ) {
        kDebug(33100) << "EMR_SETBKMODE: Transparent";
    } else if ( backgroundMode == OPAQUE ) {
        kDebug(33100) << "EMR_SETBKMODE: Opaque";
    } else {
        kDebug(33100) << "EMR_SETBKMODE: Unexpected value -" << backgroundMode;
        Q_ASSERT( 0 );
    }
}

void EmfDebugBackend::setPolyFillMode(EmfDeviceContext &context, const quint32 polyFillMode )
{
    if ( polyFillMode == ALTERNATE ) {
	kDebug(33100) << "EMR_SETPOLYFILLMODE: OddEvenFill";
    } else if ( polyFillMode == WINDING ) {
	kDebug(33100) << "EMR_SETPOLYFILLMODE: WindingFill";
    } else {
	kDebug(33100) << "EMR_SETPOLYFILLMODE: Unexpected value -" << polyFillMode;
	Q_ASSERT( 0 );
    }
}

void EmfDebugBackend::setLayout(EmfDeviceContext &context, const quint32 layoutMode )
{
    kDebug(33100) << "EMR_SETLAYOUT:" << layoutMode;
}

void EmfDebugBackend::extCreateFontIndirectW(EmfDeviceContext &context, const ExtCreateFontIndirectWRecord &extCreateFontIndirectW )
{
    kDebug(33100) << "EMR_CREATEFONTINDIRECTW:" << extCreateFontIndirectW.fontFace();
}

void EmfDebugBackend::setTextAlign(EmfDeviceContext &context, const quint32 textAlignMode )
{
    kDebug(33100) << "EMR_SETTEXTALIGN:" << textAlignMode;
}

void EmfDebugBackend::setBkColor(EmfDeviceContext &context, const quint8 red, const quint8 green, const quint8 blue,
                              const quint8 reserved )
{
    Q_UNUSED( reserved );
    kDebug(33100) << "EMR_SETBKCOLOR" << QColor( red, green, blue );
}

void EmfDebugBackend::createPen(EmfDeviceContext &context, quint32 ihPen, quint32 penStyle, quint32 x, quint32 y,
			       quint8 red, quint8 green, quint8 blue, quint8 reserved )
{
    Q_UNUSED( y );
    Q_UNUSED( reserved );

    kDebug(33100) << "EMR_CREATEPEN" << "ihPen:" << ihPen << ", penStyle:" << penStyle
                  << "width:" << x << "color:" << QColor( red, green, blue );
}

void EmfDebugBackend::createBrushIndirect(EmfDeviceContext &context, quint32 ihBrush, quint32 BrushStyle, quint8 red,
				       quint8 green, quint8 blue, quint8 reserved,
				       quint32 BrushHatch )
{
    Q_UNUSED( reserved );

    kDebug(33100) << "EMR_CREATEBRUSHINDIRECT:" << ihBrush << "style:" << BrushStyle
             << "Colour:" << QColor( red, green, blue ) << ", Hatch:" << BrushHatch;
}

void EmfDebugBackend::createMonoBrush(EmfDeviceContext &context, quint32 ihBrush, Bitmap *bitmap )
{
    kDebug(33100) << "EMR_CREATEMONOBRUSH:" << ihBrush << "bitmap:" << bitmap;
}

void EmfDebugBackend::selectObject(EmfDeviceContext &context, const quint32 ihObject )
{
    kDebug(33100) << "EMR_SELECTOBJECT" << ihObject;
}

void EmfDebugBackend::extTextOut(EmfDeviceContext &context, const QRect &bounds, const EmrTextObject &textObject )
{
    kDebug(33100) << "EMR_EXTTEXTOUTW:" << bounds
                  << textObject.referencePoint()
                  << textObject.textString();
}

void EmfDebugBackend::moveToEx(EmfDeviceContext &context, const qint32 x, const qint32 y )
{
    kDebug(33100) << "EMR_MOVETOEX" << QPoint( x, y );
}

void EmfDebugBackend::lineTo(EmfDeviceContext &context, const QPoint &finishPoint )
{
    kDebug(33100) << "EMR_LINETO" << finishPoint;
}

void EmfDebugBackend::arcTo(EmfDeviceContext &context, const QRect &box, const QPoint &start, const QPoint &end )
{
    kDebug(33100) << "EMR_ARCTO" << box << start << end;
}

void EmfDebugBackend::polygon16(EmfDeviceContext &context, const QRect &bounds, const QList<QPoint> points )
{
    kDebug(33100) << "EMR_POLYGON16" << bounds << points;
}

void EmfDebugBackend::polyLine(EmfDeviceContext &context, const QRect &bounds, const QList<QPoint> points )
{
    kDebug(33100) << "EMR_POLYLINE" << bounds << points;
}

void EmfDebugBackend::polyLine16(EmfDeviceContext &context, const QRect &bounds, const QList<QPoint> points )
{
    kDebug(33100) << "EMR_POLYLINE16" << bounds << points;
}

void EmfDebugBackend::polyPolyLine16(EmfDeviceContext &context, const QRect &bounds, const QList< QVector< QPoint > > &points )
{
    kDebug(33100) << "EMR_POLYPOLYLINE16" << bounds << points;
}

void EmfDebugBackend::polyPolygon16(EmfDeviceContext &context, const QRect &bounds, const QList< QVector< QPoint > > &points )
{
    kDebug(33100) << "EMR_POLYPOLYGON16" << bounds << points;
}

void EmfDebugBackend::polyLineTo16(EmfDeviceContext &context, const QRect &bounds, const QList<QPoint> points )
{
    kDebug(33100) << "EMR_POLYLINETO16" << bounds << points;
}

void EmfDebugBackend::polyBezier16(EmfDeviceContext &context, const QRect &bounds, const QList<QPoint> points )
{
    kDebug(33100) << "EMR_POLYBEZIER16" << bounds << points;
}

void EmfDebugBackend::polyBezierTo16(EmfDeviceContext &context, const QRect &bounds, const QList<QPoint> points )
{
    kDebug(33100) << "EMR_POLYBEZIERTO16" << bounds << points;
}

void EmfDebugBackend::fillPath(EmfDeviceContext &context, const QRect &bounds )
{
    kDebug(33100) << "EMR_FILLPATH" << bounds;
}

void EmfDebugBackend::strokeAndFillPath(EmfDeviceContext &context, const QRect &bounds )
{
    kDebug(33100) << "EMR_STROKEANDFILLPATH" << bounds;
}

void EmfDebugBackend::strokePath(EmfDeviceContext &context, const QRect &bounds )
{
    kDebug(33100) << "EMR_STROKEPATH" << bounds;
}

void EmfDebugBackend::setClipPath(EmfDeviceContext &context, quint32 regionMode )
{
   kDebug(33100) << "EMR_SETCLIPPATH:" << regionMode;
}

void EmfDebugBackend::bitBlt(EmfDeviceContext &context, BitBltRecord &bitBltRecord )
{
    kDebug(33100) << "EMR_BITBLT:" << bitBltRecord.destinationRectangle();
}

void EmfDebugBackend::setStretchBltMode(EmfDeviceContext &context, const quint32 stretchMode )
{
    switch ( stretchMode ) {
    case 0x01:
        kDebug(33100) << "EMR_STRETCHBLTMODE: STRETCH_ANDSCANS";
        break;
    case 0x02:
        kDebug(33100) << "EMR_STRETCHBLTMODE: STRETCH_ORSCANS";
        break;
    case 0x03:
        kDebug(33100) << "EMR_STRETCHBLTMODE: STRETCH_DELETESCANS";
        break;
    case 0x04:
        kDebug(33100) << "EMR_STRETCHBLTMODE: STRETCH_HALFTONE";
        break;
    default:
        kDebug(33100) << "EMR_STRETCHBLTMODE - unknown stretch mode:" << stretchMode;
    }
}

void EmfDebugBackend::stretchDiBits(EmfDeviceContext &context, StretchDiBitsRecord &stretchDiBitsRecord )
{
    kDebug(33100) << "EMR_STRETCHDIBITS:" << stretchDiBitsRecord.sourceRectangle()
                  << "," << stretchDiBitsRecord.destinationRectangle();
}


} // xnamespace...
