/* This file is part of the KOffice project
   Copyright (C) 2003 Werner Trobin <trobin@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>
   Copyright (C) 2010 KO GmbH <jos.van.den.oever@kogmbh.com>
   Copyright (C) 2010, 2011 Matus Uzak <matus.uzak@ixonos.com>
   Copyright (C) 2010, 2011 Matus Hanzes <matus.hanzes@ixonos.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the Library GNU General Public
   version 2 of the License, or (at your option) version 3 or,
   at the discretion of KDE e.V (which shall act as a proxy as in
   section 14 of the GPLv3), any later version..

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "generated/leinputstream.h"
#include "ODrawToOdf.h"
#include "drawstyle.h"
#include "pictures.h"

#include "graphicshandler.h"
#include "document.h"
#include "conversion.h"
#include "msodraw.h"

#include <KoStoreDevice.h>
#include <KoGenStyle.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <QtGui/QColor>
#include <QByteArray>

using namespace wvWare;
using namespace MSO;

using Conversion::twipsToPt;

//#define DEBUG_GHANDLER

// Specifies the format of the picture data for the PICF structure.
enum
{
    MM_SHAPE = 0x0064,
    MM_SHAPEFILE = 0x0066
};

namespace
{
QString format(double v) {
    static const QString f("%1");
    static const QString e("");
    static const QRegExp r("\\.?0+$");
    return f.arg(v, 0, 'f').replace(r, e);
}
QString pt(double v) {
    static const QString pt("pt");
    return format(v) + pt;
}
QString percent(double v) {
    return format(v) + '%';
}
QString mm(double v) {
    static const QString mm("mm");
    return format(v) + mm;
}
}

/*
 * ************************************************
 * Drawing Writer
 * ************************************************
 */
DrawingWriter::DrawingWriter(KoXmlWriter& xmlWriter, KoGenStyles& styles,  bool stylesxml_)
        : Writer(xmlWriter, styles, stylesxml_),
          xLeft(0),
          xRight(0),
          yTop(0),
          yBottom(0)
{
    scaleX = 25.4 / 1440;
    scaleY = 25.4 / 1440;
}

qreal DrawingWriter::vLength()
{
    return Writer::vLength(yBottom - yTop);
}

qreal DrawingWriter::hLength()
{
    return Writer::hLength(xRight - xLeft);
}

qreal DrawingWriter::vOffset()
{
    return Writer::vOffset(yTop);
}

qreal DrawingWriter::hOffset()
{
    return Writer::hOffset(xLeft);
}

void DrawingWriter::setRectangle(wvWare::Word97::FSPA& spa)
{
    xLeft = spa.xaLeft;
    xRight = spa.xaRight;
    yTop = spa.yaTop;
    yBottom = spa.yaBottom;
}

//FIXME: It doesn't make sense with current initialization, because when first
//time called, scaleX and scaleY are both set to zero!  Both xOffset and
//yOffset doesn't change!
void DrawingWriter::setGroupRectangle(MSO::OfficeArtFSPGR& fspgr)
{
    if (fspgr.xRight == fspgr.xLeft) {
        return;
    }

    if (fspgr.yBottom == fspgr.yTop) {
        return;
    }

    xOffset = xOffset + xLeft*scaleX;
    yOffset = yOffset + yTop*scaleY;

    scaleX = scaleX * (xRight - xLeft)/(qreal)(fspgr.xRight - fspgr.xLeft);
    scaleY = scaleY * (yBottom - yTop)/(qreal)(fspgr.yBottom - fspgr.yTop);

    xOffset = xOffset - fspgr.xLeft * scaleX;
    yOffset = yOffset - fspgr.yTop * scaleY;
}

void DrawingWriter::setChildRectangle(MSO::OfficeArtChildAnchor& anchor)
{
    xLeft = anchor.xLeft;
    xRight = anchor.xRight;
    yTop = anchor.yTop;
    yBottom = anchor.yBottom;
}

/*
 * ************************************************
 * Graphics Handler
 * ************************************************
 */
KWordGraphicsHandler::KWordGraphicsHandler(Document* doc,
                                           KoXmlWriter* bodyWriter, KoXmlWriter* manifestWriter,
                                           KoStore* store, KoGenStyles* mainStyles,
                                           const wvWare::Drawings* p_drawings,
                                           const wvWare::Word97::FIB& fib)
: QObject()
, m_document(doc)
, m_store(store)
, m_bodyWriter(bodyWriter)
, m_manifestWriter(manifestWriter)
, m_mainStyles(mainStyles)
, m_drawings(p_drawings)
, m_fib(fib)
, m_pOfficeArtHeaderDgContainer(0)
, m_pOfficeArtBodyDgContainer(0)
, m_processingGroup(false)
, m_objectType(Inline)
, m_rgbUid(0)
, m_zIndex(0)
, m_picf(0)
, m_pSpa(0)
{
    kDebug(30513) ;
    init();
}

KWordGraphicsHandler::~KWordGraphicsHandler()
{
    delete m_pOfficeArtHeaderDgContainer;
    delete m_pOfficeArtBodyDgContainer;
}

/*
 * NOTE: All containers parsed by this function are optional.
 */
void KWordGraphicsHandler::init()
{
    kDebug(30513);

    parseOfficeArtContainer();

    //create default GraphicStyle using information from OfficeArtDggContainer
    defineDefaultGraphicStyle(m_mainStyles);

    //parse and store floating pictures  
    parseFloatingPictures();
    m_picNames = createFloatingPictures(m_store, m_manifestWriter);

    //Provide the backgroud color information to the Document
    DrawStyle ds = getBgDrawStyle();
    if (ds.fFilled()) {
        MSO::OfficeArtCOLORREF fc = ds.fillColor();
        QColor color = QColor(fc.red, fc.green, fc.blue);
        QString tmp = color.name();
        if (tmp != m_document->currentBgColor()) {
            m_document->updateBgColor(tmp);
        }
    }
}

void KWordGraphicsHandler::emitTextBoxFound(unsigned int index, bool stylesxml)
{
    emit textBoxFound(index, stylesxml);
}

void KWordGraphicsHandler::setBodyWriter(KoXmlWriter* writer)
{
    m_bodyWriter = writer;
}

DrawStyle KWordGraphicsHandler::getBgDrawStyle()
{
    const OfficeArtSpContainer* shape = 0;
    if (m_pOfficeArtBodyDgContainer) {
        shape = (m_pOfficeArtBodyDgContainer->shape).data();
    }
    return DrawStyle(&m_officeArtDggContainer, 0, shape);
}

void KWordGraphicsHandler::handleInlineObject(const wvWare::PictureData& data)
{
    kDebug(30513) ;
    quint32 size = (data.picf->lcb - data.picf->cbHeader);

#ifdef DEBUG_GHANDLER
    kDebug(30513) << "\nPICF DEBUG:"
                  << "\nPICF size: 0x" << hex << data.picf->cbHeader
                  << "\nOfficeArtInlineSpContainer size:" << dec << size
                  << "\nStorage Format: 0x" << hex << data.picf->mfp.mm;
#endif

    //the picture is store in some external file
    if (data.picf->mfp.mm == MM_SHAPEFILE) {
        DrawingWriter out(*m_bodyWriter, *m_mainStyles, m_document->writingHeader());
        m_objectType = Inline;
        m_picf = data.picf;
        insertEmptyInlineFrame(out);
        return;
    }

    // going to parse and process the Data stream content
    LEInputStream* in = m_document->dataStream();
    if (!in) {
        kDebug(30513) << "Data stream not provided, no access to inline shapes!";
        return;
    }
    if (data.fcPic > in->getSize()) {
        kDebug(30513) << "OfficeArtInlineSpContainer offset out of range, skipping!";
        return;
    }

#ifdef DEBUG_GHANDLER
    kDebug(30513) << "\nCurrent stream position:" << in->getPosition()
                  << "\nOfficeArtInlineSpContainer offset:" << dec << data.fcPic;
#endif

    // parse the OfficeArtInlineSpContainer and rewind the stream
    LEInputStream::Mark _zero;
    _zero = in->setMark();
    in->skip(data.fcPic);

    OfficeArtInlineSpContainer co;
    try {
        parseOfficeArtInlineSpContainer(*in, co);
    } catch (IncorrectValueException _e) {
        kDebug(30513) << _e.msg;
        return;
    } catch (EOFException _e) {
        kDebug(30513) << _e.msg;
        return;
    }
    in->rewind(_zero);

    int n = (data.fcPic + size) - in->getPosition();
    if (n) {
        kDebug(30513) << n << "bytes left while parsing OfficeArtInlineSpContainer";
    }

    PictureReference ref;
    // store picture data if present and update m_picNames
    m_store->enterDirectory("Pictures");
    foreach (const OfficeArtBStoreContainerFileBlock& block, co.rgfb) {
        const OfficeArtFBSE* fbse = block.anon.get<MSO::OfficeArtFBSE>();
        if (!fbse) {
            kDebug(30513) << "Warning: FBSE container not found, skipping ";
        } else {
            //check if this BLIP is already in hash table
            if (m_picNames.contains(fbse->rgbUid)) {
                ref.uid = fbse->rgbUid;
                continue;
            } else {
                ref = savePicture(block, m_store);
                if (ref.name.length() == 0) {
                    kDebug(30513) << "empty name in picture reference";
                    break;
                }
                m_manifestWriter->addManifestEntry("Pictures/" + ref.name, ref.mimetype);
                m_picNames[ref.uid] = ref.name;
            }
        }
    }
    m_store->leaveDirectory();

    bool inStylesXml = m_document->writingHeader();
    DrawingWriter out(*m_bodyWriter, *m_mainStyles, inStylesXml);

    //global attributes
    m_objectType = Inline;
    m_rgbUid = ref.uid;
    m_picf = data.picf;

    const OfficeArtSpContainer* o = &(co.shape);
    processDrawingObject(*o, out);
}

void KWordGraphicsHandler::handleFloatingObject(unsigned int globalCP)
{
    kDebug(30513) << "globalCP" << globalCP ;

    if (!m_drawings) {
        return;
    }

    const PLCF<Word97::FSPA>* plcfSpa = 0;
    MSO::OfficeArtDgContainer* dg = 0;
    uint threshold = 0;

    if (m_document->writingHeader()) {
        plcfSpa = m_drawings->getSpaHdr();
        dg = m_pOfficeArtHeaderDgContainer;
        threshold = m_fib.ccpText + m_fib.ccpFtn;
    } else {
        plcfSpa = m_drawings->getSpaMom();
        dg = m_pOfficeArtBodyDgContainer;
    }

    if (!plcfSpa) {
        kDebug(30513) << "MISSING plcfSpa!";
        return;
    }
    if (!dg) {
        kDebug(30513) << "MISSING OfficeArtDgContainer!";
        return;
    }

    PLCFIterator<Word97::FSPA> it(plcfSpa->at(0));
    for (size_t i = 0; i < plcfSpa->count(); i++, ++it) {
        kDebug(30513) << "FSPA start:" << it.currentStart();
        kDebug(30513) << "FSPA spid:" << it.current()->spid;

        if ((it.currentStart() + threshold) == globalCP) {
            bool inStylesXml = m_document->writingHeader();
            DrawingWriter out(*m_bodyWriter, *m_mainStyles, inStylesXml);

            //global attributes
            m_objectType = Floating;
            m_pSpa = it.current();
            m_zIndex = 0;

            locateDrawing((dg->groupShape).data(), it.current(), (uint)it.current()->spid, out);
            return;
        }
    }
}

void KWordGraphicsHandler::locateDrawing(const MSO::OfficeArtSpgrContainer* spgr,
                                         wvWare::Word97::FSPA* spa, uint spid,
                                         DrawingWriter& out)
{
    if (!spgr) {
        return;
    }

    //FIXME: combine childAnchor, shapeGroup coordinates with information from
    //clientAnchor pointing to the SPA structure!

    //NOTE: The OfficeArtSpgrContainer record specifies a container for groups
    //(4) of shapes.  The group (4) container contains a variable number of
    //shape containers and other group (4) containers.  Each group (4) is a
    //shape.  The first container MUST be an OfficeArtSpContainer record, which
    //MUST contain shape information for the group.  MS-ODRAW, 2.2.16

    foreach (const OfficeArtSpgrContainerFileBlock& co, spgr->rgfb) {

        if (co.anon.is<OfficeArtSpgrContainer>()) {
            const OfficeArtSpContainer* first = 
                (*co.anon.get<OfficeArtSpgrContainer>()).rgfb[0].anon.get<OfficeArtSpContainer>();
            if (first && first->shapeProp.spid == spid) {
                out.setRectangle(*spa);
                processGroupShape(*co.anon.get<OfficeArtSpgrContainer>(), out);
                break;
            } else {
                m_zIndex = m_zIndex + (*co.anon.get<OfficeArtSpgrContainer>()).rgfb.size();
            }
        } else {
            const OfficeArtSpContainer &sp = *co.anon.get<OfficeArtSpContainer>();
            if (sp.shapeProp.fGroup) {
		if (sp.shapeGroup) {
                    out.setGroupRectangle(*sp.shapeGroup);
                }
                if (sp.shapeProp.spid == spid) {
                    kDebug(30513) << "An unprocessed shape storing information for the group is referred from text!";
                }
            } else if (sp.shapeProp.spid == spid) {
                out.setRectangle(*spa);
                processDrawingObject(sp, out);
                break;
            }
            m_zIndex++;
        }
    }
}

void KWordGraphicsHandler::processGroupShape(const MSO::OfficeArtSpgrContainer& o, DrawingWriter& out)
{
    if (o.rgfb.size() < 2) {
        return;
    }
    const OfficeArtSpContainer *first = o.rgfb[0].anon.get<OfficeArtSpContainer>();

    //create graphic style for the group shape
    QString styleName;
    KoGenStyle style(KoGenStyle::GraphicAutoStyle, "graphic");
    style.setAutoStyleInStylesDotXml(out.stylesxml);

    DrawStyle ds(&m_officeArtDggContainer, first);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, out.styles);
    definePositionAttributes(style, ds);
    defineWrappingAttributes(style, ds);
    styleName = out.styles.insert(style, "gr");

    out.xml.startElement("draw:g");
    out.xml.addAttribute("draw:style-name", styleName);
    setAnchorTypeAttribute(out);
    setZIndexAttribute(out);
    m_processingGroup = true;

    if (first && first->shapeGroup) {
        //process shape information for the group
        out.setGroupRectangle(*first->shapeGroup);
    }

    for (int i = 1; i < o.rgfb.size(); ++i) {
        if (o.rgfb[i].anon.is<OfficeArtSpContainer>()) {
            OfficeArtSpContainer sp = *o.rgfb[i].anon.get<OfficeArtSpContainer>();
            if (sp.childAnchor) {
                out.setChildRectangle(*sp.childAnchor);
            }
            processDrawingObject(sp, out);
        } else {
            kDebug(30513) << "Nested group shapes not supported!";
            //TODO: nested group shape!
        }
    }
    out.xml.endElement(); // draw:g
    m_processingGroup = false;
}

void KWordGraphicsHandler::processDrawingObject(const MSO::OfficeArtSpContainer& o, DrawingWriter out)
{
    kDebug(30513);

    DrawStyle ds(0, &o);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);

    kDebug(30513) << "shapeType: " << hex << o.shapeProp.rh.recInstance;
    kDebug(30513) << "grupShape: " << o.shapeProp.fGroup;
    kDebug(30513) << "Selected properties: ";
    kDebug(30513) << "pib: " << ds.pib();

    switch (o.shapeProp.rh.recInstance) {
    case msosptTextBox:
        kDebug(30513)<< "processing TextBox";
        processTextBox(o, out);
        break;
    case msosptRectangle:
        if (ds.fHorizRule()) {
            kDebug(30513)<< "processing Line";
            processLineShape(o, out);
        } else {
            odrawtoodf.processDrawingObject(o, out);
        }
        break;
    case msosptPictureFrame:
        kDebug(30513)<< "processing PictureFrame";
        if (m_objectType == Inline) {
            processInlinePictureFrame(o, out);
        } else {
            processFloatingPictureFrame(o, out);
        }
        break;
    case msosptHostControl:
        kDebug(30513)<< "processing Host Control";
        processTextBox(o, out);
        break;
    default:
        odrawtoodf.processDrawingObject(o, out);
        break;
    }
}

void KWordGraphicsHandler::parseOfficeArtContainer()
{
    kDebug(30513);

    if (!m_fib.lcbDggInfo) return;

    POLE::Stream& stream = m_document->poleTableStream();
    if (stream.fail()) {
        kDebug(30513) << "Table stream not provided, no access to OfficeArt file records!";
        return;
    }

    QByteArray array;
    QBuffer buffer;
    array.resize(m_fib.lcbDggInfo);
    stream.seek(m_fib.fcDggInfo);
    unsigned long n = stream.read((unsigned char*) array.data(), m_fib.lcbDggInfo);
    if (n != m_fib.lcbDggInfo) {
        kError(30513) << "Error while reading from " << stream.fullName().data() << "stream";
        return;
    }

    buffer.setData(array);
    buffer.open(QIODevice::ReadOnly);
    LEInputStream in(&buffer);

    //parse OfficeArfDggContainer from msdoc
    try {
        parseOfficeArtDggContainer(in, m_officeArtDggContainer);
    }
    catch (IOException e) {
        kDebug(30513) << "caught IOException while parsing parseOfficeArtDggContainer";
        return;
    }
    catch (...) {
        kDebug(30513) << "caught unknown exception while parsing parseOfficeArtDggContainer";
        return;
    }
    kDebug(30513) << "OfficeArtDggContainer parsed successfully" ;

    // parse drawingsVariable from msdoc
    // 0 - next OfficeArtDgContainer belongs to Main document;
    // 1 - next OfficeArtDgContainer belongs to Header Document
    unsigned char drawingsVariable = 0;
    try {
        drawingsVariable = in.readuint8();
    }
    catch (IOException e) {
        kDebug(30513) << "caught IOException while parsing drawingsVariable ";
        return;
    }
    catch (...) {
        kDebug(30513) << "caught unknown exception while parsing drawingsVariable";
        return;
    }

    //parse OfficeArfDgContainer from msdoc
    OfficeArtDgContainer *pDgContainer = 0;
    try {
        pDgContainer = new OfficeArtDgContainer();
        if (drawingsVariable == 0) {
            m_pOfficeArtBodyDgContainer = pDgContainer;
        } else {
            m_pOfficeArtHeaderDgContainer = pDgContainer;
        }
        parseOfficeArtDgContainer(in, *pDgContainer);
    }
    catch (IOException e) {
        kDebug(30513) << "caught IOException while parsing OfficeArtDgContainer ";
        return;
    }
    catch (...) {
        kDebug(30513) << "caught unknown exception while parsing OfficeArtDgContainer";
        return;
    }

    // parse drawingsVariable from msdoc
    // 0 - next OfficeArtDgContainer belongs to Main document;
    // 1 - next OfficeArtDgContainer belongs to Header Document
    try {
        drawingsVariable = in.readuint8();
    }
    catch (IOException e) {
        kDebug(30513) << "caught IOException while parsing second drawingsVariable ";
        //wvlog << "in position: " << in.getPosition() << std::endl;
        return;
    }
    catch (...) {
        kDebug(30513) << "caught unknown exception while parsing second drawingsVariable";
        return;
    }

    //parse OfficeArfDgContainer from msdoc
    pDgContainer = 0;
    try {
        pDgContainer = new OfficeArtDgContainer();
        if (drawingsVariable == 0) {
            if (m_pOfficeArtBodyDgContainer != 0){
                delete m_pOfficeArtBodyDgContainer;
            }
            m_pOfficeArtBodyDgContainer = pDgContainer;
        }
        else {
            if (m_pOfficeArtHeaderDgContainer != 0) {
                delete m_pOfficeArtHeaderDgContainer;
            }
            m_pOfficeArtHeaderDgContainer = pDgContainer;
        }
        parseOfficeArtDgContainer(in, *pDgContainer);
    }
    catch (IOException e) {
        kDebug(30513) << "caught IOException while parsing second OfficeArtDgContainer ";
        return;
    }
    catch (...) {
        kDebug(30513) << "caught unknown exception while parsing second OfficeArtDgContainer";
        return;
    }

    quint32 r = buffer.size() - in.getPosition();
    if (r > 0) {
        kError(30513) << r << "bytes left to parse from the OfficeArtDggContainer";
    }
}

void KWordGraphicsHandler::parseFloatingPictures(void)
{
    kDebug(30513);

    // WordDocument stream equals the Delay stream, [MS-DOC] — v20101219
    LEInputStream& in = m_document->wdocumentStream();
    const OfficeArtBStoreContainer* blipStore = m_officeArtDggContainer.blipStore.data();
    if (!blipStore) return;

    for (int i = 0; i < blipStore->rgfb.size(); i++) {
        OfficeArtBStoreContainerFileBlock block = blipStore->rgfb[i];

	//looking for the missing content of OfficeArtFBSE
        if (block.anon.is<OfficeArtFBSE>()) {
            OfficeArtFBSE* fbse = block.anon.get<OfficeArtFBSE>();
            if (!fbse->embeddedBlip) {

                //NOTE: An foDelay value of 0xffffffff specifies that the
                //file is not in the delay stream and cRef must be zero.

                //NOTE: A cRef value of 0x00000000 specifies an empty slot
                //in the OfficeArtBStoreContainer.

                if (fbse->foDelay != 0xffffffff) {
                    if (!fbse->cRef) {
                        kDebug(30513) << "Strange, no references to this BLIP, skipping";
                        continue;
                    }
                    LEInputStream::Mark _zero;
                    _zero = in.setMark();
                    in.skip(fbse->foDelay);

                    //let's check the record header if there's a BLIP stored
                    LEInputStream::Mark _m;
                    _m = in.setMark();
                    OfficeArtRecordHeader rh;
                    parseOfficeArtRecordHeader(in, rh);
                    in.rewind(_m);
                    if ( !(rh.recType >= 0xF018 && rh.recType <= 0xF117) ) {
                        continue;
                    }
                    fbse->embeddedBlip = QSharedPointer<OfficeArtBlip>(new OfficeArtBlip(fbse));
                    parseOfficeArtBlip(in, *(fbse->embeddedBlip.data()));
                    in.rewind(_zero);
                }
            } //else there's an OfficeArtBlip inside
        }
    }
}

QMap<QByteArray, QString>
KWordGraphicsHandler::createFloatingPictures(KoStore* store, KoXmlWriter* manifest)
{
    PictureReference ref;
    QMap<QByteArray, QString> fileNames;

    const OfficeArtBStoreContainer* blipStore = m_officeArtDggContainer.blipStore.data();
    if (blipStore) {
        store->enterDirectory("Pictures");
        foreach (const OfficeArtBStoreContainerFileBlock& block, blipStore->rgfb) {
            ref = savePicture(block, store);
            if (ref.name.length() == 0) {
                kDebug(30513) << "Note: Empty picture reference, probably an empty slot";
                continue;
	    }
            manifest->addManifestEntry("Pictures/" + ref.name, ref.mimetype);
            fileNames[ref.uid] = ref.name;
        }
        store->leaveDirectory();
    }
    return fileNames;
}

QString KWordGraphicsHandler::getPicturePath(quint32 pib) const
{
    quint32 n = pib - 1;
    quint32 offset = 0;
    QByteArray rgbUid = getRgbUid(m_officeArtDggContainer, n, offset);

    if (rgbUid.length()) {
        if (m_picNames.contains(rgbUid)) {
            return "Pictures/" + m_picNames[rgbUid];
        } else {
            qDebug() << "UNKNOWN picture reference!";
        }
    }
    return QString();
}

void KWordGraphicsHandler::defineDefaultGraphicStyle(KoGenStyles* styles)
{
    // write style <style:default-style style:family="graphic">
    KoGenStyle style(KoGenStyle::GraphicStyle, "graphic");
    style.setDefaultStyle(true);
    DrawStyle ds(&m_officeArtDggContainer);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, *styles);
    styles->insert(style);
}

void KWordGraphicsHandler::defineWrappingAttributes(KoGenStyle& style, const DrawStyle& ds)
{
    if (m_processingGroup) return;
    if (m_objectType == Inline) return;

    const KoGenStyle::PropertyType gt = KoGenStyle::GraphicType;
    wvWare::Word97::FSPA* spa = m_pSpa;

    // style:number-wrapped-paragraphs
    // style:run-through
    // style:wrap
    // style:wrap-contour
    // style:wrap-contour-mode
    // style:wrap-dynamic-threshold
    if (spa != 0) {
        bool check_wrk = false;
        switch (spa->wr) {
        case 0: //wrap around the object
        case 2: //square wrapping
            check_wrk = true;
            break;
        case 1: //top and bottom wrapping
            style.addProperty("style:wrap", "none", gt);
            break;
        case 3: //in front or behind the text
            style.addProperty("style:wrap", "run-through", gt);
            //check if shape is behind the text
            if ((spa->fBelowText == 1) || (ds.fBehindDocument())) {
                style.addProperty("style:run-through", "background", gt);
            } else {
                style.addProperty("style:run-through", "foreground", gt);
            }
            break;
        case 4: //tight wrapping
            check_wrk = true;
            style.addProperty("style:wrap-contour", "true", gt);
            style.addProperty("style:wrap-contour-mode", "outside", gt);
            break;
        case 5: //through wrapping
            check_wrk = true;
            style.addProperty("style:wrap-contour", "true", gt);
            style.addProperty("style:wrap-contour-mode", "full", gt);
            break;
        }
        //check details of the text wrapping around this shape
        if (check_wrk) {
            switch (spa->wrk) {
            case 0:
                style.addProperty("style:wrap", "parallel", gt);
                break;
            case 1:
                style.addProperty("style:wrap", "left", gt);
                break;
            case 2:
                style.addProperty("style:wrap", "right", gt);
                break;
            case 3:
                style.addProperty("style:wrap", "biggest", gt);
                break;
            }
        }
        // ODF-1.2: specifies the number of paragraphs that can wrap around a
        // frame if wrap mode is in {left, right, parallel, dynamic} and anchor
        // type is in {char, paragraph}
        if ((spa->wr != 1) && (spa->wr != 3)) {
            style.addProperty("style:number-wrapped-paragraphs", "no-limit");
        }
    } else {
        style.addProperty("style:wrap", "run-through", gt);
        if (ds.fBehindDocument()) {
            style.addProperty("style:run-through", "background", gt);
        } else {
            style.addProperty("style:run-through", "foreground", gt);
        }
    }

    // margins are related to text wrapping
    // fo:margin-bottom
    // fo:margin-left
    // fo:margin-right
    // fo:margin-top
    style.addPropertyPt("style:margin-bottom", ds.dyWrapDistBottom()/12700., gt);
    style.addPropertyPt("style:margin-left", ds.dxWrapDistLeft()/12700., gt);
    style.addPropertyPt("style:margin-right", ds.dxWrapDistRight()/12700., gt);
    style.addPropertyPt("style:margin-top", ds.dyWrapDistTop()/12700., gt);
}

void KWordGraphicsHandler::definePositionAttributes(KoGenStyle& style, const DrawStyle& ds)
{
    if (m_processingGroup) return;

    const KoGenStyle::PropertyType gt = KoGenStyle::GraphicType;
    if (m_objectType == Inline) {
        style.addProperty("style:vertical-rel", "baseline", gt);
        style.addProperty("style:vertical-pos", "top", gt);
    } else {
        style.addProperty("style:horizontal-pos", getHorizontalPos(ds.posH()), gt);
        style.addProperty("style:horizontal-rel", getHorizontalRel(ds.posRelH()), gt);
        style.addProperty("style:vertical-pos", getVerticalPos(ds.posV()), gt);
        style.addProperty("style:vertical-rel", getVerticalRel(ds.posRelV()), gt);
    }
}

void KWordGraphicsHandler::setAnchorTypeAttribute(DrawingWriter& out)
{
    if (m_processingGroup) return;

    // text:anchor-type
    if (m_objectType == Inline) {
        out.xml.addAttribute("text:anchor-type", "as-char");
    } else {
        out.xml.addAttribute("text:anchor-type", "char");
    }
}

void KWordGraphicsHandler::setZIndexAttribute(DrawingWriter& out)
{
    if (m_processingGroup) return;

    // draw:z-index
    if (m_objectType == Floating) {
        out.xml.addAttribute("draw:z-index", m_zIndex);
    }
}

void KWordGraphicsHandler::processTextBox(const MSO::OfficeArtSpContainer& o, DrawingWriter out)
{
    QString styleName;
    KoGenStyle style(KoGenStyle::GraphicAutoStyle, "graphic");
    style.setAutoStyleInStylesDotXml(out.stylesxml);

    DrawStyle ds(&m_officeArtDggContainer, &o);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, out.styles);
    definePositionAttributes(style, ds);
    defineWrappingAttributes(style, ds);
    styleName = out.styles.insert(style);

    out.xml.startElement("draw:frame");
    out.xml.addAttribute("draw:style-name", styleName);

    setAnchorTypeAttribute(out);
    setZIndexAttribute(out);

    switch(ds.txflTextFlow()) {
    case 1: //msotxflTtoBA up-down
    case 3: //msotxflTtoBN up-down
    case 5: //msotxflVertN up-down
        out.xml.addAttribute("svg:width", mm(out.vLength()));
        out.xml.addAttribute("svg:height", mm(out.hLength()));
        out.xml.addAttribute("draw:transform","matrix(0 1 -1 0 " +
                mm(((Writer *)&out)->hOffset(out.xRight)) + " " + mm(out.vOffset()) + ")");
        break;
    case 2: //msotxflBtoT down-up
        out.xml.addAttribute("svg:width", mm(out.vLength()));
        out.xml.addAttribute("svg:height", mm(out.hLength()));
        out.xml.addAttribute("draw:transform","matrix(0 -1 1 0 " +
               mm(out.hOffset()) + " " + mm(((Writer *)&out)->vOffset(out.yBottom)) + ")");
         break;
    default : //standard text flow
        out.xml.addAttribute("svg:width", mm(out.hLength()));
        out.xml.addAttribute("svg:height", mm(out.vLength()));
        out.xml.addAttribute("svg:x", mm(out.hOffset()));
        out.xml.addAttribute("svg:y", mm(out.vOffset()));
    }

    out.xml.startElement("draw:text-box");

    if (o.clientTextbox) {
        const DocOfficeArtClientTextBox* tb = o.clientTextbox->anon.get<DocOfficeArtClientTextBox>();
        if (tb) {
            uint index = (tb->clientTextBox / 0x10000) - 1;
            emit textBoxFound(index, out.stylesxml);
        } else {
            kDebug(30513) << "DocOfficeArtClientTextBox missing!";
        }
    } else {
        kDebug(30513) << "OfficeArtClientTextBox missing!";
    }

    out.xml.endElement(); //draw:text-box
    out.xml.endElement(); //draw:frame
}

void KWordGraphicsHandler::processInlinePictureFrame(const MSO::OfficeArtSpContainer& o, DrawingWriter& out)
{
    kDebug(30513) ;

    QString styleName;
    KoGenStyle style(KoGenStyle::GraphicAutoStyle, "graphic");
    style.setAutoStyleInStylesDotXml(out.stylesxml);

    DrawStyle ds(&m_officeArtDggContainer, &o);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, out.styles);
    definePositionAttributes(style, ds);
    styleName = out.styles.insert(style);

    out.xml.startElement("draw:frame");
    out.xml.addAttribute("draw:style-name", styleName);
    setAnchorTypeAttribute(out);
    setZIndexAttribute(out);

    double hscale = m_picf->mx / 1000.0;
    double vscale = m_picf->my / 1000.0;
    out.xml.addAttributePt("svg:width", twipsToPt(m_picf->dxaGoal) * hscale);
    out.xml.addAttributePt("svg:height", twipsToPt(m_picf->dyaGoal) * vscale);

    QString url;
    QString name = m_picNames.value(m_rgbUid);
    if (!name.isEmpty()) {
        url.append("Pictures/");
        url.append(name);
    } else {
        // if the image cannot be found, just place an empty frame
        out.xml.endElement(); //draw:frame
        return;
    }
    //TODO: process border information (complex properties)

    out.xml.startElement("draw:image");
    out.xml.addAttribute("xlink:href", url);
    out.xml.addAttribute("xlink:type", "simple");
    out.xml.addAttribute("xlink:show", "embed");
    out.xml.addAttribute("xlink:actuate", "onLoad");
    out.xml.endElement(); //draw:image
    out.xml.endElement(); //draw:frame
    return;
}

void KWordGraphicsHandler::processFloatingPictureFrame(const MSO::OfficeArtSpContainer& o, DrawingWriter& out)
{
    kDebug(30513) ;

    QString styleName;
    KoGenStyle style(KoGenStyle::GraphicAutoStyle, "graphic");
    style.setAutoStyleInStylesDotXml(out.stylesxml);

    DrawStyle ds(&m_officeArtDggContainer, &o);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, out.styles);
    definePositionAttributes(style, ds);
    defineWrappingAttributes(style, ds);
    styleName = out.styles.insert(style);

    QString url;
    if (ds.pib()) {
        url = getPicturePath(ds.pib());
    } else {
        // Does not make much sense to display an empty frame, following
        // PPT->ODP filters of both OOo and MS Office 2007.
        return;
    }
    out.xml.startElement("draw:frame");
    out.xml.addAttribute("draw:style-name", styleName);
    setAnchorTypeAttribute(out);
    setZIndexAttribute(out);

    out.xml.addAttribute("svg:width", mm(out.hLength()));
    out.xml.addAttribute("svg:height", mm(out.vLength()));
    out.xml.addAttribute("svg:x", mm(out.hOffset()));
    out.xml.addAttribute("svg:y", mm(out.vOffset()));

    //if the image cannot be found, just place an empty frame
    if (url.isEmpty()) {
        out.xml.endElement(); //draw:frame
        return;
    }
    out.xml.startElement("draw:image");
    out.xml.addAttribute("xlink:href", url);
    out.xml.addAttribute("xlink:type", "simple");
    out.xml.addAttribute("xlink:show", "embed");
    out.xml.addAttribute("xlink:actuate", "onLoad");
    out.xml.endElement(); //draw:image

    //check for user edited wrap points
    if (ds.fEditedWrap()) {
        QString points;
        IMsoArray _v = ds.pWrapPolygonVertices_complex();
        if (_v.data.size()) {
            //_v.data is an array of POINTs, MS-ODRAW, page 89
            QByteArray a, a2;
            int* p;

            for (int i = 0, offset = 0; i < _v.nElems; i++, offset += _v.cbElem) {
                // x coordinate of this point
                a = _v.data.mid(offset, _v.cbElem);
                a2 = a.mid(0, _v.cbElem / 2);
                p = (int*) a2.data();
                points.append(QString::number(twipsToPt(*p)));
                points.append(",");
                // y coordinate of this point
                a2 = a.mid(_v.cbElem / 2, _v.cbElem / 2);
                p = (int*) a2.data();
                points.append(QString::number(twipsToPt(*p)));
                points.append(" ");
            }
            points.chop(1); //remove last space
        }
        out.xml.startElement("draw:contour-polygon");
        out.xml.addAttribute("draw:points", points);
        out.xml.endElement(); //draw:contour-polygon
    }
    out.xml.endElement(); //draw:frame
    return;
}

void KWordGraphicsHandler::processLineShape(const MSO::OfficeArtSpContainer& o, DrawingWriter& out)
{
    kDebug(30513) ;

    QString styleName;
    KoGenStyle style(KoGenStyle::GraphicAutoStyle, "graphic");
    style.setAutoStyleInStylesDotXml(out.stylesxml);

    DrawStyle ds(&m_officeArtDggContainer, &o);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, out.styles);
    definePositionAttributes(style, ds);
    //TODO: maybe wrapping related attributes have to be set

    //NOTE: also the dxWidthHR propertie may store the width information
    float width = ds.pctHR() / 10.0;

    QString hrAlign;
    QString xPos = QString::number(0.0f).append("in");
    const float base_width = 6.1378;

    switch (ds.alignHR()) {
    case hAlignLeft:
        hrAlign = QString("left");
        xPos = QString::number(0.0f).append("in");
        break;
    case hAlignCenter:
        hrAlign = QString("center");
        xPos = QString::number((base_width / 2.0) - ((width * base_width) / 200.0)).append("in");
        break;
    case hAlignRight:
        hrAlign = QString("right");
        xPos = QString::number(base_width - (width * base_width) / 100.0).append("in");
        break;
    }
    //process the content of HR specific properties
    style.addProperty("draw:textarea-horizontal-align", hrAlign);
    style.addProperty("draw:textarea-vertical-align", "top");
    if (ds.fNoshadeHR()) {
        style.addProperty("draw:shadow", "hidden");
    }
    else {
        style.addProperty("draw:shadow", "visible");
    }
    styleName = out.styles.insert(style);

    //create a custom shape
    out.xml.startElement("draw:custom-shape");
    out.xml.addAttribute("draw:style-name", styleName);

    setAnchorTypeAttribute(out);
    setZIndexAttribute(out);

    QString height = QString::number(ds.dxHeightHR() / 1440.0f).append("in");
    out.xml.addAttribute("svg:height", height);

    QString width_str = QString::number(width * base_width / 100.0f).append("in");
    out.xml.addAttribute("svg:width", width_str);
    out.xml.addAttribute("svg:x", xPos);

    //--------------------
    out.xml.startElement("draw:enhanced-geometry");
    out.xml.addAttribute("svg:viewBox", "0 0 21600 21600");
    out.xml.addAttribute("draw:type", "rectangle");
    out.xml.addAttribute("draw:enhanced-path", "M 0 0 L 21600 0 21600 21600 0 21600 0 0 Z N");
    out.xml.endElement(); //enhanced-geometry
    out.xml.endElement(); //custom-shape
}

void KWordGraphicsHandler::insertEmptyInlineFrame(DrawingWriter& out)
{
    if (m_objectType != Inline) return;

    QString styleName;
    KoGenStyle style(KoGenStyle::GraphicAutoStyle, "graphic");
    style.setAutoStyleInStylesDotXml(out.stylesxml);

    DrawStyle ds(0, 0);
    DrawClient drawclient(this);
    ODrawToOdf odrawtoodf(drawclient);
    odrawtoodf.defineGraphicProperties(style, ds, out.styles);
    definePositionAttributes(style, ds);
    defineWrappingAttributes(style, ds);
    styleName = out.styles.insert(style);

    out.xml.startElement("draw:frame");
    out.xml.addAttribute("draw:style-name", styleName);
    setAnchorTypeAttribute(out);
    setZIndexAttribute(out);
    double hscale = m_picf->mx / 1000.0;
    double vscale = m_picf->my / 1000.0;
    out.xml.addAttributePt("svg:width", twipsToPt(m_picf->dxaGoal) * hscale);
    out.xml.addAttributePt("svg:height", twipsToPt(m_picf->dyaGoal) * vscale);
    out.xml.endElement(); //draw:frame
}

#include "graphicshandler.moc"
