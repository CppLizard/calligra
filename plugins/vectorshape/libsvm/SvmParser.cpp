/* This file is part of the Calligra project

  Copyright 2011 Inge Wallin <inge@lysator.liu.se>
  Copyright 2011 Pierre Ducroquet <pinaraf@pinaraf.info>

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


// Own
#include "SvmParser.h"

// Qt
#include <QByteArray>
#include <QBuffer>
#include <QDataStream>
#include <QString>
#include <QPolygon>

// KDE
#include <KDebug>

// Libsvm
#include "SvmEnums.h"
#include "SvmStructs.h"

// 0 - No debug
// 1 - Print a lot of debug info
// 2 - Just print all the records instead of parsing them
#define DEBUG_SVMPARSER 1

namespace Libsvm
{


static void soakBytes( QDataStream &stream, int numBytes )
{
    quint8 scratch;
    for ( int i = 0; i < numBytes; ++i ) {
        stream >> scratch;
    }
}


SvmParser::SvmParser()
    : mContext()
    , mBackend(0)
{
}


static const struct ActionNames {
    int     actionNumber;
    QString actionName;
} actionNames[] = {
    { META_NULL_ACTION,                  "META_NULL_ACTION" },
    { META_PIXEL_ACTION,                 "META_PIXEL_ACTION" },
    { META_POINT_ACTION,                 "META_POINT_ACTION" },
    { META_LINE_ACTION,                  "META_LINE_ACTION" },
    { META_RECT_ACTION,                  "META_RECT_ACTION" },
    { META_ROUNDRECT_ACTION,             "META_ROUNDRECT_ACTION" },
    { META_ELLIPSE_ACTION,               "META_ELLIPSE_ACTION" },
    { META_ARC_ACTION,                   "META_ARC_ACTION" },
    { META_PIE_ACTION,                   "META_PIE_ACTION" },
    { META_CHORD_ACTION,                 "META_CHORD_ACTION" },
    { META_POLYLINE_ACTION,              "META_POLYLINE_ACTION" },
    { META_POLYGON_ACTION,               "META_POLYGON_ACTION" },
    { META_POLYPOLYGON_ACTION,           "META_POLYPOLYGON_ACTION" },
    { META_TEXT_ACTION,                  "META_TEXT_ACTION" },
    { META_TEXTARRAY_ACTION,             "META_TEXTARRAY_ACTION" },
    { META_STRETCHTEXT_ACTION,           "META_STRETCHTEXT_ACTION" },
    { META_TEXTRECT_ACTION,              "META_TEXTRECT_ACTION" },
    { META_BMP_ACTION,                   "META_BMP_ACTION" },
    { META_BMPSCALE_ACTION,              "META_BMPSCALE_ACTION" },
    { META_BMPSCALEPART_ACTION,          "META_BMPSCALEPART_ACTION" },
    { META_BMPEX_ACTION,                 "META_BMPEX_ACTION" },
    { META_BMPEXSCALE_ACTION,            "META_BMPEXSCALE_ACTION" },
    { META_BMPEXSCALEPART_ACTION,        "META_BMPEXSCALEPART_ACTION" },
    { META_MASK_ACTION,                  "META_MASK_ACTION" },
    { META_MASKSCALE_ACTION,             "META_MASKSCALE_ACTION" },
    { META_MASKSCALEPART_ACTION,         "META_MASKSCALEPART_ACTION" },
    { META_GRADIENT_ACTION,              "META_GRADIENT_ACTION" },
    { META_HATCH_ACTION,                 "META_HATCH_ACTION" },
    { META_WALLPAPER_ACTION,             "META_WALLPAPER_ACTION" },
    { META_CLIPREGION_ACTION,            "META_CLIPREGION_ACTION" },
    { META_ISECTRECTCLIPREGION_ACTION,   "META_ISECTRECTCLIPREGION_ACTION" },
    { META_ISECTREGIONCLIPREGION_ACTION, "META_ISECTREGIONCLIPREGION_ACTION" },
    { META_MOVECLIPREGION_ACTION,        "META_MOVECLIPREGION_ACTION" },
    { META_LINECOLOR_ACTION,             "META_LINECOLOR_ACTION" },
    { META_FILLCOLOR_ACTION,             "META_FILLCOLOR_ACTION" },
    { META_TEXTCOLOR_ACTION,             "META_TEXTCOLOR_ACTION" },
    { META_TEXTFILLCOLOR_ACTION,         "META_TEXTFILLCOLOR_ACTION" },
    { META_TEXTALIGN_ACTION,             "META_TEXTALIGN_ACTION" },
    { META_MAPMODE_ACTION,               "META_MAPMODE_ACTION" },
    { META_FONT_ACTION,                  "META_FONT_ACTION" },
    { META_PUSH_ACTION,                  "META_PUSH_ACTION" },
    { META_POP_ACTION,                   "META_POP_ACTION" },
    { META_RASTEROP_ACTION,              "META_RASTEROP_ACTION" },
    { META_TRANSPARENT_ACTION,           "META_TRANSPARENT_ACTION" },
    { META_EPS_ACTION,                   "META_EPS_ACTION" },
    { META_REFPOINT_ACTION,              "META_REFPOINT_ACTION" },
    { META_TEXTLINECOLOR_ACTION,         "META_TEXTLINECOLOR_ACTION" },
    { META_TEXTLINE_ACTION,              "META_TEXTLINE_ACTION" },
    { META_FLOATTRANSPARENT_ACTION,      "META_FLOATTRANSPARENT_ACTION" },
    { META_GRADIENTEX_ACTION,            "META_GRADIENTEX_ACTION" },
    { META_LAYOUTMODE_ACTION,            "META_LAYOUTMODE_ACTION" },
    { META_TEXTLANGUAGE_ACTION,          "META_TEXTLANGUAGE_ACTION" },
    { META_OVERLINECOLOR_ACTION,         "META_OVERLINECOLOR_ACTION" },
    { META_SVG_SOMETHING_ACTION,         "META_SVG_SOMETHING_ACTION" },
    { META_COMMENT_ACTION,               "META_COMMENT_ACTION" }
};


void SvmParser::setBackend(SvmAbstractBackend *backend)
{
    mBackend = backend;
}


bool SvmParser::parse(const QByteArray &data)
{
    // Check the signature "VCLMTF"
    if (!data.startsWith("VCLMTF"))
        return false;

    QBuffer buffer((QByteArray *) &data);
    buffer.open(QIODevice::ReadOnly);

    QDataStream mainStream(&buffer);
    mainStream.setByteOrder(QDataStream::LittleEndian);

    // Start reading from the stream: read past the signature and get the header.
    soakBytes(mainStream, 6);
    SvmHeader header(mainStream);
#if DEBUG_SVMPARSER
    kDebug(31000) << "================ SVM HEADER ================";
    kDebug(31000) << "version, length:" << header.versionCompat.version << header.versionCompat.length;
    kDebug(31000) << "compressionMode:" << header.compressionMode;
    kDebug(31000) << "mapMode:" << "...";
    kDebug(31000) << "size:" << header.width << header.height;
    kDebug(31000) << "actionCount:" << header.actionCount;
    kDebug(31000) << "================ SVM HEADER ================";
#endif    

    mBackend->init(header);

    for (uint action = 0; action < header.actionCount; ++action) {
        quint16  actionType;
        quint16  version;
        quint32  totalSize;

        // Here starts the Action itself. The first two bytes is the action type. 
        mainStream >> actionType;

        // The VersionCompat object
        mainStream >> version;
        mainStream >> totalSize;
        
        char *rawData = new char[totalSize];
        mainStream.readRawData(rawData, totalSize);
        QByteArray dataArray(rawData, totalSize);
        QDataStream stream(&dataArray, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::LittleEndian);

        // Debug
#if DEBUG_SVMPARSER
        {
            QString name;
            if (actionType == 0)
                name = actionNames[0].actionName;
            else if (100 <= actionType && actionType <= META_LAST_ACTION)
                name = actionNames[actionType - 99].actionName;
            else if (actionType == 512)
                name = "META_COMMENT_ACTION";
            else
                name = "(out of bounds)";

            kDebug(31000) << name << "(" << actionType << ")" << "version" << version
                          << "totalSize" << totalSize;
        }
#endif

        // Parse all actions.
        switch (actionType) {
        case META_NULL_ACTION:
        case META_PIXEL_ACTION:
        case META_POINT_ACTION:
        case META_LINE_ACTION:
            break;
        case META_RECT_ACTION:
            {
                QRect  rect;

                parseRect(stream, rect);
                mBackend->rect(mContext, rect);
            }
            break;
        case META_ROUNDRECT_ACTION:
        case META_ELLIPSE_ACTION:
        case META_ARC_ACTION:
        case META_PIE_ACTION:
        case META_CHORD_ACTION:
            break;
        case META_POLYLINE_ACTION:
            {
                QPolygon  polygon;

                parsePolygon(stream, polygon);
                mBackend->polyLine(mContext, polygon);

                // FIXME: Version 2: Lineinfo, Version 3: polyflags
                if (version > 1)
                    soakBytes(stream, totalSize - 2 - 4 * 2 * polygon.size());
            }
            break;
        case META_POLYGON_ACTION:
            {
                QPolygon  polygon;

                parsePolygon(stream, polygon);
                mBackend->polygon(mContext, polygon);

                // FIXME: Version 2: Lineinfo, Version 3: polyflags
                if (version > 1)
                    soakBytes(stream, totalSize - 2 - 4 * 2 * polygon.size());
            }
            break;
        case META_POLYPOLYGON_ACTION:
            {
                quint16 polygonCount;
                stream >> polygonCount;
                
                QList<QPolygon> polygons;
                for (quint16 i = 0 ; i < polygonCount ; i++) {
                    QPolygon polygon;
                    parsePolygon(stream, polygon);
                    polygons << polygon;
                }
                
                if (version > 1) {
                    quint16 complexPolygonCount;
                    stream >> complexPolygonCount;
                    for (quint16 i = 0 ; i < complexPolygonCount ; i++) {
                        quint16 complexPolygonIndex;
                        stream >> complexPolygonIndex;
                        QPolygon polygon;
                        parsePolygon(stream, polygon);
                        polygons[complexPolygonIndex] = polygon;
                    }
                }
                
                foreach (QPolygon polygon, polygons) {
                    mBackend->polygon(mContext, polygon);
                }
            }
            break;
        case META_TEXT_ACTION:
        case META_TEXTARRAY_ACTION:
        case META_STRETCHTEXT_ACTION:
        case META_TEXTRECT_ACTION:
        case META_BMP_ACTION:
        case META_BMPSCALE_ACTION:
        case META_BMPSCALEPART_ACTION:
        case META_BMPEX_ACTION:
        case META_BMPEXSCALE_ACTION:
        case META_BMPEXSCALEPART_ACTION:
        case META_MASK_ACTION:
        case META_MASKSCALE_ACTION:
        case META_MASKSCALEPART_ACTION:
        case META_GRADIENT_ACTION:
        case META_HATCH_ACTION:
        case META_WALLPAPER_ACTION:
        case META_CLIPREGION_ACTION:
        case META_ISECTRECTCLIPREGION_ACTION:
        case META_ISECTREGIONCLIPREGION_ACTION:
        case META_MOVECLIPREGION_ACTION:
            break;
        case META_LINECOLOR_ACTION:
            {
                quint32  colorData;
                bool     doSet;

                stream >> colorData;
                stream >> doSet;

                mContext.lineColor = doSet ? QColor::fromRgb(colorData) : Qt::NoPen;
                mContext.changedItems |= GCLineColor;
            }
            break;
        case META_FILLCOLOR_ACTION:
            {
                quint32  colorData;
                bool     doSet;

                stream >> colorData;
                stream >> doSet;
                
                kDebug(31000) << "Fill color :" << colorData << '(' << doSet << ')';

                mContext.fillBrush = doSet ? QBrush(QColor::fromRgb(colorData)) : Qt::NoBrush;
                mContext.changedItems |= GCFillBrush;
            }
            break;
        case META_TEXTCOLOR_ACTION:
        case META_TEXTFILLCOLOR_ACTION:
        case META_TEXTALIGN_ACTION:
            break;
        case META_MAPMODE_ACTION:
            {
                stream >> mContext.mapMode;
                mContext.changedItems |= GCMapMode;
            }
            break;
        case META_FONT_ACTION:
            break;
        case META_PUSH_ACTION:
            {
                kDebug(31000) << "Push action : " << totalSize;
                quint16 pushValue;
                stream >> pushValue;
                kDebug(31000) << "Push value : " << pushValue;
            }
            break;
        case META_POP_ACTION:
            {
                kDebug(31000) << "Pop action : " << totalSize;
                /*quint16 pushValue;
                stream >> pushValue;
                kDebug(31000) << "Push value : " << pushValue;*/
            }
            break;
        case META_RASTEROP_ACTION:
        case META_TRANSPARENT_ACTION:
        case META_EPS_ACTION:
        case META_REFPOINT_ACTION:
        case META_TEXTLINECOLOR_ACTION:
        case META_TEXTLINE_ACTION:
        case META_FLOATTRANSPARENT_ACTION:
        case META_GRADIENTEX_ACTION:
        case META_LAYOUTMODE_ACTION:
        case META_TEXTLANGUAGE_ACTION:
        case META_OVERLINECOLOR_ACTION:
        case META_COMMENT_ACTION:
            break;

        default:
#if DEBUG_SVMPARSER
            kDebug(31000) << "unknown action type:" << actionType;
#endif
        }

        delete rawData;
        
        // Security measure
        if (mainStream.atEnd())
            break;
    }

    mBackend->cleanup();

    return true;
}


// ----------------------------------------------------------------
//                         Private methods


void SvmParser::parseRect( QDataStream &stream, QRect &rect)
{
    qint32 left;
    qint32 top;
    qint32 right;
    qint32 bottom;

    stream >> left;
    stream >> top;
    stream >> right;
    stream >> bottom;

    rect.setLeft(left);
    rect.setTop(top);
    rect.setRight(right);
    rect.setBottom(bottom);
}

void SvmParser::parsePolygon( QDataStream &stream, QPolygon &polygon)
{
    quint16   numPoints;
    QPoint    point;

    stream >> numPoints;
    for (uint i = 0; i < numPoints; ++i) {
        stream >> point;
        polygon << point;
    }
}

} // namespace Libsvm
