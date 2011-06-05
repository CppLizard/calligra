/* This file is part of the KDE project
   Copyright (C) 2010 KO GmbH <jos.van.den.oever@kogmbh.com>
   Copyright (C) 2011 Lukáš Tvrdý <lukas.tvrdy@ixonos.com>

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
 * Boston, MA 02110-1301, USA.
*/
#include "ODrawToOdf.h"
#include "drawstyle.h"
#include "msodraw.h"
#include "generated/leinputstream.h"

#include <KoXmlWriter.h>
#include <kdebug.h>

#include <QTransform>
#include <qbuffer.h>

#include <cmath>


using namespace MSO;

namespace
{
void equation(Writer& out, const char* name, const char* formula)
{
    out.xml.startElement("draw:equation");
    out.xml.addAttribute("draw:name", name);
    out.xml.addAttribute("draw:formula", formula);
    out.xml.endElement();
}
}

qint16
ODrawToOdf::normalizeRotation(qreal rotation)
{
    qint16 angle = ((qint16)rotation) % 360;
    if (angle < 0) {
        angle = angle + 360;
    }
    return angle;
}

/**
 * Return the bounding rectangle for this object.
 **/
QRectF
ODrawToOdf::getRect(const OfficeArtSpContainer &o)
{
    if (o.childAnchor) {
        const OfficeArtChildAnchor& r = *o.childAnchor;
        return QRect(r.xLeft, r.yTop, r.xRight - r.xLeft, r.yBottom - r.yTop);
    } else if (o.clientAnchor && client) {
        return client->getRect(*o.clientAnchor);
    }
    return QRect(0, 0, 1, 1);
}

QRectF
ODrawToOdf::processRect(const quint16 shapeType, const qreal rotation, QRectF &rect)
{
    bool transform_anchor = false;
    qint16 nrotation = normalizeRotation(rotation);

    //TODO: Add other shapes here!

    switch (shapeType) {
    case msosptNotPrimitive:
        if ( ((nrotation >= 45) && (nrotation < 135)) ||
             ((nrotation >= 225) && (nrotation < 315)))
        {
            transform_anchor = true;
        }
        break;
    default:
        break;
    }
    if (transform_anchor) {
        QPointF center = rect.center();
        QTransform transform;
        transform.rotate(90);
        rect = transform.mapRect(rect.translated(-center)).translated(center);
    }
    return rect;
}


void ODrawToOdf::processRectangle(const OfficeArtSpContainer& o, Writer& out)
{
    out.xml.startElement("draw:frame");
    processStyle(o, out);
    out.xml.startElement("draw:text-box");
    processText(o, out);
    out.xml.endElement(); // draw:text-box
    out.xml.endElement(); // draw:frame
}


void ODrawToOdf::processLine(const OfficeArtSpContainer& o, Writer& out)
{
    const QRectF rect = getRect(o);
    qreal x1 = rect.x();
    qreal y1 = rect.y();
    qreal x2 = rect.x() + rect.width();
    qreal y2 = rect.y() + rect.height();

    // shape mirroring
    if (o.shapeProp.fFlipV) {
        qSwap(y1, y2);
    }
    if (o.shapeProp.fFlipH) {
        qSwap(x1, x2);
    }

    out.xml.startElement("draw:line");
    out.xml.addAttribute("svg:y1", client->formatPos(out.vOffset(y1)));
    out.xml.addAttribute("svg:y2", client->formatPos(out.vOffset(y2)));
    out.xml.addAttribute("svg:x1", client->formatPos(out.hOffset(x1)));
    out.xml.addAttribute("svg:x2", client->formatPos(out.hOffset(x2)));
    addGraphicStyleToDrawElement(out, o);
    out.xml.addAttribute("draw:layer", "layout");
    processText(o, out);

    out.xml.endElement();
}

void ODrawToOdf::processStraightConnector1(const OfficeArtSpContainer& o, Writer& out)
{
    const QRectF rect = getRect(o);
    qreal x1 = rect.x();
    qreal y1 = rect.y();
    qreal x2 = rect.x() + rect.width();
    qreal y2 = rect.y() + rect.height();

    // shape mirroring
    if (o.shapeProp.fFlipV) {
        qSwap(y1, y2);
    }
    if (o.shapeProp.fFlipH) {
        qSwap(x1, x2);
    }

    out.xml.startElement("draw:connector");
    out.xml.addAttribute("svg:x1", client->formatPos(out.hOffset(x1)));
    out.xml.addAttribute("svg:y1", client->formatPos(out.vOffset(y1)));
    out.xml.addAttribute("svg:x2", client->formatPos(out.hOffset(x2)));
    out.xml.addAttribute("svg:y2", client->formatPos(out.vOffset(y2)));
    addGraphicStyleToDrawElement(out, o);
    out.xml.addAttribute("draw:layer", "layout");
    out.xml.addAttribute("draw:type", "line");
    processText(o, out);
    out.xml.endElement();

}

void ODrawToOdf::processBentConnector3(const OfficeArtSpContainer& o, Writer& out)
{
    const OfficeArtDggContainer * drawingGroup = 0;
    if (client) {
        drawingGroup = client->getOfficeArtDggContainer();
    }

    const OfficeArtSpContainer* master = 0;
    const DrawStyle ds(drawingGroup, master, &o);
    qreal rotation = toQReal( ds.rotation() );

    const QRectF rect = getRect(o);
    qreal x1 = rect.x();
    qreal y1 = rect.y();
    qreal x2 = rect.x() + rect.width();
    qreal y2 = rect.y() + rect.height();

    QRectF shapeRect = rect;

    qreal sx1 = x1;
    qreal sy1 = y1;
    qreal sx2 = x2;
    qreal sy2 = y2;

    if (rotation != 0.0) {
        QTransform m;
        m.rotate( -rotation );
        shapeRect = m.mapRect(rect.translated(-rect.center())).translated(rect.center());

        sx1 = shapeRect.topLeft().x();
        sy1 = shapeRect.topLeft().y();
        sx2 = shapeRect.bottomRight().x();
        sy2 = shapeRect.bottomRight().y();
    }

    // compute path
    QPainterPath shapePath;
    shapePath.moveTo(sx1,sy1);
    shapePath.lineTo((sx1 + sx2)/2.0, sy1);
    shapePath.lineTo((sx1 + sx2)/2.0, sy2);
    shapePath.lineTo(sx2,sy2);

    // transform the path according the shape properties like flip and rotation
    QTransform m;
    m.reset();
    m.translate( -shapeRect.center().x(), -shapeRect.center().y() );

    // shape mirroring
    if (o.shapeProp.fFlipH){
        m.scale(-1,1);
    }

    if (o.shapeProp.fFlipV){
        m.scale(1,-1);
    }

    if (rotation != 0) {
        m.rotate(rotation);
    }

    m.translate( shapeRect.center().x(), shapeRect.center().y() );
    shapePath = m.map(shapePath);

    // translate the QPainterPath into svg:d attribute
    QString path = path2svg(shapePath);

    out.xml.startElement("draw:connector");
    addGraphicStyleToDrawElement(out, o);
    out.xml.addAttribute("draw:layer", "layout");
    out.xml.addAttribute("svg:x1", client->formatPos(out.hOffset(x1)));
    out.xml.addAttribute("svg:y1", client->formatPos(out.vOffset(y1)));
    out.xml.addAttribute("svg:x2", client->formatPos(out.hOffset(x2)));
    out.xml.addAttribute("svg:y2", client->formatPos(out.vOffset(y2)));
    if (!path.isEmpty()) {
        out.xml.addAttribute("svg:d", path);
    }

    processText(o, out);
    out.xml.endElement();

}


void ODrawToOdf::processPictureFrame(const OfficeArtSpContainer& o, Writer& out)
{
    DrawStyle ds(0, &o);

    // A value of 0x00000000 MUST be ignored.  [MS-ODRAW] — v20101219
    if (!ds.pib()) return;

    out.xml.startElement("draw:frame");
    processStyleAndText(o, out);

    QString url;
    if (client) {
        url = client->getPicturePath(ds.pib());
    }
    // if the image cannot be found, just place an empty frame
    if (url.isEmpty()) {
        out.xml.endElement(); //draw:frame
        return;
    }
    out.xml.startElement("draw:image");
    out.xml.addAttribute("xlink:href", url);
    out.xml.addAttribute("xlink:type", "simple");
    out.xml.addAttribute("xlink:show", "embed");
    out.xml.addAttribute("xlink:actuate", "onLoad");
    out.xml.endElement(); // image
    out.xml.endElement(); // frame
}

void ODrawToOdf::processNotPrimitive(const MSO::OfficeArtSpContainer& o, Writer& out)
{
    out.xml.startElement("draw:custom-shape");
    processStyleAndText(o, out);

    out.xml.startElement("draw:enhanced-geometry");
    setEnhancedGeometry(o, out);
    out.xml.endElement(); //draw:enhanced-geometry

    out.xml.endElement(); //draw:custom-shape
}


void ODrawToOdf::processDrawingObject(const OfficeArtSpContainer& o, Writer& out)
{
    quint16 shapeType = o.shapeProp.rh.recInstance;
    switch (shapeType) {
    case msosptNotPrimitive:
        processNotPrimitive(o, out);
        break;
    case msosptRectangle:
        processRectangle(o, out);
        break;
    case msosptRoundRectangle:
        processRoundRectangle(o, out);
        break;
    case msosptEllipse:
        // TODO: Something has to be done here (LukasT).
        processEllipse(o, out);
        break;
    case msosptDiamond:
        processDiamond(o, out);
        break;
    case msosptIsocelesTriangle:
        processIsocelesTriangle(o, out);
        break;
    case msosptRightTriangle:
        processRightTriangle(o, out);
        break;
    case msosptParallelogram:
        processParallelogram(o, out);
        break;
    case msosptTrapezoid:
        processTrapezoid(o, out);
        break;
    case msosptHexagon:
        processHexagon(o, out);
        break;
    case msosptOctagon:
        processOctagon(o, out);
        break;
    case msosptPlus:
        processPlus(o, out);
        break;
    case msosptStar:
        processStar(o, out);
        break;
    case msosptArrow:
        processArrow(o, out);
        break;
    //
    // TODO: msosptThickArrow
    //
    case msosptHomePlate:
        processHomePlate(o, out);
        break;
    case msosptCube:
        processCube(o, out);
        break;
    //
    // TODO: msosptBaloon, msosptSeal
    //

    // NOTE: OpenOffice treats msosptNotchedCircularArrow as msosptArc.  The
    // msosptNotchedCircularArrow value SHOULD NOT be used according to the
    // MS-ODRAW spec.  However it occures in many Word8 files.
    case msosptArc:
        processNotchedCircularArrow(o, out);
        break;
    case msosptLine:
        processLine(o, out);
        break;
    case msosptPlaque:
        processPlaque(o, out);
        break;
    case msosptCan:
        processCan(o, out);
        break;
    case msosptDonut:
        processDonut(o, out);
        break;
    //
    // TODO: msosptTextSimple, msosptTextOctagon, msosptTextHexagon,
    // msosptTextCurve, msosptTextWave, msosptTextRing, msosptTextOnCurve,
    // msosptTextOnRing
    //
    case msosptStraightConnector1:
        processStraightConnector1(o, out);
        break;
    case msosptBentConnector3:
        processBentConnector3(o, out);
        break;
    //
    // TODO: msosptBentConnector1, msosptBentConnector2, msosptBentConnector4,
    // msosptBentConnector5, msosptCurvedConnector2, msosptCurvedConnector3,
    // msosptCurvedConnector4, msosptCurvedConnector5
    //
    case msosptCallout1:
        processCallout1(o, out);
        break;
    case msosptCallout2:
        processCallout2(o, out);
        break;
    case msosptCallout3:
        processCallout3(o, out);
        break;
    case msosptAccentCallout1:
        processAccentCallout1(o, out);
        break;
    case msosptAccentCallout2:
        processAccentCallout2(o, out);
        break;
    case msosptAccentCallout3:
        processAccentCallout3(o, out);
        break;
    case msosptBorderCallout1:
        processBorderCallout1(o, out);
        break;
    case msosptBorderCallout2:
        processBorderCallout2(o, out);
        break;
    case msosptBorderCallout3:
        processBorderCallout3(o, out);
        break;
    case msosptAccentBorderCallout1:
        processAccentBorderCallout1(o, out);
        break;
    case msosptAccentBorderCallout2:
        processAccentBorderCallout2(o, out);
        break;
    case msosptAccentBorderCallout3:
        processAccentBorderCallout3(o, out);
        break;
    case msosptRibbon:
        processRibbon(o, out);
        break;
    case msosptRibbon2:
        processRibbon2(o, out);
        break;
    case msosptChevron:
        processChevron(o, out);
        break;
    case msosptPentagon:
        processPentagon(o, out);
        break;
    case msosptNoSmoking:
        processNoSmoking(o, out);
        break;
    case msosptSeal8:
        processSeal8(o, out);
        break;
    case msosptSeal16:
        processSeal16(o, out);
        break;
    case msosptSeal32:
        processSeal32(o, out);
        break;
    case msosptWedgeRectCallout:
        processWedgeRectCallout(o, out);
        break;
    case msosptWedgeRRectCallout:
        processWedgeRRectCallout(o, out);
        break;
    case msosptWedgeEllipseCallout:
        processWedgeEllipseCallout(o, out);
        break;
    case msosptWave:
        processWave(o, out);
        break;
    case msosptFoldedCorner:
        processFoldedCorner(o, out);
        break;
    case msosptLeftArrow:
        processLeftArrow(o, out);
        break;
    case msosptDownArrow:
        processDownArrow(o, out);
        break;
    case msosptUpArrow:
        processUpArrow(o, out);
        break;
    case msosptLeftRightArrow:
        processLeftRightArrow(o, out);
        break;
    case msosptUpDownArrow:
        processUpDownArrow(o, out);
        break;
    case msosptIrregularSeal1:
        processIrregularSeal1(o, out);
        break;
    case msosptIrregularSeal2:
        processIrregularSeal2(o, out);
        break;
    case msosptLightningBolt:
        processLightningBolt(o, out);
        break;
    case msosptHeart:
        processHeart(o, out);
        break;
    case msosptPictureFrame:
        processPictureFrame(o, out);
        break;
    case msosptQuadArrow:
        processQuadArrow(o, out);
        break;
    case msosptLeftArrowCallout:
        processLeftArrowCallout(o, out);
        break;
    case msosptRightArrowCallout:
        processRightArrowCallout(o, out);
        break;
    case msosptUpArrowCallout:
        processUpArrowCallout(o, out);
        break;
    case msosptDownArrowCallout:
        processDownArrowCallout(o, out);
        break;
    case msosptLeftRightArrowCallout:
        processLeftRightArrowCallout(o, out);
        break;
    case msosptUpDownArrowCallout:
        processUpDownArrowCallout(o, out);
        break;
    case msosptQuadArrowCallout:
        processQuadArrowCallout(o, out);
        break;
    case msosptBevel:
        processBevel(o, out);
        break;
    case msosptLeftBracket:
        processLeftBracket(o, out);
        break;
    case msosptRightBracket:
        processRightBracket(o, out);
        break;
    case msosptLeftBrace:
        processLeftBrace(o, out);
        break;
    case msosptRightBrace:
        processRightBrace(o, out);
        break;
    case msosptLeftUpArrow:
        processLeftUpArrow(o, out);
        break;
    case msosptBentUpArrow:
        processBentUpArrow(o, out);
        break;
    case msosptBentArrow:
        processBentArrow(o, out);
        break;
    case msosptSeal24:
        processSeal24(o, out);
        break;
    case msosptStripedRightArrow:
        processStripedRightArrow(o, out);
        break;
    case msosptNotchedRightArrow:
        processNotchedRightArrow(o, out);
        break;
    case msosptBlockArc:
        processBlockArc(o, out);
        break;
    case msosptSmileyFace:
        processSmileyFace(o, out);
        break;
    case msosptVerticalScroll:
        processVerticalScroll(o, out);
        break;
    case msosptHorizontalScroll:
        processHorizontalScroll(o, out);
        break;
    case msosptCircularArrow:
        processCircularArrow(o, out);
        break;
    case msosptNotchedCircularArrow:
        processNotchedCircularArrow(o, out);
        break;
    case msosptUturnArrow:
        processUturnArrow(o, out);
        break;
    case msosptCurvedRightArrow:
        processCurvedRightArrow(o, out);
        break;
    case msosptCurvedLeftArrow:
        processCurvedLeftArrow(o, out);
        break;
    case msosptCurvedUpArrow:
        processCurvedUpArrow(o, out);
        break;
    case msosptCurvedDownArrow:
        processCurvedDownArrow(o, out);
        break;
    case msosptCloudCallout:
        processCloudCallout(o, out);
        break;
    case msosptEllipseRibbon:
        processEllipseRibbon(o, out);
        break;
    case msosptEllipseRibbon2:
        processEllipseRibbon2(o, out);
        break;
    case msosptFlowChartProcess:
        processFlowChartProcess(o, out);
        break;
    case msosptFlowChartDecision:
        processFlowChartDecision(o, out);
        break;
    case msosptFlowChartInputOutput:
        processFlowChartInputOutput(o, out);
        break;
    case msosptFlowChartPredefinedProcess:
        processFlowChartPredefinedProcess(o, out);
        break;
    case msosptFlowChartInternalStorage:
        processFlowChartInternalStorage(o, out);
        break;
    case msosptFlowChartDocument:
        processFlowChartDocument(o, out);
        break;
    case msosptFlowChartMultidocument:
        processFlowChartMultidocument(o, out);
        break;
    case msosptFlowChartTerminator:
        processFlowChartTerminator(o, out);
        break;
    case msosptFlowChartPreparation:
        processFlowChartPreparation(o, out);
        break;
    case msosptFlowChartManualInput:
        processFlowChartManualInput(o, out);
        break;
    case msosptFlowChartManualOperation:
        processFlowChartManualOperation(o, out);
        break;
    case msosptFlowChartConnector:
        processFlowChartConnector(o, out);
        break;
    case msosptFlowChartPunchedCard:
        processFlowChartPunchedCard(o, out);
        break;
    case msosptFlowChartPunchedTape:
        processFlowChartPunchedTape(o, out);
        break;
    case msosptFlowChartSummingJunction:
        processFlowChartSummingJunction(o, out);
        break;
    case msosptFlowChartOr:
        processFlowChartOr(o, out);
        break;
    case msosptFlowChartCollate:
        processFlowChartCollate(o, out);
        break;
    case msosptFlowChartSort:
        processFlowChartSort(o, out);
        break;
    case msosptFlowChartExtract:
        processFlowChartExtract(o, out);
        break;
    case msosptFlowChartMerge:
        processFlowChartMerge(o, out);
        break;
    //
    // TODO: msosptFlowChartOfflineStorage
    //
    case msosptFlowChartOnlineStorage:
        processFlowChartOnlineStorage(o, out);
        break;
    case msosptFlowChartMagneticTape:
        processFlowChartMagneticTape(o, out);
        break;
    case msosptFlowChartMagneticDisk:
        processFlowChartMagneticDisk(o, out);
        break;
    case msosptFlowChartMagneticDrum:
        processFlowChartMagneticDrum(o, out);
        break;
    case msosptFlowChartDisplay:
        processFlowChartDisplay(o, out);
        break;
    case msosptFlowChartDelay:
        processFlowChartDelay(o, out);
        break;
    //
    // TODO: msosptTextPlainText, msosptTextStop, msosptTextTriangle,
    // msosptTextTriangleInverted, msosptTextChevron,
    // msosptTextChevronInverted, msosptTextRingInside, msosptTextRingOutside,
    // msosptTextArchUpCurve, msosptTextArchDownCurve, msosptTextCircleCurve,
    // msosptTextButtonCurve, msosptTextArchUpPour, msosptTextArchDownPour,
    // msosptTextCirclePour, msosptTextButtonPour, msosptTextCurveUp,
    // msosptTextCurveDown, msosptTextCascadeUp, msosptTextCascadeDown,
    // msosptTextWave1, msosptTextWave2, msosptTextWave3, msosptTextWave4,
    // msosptTextInflate, msosptTextDeflate, msosptTextInflateBottom,
    // msosptTextDeflateBottom, msosptTextInflateTop, msosptTextDeflateTop,
    // msosptTextDeflateInflate, msosptTextDeflateInflateDeflate,
    // msosptTextFadeRight, msosptTextFadeLeft, msosptTextFadeUp,
    // msosptTextFadeDown, msosptTextSlantUp, msosptTextSlantDown,
    // msosptTextCanUp, msosptTextCanDown
    //
    case msosptFlowChartAlternateProcess:
        processFlowChartAlternateProcess(o, out);
        break;
    case msosptFlowChartOffpageConnector:
        processFlowChartOffpageConnector(o, out);
        break;
    case msosptCallout90:
        processCallout90(o, out);
        break;
    case msosptAccentCallout90:
        processAccentCallout90(o, out);
        break;
    case msosptBorderCallout90:
        processBorderCallout90(o, out);
        break;
    case msosptAccentBorderCallout90:
        processAccentBorderCallout90(o, out);
        break;
    case msosptLeftRightUpArrow:
        processLeftRightUpArrow(o, out);
        break;
    case msosptSun:
        processSun(o, out);
        break;
    case msosptMoon:
        processMoon(o, out);
        break;
    case msosptBracketPair:
        processBracketPair(o, out);
        break;
    case msosptBracePair:
        processBracePair(o, out);
        break;
    case msosptSeal4:
        processSeal4(o, out);
        break;
    case msosptDoubleWave:
        processDoubleWave(o, out);
        break;
    case msosptActionButtonBlank:
        processActionButtonBlank(o, out);
        break;
    case msosptActionButtonHome:
        processActionButtonHome(o, out);
        break;
    case msosptActionButtonHelp:
        processActionButtonHelp(o, out);
        break;
    case msosptActionButtonInformation:
        processActionButtonInformation(o, out);
        break;
    case msosptActionButtonForwardNext:
        processActionButtonForwardNext(o, out);
        break;
    case msosptActionButtonBackPrevious:
        processActionButtonBackPrevious(o, out);
        break;
    case msosptActionButtonEnd:
        processActionButtonEnd(o, out);
        break;
    case msosptActionButtonBeginning:
        processActionButtonBeginning(o, out);
        break;
    case msosptActionButtonReturn:
        processActionButtonReturn(o, out);
        break;
    case msosptActionButtonDocument:
        processActionButtonDocument(o, out);
        break;
    case msosptActionButtonSound:
        processActionButtonSound(o, out);
        break;
    case msosptActionButtonMovie:
        processActionButtonMovie(o, out);
        break;
    case msosptHostControl:
        processPictureFrame(o, out);
        break;
    // TODO: Implement processTextBox, do not process msosptTextBox as
    // msosptRectangle.
    case msosptTextBox:
        processRectangle(o, out);
    default:
        qDebug() << "Cannot handle shape 0x" << hex << shapeType;
        break;
    }
}

void ODrawToOdf::processStyleAndText(const MSO::OfficeArtSpContainer& o,
                                     Writer& out)
{
    processStyle(o, out);
    processText(o, out);
}

void ODrawToOdf::processStyle(const MSO::OfficeArtSpContainer& o,
                              Writer& out)
{
    addGraphicStyleToDrawElement(out, o);
    set2dGeometry(o, out);
}

void ODrawToOdf::processText(const MSO::OfficeArtSpContainer& o,
                             Writer& out)
{
    if (o.clientData && client && client->onlyClientData(*o.clientData)) {
        client->processClientData(o.clientTextbox.data(), *o.clientData, out);
    } else if (o.clientTextbox) {
        client->processClientTextBox(*o.clientTextbox, o.clientData.data(), out);
    }
}

void ODrawToOdf::processModifiers(const MSO::OfficeArtSpContainer &o, Writer &out, const QList<int>& defaults)
{
    const AdjustValue* val1 = get<AdjustValue>(o);
    if (!val1 && defaults.isEmpty()) return;
    const Adjust2Value* val2 = get<Adjust2Value>(o);
    const Adjust3Value* val3 = get<Adjust3Value>(o);
    const Adjust4Value* val4 = get<Adjust4Value>(o);
    const Adjust5Value* val5 = get<Adjust5Value>(o);
    const Adjust6Value* val6 = get<Adjust6Value>(o);
    const Adjust7Value* val7 = get<Adjust7Value>(o);
    const Adjust8Value* val8 = get<Adjust8Value>(o);

    QString modifiers = QString::number(val1 ? val1->adjustvalue : defaults[0]);
    if (val2 || defaults.size() > 1) {
        modifiers += QString(" %1").arg(val2 ? val2->adjust2value : defaults[1]);
        if (val3 || defaults.size() > 2) {
            modifiers += QString(" %1").arg(val3 ? val3->adjust3value : defaults[2]);
            if (val4 || defaults.size() > 3) {
                modifiers += QString(" %1").arg(val4 ? val4->adjust4value : defaults[3]);
                if (val5 || defaults.size() > 4) {
                    modifiers += QString(" %1").arg(val5 ? val5->adjust5value : defaults[4]);
                    if (val6 || defaults.size() > 5) {
                        modifiers += QString(" %1").arg(val6 ? val6->adjust6value : defaults[5]);
                        if (val7 || defaults.size() > 6) {
                            modifiers += QString(" %1").arg(val7 ? val7->adjust7value : defaults[6]);
                            if (val8 || defaults.size() > 7) {
                                modifiers += QString(" %1").arg(val8 ? val8->adjust8value : defaults[7]);
                            }
                        }
                    }
                }
            }
        }
    }

    out.xml.addAttribute("draw:modifiers", modifiers);
}

// Position the shape into the slide or into a group shape
void ODrawToOdf::set2dGeometry(const OfficeArtSpContainer& o, Writer& out)
{
    const OfficeArtDggContainer* dgg = 0;
    const OfficeArtSpContainer* master = 0;
    const DrawStyle ds(dgg, master, &o);
    const qreal rotation = toQReal(ds.rotation());

    //transform the rectangle into the coordinate system of the group shape
    QRectF rect = getRect(o);
    QRectF trect (out.hOffset(rect.x()), out.vOffset(rect.y()),
                  out.hLength(rect.width()), out.vLength(rect.height()));

    //draw:caption-id
    //draw:class-names
    //draw:data
    //draw:engine
    //draw:id
    //draw:layer
    out.xml.addAttribute("draw:layer", "layout");
    //draw:name
    //draw:style-name
    //draw:text-style-name
    //draw:transform
    if (rotation) {

        const quint16 shapeType = o.shapeProp.rh.recInstance;
        const quint16 nrotation = normalizeRotation(rotation);
        const qreal angle = (nrotation / (qreal)180) * M_PI;

        trect = processRect(shapeType, rotation, trect);

        static const QString transform_str("translate(%1 %2) rotate(%3) translate(%4 %5)");
        const QPointF center = trect.center();
        const qreal height = trect.height();
        const qreal width = trect.width();

        out.xml.addAttribute("draw:transform",
                             transform_str.arg(client->formatPos(-width/2)).arg(client->formatPos(-height/2)).arg(-angle).arg(client->formatPos(center.x())).arg(client->formatPos(center.y())));
    }
    //svg:x
    //svg:y
    else {
        out.xml.addAttribute("svg:x", client->formatPos(trect.x()));
        out.xml.addAttribute("svg:y", client->formatPos(trect.y()));
    }
    //draw:z-index
    //presentation:class-names
    //presentation:style-name
    //svg:height
    out.xml.addAttribute("svg:height", client->formatPos(trect.height()));
    //svg:width
    out.xml.addAttribute("svg:width", client->formatPos(trect.width()));
    //table:end-cell-address
    //table:end-x
    //table:end-y
    //table:table-background
    //text:anchor-page-number
    //text:anchor-type
    //xml:id
}

void ODrawToOdf::setEnhancedGeometry(const MSO::OfficeArtSpContainer& o, Writer& out)
{
    const OfficeArtDggContainer* drawingGroup = 0;
    const OfficeArtSpContainer* master = 0;
    const DrawStyle ds(drawingGroup, master, &o);

    IMsoArray _v = ds.pVertices_complex();
    IMsoArray _c = ds.pSegmentInfo_complex();

    if (!_v.data.isEmpty() && !_c.data.isEmpty()) {

        QVector<QPoint> verticesPoints;

        //_v.data is an array of POINTs, MS-ODRAW, page 89
        QByteArray xArray(sizeof(int), 0), yArray(sizeof(int), 0);
        int step = _v.cbElem;
        if (step == 0xfff0) {
            step = 4;
        }

        int maxX = 0, minX = INT_MAX, maxY = 0, minY = INT_MAX;
        int x,y;

        //get vertice points
        for (int i = 0, offset = 0; i < _v.nElems; i++, offset += step) {
            // x coordinate of this point
            xArray.replace(0, step/2, _v.data.mid(offset, step/2));
            x = *(int*) xArray.data();

            // y coordinate of this point
            yArray.replace(0, step/2, _v.data.mid(offset + step/2, step/2));
            y = *(int*) yArray.data();

            verticesPoints.append(QPoint(x, y));

            // find maximum and minimum coordinates
            if (maxY < y) {
                maxY = y;
            }
            if (minY > y) {
                minY = y ;
            }
            if (maxX < x) {
                maxX = x;
            }
            if (minX > x) {
                minX = x;
            }
        }

        //TODO: geoLeft, geoTop, geoRight, geoBottom
        QString viewBox = QString::number(minX) + ' ' + QString::number(minY) + ' ' +
                          QString::number(maxX) + ' ' + QString::number(maxY);

        // combine segmentationInfoData and verticePoints into enhanced-path string
        QString enhancedPath;
        ushort msopathtype;
        bool nOffRange = false;

        for (int i = 0, n = 0; ((i < _c.nElems) && !nOffRange); i++) {

            msopathtype = (((*(ushort *)(_c.data.data() + i * 2)) >> 13) & 0x7);

            switch (msopathtype) {
            case msopathLineTo:
            {
                if (n >= verticesPoints.size()) {
                    qDebug() << "EnhancedGeometry: index into verticesPoints out of range!";
                    nOffRange = true;
                    break;
                }
                enhancedPath = enhancedPath + "L " + QString::number(verticesPoints[n].x()) + ' ' +
                               QString::number(verticesPoints[n].y()) + ' ';
                n++;
                break;
            }
            case msopathCurveTo:
            {
                if (n + 2 > verticesPoints.size()) {
                    qDebug() << "EnhancedGeometry: index into verticesPoints out of range!";
                    nOffRange = true;
                    break;
                }
                QPoint pt1 = verticesPoints.at(n);
                QPoint pt2 = verticesPoints.at(n + 1);
                QPoint pt3 = verticesPoints.at(n + 2);

                enhancedPath = enhancedPath + "C " +
                        QString::number(pt1.x()) + ' ' +
                        QString::number(pt1.y()) + ' ' +
                        QString::number(pt2.x()) + ' ' +
                        QString::number(pt2.y()) + ' ' +
                        QString::number(pt3.x()) + ' ' +
                        QString::number(pt3.y()) + ' ';
                n = n + 3;
                break;
            }
            case msopathMoveTo:
            {
                if (n >= verticesPoints.size()) {
                    qDebug() << "EnhancedGeometry: index into verticesPoints out of range!";
                    nOffRange = true;
                    break;
                }
                enhancedPath = enhancedPath + "M " + QString::number(verticesPoints[n].x()) + ' ' +
                               QString::number(verticesPoints[n].y()) + ' ';
                n++;
                break;
            }
            case msopathClose:
                enhancedPath = enhancedPath + "Z ";
                break;
            case msopathEnd:
                enhancedPath = enhancedPath + "N ";
                break;
            case msopathEscape:
            case msopathClientEscape:
                 break;
            }
        }
        //dr3d:projection
        //dr3d:shade-mode
        //draw:concentric-gradient-fill-allowed
        //draw:enhanced-path
        out.xml.addAttribute("draw:enhanced-path", enhancedPath);
        //draw:extrusion
        //draw:extrusion-allowed
        //draw:extrusion-brightness
        //draw:extrusion-color
        //draw:extrusion-depth
        //draw:extrusion-diffusion
        //draw:extrusion-first-light-direction
        //draw:extrusion-first-light-harsh
        //draw:extrusion-first-light-level
        //draw:extrusion-light-face
        //draw:extrusion-metal
        //draw:extrusion-number-of-line-segments
        //draw:extrusion-origin
        //draw:extrusion-rotation-angle
        //draw:extrusion-rotation-center
        //draw:extrusion-second-light-direction
        //draw:extrusion-second-light-harsh
        //draw:extrusion-second-light-level
        //draw:extrusion-shininess
        //draw:extrusion-skew
        //draw:extrusion-specularity
        //draw:extrusion-viewpoint
        //draw:glue-point-leaving-directions
        //draw:glue-points
        //draw:glue-point-type
        //draw:mirror-horizontal
        if (o.shapeProp.fFlipH) {
            out.xml.addAttribute("draw:mirror-horizontal", "true");
        }
        //draw:mirror-vertical
        if (o.shapeProp.fFlipV) {
            out.xml.addAttribute("draw:mirror-vertical", "true");
        }
        //draw:modifiers
        //draw:path-stretchpoint-x
        //draw:path-stretchpoint-y
        //draw:text-areas
        //draw:text-path
        //draw:text-path-allowed
        //draw:text-path-mode
        //draw:text-path-same-letter-heights
        //draw:text-path-scale
        //draw:text-rotate-angle
        //draw:type
        out.xml.addAttribute("draw:type", "non-primitive");
        //svg:viewBox
        out.xml.addAttribute("svg:viewBox", viewBox);
    }
}

QString ODrawToOdf::path2svg(const QPainterPath &path)
{
    QString d;

    int count = path.elementCount();
    for (int i = 0; i < count; i++) {

        QPainterPath::Element e = path.elementAt(i);
        switch(e.type) {
        case QPainterPath::MoveToElement:
            d.append(QString("M %1 %2").arg(e.x).arg(e.y));
            break;
        case QPainterPath::LineToElement:
            d.append(QString("L %1 %2").arg(e.x).arg(e.y));
            break;
        default:
            //TODO: CurveToElement, CurveToElementDataElement
            qDebug() << "This element unhandled";
        }
    }
    return d;
}

void ODrawToOdf::setShapeMirroring(const MSO::OfficeArtSpContainer& o, Writer& out)
{
    if (o.shapeProp.fFlipV) {
        out.xml.addAttribute("draw:mirror-vertical", "true");
    }
    if (o.shapeProp.fFlipH) {
        out.xml.addAttribute("draw:mirror-horizontal", "true");
    }
}
