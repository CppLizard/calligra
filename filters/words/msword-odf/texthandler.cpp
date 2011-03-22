/* This file is part of the KOffice project
   Copyright (C) 2002 Werner Trobin <trobin@kde.org>
   Copyright (C) 2002 David Faure <faure@kde.org>
   Copyright (C) 2008 Benjamin Cail <cricketc@gmail.com>
   Copyright (C) 2009 Inge Wallin   <inge@lysator.liu.se>
   Copyright (C) 2010, 2011 Matus Uzak <matus.uzak@ixonos.com>

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

#include "texthandler.h"
#include "conversion.h"
#include "NumberFormatParser.h"

#include <wv2/src/styles.h>
#include <wv2/src/paragraphproperties.h>
#include <wv2/src/functor.h>
#include <wv2/src/functordata.h>
#include <wv2/src/ustring.h>
#include <wv2/src/parser.h>
#include <wv2/src/fields.h>

#include <QFont>
#include <QUrl>
#include <QBuffer>
#include <qfontinfo.h>
#include <kdebug.h>
#include <klocale.h>
#include <KoGenStyle.h>
#include <KoFontFace.h>

#include "document.h"

wvWare::U8 KWordReplacementHandler::hardLineBreak()
{
    return '\n';
}

wvWare::U8 KWordReplacementHandler::nonBreakingHyphen()
{
    return '-'; // normal hyphen for now
}

wvWare::U8 KWordReplacementHandler::nonRequiredHyphen()
{
    return 0xad; // soft hyphen, according to kword.dtd
}

KWordTextHandler::KWordTextHandler(wvWare::SharedPtr<wvWare::Parser> parser, KoXmlWriter* bodyWriter, KoGenStyles* mainStyles)
    : m_mainStyles(0)
    , m_document(0)
    , m_parser(parser)
    , m_sectionNumber(0)
    , m_tocNumber(0)
    , m_footNoteNumber(0)
    , m_endNoteNumber(0)
    , m_currentTable(0)
    , m_paragraph(0)
    , m_hasStoredDropCap(false)
    , m_breakBeforePage(false)
    , m_insideFootnote(false)
    , m_footnoteWriter(0)
    , m_footnoteBuffer(0)
    , m_insideAnnotation(false)
    , m_annotationWriter(0)
    , m_annotationBuffer(0)
    , m_insideDrawing(false)
    , m_drawingWriter(0)
    , m_listLevelStyleRequired(false)
    , m_currentListDepth(-1)
    , m_currentListID(0)
    , m_fld(new fld_State())
    , m_fldStart(0)
    , m_fldEnd(0)
    , m_fldChp(0)
//     , m_index(0)
{
#ifdef IMAGE_IMPORT
    kDebug(30513) << "we have image support";
#else
    kDebug(30513) << "no image support";
#endif
    if (bodyWriter) {
        m_bodyWriter = bodyWriter; //set the pointer to bodyWriter for writing to content.xml in office:text
    } else {
        kWarning() << "No bodyWriter!";
    }
    if (mainStyles) {
        m_mainStyles = mainStyles; //for collecting most of the styles
    } else {
        kWarning() << "No mainStyles!";
    }

    //[MS-DOC] — v20101219 - 2.7.2 DopBase
    if ((m_parser->fib().nFib <= 0x00D9) && (m_parser->dop().nfcFtnRef2 == 0)) {
        m_footNoteNumber = m_parser->dop().nFtn - 1;
    }
}

KoXmlWriter* KWordTextHandler::currentWriter() const
{
    KoXmlWriter* writer = NULL;

    if (document()->writingHeader()) {
        writer = document()->headerWriter();
    }
    else if (m_insideFootnote) {
        writer = m_footnoteWriter;
    }
    else if (m_insideDrawing) {
        writer = m_drawingWriter;
    }
    else if (m_insideAnnotation) {
        writer = m_annotationWriter;
    }
    else {
        writer = m_bodyWriter;
    }
    return writer;
}

//increment m_sectionNumber
void KWordTextHandler::sectionStart(wvWare::SharedPtr<const wvWare::Word97::SEP> sep)
{
    kDebug(30513) ;

    m_sectionNumber++;
    m_sep = sep; //store sep for section end

    //type of the section break
    kDebug(30513) << "section" << m_sectionNumber << "| sep->bkc:" << sep->bkc;

    //page layout could change
    if (sep->bkc != 1) {
        emit sectionFound(sep);
    }
    //check for a column break
    if (sep->bkc == 1) {
    }
    int numColumns = sep->ccolM1 + 1;

    //NOTE: We used to save the content of a section having a "continuous
    //section break" (sep-bkc == 0) into the <text:section> element.  We are
    //now creating a <master-page> for it because the page layout or
    //header/footer content could change.
    //
    //But this way the section content is placed at a new page, which is wrong.
    //There's actually no direct support for "continuous section break" in ODF.

    //check to see if we have more than the usual one column
    if ( numColumns > 1 )
    {
        QString sectionStyleName = "Sect";
        sectionStyleName.append(QString::number(m_sectionNumber));
        KoGenStyle sectionStyle(KoGenStyle::SectionAutoStyle, "section");
        //parse column info
        QBuffer buf;
        buf.open(QIODevice::WriteOnly);
        KoXmlWriter writer(&buf);
        writer.startElement("style:columns");
        //ccolM1 = number of columns in section
        kDebug(30513) << "ccolM1 = " << sep->ccolM1;
        writer.addAttribute("fo:column-count", numColumns);
        //dxaColumns = distance that will be maintained between columns
        kDebug(30513) << "dxaColumns = " << sep->dxaColumns;
        writer.addAttributePt("fo:column-gap", sep->dxaColumns / 20.0);
        //check for vertical separator
        if (sep->fLBetween) {
            writer.startElement("style:column-sep");
            //NOTE: just a random default for now
            writer.addAttribute("style:width", "0.0693in");
            writer.endElement();//style:column-sep
        }
        //individual column styles
        if (numColumns > 1) {
            for (int i = 0; i < numColumns; ++i) {
                writer.startElement("style:column");
                //this should give even columns for now
                writer.addAttribute("style:rel-width", "1*");
                writer.endElement();//style:column
            }
        }
        writer.endElement();//style:columns
        QString contents = QString::fromUtf8(buf.buffer(), buf.buffer().size());
        sectionStyle.addChildElement("style:columns", contents);

//         QString footconfig("<text:notes-configuration ");
//         footconfig.append("style:num-format=\"a\"");
//         footconfig.append("/>");
//         sectionStyle.addChildElement("text:notes-configuration", footconfig);

        //add style to the collection
        sectionStyleName = m_mainStyles->insert(sectionStyle, sectionStyleName,
                                                KoGenStyles::DontAddNumberToName);
        //put necessary tags in the content
        m_bodyWriter->startElement("text:section");
        QString sectionName = "Section";
        sectionName.append(QString::number(m_sectionNumber));
        m_bodyWriter->addAttribute("text:name", sectionName);
        m_bodyWriter->addAttribute("text:style-name", sectionStyleName);
    }
    // if line numbering modulus is not 0, do line numbering
    if (sep->nLnnMod != 0) {
        if (m_mainStyles) {
            // set default line numbers style
            QString lineNumbersStyleName = QString("Standard");
            // if we have m_document
            if (m_document) {
                QString lineNumbersStyleNameTemp = m_document->lineNumbersStyleName();
                // if the style name is not empty, use it
                if (lineNumbersStyleNameTemp.isEmpty() == false) {
                    lineNumbersStyleName = lineNumbersStyleNameTemp;
                }
            }
            // prepare string for line numbering configuration
            QString lineNumberingConfig("<text:linenumbering-configuration text:style-name=\"%1\" "
                                        "style:num-format=\"1\" text:number-position=\"left\" text:increment=\"1\"/>");

            m_mainStyles->insertRawOdfStyles(KoGenStyles::DocumentStyles,
                                             lineNumberingConfig.arg(lineNumbersStyleName).toLatin1());

            KoGenStyle *normalStyle = m_mainStyles->styleForModification(QString("Normal"));

            // if got Normal style, insert line numbering configuration in it
            if (normalStyle) {
                normalStyle->addProperty(QString("text:number-lines"), QString("true"));
                normalStyle->addProperty(QString("text:line-number"), QString("0"));
            } else {
                kWarning() << "Could not find Normal style, numbering not added!";
            }

        }
    }
} //end sectionStart()

void KWordTextHandler::sectionEnd()
{
    kDebug(30513);

    //check for a table to be parsed and processed
    if (m_currentTable) {
        kWarning(30513) << "==> WOW, unprocessed table: ignoring";
    }

    if (m_sep->bkc != 1) {
        emit sectionEnd(m_sep);
    }
    if (m_sep->ccolM1 > 0) {
        m_bodyWriter->endElement();//text:section
    }
}

void KWordTextHandler::pageBreak(void)
{
    m_breakBeforePage = true;
}

//signal that there's another subDoc to parse
void KWordTextHandler::headersFound(const wvWare::HeaderFunctor& parseHeaders)
{
    kDebug(30513);

    if (m_document->omittMasterPage() || m_document->useLastMasterPage()) {
        kDebug(30513) << "Processing of headers/footers cancelled, master-page creation omitted.";
        return;
    }
    //NOTE: only parse headers if we're in a section that can have new headers
    //ie. new sections for columns trigger this function again, but we've
    //already parsed the headers
    if (m_sep->bkc != 1) {
        emit headersFound(new wvWare::HeaderFunctor(parseHeaders), 0);
    }
}


//this part puts the marker in the text, and signals for the rest to be parsed later
void KWordTextHandler::footnoteFound(wvWare::FootnoteData::Type type,
                                     wvWare::UString characters,
                                     wvWare::SharedPtr<const wvWare::Word97::CHP> chp,
                                     const wvWare::FootnoteFunctor& parseFootnote)
{
    Q_UNUSED(chp);

    kDebug(30513) ;

    m_insideFootnote = true;

    //create temp writer for footnote content that we'll add to m_paragraph
    m_footnoteBuffer = new QBuffer();
    m_footnoteBuffer->open(QIODevice::WriteOnly);
    m_footnoteWriter = new KoXmlWriter(m_footnoteBuffer);

    m_footnoteWriter->startElement("text:note");
    //set footnote or endnote
    m_footnoteWriter->addAttribute("text:note-class", type == wvWare::FootnoteData::Endnote ? "endnote" : "footnote");
    //autonumber or character
    m_footnoteWriter->startElement("text:note-citation");

    //autonumbering: 1,2,3,... for footnote; i,ii,iii,... for endnote

    //NOTE: besides converting the number to text here the format is specified
    //in section-properties -> notes-configuration too
    if (characters[0].unicode() == 2) {

        int noteNumber = (type == wvWare::FootnoteData::Endnote ? ++m_endNoteNumber : ++m_footNoteNumber);
        QString noteNumberString;
        char letter = 'a';

        switch (m_parser->dop().nfcFtnRef2) {
        case 0:
            noteNumberString = QString::number(noteNumber);
            break;
        case 1: // uppercase roman
        case 2:  { // lowercase roman
                QString numDigitsLower[] = {"m", "cm", "d", "cd", "c", "xc", "l", "xl", "x", "ix", "v", "iv", "i" };
                QString numDigitsUpper[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I" };
                QString *numDigits = (m_parser->dop().nfcFtnRef2 == 1 ? numDigitsUpper : numDigitsLower);
                int numValues[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};

                for (int i = 0; i < 13; ++i) {
                    while (noteNumber >= numValues[i]) {
                        noteNumber -= numValues[i];
                        noteNumberString += numDigits[i];
                    }
                }
                break;
            }
        case 3: // uppercase letter
            letter = 'A';
        case 4: { // lowercase letter
                while (noteNumber / 25 > 0) {
                    noteNumberString += QString::number(noteNumber / 25);
                    noteNumber = noteNumber % 25;
                    noteNumberString += QChar(letter - 1 + noteNumber / 25);
                }
                noteNumberString += QChar(letter - 1 + noteNumber);
                break;
            }
        case 9: {
                QChar chicagoStyle[] =  {42, 8224, 8225, 167};
                int styleIndex = (noteNumber - 1) % 4;
                int repeatCount = (noteNumber - 1) / 4;
                noteNumberString = QString(chicagoStyle[styleIndex]);
                while (repeatCount > 0) {
                    noteNumberString += QString(chicagoStyle[styleIndex]);
                    repeatCount--;
                }
                break;
            }
        default:
            noteNumberString = QString::number(noteNumber);
            break;
        }

        m_footnoteWriter->addTextNode(noteNumberString);
    } else {
        int index = 0;
        int length = characters.length();
        QString customNote;
        while (index != length) {
            customNote.append(characters[index].unicode());
            ++index;
        }
        m_footnoteWriter->addTextNode(customNote);
    }

    m_footnoteWriter->endElement(); //text:note-citation
    //start the body of the footnote
    m_footnoteWriter->startElement("text:note-body");

    //save the state of tables/paragraphs/lists
    saveState();
    //signal Document to parse the footnote
    emit footnoteFound(new wvWare::FootnoteFunctor(parseFootnote), type);

    //TODO: we should really improve processing of lists somehow
    if (listIsOpen()) {
        closeList();
    }
    restoreState();

    //end the elements
    m_footnoteWriter->endElement();//text:note-body
    m_footnoteWriter->endElement();//text:note

    m_insideFootnote = false;

    QString contents = QString::fromUtf8(m_footnoteBuffer->buffer(), m_footnoteBuffer->buffer().size());
    m_paragraph->addRunOfText(contents, 0, QString(""), m_parser->styleSheet());

    //cleanup
    delete m_footnoteWriter;
    m_footnoteWriter = 0;
    delete m_footnoteBuffer;
    m_footnoteBuffer = 0;

    //bool autoNumbered = (character.unicode() == 2);
    //QDomElement varElem = insertVariable( 11 /*KWord code for footnotes*/, chp, "STRI" );
    //QDomElement footnoteElem = varElem.ownerDocument().createElement( "FOOTNOTE" );
    //if ( autoNumbered )
    //    footnoteElem.setAttribute( "value", 1 ); // KWord will renumber anyway
    //else
    //    footnoteElem.setAttribute( "value", QString(QChar(character.unicode())) );
    //footnoteElem.setAttribute( "notetype", type == wvWare::FootnoteData::Endnote ? "endnote" : "footnote" );
    //footnoteElem.setAttribute( "numberingtype", autoNumbered ? "auto" : "manual" );
    //if ( type == wvWare::FootnoteData::Endnote )
    // Keep name in sync with Document::startFootnote
    //    footnoteElem.setAttribute( "frameset", i18n("Endnote %1", ++m_endNoteNumber ) );
    //else
    // Keep name in sync with Document::startFootnote
    //    footnoteElem.setAttribute( "frameset", i18n("Footnote %1", ++m_footNoteNumber ) );
    //varElem.appendChild( footnoteElem );
} //end footnoteFound()

void KWordTextHandler::bookmarkStart( const wvWare::BookmarkData& data )
{
    KoXmlWriter* writer;
    QBuffer buf;

    if (!m_fld->m_insideField) {
        buf.open(QIODevice::WriteOnly);
        writer = new KoXmlWriter(&buf);
    } else {
        if (!m_fld->m_afterSeparator) {
            kWarning(30513) << "bookmark interfers with field instructions, omitting";
            return;
        } else {
            writer = m_fld->m_writer;
        }
    }
    //get the name of the bookmark
    QString bookmarkName;
    int nameIndex = 0;
    int nameLength = data.name.length();
    while (nameIndex != nameLength) {
        bookmarkName.append(data.name[nameIndex].unicode());
        ++nameIndex;
    }
    if ((data.limCP - data.startCP) > 0) {
        writer->startElement("text:bookmark-start");
        writer->addAttribute("text:name", bookmarkName);
        writer->endElement();
    } else {
        writer->startElement("text:bookmark");
        writer->addAttribute("text:name", bookmarkName);
        writer->endElement();
    }

    if (!m_fld->m_insideField) {
        QString content = QString::fromUtf8(buf.buffer(), buf.buffer().size());
        m_paragraph->addRunOfText(content, 0, QString(""), m_parser->styleSheet(), 1);
        delete writer;
    }
}

void KWordTextHandler::bookmarkEnd( const wvWare::BookmarkData& data )
{
    KoXmlWriter* writer;
    QBuffer buf;

     if (!m_fld->m_insideField) {
        buf.open(QIODevice::WriteOnly);
        writer = new KoXmlWriter(&buf);
    } else {
        if (!m_fld->m_afterSeparator) {
            kWarning(30513) << "bookmark interfers with field instructions, omitting";
            return;
        } else {
            writer = m_fld->m_writer;
        }
    }

    if ((data.limCP - data.startCP)) {
        QString bookmarkName;
        int nameIndex = 0;
        int nameLength = data.name.length();
        while (nameIndex != nameLength) {
            bookmarkName.append(data.name[nameIndex].unicode());
            ++nameIndex;
        }
        writer->startElement("text:bookmark-end");
        writer->addAttribute("text:name", bookmarkName);
        writer->endElement();
    }

    if (!m_fld->m_insideField) {
        QString content = QString::fromUtf8(buf.buffer(), buf.buffer().size());
        m_paragraph->addRunOfText(content, 0, QString(""), m_parser->styleSheet(), 1);
        delete writer;
    }
}

void KWordTextHandler::annotationFound( wvWare::UString characters, wvWare::SharedPtr<const wvWare::Word97::CHP> chp,
                                        const wvWare::AnnotationFunctor& parseAnnotation)
{
    Q_UNUSED(characters);
    Q_UNUSED(chp);
    m_insideAnnotation = true;

    m_annotationBuffer = new QBuffer();
    m_annotationBuffer->open(QIODevice::WriteOnly);
    m_annotationWriter = new KoXmlWriter(m_annotationBuffer);

    m_annotationWriter->startElement("office:annotation");
    m_annotationWriter->startElement("dc:creator");
    // XXX: get the creator from the .doc
    m_annotationWriter->endElement();
    m_annotationWriter->startElement("dc:date");
    // XXX: get the date from the .doc
    m_annotationWriter->endElement();

    //save the state of tables & paragraphs because we'll get new ones in the annotation
    saveState();
    //signal Document to parse the annotation
    emit annotationFound(new wvWare::AnnotationFunctor(parseAnnotation), 0);
    //and now restore state
    restoreState();

    //end the elements
    m_annotationWriter->endElement();//office:annotation

    m_insideAnnotation = false;

    QString contents = QString::fromUtf8(m_annotationBuffer->buffer(), m_annotationBuffer->buffer().size());
    m_paragraph->addRunOfText(contents, 0, QString(""), m_parser->styleSheet());

    //cleanup
    delete m_annotationWriter;
    m_annotationWriter = 0;
    delete m_annotationBuffer;
    m_annotationBuffer = 0;
}

void KWordTextHandler::tableRowFound(const wvWare::TableRowFunctor& functor, wvWare::SharedPtr<const wvWare::Word97::TAP> tap)
{
    kDebug(30513) ;

    //odf doesn't support tables in annotations
    if (m_insideAnnotation) {
        return;
    }

    if (!m_currentTable) {
        // We need to put the table in a paragraph. For wv2 tables are between paragraphs.
        //Q_ASSERT( !m_bInParagraph );
        //paragraphStart( 0L );
        static int s_tableNumber = 0;
        m_currentTable = new KWord::Table();
        m_currentTable->name = i18n("Table %1", ++s_tableNumber);
        m_currentTable->tap = tap;
        //insertAnchor( m_currentTable->name );
    }

    // Add all cell edges to our array.
    for (int i = 0; i <= tap->itcMac; i++)
        m_currentTable->cacheCellEdge(tap->rgdxaCenter[ i ]);

    KWord::Row row(new wvWare::TableRowFunctor(functor), tap);
    m_currentTable->rows.append(row);
}

void KWordTextHandler::tableEndFound()
{
    kDebug(30513) ;

    //odf doesn't support tables in annotations
    if (m_insideAnnotation) {
        return;
    }

    if (!m_currentTable) {
        kWarning(30513) << "Looks like we lost a table somewhere: return";
        return;
    }

    //TODO: FIX THE OPEN LIST PROBLEM !!!!!!
    //we cant have an open list when entering a table
    if (listIsOpen()) {
        //kDebug(30513) << "closing list " << m_currentListID;
        closeList();
    }

    KWord::Table* table = m_currentTable;
    //reset m_currentTable
    m_currentTable = 0L;
    //must delete table in Document!

    emit tableFound(table);
}

#ifdef IMAGE_IMPORT

//TODO: merge inlineObjectFound with floatingObjectFound, both of them are
//stable actually

void KWordTextHandler::inlineObjectFound(const wvWare::PictureData& data)
{
    kDebug(30513);
    //inline object should be inside of a pragraph
    Q_ASSERT(m_paragraph);

    //ignore if field instructions are processed 
    if (m_fld->m_insideField && !m_fld->m_afterSeparator) {
        kWarning(30513) << "Warning: Object located in field instractions, Ignoring!";
        return;
    }

    m_insideDrawing = true;

    //create temporary writer for the picture tags
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    m_drawingWriter = new KoXmlWriter(&buf);
    KoXmlWriter* writer = m_drawingWriter;

    //frame or drawing shape acting as a hyperlink
    if (m_fld->m_hyperLinkActive) {
        writer->startElement("draw:a");
        writer->addAttribute("xlink:type", "simple");
        writer->addAttribute("xlink:href", QUrl(m_fld->m_hyperLinkUrl).toEncoded());
    }

    //save the state of tables/paragraphs/lists (text-box)
    saveState();
    emit inlineObjectFound(data, m_drawingWriter);

    //TODO: we should really improve processing of lists somehow
    if (listIsOpen()) {
        closeList();
    }
    restoreState();

    if (m_fld->m_hyperLinkActive) {
        writer->endElement();
        m_fld->m_hyperLinkActive = false;
    }

    //now add content to our current paragraph
    QString contents = QString::fromUtf8(buf.buffer(), buf.buffer().size());
    m_paragraph->addRunOfText(contents, 0, QString(""), m_parser->styleSheet(), true);

    m_insideDrawing = false;

    //cleanup
    delete m_drawingWriter;
    m_drawingWriter = 0;
}

void KWordTextHandler::floatingObjectFound(unsigned int globalCP)
{
    kDebug(30513);
    //floating object should be inside of a pragraph (or at least it's anchor)
    Q_ASSERT(m_paragraph);

    //ignore if field instructions are processed
    if (m_fld->m_insideField && !m_fld->m_afterSeparator) {
        kWarning(30513) << "Warning: Object located in field instractions, Ignoring!";
        return;
    }

    m_insideDrawing = true;

    //create temporary writer for the picture tags
    QBuffer drawingBuffer;
    drawingBuffer.open(QIODevice::WriteOnly);
    m_drawingWriter = new KoXmlWriter(&drawingBuffer);
    KoXmlWriter* writer = m_drawingWriter;

    //frame or drawing shape acting as a hyperlink
    if (m_fld->m_hyperLinkActive) {
        writer->startElement("draw:a");
        writer->addAttribute("xlink:type", "simple");
        writer->addAttribute("xlink:href", QUrl(m_fld->m_hyperLinkUrl).toEncoded());
    }

    //save the state of tables/paragraphs/lists (text-box)
    saveState();
    emit floatingObjectFound(globalCP, m_drawingWriter);

    //TODO: we should really improve processing of lists somehow
    if (listIsOpen()) {
        closeList();
    }
    restoreState();

    if (m_fld->m_hyperLinkActive) {
        writer->endElement();
        m_fld->m_hyperLinkActive = false;
    }

    //now add content to our current paragraph
    QString contents = QString::fromUtf8(drawingBuffer.buffer(), drawingBuffer.buffer().size());
    m_paragraph->addRunOfText(contents, 0, QString(""), m_parser->styleSheet());

    m_insideDrawing = false;

    //cleanup
    delete m_drawingWriter;
    m_drawingWriter = 0;
}
#endif // IMAGE_IMPORT

// Sets m_currentStyle with PAP->istd (index to STSH structure)

void KWordTextHandler::paragraphStart(wvWare::SharedPtr<const wvWare::ParagraphProperties> paragraphProperties)
{
    kDebug(30513) << "**********************************************";

    //check for a table to be parsed and processed
    if (m_currentTable) {
        kWarning(30513) << "==> WOW, unprocessed table: ignoring";
    }

    //set correct writer and style location
    KoXmlWriter* writer = currentWriter();
    bool inStylesDotXml = false;

    if (document()->writingHeader()) {
        inStylesDotXml = true;
    }

    m_currentPPs = paragraphProperties;

    // Check list information, because that's bigger than a paragraph, and
    // we'll track that here in the TextHandler. NOT TRUE any more according to
    // [MS-DOC] - v20100926
    //
    bool isHeading = false;
    int outlineLevel = 0;

    //TODO: <text:numbered-paragraph>

    //TODO: Put all the heading related logic here!
    if (paragraphProperties) {
        uint istd = paragraphProperties->pap().istd;

        //Applying a heading style, paragraph is a heading.
        if ( (istd >= 0x1) && (istd <= 0x9) ) {
            isHeading = true;
            //according to [MS-DOC] - v20100926
            outlineLevel = istd - 1;
            //MS-DOC outline level is ZERO based, whereas ODF has ONE based.
            outlineLevel++;
        }
    }

    // ilfo = when non-zero, (1-based) index into the pllfo identifying the
    // list to which the paragraph belongs.
    if (!paragraphProperties) {
        // TODO: What to do here?
        kDebug(30513) << "PAP Missing (Big mess-up!)";
    } else if ( (paragraphProperties->pap().ilfo == 0)) {

        // Not in a list at all in the word document, so check if we need to
        // close one in the odt.

        //kDebug(30513) << "Not in a list, so we may need to close a list.";
        if (listIsOpen()) {
            //kDebug(30513) << "closing list " << m_currentListID;
            closeList();
        }
    } else if (paragraphProperties->pap().ilfo > 0) {

        // We're in a list in the word document.
        //
        // At the moment <text:numbered-paragraph> is not supported, we process
        // the paragraph as an list-item instead.
        kDebug(30513) << "we're in a list or numbered paragraph";

        // listInfo is our list properties object.
        const wvWare::ListInfo* listInfo = paragraphProperties->listInfo();

        //error (or currently unknown case)
        if (!listInfo) {
            kWarning() << "pap.ilfo is non-zero but there's no listInfo!";

            // Try to make it a heading for now.
            isHeading = true;
            outlineLevel = paragraphProperties->pap().ilvl + 1;
        } else if (listInfo->lsid() == 1 && listInfo->numberFormat() == 255) {
            kDebug(30513) << "found heading, pap().ilvl="
                    << paragraphProperties->pap().ilvl;
            isHeading = true;
            outlineLevel = paragraphProperties->pap().ilvl + 1;
        } else {
            // List processing
            // This takes care of all the cases:
            //  - A new list
            //  - A list with higher level than before
            //  - A list with lower level than before
            writeListInfo(writer, paragraphProperties->pap(), listInfo);
        }
    } //end pap.ilfo > 0 (ie. we're in a list or heading)

    // Now that the bookkeeping is taken care of for old paragraphs,
    // then actually create the new one.
    kDebug(30513) << "create new Paragraph";
    m_paragraph = new Paragraph(m_mainStyles, inStylesDotXml, isHeading, m_document->writingHeader(), outlineLevel);

    //provide the background color information
    m_paragraph->setBgColor(m_document->currentBgColor());

    //if ( m_bInParagraph )
    //    paragraphEnd();
    //m_bInParagraph = true;
    //kDebug(30513) <<"paragraphStart. style index:" << paragraphProperties->pap().istd;

    kDebug(30513) << "set paragraph properties";
    m_paragraph->setParagraphProperties(paragraphProperties);
    const wvWare::StyleSheet& styles = m_parser->styleSheet();
    //m_currentStyle = 0;
    //check for a named style for this paragraph
    if (paragraphProperties) { // Always set when called by wv2. But not set when called by tableStart.
        kDebug(30513) << "set paragraph style";
        const wvWare::Style* paragraphStyle = styles.styleByIndex(paragraphProperties->pap().istd);
        Q_ASSERT(paragraphStyle);
        //set current named style in m_paragraph
        m_paragraph->setParagraphStyle(paragraphStyle);

        //write the paragraph formatting
        //KoGenStyle* paragraphStyle = new KoGenStyle(KoGenStyle::ParagraphAutoStyle, "paragraph");
        //writeLayout(*paragraphProperties, paragraphStyle, m_currentStyle, true, namedStyleName);
    } else {
        kWarning() << "paragraphProperties was NOT set";
    }

    KoGenStyle* style = m_paragraph->getOdfParagraphStyle();

    //check if the master-page-name attribute is required
    if (document()->writeMasterPageName() && !document()->writingHeader())
    {
        style->addAttribute("style:master-page-name", document()->masterPageName());
        document()->set_writeMasterPageName(false);
    }
    //check if the break-before property is required
    if (m_breakBeforePage &&
        !document()->writingHeader() && !paragraphProperties->pap().fInTable)
    {
        style->addProperty("fo:break-before", "page", KoGenStyle::ParagraphType);
        m_breakBeforePage = false;
    }

} //end paragraphStart()

void KWordTextHandler::paragraphEnd()
{
    kDebug(30513) << "-----------------------------------------------";

    bool chck_dropcaps = false;

    // If the last paragraph was a drop cap paragraph, combine it with
    // this one.
    if (m_hasStoredDropCap) {
        kDebug(30513) << "combine paragraphs for drop cap" << m_dropCapString;
        m_paragraph->addDropCap(m_dropCapString, m_dcs_fdct, m_dcs_lines, m_dropCapDistance, m_dropCapStyleName);
    }

    //clear our paragraph flag
    //m_bInParagraph = false;

    //write some debug messages
    if (m_insideFootnote) {
        kDebug(30513) << "writing a footnote";
    } else if (m_insideAnnotation) {
        kDebug(30513) << "writing an annotation";
    } else if (m_insideDrawing) {
        kDebug(30513) << "writing a drawing";
    } else if (!document()->writingHeader()) {
        kDebug(30513) << "writing to body";
        chck_dropcaps = true;
    } else {
        kDebug(30513) << "writing a header/footer";
    }

    //set correct writer and style location
    KoXmlWriter* writer = currentWriter();

    //add nested field snippets to this paragraph
    if (m_fld->m_insideField && !m_fld_snippets.isEmpty()) {
        QList<QString>* flds = &m_fld_snippets;
        while (!flds->isEmpty()) {
            //add writer content to m_paragraph as a runOfText with text style
            m_paragraph->addRunOfText(flds->takeFirst(), m_fldChp, QString(""), m_parser->styleSheet(), true);
        }
        writer = m_fld->m_writer;
    }

    //write paragraph content, reuse text/paragraph style name if applicable
    QString styleName = m_paragraph->writeToFile(writer, &m_fld->m_tabLeader);

    //provide the styleName to the current field
    m_fld->m_styleName = styleName;

    if (chck_dropcaps) {
        // If this paragraph is a drop cap, it should be combined with the next
        // paragraph.  So store the drop cap data for future use.  However,
        // only do this if the last paragraph was *not* a dropcap.
        if (!m_hasStoredDropCap && m_paragraph->dropCapStatus() == Paragraph::IsDropCapPara) {
            m_paragraph->getDropCapData(&m_dropCapString, &m_dcs_fdct, &m_dcs_lines,
                                        &m_dropCapDistance, &m_dropCapStyleName);
            m_hasStoredDropCap = true;
            kDebug(30513) << "saving drop cap data in texthandler" << m_dropCapString;
        }
        else {
            // Remove the traces of the drop cap for the next round.
            m_hasStoredDropCap = false;
            m_dropCapString.clear();
        }
    }
    //add the list-level-style information to the list-style if required
    if (m_listLevelStyleRequired) {
        updateListStyle(styleName);
        m_listLevelStyleRequired = false;
    }

    delete m_paragraph;
    m_paragraph = 0;
}//end paragraphEnd()

void KWordTextHandler::fieldStart(const wvWare::FLD* fld, wvWare::SharedPtr<const wvWare::Word97::CHP> /*chp*/)
{
    //NOTE: The content between fieldStart and fieldSeparator represents field
    //instructions and the content between fieldSeparator and fieldEnd
    //represents the field RESULT [optional].  In most cases the field RESULT
    //stores the complete information (instruction are applied by msword).
    kDebug(30513) << "fld->flt:" << fld->flt << "(" << hex << fld->flt << ")";

    //nested field
    if (m_fld->m_insideField) {
        fld_saveState();
    } else {
        delete m_fld;
    }

    m_fld = new fld_State((fldType)fld->flt);
    m_fld->m_insideField = true;

    switch (m_fld->m_type) {
    case EQ:
        kDebug(30513) << "processing field... EQ (Combined Characters)";
        break;
    case CREATEDATE:
    case DATE:
    case HYPERLINK:
    case PAGEREF:
    case SAVEDATE:
    case TIME:
    case TOC:
        kDebug(30513) << "Processing only a subset of field instructions!";
        kDebug(30513) << "Processing field result!";
        break;
    case LAST_REVISED_BY:
    case NUMPAGES:
    case PAGE:
    case SUBJECT:
    case TITLE:
        kWarning(30513) << "Warning: field instructions not supported, storing as ODF field!";
        kWarning(30513) << "Warning: ignoring field result!";
        break;
    case MACROBUTTON:
    case SYMBOL:
        kWarning(30513) << "Warning: processing only a subset of field instructions!";
        kWarning(30513) << "Warning: ignoring field result!";
        break;
    case AUTHOR:
    case EDITTIME:
    case FILENAME:
    case MERGEFIELD:
    case SEQ:
    case SHAPE:
        kWarning(30513) << "Warning: field instructions not supported!";
        kWarning(30513) << "Warning: processing field result!";
        break;
    default:
        kWarning(30513) << "Warning: unrecognized field type" << m_fld->m_type << ", ignoring!";
        m_fld->m_type = UNSUPPORTED;
        break;
    }

    switch (m_fld->m_type) {
    case NUMPAGES:
    case PAGE:
        m_paragraph->setContainsPageNumberField(true);
        break;
    case TOC:
        m_tocNumber++;
        break;
    default:
        break;
    }

    m_fldStart++;
}//end fieldStart()

void KWordTextHandler::fieldSeparator(const wvWare::FLD* /*fld*/, wvWare::SharedPtr<const wvWare::Word97::CHP> /*chp*/)
{
    kDebug(30513) ;
    m_fld->m_afterSeparator = true;
    QString* inst = &m_fld->m_instructions;

    //process field instructions if required
    switch (m_fld->m_type) {
    case HYPERLINK:
    {
        // Syntax: HYPERLINK field-argument [ switches ]
        //
        // When selected, causes control to jump to the location specified by
        // text in field-argument.  That location can be a bookmark or a URL.
        //
        // Field Value: None
        //
        // TODO:
        //
        // \o field-argument - Text in this switch's field-argument specifies
        // the ScreenTip text for the hyperlink.  field-argument which
        // specifies a location in the file, such as bookmark, where this
        // hyperlink will jump.
        //
        // \t field-argument - Text in this switch's field-argument specifies
        // the target to which the link should be redirected.  Use this switch
        // to link from a frames page to a page that you want to appear outside
        // of the frames page.  The permitted values for text are: _top, whole
        // page (the default); _self, same frame; _blank, new window; _parent,
        // parent frame
        //
        // \m - Appends coordinates to a hyperlink for a server-side image map.
        // \n - Causes the destination site to be opened in a new window.

        // \l field-argument - Text in this switch's field-argument specifies a
        // location in the file, such as a bookmark, where to jump.
        QRegExp rx("\\s\\\\l\\s\"(\\S+)\"");
        m_fld->m_hyperLinkActive = true;

        if (rx.indexIn(*inst) >= 0) {
            m_fld->m_hyperLinkUrl = rx.cap(1).prepend("#");
        } else {
            rx = QRegExp("HYPERLINK\\s\"(\\S+)\"");
            if (rx.indexIn(*inst) >= 0) {
                m_fld->m_hyperLinkUrl = rx.cap(1);
            } else {
                kDebug(30513) << "HYPERLINK: missing URL";
            }
        }
        break;
    }
    case PAGEREF:
    {
        // Syntax: PAGEREF field-argument [ switches ]
        //
        // Inserts the number of the page containing the bookmark specified by
        // text in field-argument for a cross-reference.
        //
        // Field Value: The number of the page containing the bookmark.
        //
        // TODO:
        //
        // \p - Causes the field to display its position relative to the source
        // bookmark.  If the PAGEREF field is on the same page as the bookmark,
        // it omits "on page #" and returns "above" or "below" only.  If the
        // PAGEREF field is not on the same page as the bookmark, the string
        // "on page #" is used.

        // \h - Creates a hyperlink to the bookmarked paragraph.
        QRegExp rx("PAGEREF\\s(\\S+)");
        if (rx.indexIn(*inst) >= 0) {
            m_fld->m_hyperLinkUrl = rx.cap(1);
        }
        rx = QRegExp("\\s\\\\h\\s");
        if (rx.indexIn(*inst) >= 0) {
            m_fld->m_hyperLinkActive = true;
            m_fld->m_hyperLinkUrl.prepend("#");
        }
        break;
    }
    case TIME:
    case DATE: {
        // Extract the interesting format-string. That means we translate
        // something like 'TIME \@ "MMMM d, yyyy"' into 'MMMM d, yyyy' cause
        // the NumberFormatParser doesn't handle it correct else.
        QRegExp rx(".*\"(.*)\".*");
        if (rx.indexIn(*inst) >= 0)
            m_fld->m_instructions = rx.cap(1);
        break;
    }
    default:
        break;
    }
} //end fieldSeparator()

/**
 * Fields which are supported by inline variables can be dealt with by emitting
 * the necessary markup here. For example:
 * 
 *	case LAST_REVISED_BY:
 *	    writer.startElement("text:creator");
 *	    writer.endElement();
 *	    break;
 *
 * However, fields which do not enjoy such support are dealt with by emitting
 * the "result" text generated by Word as vanilla text in @ref runOftext.
 */
void KWordTextHandler::fieldEnd(const wvWare::FLD* /*fld*/, wvWare::SharedPtr<const wvWare::Word97::CHP> chp)
{
//    Q_UNUSED(chp);
    kDebug(30513);
    //process different fields, we could be writing to content.xml or
    //styles.xml (header/footer)

    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    KoXmlWriter writer(&buf);
    QString* inst = &m_fld->m_instructions;
    QString tmp;

    switch (m_fld->m_type) {
    case EQ:
    {
        //TODO: nested fields support required
        //NOTE: actually combined characters stored as 'equation'
        QRegExp rx("eq \\\\o\\(\\\\s\\\\up 36\\(([^\\)]*)\\),\\\\s\\\\do 12\\(([^\\)]*)\\)\\)");
        int where = rx.indexIn(*inst);

        if (where != -1) {
            QString cc = rx.cap(1) + rx.cap(2);
            if (!cc.isEmpty()) {
                m_paragraph->setCombinedCharacters(true);
                m_paragraph->addRunOfText(cc, chp, QString(""), m_parser->styleSheet());
                m_paragraph->setCombinedCharacters(false);
            }
        }
        break;
    }
    case HYPERLINK:
    {
        if (m_fld->m_hyperLinkActive) {
            writer.startElement("text:a", false);
            writer.addAttribute("xlink:type", "simple");
            writer.addAttribute("xlink:href", QUrl(m_fld->m_hyperLinkUrl).toEncoded());
            writer.addCompleteElement(m_fld->m_buffer);
            writer.endElement(); //text:a
        }
        //else a frame or drawing shape acting as a hyperlink already processed
        break;
    }
    case LAST_REVISED_BY:
        writer.startElement("text:creator");
        writer.endElement();
        break;
    case MACROBUTTON:
    {
        QRegExp rx("MACROBUTTON\\s\\s?\\w+\\s\\s?(.+)$");
        if (rx.indexIn(*inst) >= 0) {
            writer.addTextNode(rx.cap(1));
        }
        break;
    }
    case NUMPAGES:
        writer.startElement("text:page-count");
        writer.endElement();
        break;
    case PAGE:
        writer.startElement("text:page-number");
        writer.addAttribute("text:select-page", "current");
        writer.endElement();
        break;
    case PAGEREF:
    {
        if (m_fld->m_hyperLinkActive) {
            writer.startElement("text:a", false);
            writer.addAttribute("xlink:type", "simple");
            writer.addAttribute("xlink:href", QUrl(m_fld->m_hyperLinkUrl).toEncoded());
            writer.addCompleteElement(m_fld->m_buffer);
            writer.endElement(); //text:a
        } else {
            writer.startElement("text:bookmark-ref");
            writer.addAttribute("text:reference-format", "page");
            writer.addAttribute("text:ref-name", QUrl(m_fld->m_hyperLinkUrl).toEncoded());
            writer.addCompleteElement(m_fld->m_buffer);
            writer.endElement(); //text:bookmark-ref
        }
        break;
    }
    case SUBJECT:
        writer.startElement("text:subject");
        writer.endElement();
        break;
    case DATE:
    {
        writer.startElement("text:date");

        NumberFormatParser::setStyles(m_mainStyles);
        KoGenStyle style = NumberFormatParser::parse(m_fld->m_instructions, KoGenStyle::NumericDateStyle);
        writer.addAttribute("style:data-style-name", m_mainStyles->insert(style, "N"));

        //writer.addAttribute("text:fixed", "true");
        //writer.addAttribute("text:date-value", "2011-01-14T12:38:31.99");
        writer.addCompleteElement(m_fld->m_buffer); // January 14, 2011
        writer.endElement(); //text:date
        break;
    }
    case TIME:
    {
        writer.startElement("text:time");

        NumberFormatParser::setStyles(m_mainStyles);
        KoGenStyle style = NumberFormatParser::parse(m_fld->m_instructions, KoGenStyle::NumericTimeStyle);
        writer.addAttribute("style:data-style-name", m_mainStyles->insert(style, "N"));

        //writer.addAttribute("text:fixed", "true");
        //writer.addAttribute("text:time-value", );
        writer.addCompleteElement(m_fld->m_buffer);
        writer.endElement(); //text:time
        break;
    }
    case CREATEDATE:
    {
        writer.startElement("text:creation-date");
        NumberFormatParser::setStyles(m_mainStyles);
        KoGenStyle style = NumberFormatParser::parse(m_fld->m_instructions, KoGenStyle::NumericTimeStyle);
        writer.addAttribute("style:data-style-name", m_mainStyles->insert(style, "N"));
        writer.addCompleteElement(m_fld->m_buffer);
        writer.endElement(); //text:creation-date
        break;
    }
    case SAVEDATE:
    {
        writer.startElement("text:modification-date");
        NumberFormatParser::setStyles(m_mainStyles);
        KoGenStyle style = NumberFormatParser::parse(m_fld->m_instructions, KoGenStyle::NumericTimeStyle);
        writer.addAttribute("style:data-style-name", m_mainStyles->insert(style, "N"));
        writer.addCompleteElement(m_fld->m_buffer);
        writer.endElement(); //text:modification-date
        break;
    }
    case SYMBOL:
    {
        QRegExp rx_txt("SYMBOL\\s{2}(\\S+)\\s+.+$");
        QString txt;
        *inst = inst->trimmed();

        //check for text in field instructions
        if (rx_txt.indexIn(*inst) >= 0) {
            txt = rx_txt.cap(1);

            //ascii code
            if (inst->contains("\\a")) {
                QRegExp rx16("0\\D.+");
                bool ok = false;
                int n;

                if (rx16.indexIn(txt) >= 0) {
                    n = txt.toInt(&ok, 16);
                } else {
                    n = txt.toInt(&ok, 10);
                }
                if (ok) {
                    tmp.append((char) n);
                }
            }
            //unicode
            if (inst->contains("\\u")) {
                qDebug() << "Warning: unicode symbols not supported!";
            }
        }
        //default value (check the corresponding test)
        if (tmp.isEmpty()) {
            tmp = "###";
        }
        writer.addTextNode(tmp);
        break;
    }
    case TITLE:
        writer.startElement("text:title");
        writer.endElement();
        break;
    case TOC:
    {
        // Syntax: TOC [ switches ]
        //
        // Field Value: The table of contents.
        //
        // TODO:
        //
        // \a field-argument - Includes captioned items, but omits caption
        // labels and numbers.  The identifier designated by text in this
        // switch's field-argument corresponds to the caption label.
        //
        // \b field-argument - Includes entries only from the portion of the
        // document marked by the bookmark named by text in this switch's
        // field-argument.
        //
        // \c field-argument - Includes figures, tables, charts, and other
        // items that are numbered by a SEQ field.  The sequence identifier
        // designated by text in this switch's field-argument, which
        // corresponds to the caption label, shall match the identifier in the
        // corresponding SEQ field.
        //
        // \d field-argument - When used with \s, the text in this switch's
        // field-argument defines the separator between sequence and page
        // numbers.  The default separator is a hyphen (-).
        //
        // \f field-argument - Includes only those TC fields whose identifier
        // exactly matches the text in this switch's field-argument (which is
        // typically a letter).
        //
        // \l field-argument - Includes TC fields that assign entries to one of
        // the levels specified by text in this switch's field-argument as a
        // range having the form startLevel-endLevel, where startLevel and
        // endLevel are integers, and startLevel has a value equal-to or
        // less-than endLevel.  TC fields that assign entries to lower levels
        // are skipped.
        //
        // \s field-argument - For entries numbered with a SEQ field , adds a
        // prefix to the page number.  The prefix depends on the type of entry.
        // Text in this switch's field-argument shall match the identifier in
        // the SEQ field.
        //
        // \t field-argument - Uses paragraphs formatted with styles other than
        // the built-in heading styles.  Text in this switch's field-argument
        // specifies those styles as a set of comma-separated doublets, with
        // each doublet being a comma-separated set of style name and table of
        // content level.  \t can be combined with \o.
        //
        // \u - Uses the applied paragraph outline level.
        // \w - Preserves tab entries within table entries.
        // \x - Preserves newline characters within table entries.
        // \z - Hides tab leader and page numbers in Web layout view.

        //NOTE: Nested fields had been processed and wrote into m_fld->m_writer
        //by the writeToFile function.  The m_fldStates stack should be empty.
        Q_ASSERT(m_fldStates.empty());

        QRegExp rx;
        QString hlinkStyleName;
        QList<QString> styleNames = document()->tocStyleNames();

        // \h - Makes the table of contents entries hyperlinks.
        rx = QRegExp("\\s\\\\h\\s");
        bool hyperlink = false;

        if (rx.indexIn(*inst) >= 0) {
            hyperlink = true;
        }

        //hyperlink style info not provided by the TOC field, reusing the text
        //style of text:index-body content, expecting a text style
        if (m_fld->m_styleName.contains('T')) {
            hlinkStyleName = m_mainStyles->style(m_fld->m_styleName)->parentName();
        } else {
            kDebug(30513) << "TOC: Missing text style to format the hyperlink!";
        }

        // \n field-argument - Without field-argument, omits page numbers from
        // the table of contents.  Page numbers are omitted from all levels
        // unless a range of entry levels is specified by text in this switch's
        // field-argument.  A range is specified as for \l.
        rx = QRegExp("\\s\\\\n\\s");
        bool pgnum = true;

        if (rx.indexIn(*inst) >= 0) {
            pgnum = false;
        }

        // \o field-argument - Uses paragraphs formatted with all or the
        // specified range of built-in heading styles.  Headings in a style
        // range are specified by text in field-argument.  If no heading range
        // is specified, all heading levels used in the document are listed.
        rx = QRegExp("\\s\\\\o\\s\"(\\S+)\"");
        int levels;

        if (rx.indexIn(*inst) >= 0) {
            QStringList levels_lst = rx.cap(1).split("-");
            levels = levels_lst.last().toInt();
        } else {
            levels = styleNames.size();
        }

        // \p field-argument - text in this switch's field-argument specifies a
        // sequence of characters that separate an entry and its page number.
        // The default is a tab with leader dots.
        rx = QRegExp("\\s\\\\p\\s\"(\\s)\"");
        QString separator;

        if (rx.indexIn(*inst) >= 0) {
            separator = rx.cap(1);
        } else {
            // The tab leader character is not provided by the TOC field,
            // reusing from the last paragraph style applied in index-body.
            // If not provided, use the default value.
            if (m_fld->m_tabLeader.isNull()) {
                m_fld->m_tabLeader = QChar('.');
            }
        }

        //post-process m_tocStyleNames
        if (styleNames.size() < levels) {
            if (styleNames.isEmpty()) {
                for (int i = 0; i < levels; i++) {
                    styleNames.append("Normal");
                }
            } else {
                rx = QRegExp("\\S+(\\d+)$");
                if (rx.indexIn(styleNames.first()) >= 0) {
                    bool ok = false;
                    uint n = rx.cap(1).toUInt(&ok, 10);
                    if (!ok) {
                        kDebug(30513) << "Conversion of QString to uint failed!";
                    } else {
                        for (uint i = 1; i < n; i++) {
                            styleNames.prepend("Normal");
                        }
                    }
                } else {
                    kDebug(30513) << "Missing TOC related style with level info.";
                    kDebug(30513) << "Not that bad actually.";
                }
                while (styleNames.size() < levels) {
                    styleNames.append("Normal");
                }
            }
        }

        //NOTE: TOC content is not encapsulated in a paragraph
        KoXmlWriter* cwriter = currentWriter();
        cwriter->startElement("text:table-of-content");
        cwriter->addAttribute("text:name", QString().setNum(m_tocNumber).prepend("_TOC"));
        cwriter->startElement("text:table-of-content-source");
        cwriter->addAttribute("text:index-scope", "document");
        cwriter->addAttribute("text:outline-level", levels);
        cwriter->addAttribute("text:relative-tab-stop-position", "false");
        cwriter->addAttribute("text:use-index-marks", "false");
        cwriter->addAttribute("text:use-index-source-styles", "false");
        cwriter->addAttribute("text:use-outline-level", "true");

        for (int i = 0; i < levels; i++) {
            cwriter->startElement("text:table-of-content-entry-template");
            cwriter->addAttribute("text:outline-level", i + 1);
            cwriter->addAttribute("text:style-name", styleNames[i].toUtf8());

            if (hyperlink) {
                cwriter->startElement("text:index-entry-link-start");
                if (!hlinkStyleName.isEmpty()) {
                    cwriter->addAttribute("text:style-name", hlinkStyleName);
                }
                cwriter->endElement(); //text:index-entry-link-start
            }

            //Represents the chapter number where an index entry is located.
            //FIXME: we need some logic here!
            cwriter->startElement("text:index-entry-chapter");
            cwriter->endElement(); //text:index-entry-chapter

            cwriter->startElement("text:index-entry-text");
            cwriter->endElement(); //text:index-entry-text
            if (pgnum) {
                if (separator.isEmpty()) {
                    cwriter->startElement("text:index-entry-tab-stop");
                    cwriter->addAttribute("style:leader-char", m_fld->m_tabLeader);
                    //NOTE: "right" is the only option available
                    cwriter->addAttribute("style:type", "right");
                    cwriter->endElement(); //text:index-entry-tab-stop
                } else {
                    cwriter->startElement("text:index-entry-span");
                    cwriter->addTextNode(separator);
                    cwriter->endElement(); //text:index-entry-span
                }
                cwriter->startElement("text:index-entry-page-number");
                cwriter->endElement(); //text:index-entry-page-number
            }
            if (hyperlink) {
                cwriter->startElement("text:index-entry-link-end");
                cwriter->endElement(); //text:index-entry-link-end
            }
            cwriter->endElement(); //text:table-of-content-entry-template
        }

        cwriter->endElement(); //text:table-of-content-source
        cwriter->startElement("text:index-body");
        cwriter->addCompleteElement(m_fld->m_buffer);
        cwriter->endElement(); //text:index-body
        cwriter->endElement(); //text:table-of-content
        break;
    }
    default:
        break;
    }
    QString contents = QString::fromUtf8(buf.buffer(), buf.buffer().size());
    if (!contents.isEmpty()) {
        //nested field
        if (!m_fldStates.empty()) {
            m_fld_snippets.prepend(contents);
        } else {
            //add writer content to m_paragraph as a runOfText with text style
            m_paragraph->addRunOfText(contents, m_fldChp, QString(""), m_parser->styleSheet(), true);
        }
    }

    //reset
    delete m_fld;
    m_fldEnd++;

    //nested field
    if (!m_fldStates.empty()) {
        fld_restoreState();
    } else {
        m_fld = new fld_State();
        QList<QString>* list = &m_fld_snippets;
        while (!list->isEmpty()) {
            //add writer content to m_paragraph as a runOfText with text style
            m_paragraph->addRunOfText(list->takeFirst(), m_fldChp, QString(""), m_parser->styleSheet(), true);
        }
        m_fldChp = 0;
    }
} //end fieldEnd()

/**
 * This handles a basic section of text.
 * 
 * Fields which are not supported by inline variables in @ref fieldEnd are also dealt with by 
 * emitting the "result" text generated by Word as vanilla text here.
 */
void KWordTextHandler::runOfText(const wvWare::UString& text, wvWare::SharedPtr<const wvWare::Word97::CHP> chp)
{
    bool common_flag = false;
    QString newText(Conversion::string(text));
    kDebug(30513) << newText;

    //we don't want to do anything with an empty string
    if (newText.isEmpty()) {
        return;
    }

    if (m_fld->m_insideField) {
        //processing field instructions
        if (!m_fld->m_afterSeparator) {
            switch (m_fld->m_type) {
            case EQ:
            case HYPERLINK:
            case MACROBUTTON:
            case PAGEREF:
            case SYMBOL:
            case TOC:
            case PAGE:
            case CREATEDATE:
            case SAVEDATE:
            case TIME:
            case DATE:
                m_fld->m_instructions.append(newText);
                break;
            default:
                kDebug(30513) << "Ignoring field instructions!";
                break;
            }
        }
        //processing the field result
        else {
            KoXmlWriter* writer = m_fld->m_writer;
            switch (m_fld->m_type) {
            case CREATEDATE:
            case SAVEDATE:
            case DATE:
            case TIME:
            case PAGEREF:
            case HYPERLINK:
                if (newText == "\t") {
                    writer->startElement("text:tab", false);
                    writer->endElement();
                } else {
                    writer->addTextNode(newText);
                }
                break;
            case AUTHOR:
            case EDITTIME:
            case FILENAME:
            case MERGEFIELD:
            case SEQ:
            case SHAPE:
            case TOC:
                //NOTE: Ignoring bookmarks in the field result!
                kDebug(30513) << "Processing field result as vanilla text string.";
                common_flag = true;
                break;
            default:
                kDebug(30513) << "Ignoring the field result.";
                break;
            }
            if (!m_fldChp.data()) {
                m_fldChp = chp;
            }
        }
        if (common_flag) {
            //the root field controls formatting of child fields
            if (m_fldChp.data()) {
                chp = m_fldChp;
            }
        } else {
            return;
        }
    }

    // Font name, TODO: We only use the Ascii font code. We should work out
    // how/when to use the FE and Other font codes.  ftcAscii = (rgftc[0]) font
    // for ASCII text
    QString fontName = getFont(chp->ftcAscii);
    if (!fontName.isEmpty()) {
        m_mainStyles->insertFontFace(KoFontFace(fontName));
    }

    //only show text that is not hidden, TODO use text:display="none"
    if (chp->fVanish != 1) {
        //add text string and formatting style to m_paragraph
        m_paragraph->addRunOfText(newText, chp, fontName, m_parser->styleSheet());
    }

} //end runOfText()

//#define FONT_DEBUG

// Return the name of a font. We have to convert the Microsoft font names to
// something that might just be present under X11.
QString KWordTextHandler::getFont(unsigned ftc) const
{ 
    kDebug(30513) ;
    Q_ASSERT(m_parser);

    if (!m_parser) {
        return QString();
    }
    const wvWare::Word97::FFN& ffn(m_parser->font(ftc));
    QString fontName(Conversion::string(ffn.xszFfn));
    return fontName;
    /*
    #ifdef FONT_DEBUG
        kDebug(30513) <<"    MS-FONT:" << font;
    #endif

        static const unsigned ENTRIES = 6;
        static const char* const fuzzyLookup[ENTRIES][2] =
        {
            // MS contains      X11 font family
            // substring.       non-Xft name.
            { "times",          "times" },
            { "courier",        "courier" },
            { "andale",         "monotype" },
            { "monotype.com",   "monotype" },
            { "georgia",        "times" },
            { "helvetica",      "helvetica" }
        };

        // When Xft is available, Qt will do a good job of looking up our local
        // equivalent of the MS font. But, we want to work even without Xft.
        // So, first, we do a fuzzy match of some common MS font names.
        unsigned i;

        for (i = 0; i < ENTRIES; i++)
        {
            // The loop will leave unchanged any MS font name not fuzzy-matched.
            if (font.find(fuzzyLookup[i][0], 0, false) != -1)
            {
                font = fuzzyLookup[i][1];
                break;
            }
        }

    #ifdef FONT_DEBUG
        kDebug(30513) <<"    FUZZY-FONT:" << font;
    #endif

        // Use Qt to look up our canonical equivalent of the font name.
        QFont xFont( font );
        QFontInfo info( xFont );

    #ifdef FONT_DEBUG
        kDebug(30513) <<"    QT-FONT:" << info.family();
    #endif

        return info.family();
        */
}//end getFont()

bool KWordTextHandler::writeListInfo(KoXmlWriter* writer, const wvWare::Word97::PAP& pap, const wvWare::ListInfo* listInfo)
{
    kDebug(30513);

    m_listLevelStyleRequired = false;
    int nfc = listInfo->numberFormat();

    //check to see if we're in a heading instead of a list
    //if so, just return false so writeLayout can process the heading
    if (listInfo->lsid() == 1 && nfc == 255) {
        return false;
    }

    //put the currently used writer in the stack
    m_usedListWriters.push(writer);

    //process the different places we could be in a list
    if (m_currentListID != listInfo->lsid()) {
        //we're entering a different or new list...
        kDebug(30513) << "opening list " << listInfo->lsid();

        //just in case another list was already open
        if (listIsOpen()) {
            //kDebug(30513) << "closing list " << m_currentListID;
            closeList();
        }
        // Open <text:list> in the body
        writer->startElement("text:list");

        //check for a continued list
        if (m_previousLists.contains(listInfo->lsid())) {
            writer->addAttribute("text:continue-numbering", "true");
            writer->addAttribute("text:style-name", m_previousLists[listInfo->lsid()]);
            m_listStyleName = m_previousLists[listInfo->lsid()];
        } else {
            //need to create a style for this list
            KoGenStyle listStyle(KoGenStyle::ListAutoStyle);

            // If writing to styles.xml, the list style needs to go there as well.
            if (document()->writingHeader())
                listStyle.setAutoStyleInStylesDotXml(true);

            // Write styleName to the text:list tag.
            m_listStyleName = m_mainStyles->insert(listStyle);
            writer->addAttribute("text:style-name", m_listStyleName);
        }
        //set the list ID - now is safe as we are done using the old value
        m_currentListID = listInfo->lsid();

        //a new list-level-style is required
        m_listLevelStyleRequired = true;

        m_currentListDepth = pap.ilvl;
        if (m_currentListDepth > 0) {
            for (int i = 0; i < m_currentListDepth; i++) {
                writer->startElement("text:list-item");
                writer->startElement("text:list");
            }
        }
    }
    else if (pap.ilvl > m_currentListDepth) {
        //we're going to a new level in the list
        kDebug(30513) << "going to a new level in list" << m_currentListID;
        //open a new <text:list> or more levels if needed
        m_currentListDepth++;
        writer->startElement("text:list");
        for (;m_currentListDepth < pap.ilvl; m_currentListDepth++) {
            writer->startElement("text:list-item");
            writer->startElement("text:list");
        }
        //a new list-level-style is required
        m_listLevelStyleRequired = true;
    }
    else {
        qDebug() << "HMMM" << m_currentListDepth << pap.ilvl;
        // We're backing out one or more levels in the list.
        kDebug(30513) << "backing out one or more levels in list" << m_currentListID;
        while (m_currentListDepth > pap.ilvl) {
            m_currentListDepth--;
            //close the last <text:list-item of the level
            writer->endElement();
            //close <text:list> for the level
            writer->endElement();
        }
        //close the <text:list-item> from the surrounding level
        writer->endElement();
    }

    //we always want to open this tag
    writer->startElement("text:list-item");

    return true;
} //end writeListInfo()


QString KWordTextHandler::createBulletStyle(const QString& textStyleName) const
{
    const KoGenStyle* textStyle = m_mainStyles->style(textStyleName);

    if (!textStyle) {
        return QString();
    }

    KoGenStyle style(KoGenStyle::TextStyle, "text");
    QString prop, value;

    //copy only selected properties

    prop = QString("fo:color");
    value = textStyle->property(prop, KoGenStyle::TextType);
    if (!value.isEmpty()) {
        style.addProperty(prop, value, KoGenStyle::TextType);
    }
    prop = QString("fo:font-size");
    value = textStyle->property(prop, KoGenStyle::TextType);
    if (!value.isEmpty()) {
        style.addProperty(prop, value, KoGenStyle::TextType);
    }
    prop = QString("fo:font-weight");
    value = textStyle->property(prop, KoGenStyle::TextType);
    if (!value.isEmpty()) {
        style.addProperty(prop, value, KoGenStyle::TextType);
    }
    prop = QString("style:font-name");
    value = textStyle->property(prop, KoGenStyle::TextType);
    if (value.isEmpty()) {
        const KoGenStyle* normal = m_mainStyles->style("Normal");
        if (normal) {
            value = normal->property(prop, KoGenStyle::TextType);
        }
    }
    if (!value.isEmpty()) {
        style.addProperty(prop, value, KoGenStyle::TextType);
    }
    //insert style into styles collection
    return m_mainStyles->insert(style, QString("T"));
}

void KWordTextHandler::updateListStyle(const QString& textStyleName)
{
    kDebug(30513) << "writing the list-level-style";

    const wvWare::Word97::PAP& pap = m_currentPPs->pap();
    const wvWare::ListInfo* listInfo = m_currentPPs->listInfo();
    if (!listInfo) {
	return;
    }
    int nfc = listInfo->numberFormat();

    //create writer for this list
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    KoXmlWriter listStyleWriter(&buf);
    KoGenStyle* listStyle = 0;
    //text() returns a struct consisting of a UString text string (called text) & a pointer to a CHP (called chp)
    wvWare::UString text = listInfo->text().text;

    //NOTE: Reuse the automatic style of the text until we manage to process
    //and store the bullet/number style.  The style will be placed into
    //office:styles.  Only selected properties are copied.
    QString bulletStyleName;

    //check the styleName, we expect a text style
    if (textStyleName.contains('T')) {
        bulletStyleName = createBulletStyle(textStyleName);
    }

    //bulleted list
    if (nfc == 23) {
        kDebug(30513) << "bullets...";
        listStyleWriter.startElement("text:list-level-style-bullet");
        listStyleWriter.addAttribute("text:level", pap.ilvl + 1);
        if (text.length() == 1) {
            // With bullets, text can only be one character, which
            // tells us what kind of bullet to use
            unsigned int code = text[0].unicode();
            if ((code & 0xFF00) == 0xF000) {  // unicode: private use area (0xf000 - 0xf0ff)
                if (code >= 0x20) {
                    // microsoft symbol charset shall apply here.
                    code = Conversion::MS_SYMBOL_ENCODING[code%256];
                } else {
                    code &= 0x00FF;
                }
            }
            listStyleWriter.addAttribute("text:bullet-char", QString(code).toUtf8());
        } else {
            kWarning(30513) << "Bullet with more than one character, not supported";
        }

        if (!bulletStyleName.isEmpty()) {
            listStyleWriter.addAttribute("text:style-name", bulletStyleName);
        }

        listStyleWriter.startElement("style:list-level-properties");
        if (listInfo->space()) {
            // This produces wrong results (see the document attached to KDE bug 244411 and it's not clear why that is so. The
            // specs say that the dxaSpace is the "minimum space between number and paragraph" and as such following should be
            // right but it is not. So, we disabled it for now till someone has an idea why that is so.
            //listStyleWriter.addAttributePt("text:min-label-distance", listInfo->space()/20.0);
        }
        if (listInfo->indent()) {
            // Is this correct?
            listStyleWriter.addAttributePt("text:min-label-width", listInfo->indent()/20.0);
        }
        listStyleWriter.endElement(); //style:list-level-properties
        //close element
        listStyleWriter.endElement(); //text:list-level-style-bullet
    }
    //numbered/outline list
    else {
        kDebug(30513) << "numbered/outline... nfc = " << nfc;
        listStyleWriter.startElement("text:list-level-style-number");
        listStyleWriter.addAttribute("text:level", pap.ilvl + 1);

        if (!bulletStyleName.isEmpty()) {
            listStyleWriter.addAttribute("text:style-name", bulletStyleName);
        }

        //*************************************
        int depth = pap.ilvl; //both are 0 based
        //int numberingType = listInfo->isWord6() && listInfo->prev() ? 1 : 0;
        // Heading styles don't set the ilvl, but must have a depth coming
        // from their heading level (the style's STI)
        //bool isHeading = style->sti() >= 1 && style->sti() <= 9;
        //if ( depth == 0 && isHeading )
        //{
        //    depth = style->sti() - 1;
        //}

        // Now we need to parse the text, to try and convert msword's powerful list template stuff
        QString prefix, suffix;
        bool depthFound = false;
        bool anyLevelFound = false;
        int displayLevels = 1;

        // We parse <0>.<2>.<1>. as "level 2 with suffix='.'" (no prefix)
        // But "Section <0>.<1>)" has both prefix and suffix.
        // The common case is <0>.<1>.<2> (display-levels=3)
        //loop through all of text
        //this just sets depthFound & displayLevels & the suffix & prefix
        for (int i = 0 ; i < text.length() ; ++i) {
            short ch = text[i].unicode();
            //kDebug(30513) << i <<":" << ch;
            if (ch < 10) {   // List level place holder
                if (ch == pap.ilvl) {
                    if (depthFound) {
                        kWarning(30513) << "ilvl " << pap.ilvl << " found twice in listInfo text...";
                    }
                    else {
                        depthFound = true;
                    }
                    suffix.clear(); // really should never do anthing so why is it here??
                } else {
                    Q_ASSERT(ch < pap.ilvl);   // Can't see how level 1 would have a <0> in it...
                    if (ch < pap.ilvl) {
                        ++displayLevels; // we found a 'parent level', to be displayed
                    }
                }
                anyLevelFound = true;
            }
            //if it's not a number < 10
            else {
                //add it to suffix if we've found the level that we're at
                if (depthFound) {
                    suffix += QChar(ch);
                }
                //or add it to prefix if we haven't
                else if (!anyLevelFound) {
                    prefix += QChar(ch);
                }
            }
        }
        if (displayLevels > 1) {
            // This is a hierarchical list numbering e.g. <0>.<1>.
            // (unless this is about a heading, in which case we've set numberingtype to 1 already
            // so it will indeed look like that).
            // The question is whether the '.' is the suffix of the parent level already..
            //do I still need to keep the m_listSuffixes stuff?
            if (depth > 0 && !prefix.isEmpty() && m_listSuffixes[ depth - 1 ] == prefix) {
                prefix.clear(); // it's already the parent's suffix -> remove it
                kDebug(30513) << "depth=" << depth << " parent suffix is" << prefix << " -> clearing";
            }
        }
        //if ( isHeading )
        //    numberingType = 1;
        //this is where we actually write the information
        if (depthFound) {
            // Word6 models "1." as nfc=5
            if (nfc == 5 && suffix.isEmpty())
                suffix = '.';
            kDebug(30513) << " prefix=" << prefix << " suffix=" << suffix;
            listStyleWriter.addAttribute("style:num-format", Conversion::numberFormatCode(nfc));
            listStyleWriter.addAttribute("style:num-prefix", prefix);
            listStyleWriter.addAttribute("style:num-suffix", suffix);
            if (displayLevels > 1) {
                listStyleWriter.addAttribute("text:display-levels", displayLevels);
            }
            kDebug(30513) << "storing suffix" << suffix << " for depth" << depth;
            m_listSuffixes[ depth ] = suffix;
        } else {
            kWarning(30513) << "Not supported: counter text without the depth in it:" << Conversion::string(text);
        }

        if (listInfo->startAtOverridden())   //||
            //( numberingType == 1 && m_previousOutlineLSID != 0 && m_previousOutlineLSID != listInfo->lsid() ) ||
            //( numberingType == 0 &&m_previousEnumLSID != 0 && m_previousEnumLSID != listInfo->lsid() ) )
        {
            //counterElement.setAttribute( "restart", "true" );
        }

        //listInfo->isLegal() hmm
        //listInfo->notRestarted() [by higher level of lists] not supported
        //listInfo->followingchar() ignored, it's always a space in KWord currently
        //*************************************
        listStyleWriter.startElement("style:list-level-properties");
        switch (listInfo->alignment()) {
        case 1:
            listStyleWriter.addAttribute("fo:text-align", "center");
            break;
        case 2:
            listStyleWriter.addAttribute("fo:text-align", "end");
            break;
        case 3:
            listStyleWriter.addAttribute("fo:text-align", "justify");
            break;
        default:
            break;
        }

        if (listInfo->space()) {
            // Disabled for now. Have a look at the comment at the other text:min-label-distance above to see why.
            //listStyleWriter.addAttributePt("text:min-label-distance", listInfo->space()/20.0);
        }
        if (listInfo->indent()) {
            listStyleWriter.addAttributePt("text:min-label-width", listInfo->indent()/20.0);
        }
        listStyleWriter.endElement(); //style:list-level-properties
        //close element
        listStyleWriter.endElement(); //text:list-level-style-number
    } //end numbered list stuff

    //now add this info to our list style
    QString contents = QString::fromUtf8(buf.buffer(), buf.buffer().size());
    listStyle = m_mainStyles->styleForModification(m_listStyleName);
    //we'll add each one with a unique name
    QString name("listlevels");
    listStyle->addChildElement(name.append(QString::number(pap.ilvl)), contents);
} //end updateListStyle()

void KWordTextHandler::closeList()
{
    kDebug(30513);
    // Set the correct XML writer, get the last used writer from stack
    KoXmlWriter *writer = m_usedListWriters.pop();

    //TODO: should probably test this more, to make sure it does work this way
    //for level 0, we need to close the last item and the list
    //for level 1, we need to close the last item and the list, and the last item and the list
    //for level 2, we need to close the last item and the list, and the last item adn the list, and again
    for (int i = 0; i <= m_currentListDepth; i++) {
        writer->endElement(); //close the last text:list-item
        writer->endElement(); //text:list
    }

    //track this list ID, in case we open it again and need to continue the numbering
    m_previousLists[m_currentListID] = m_listStyleName;
    m_currentListID = 0;
    m_currentListDepth = -1;
    m_listStyleName = "";
}

bool KWordTextHandler::listIsOpen()
{
    return m_currentListID != 0;
}

void KWordTextHandler::saveState()
{
    kDebug(30513);
    m_oldStates.push(State(m_currentTable, m_paragraph, m_listStyleName,
                           m_currentListDepth, m_currentListID, m_previousLists));
    m_currentTable = 0;
    m_paragraph = 0;
    m_listStyleName = "";
    m_currentListDepth = -1;
    m_currentListID = 0;
    m_previousLists.clear();
}

void KWordTextHandler::restoreState()
{
    kDebug(30513);
    //if the stack is corrupt, we won't even try to set it correctly
    if (m_oldStates.empty()) {
        kWarning() << "Error: save/restore stack is corrupt!";
        return;
    }
    State s(m_oldStates.top());
    m_oldStates.pop();

    //warn if pointers weren't reset properly, but restore state anyway
    if (m_paragraph != 0) {
        kWarning() << "m_paragraph pointer wasn't reset";
    }
    m_paragraph = s.paragraph;
    if (m_currentTable != 0) {
        kWarning() << "m_currentTable pointer wasn't reset";
    }
    m_currentTable = s.currentTable;
    m_listStyleName = s.listStyleName;
    m_currentListDepth = s.currentListDepth;
    m_currentListID = s.currentListID;
    m_previousLists = s.previousLists;
}

void KWordTextHandler::fld_saveState()
{
    m_fldStates.push(m_fld);

    //reset fields related variables
    m_fld = 0;
}

void KWordTextHandler::fld_restoreState()
{
    //if the stack is corrupt, we won't even try to set it correctly
    if (m_fldStates.empty()) {
        kWarning() << "Error: save/restore stack is corrupt!";
        return;
    }

    //warn if pointers weren't reset properly, but restore state anyway
    if (m_fld->m_writer != 0) {
        kWarning() << "m_fld->m_writer pointer wasn't reset";
    }
    if (m_fld->m_buffer != 0) {
        kWarning() << "m_fld->m_buffer pointer wasn't reset";
    }

    m_fld = m_fldStates.top();
    m_fldStates.pop();
}

// ************************************************
//  Obsolete
// ************************************************

#ifdef TEXTHANDLER_OBSOLETE

//create an element for the variable
QDomElement KWordTextHandler::insertVariable(int type, wvWare::SharedPtr<const wvWare::Word97::CHP> chp, const QString& format)
{
    Q_UNUSED(chp);

    kDebug(30513) ;
    //m_paragraph += '#';

    QDomElement formatElem;
    //writeFormattedText(chp, m_currentStyle ? &m_currentStyle->chp() : 0);

    //m_index += 1;

    QDomElement varElem = m_formats.ownerDocument().createElement("VARIABLE");
    QDomElement typeElem = m_formats.ownerDocument().createElement("TYPE");
    typeElem.setAttribute("type", type);
    typeElem.setAttribute("key", format);
    varElem.appendChild(typeElem);
    formatElem.appendChild(varElem);
    return varElem;
}

QDomElement KWordTextHandler::insertAnchor(const QString& fsname)
{
    Q_UNUSED(fsname);

    kDebug(30513) ;
    //m_paragraph += '#';

    // Can't call writeFormat, we have no chp.
    //QDomElement format( mainDocument().createElement( "FORMAT" ) );
    //format.setAttribute( "id", 6 );
    //format.setAttribute( "pos", m_index );
    //format.setAttribute( "len", 1 );
    //m_formats.appendChild( format );
    //QDomElement formatElem = format;

    //m_index += 1;

    //QDomElement anchorElem = m_formats.ownerDocument().createElement( "ANCHOR" );
    //anchorElem.setAttribute( "type", "frameset" );
    //anchorElem.setAttribute( "instance", fsname );
    //formatElem.appendChild( anchorElem );
    return QDomElement();
}
#endif // TEXTHANDLER_OBSOLETE

#include "texthandler.moc"
