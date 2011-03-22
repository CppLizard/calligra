/*
 * This file is part of Office 2007 Filters for KOffice
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef MSOOXMLVMLREADER_IMPL_H
#define MSOOXMLVMLREADER_IMPL_H

#include <math.h>

#undef MSOOXML_CURRENT_NS
#define MSOOXML_CURRENT_NS "v"

//! Used by read_rect() to parse CSS2.
//! See ECMA-376 Part 4, 14.1.2.16, p.465.
//! @todo reuse KHTML parser?
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::parseCSS(const QString& style)
{
    m_vmlStyle.clear();
    foreach( const QString& pair, style.split(";", QString::SkipEmptyParts)) {
        const int splitIndex = pair.indexOf(":");
        if (splitIndex < 1)
            continue;
        const QByteArray name(pair.left(splitIndex).toLatin1().trimmed());
        QString value(pair.mid(splitIndex+1).trimmed());
        if (name.isEmpty())
            continue;
        if (value.startsWith("'") && value.endsWith("'"))
            value = value.mid(1, value.length() - 2); // strip ' '
        m_vmlStyle.insert(name, value);
        kDebug() << "name:" << name << "value:" << value;
    }
    return KoFilter::OK;
}

void MSOOXML_CURRENT_CLASS::createFrameStart(FrameStartElement startType)
{
    // Todo handle here all possible shape types
    if (startType == RectStart) {
        body->startElement("draw:rect");
    }
    // Simplifying connector to be a line
    else if (startType == StraightConnectorStart) {
        body->startElement("draw:line");
    }
    else if (startType == GroupStart) {
        body->startElement("draw:g");
    }
    else if (startType == CustomStart) {
        body->startElement("draw:custom-shape");
    }
    else {
        body->startElement("draw:frame");
    }

    QString width(m_vmlStyle.value("width")); // already in "...cm" format
    QString height(m_vmlStyle.value("height")); // already in "...cm" format
    QString x_mar(m_vmlStyle.value("margin-left"));
    QString y_mar(m_vmlStyle.value("margin-top"));
    QString leftPos(m_vmlStyle.value("left"));
    QString topPos(m_vmlStyle.value("top"));
    QString position(m_vmlStyle.value("position"));
    QString hor_pos(m_vmlStyle.value("mso-position-horizontal"));
    QString ver_pos(m_vmlStyle.value("mso-position-vertical"));
    QString hor_pos_rel(m_vmlStyle.value("mso-position-horizontal-relative"));
    QString ver_pos_rel(m_vmlStyle.value("mso-position-vertical-relative"));
    const QString ver_align(m_vmlStyle.value("v-text-anchor"));
    const QString rotation(m_vmlStyle.value("rotation"));

    qreal x_position = 0;
    QString x_pos_string, y_pos_string, widthString, heightString;
    qreal y_position = 0;
    qreal widthValue = 0;
    qreal heightValue = 0;

    if (!x_mar.isEmpty()) {
        if (m_insideGroup) {
            x_position = (x_mar.toInt() - m_groupX) * m_real_groupWidth / m_groupWidth + m_groupXOffset;
            x_pos_string = QString("%1%2").arg(x_position).arg(m_groupWidthUnit);
        } else {
            x_position = x_mar.left(x_mar.length() - 2).toDouble(); // removing the unit
            x_pos_string = x_mar;
        }
    }
    else if (!leftPos.isEmpty()) {
        if (m_insideGroup) {
            x_position = (leftPos.toInt() - m_groupX) * m_real_groupWidth / m_groupWidth + m_groupXOffset;
            x_pos_string = QString("%1%2").arg(x_position).arg(m_groupWidthUnit);
        } else {
            x_position = leftPos.left(leftPos.length() - 2).toDouble();
            x_pos_string = leftPos;
        }
    }
    else {
        if (m_insideGroup) {
            x_pos_string = QString("%1%2").arg(m_groupXOffset).arg(m_groupWidthUnit);
        }
    }
    if (!y_mar.isEmpty()) {
        if (m_insideGroup) {
            y_position = (y_mar.toInt() - m_groupY) * m_real_groupHeight / m_groupHeight + m_groupYOffset;
            y_pos_string = QString("%1%2").arg(y_position).arg(m_groupHeightUnit);
        } else {
            y_position = y_mar.left(y_mar.length() -2).toDouble();
            y_pos_string = y_mar;
        }
    }
    else if (!topPos.isEmpty()) {
        if (m_insideGroup) {
            y_position = (topPos.toInt() - m_groupY) * m_real_groupHeight / m_groupHeight + m_groupYOffset;
            y_pos_string = QString("%1%2").arg(y_position).arg(m_groupHeightUnit);
        } else {
            y_position = topPos.left(topPos.length() - 2).toDouble();
            y_pos_string = topPos;
        }
    }
    else {
        if (m_insideGroup) {
            y_pos_string = QString("%1%2").arg(m_groupYOffset).arg(m_groupHeightUnit);
        }
    }
    if (!width.isEmpty()) {
        if (m_insideGroup) {
            widthValue = width.toInt() * m_real_groupWidth / m_groupWidth;
            widthString = QString("%1%2").arg(widthValue).arg(m_groupWidthUnit);
        } else {
            widthValue = width.left(width.length() - 2).toDouble();
            widthString = width;
        }
    }
    if (!height.isEmpty()) {
        if (m_insideGroup) {
            heightValue = height.toInt() * m_real_groupHeight / m_groupHeight;
            heightString = QString("%1%2").arg(heightValue).arg(m_groupHeightUnit);
        }
        else {
            heightValue = height.left(height.length() - 2).toDouble();
            heightString = height;
        }
    }

    if (startType == StraightConnectorStart) {
        QString flip(m_vmlStyle.value("flip"));
        QString y1 = y_pos_string;
        QString x2 = QString("%1%2").arg(x_position + widthValue).arg(widthString.right(2)); // right(2) takes the unit
        QString x1 = x_pos_string;
        QString y2 = QString("%1%2").arg(y_position + heightValue).arg(heightString.right(2));
        if (flip == "x") {
            QString temp = y2;
            y2 = y1;
            y1 = temp;
        }
        if (flip == "y") {
            QString temp = x2;
            x2 = x1;
            x1 = temp;
        }
        body->addAttribute("svg:x1", x1);
        body->addAttribute("svg:y1", y1);
        body->addAttribute("svg:x2", x2);
        body->addAttribute("svg:y2", y2);
    }
    else {
        if (startType != GroupStart) {
            if (!x_pos_string.isEmpty()) {
                body->addAttribute("svg:x", x_pos_string);
            }
            if (!y_pos_string.isEmpty()) {
                body->addAttribute("svg:y", y_pos_string);
            }
            body->addAttribute("svg:width", widthString);
            body->addAttribute("svg:height", heightString);
        }
    }

    if (!m_shapeColor.isEmpty()) {
        m_currentDrawStyle->addProperty("draw:fill", "solid");
        m_currentDrawStyle->addProperty("draw:fill-color", m_shapeColor);
    }

    if (!ver_align.isEmpty()) {
        m_currentDrawStyle->addProperty("draw:textarea-vertical-align", ver_align);
    }

    if (!hor_pos.isEmpty()) {
        if (hor_pos == "absolute") {
            hor_pos = "from-left";
        }
        m_currentDrawStyle->addProperty("style:horizontal-pos", hor_pos);
    }
    if (!ver_pos.isEmpty()) {
        if (ver_pos == "absolute") {
            ver_pos = "from-top";
        }
        m_currentDrawStyle->addProperty("style:vertical-pos", ver_pos);
    }
    if (!hor_pos_rel.isEmpty()) {
        if (hor_pos_rel == "outer-margin-area" || hor_pos_rel == "left-margin-area") {
            hor_pos_rel = "page-start-margin";
            m_anchorType = "paragraph"; //forced
        }
        else if (hor_pos_rel == "margin") {
            hor_pos_rel = "paragraph-start-margin";
            m_anchorType = "paragraph"; //forced
        }
        else if (hor_pos_rel == "inner-margin-area" || hor_pos_rel == "right-margin-area") {
            hor_pos_rel = "page-end-margin";
            m_anchorType = "paragraph"; //forced
        }
        m_currentDrawStyle->addProperty("style:horizontal-rel", hor_pos_rel);
    }
    if (!ver_pos_rel.isEmpty()) {
        if (ver_pos_rel == "margin" || ver_pos_rel == "line") {
            ver_pos_rel = "page-content";
        }
        else if (ver_pos_rel == "top-margin-area" || ver_pos_rel == "inner-margin-area" || ver_pos_rel == "outer-margin-area") {
            ver_pos_rel = "page";
        }
        else if (ver_pos_rel == "bottom-margin-area") {
            ver_pos_rel = "page";
            // This effectively emulates the bottom-margin-area
            m_currentDrawStyle->addProperty("style:vertical-pos", "bottom");
        }
        m_currentDrawStyle->addProperty("style:vertical-rel", ver_pos_rel);
    }

#ifdef DOCXXMLDOCREADER_H
    bool asChar = false;
    if (!m_wrapRead) {
        m_currentDrawStyle->addProperty("style:wrap", "none");
        if (position.isEmpty() || position == "static") { // Default
            asChar = true;
            body->addAttribute("text:anchor-type", "as-char");
        }
        else {
            body->addAttribute("text:anchor-type", "char");
            if (m_currentDrawStyle->property("style:vertical-rel").isEmpty()) {
                m_currentDrawStyle->addProperty("style:vertical-rel", "paragraph");
            }
            if (m_currentDrawStyle->property("style:horizontal-rel").isEmpty()) {
                m_currentDrawStyle->addProperty("style:horizontal-rel", "page-content");
            }

            if (m_vmlStyle.contains("z-index")) {
                m_currentDrawStyle->addProperty("style:wrap", "run-through");
                if (m_vmlStyle.value("z-index").toInt() > 0) {
                    m_currentDrawStyle->addProperty("style:run-through", "foreground");
                }
                else {
                    m_currentDrawStyle->addProperty("style:run-through", "background");
                }
            }
            else {
                m_currentDrawStyle->addProperty("style:wrap", "run-through");
                m_currentDrawStyle->addProperty("style:run-through", "foreground");
            }
        }
    }
    else {
        body->addAttribute("text:anchor-type", m_anchorType);
    }
    if (!asChar) {
        // These seem to be decent defaults
        if (m_currentDrawStyle->property("style:horizontal-pos").isEmpty()) {
            m_currentDrawStyle->addProperty("style:horizontal-pos", "from-left");
        }
        if (m_currentDrawStyle->property("style:vertical-pos").isEmpty()) {
            m_currentDrawStyle->addProperty("style:vertical-pos", "from-top");
        }
    }
#endif

    m_currentPen.setWidthF(m_strokeWidth);
    m_currentPen.setColor(QColor(m_strokeColor));
    m_currentPen.setJoinStyle(Qt::MiterJoin);

    KoOdfGraphicStyles::saveOdfStrokeStyle(*m_currentDrawStyle, *mainStyles, m_currentPen);

    if (!m_currentDrawStyle->isEmpty()) {
        const QString drawStyleName( mainStyles->insert(*m_currentDrawStyle, "gr") );
        if (m_moveToStylesXml) {
            mainStyles->markStyleForStylesXml(drawStyleName);
        }

        body->addAttribute("draw:style-name", drawStyleName);
    }
}

KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::createFrameEnd()
{
    if (!m_imagedataPath.isEmpty()) {
        body->startElement("draw:image");
        body->addAttribute("xlink:type", "simple");
        body->addAttribute("xlink:show", "embed");
        body->addAttribute("xlink:actuate", "onLoad");
        body->addAttribute("xlink:href", m_imagedataPath);
        body->endElement(); // draw:image
    }

    {
        {
            const QString rId(m_vmlStyle.value("v:fill@r:id"));
            if (!rId.isEmpty()) {
                body->startElement("draw:image");
                const QString sourceName(m_context->relationships->target(m_context->path, m_context->file, rId));
                kDebug() << "sourceName:" << sourceName;
                if (sourceName.isEmpty()) {
                    return KoFilter::FileNotFound;
                }

                QString destinationName = QLatin1String("Pictures/") + sourceName.mid(sourceName.lastIndexOf('/') + 1);;
                RETURN_IF_ERROR( m_context->import->copyFile(sourceName, destinationName, false ) )
                addManifestEntryForFile(destinationName);
                addManifestEntryForPicturesDir();
                body->addAttribute("xlink:href", destinationName);
                body->endElement(); //draw:image
            }
        }
    }

    body->endElement(); //draw:frame or draw:rect

    return KoFilter::OK;
}

#undef CURRENT_EL
#define CURRENT_EL rect
//! rect handler (Rectangle)
/*! ECMA-376 Part 4, 14.1.2.16, p.449.
 This element is used to draw a simple rectangle.
 The CSS2 style content width and height define the width and height of the rectangle.

 Parent elements:
 - background (Part 1, §17.2.1)
 - group (§14.1.2.7)
 - object (Part 1, §17.3.3.19)
 - [done] pict (§9.2.2.2)
 - pict (§9.5.1)

 Child elements:
 - anchorlock (Anchor Location Is Locked) §14.3.2.1
 - borderbottom (Bottom Border) §14.3.2.2
 - borderleft (Left Border) §14.3.2.3
 - borderright (Right Border) §14.3.2.4
 - bordertop (Top Border) §14.3.2.5
 - callout (Callout) §14.2.2.2
 - ClientData (Attached Object Data) §14.4.2.12
 - clippath (Shape Clipping Path) §14.2.2.3
 - extrusion (3D Extrusion) §14.2.2.11
 - [done] fill (Shape Fill Properties) §14.1.2.5
 - formulas (Set of Formulas) §14.1.2.6
 - handles (Set of Handles) §14.1.2.9
 - imagedata (Image Data) §14.1.2.11
 - lock (Shape Protections) §14.2.2.18
 - path (Shape Path) §14.1.2.14
 - shadow (Shadow Effect) §14.1.2.18
 - signatureline (Digital Signature Line) §14.2.2.30
 - skew (Skew Transform) §14.2.2.31
 - [done] stroke (Line Stroke Settings) §14.1.2.21
 - [done] textbox (Text Box) §14.1.2.22
 - textdata (VML Diagram Text) §14.5.2.2
 - textpath (Text Layout Path) §14.1.2.23
 - [done] wrap (Text Wrapping) §14.3.2.6
*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_rect()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());

    TRY_READ_ATTR_WITHOUT_NS(style)
    RETURN_IF_ERROR(parseCSS(style))

    TRY_READ_ATTR_WITHOUT_NS(fillcolor)
    TRY_READ_ATTR_WITHOUT_NS(strokecolor)
    TRY_READ_ATTR_WITHOUT_NS(strokeweight)

    m_strokeWidth = 1 ; // This seems to be the default
    m_shapeColor.clear();
    m_strokeColor.clear();
    m_currentPen = QPen();

    if (!strokeweight.isEmpty()) {
        m_strokeWidth = strokeweight.left(strokeweight.length() - 2).toDouble(); // -2 removes 'pt'
        m_strokeColor = "#000000"; // Black color seems to be default
    }

    if (!fillcolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(filled)
        if (filled != "f" && filled != "false") {
            m_shapeColor = MSOOXML::Utils::rgbColor(fillcolor);
        }
    }

    if (!strokecolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(stroked)
        if (stroked != "f" && stroked != "false") {
            m_strokeColor = MSOOXML::Utils::rgbColor(strokecolor);
        }
    }

    MSOOXML::Utils::XmlWriteBuffer frameBuf;
    body = frameBuf.setWriter(body);

    pushCurrentDrawStyle(new KoGenStyle(KoGenStyle::GraphicAutoStyle, "graphic"));

    // Note that image fill is not yet supported in fill parameter, but this should be used
    // when it's done
    bool textBoxOrImage = false;
    m_wrapRead = false;

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(fill)
            else if (qualifiedName() == "v:textbox") {
                TRY_READ(textbox)
                textBoxOrImage = true;
            }
            ELSE_TRY_READ_IF(stroke)
            else if (qualifiedName() == "w10:wrap") {
                m_wrapRead = true;
                TRY_READ(wrap)
            }
            SKIP_UNKNOWN
//! @todo add ELSE_WRONG_FORMAT
        }
    }

    body = frameBuf.originalWriter();

    // Idea here is that if we do not have a box child, we want to still produce a rect to make sure there's a visible output
    if (!textBoxOrImage) {
        createFrameStart(RectStart);
    }
    else {
        createFrameStart();
    }

    (void)frameBuf.releaseWriter();

    createFrameEnd();

    popCurrentDrawStyle();

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL stroke
//! Stroke style handler
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_stroke()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());

    TRY_READ_ATTR_WITHOUT_NS(endcap)
    Qt::PenCapStyle penCap = m_currentPen.capStyle();
    if (endcap.isEmpty() || endcap == "sq") {
       penCap = Qt::SquareCap;
    }
    else if (endcap == "round") {
        penCap = Qt::RoundCap;
    }
    else if (endcap == "flat") {
        penCap = Qt::FlatCap;
    }
    m_currentPen.setCapStyle(penCap);

    TRY_READ_ATTR_WITHOUT_NS(dashstyle)
    if (!dashstyle.isEmpty()) {
        if (dashstyle == "1 1") {
            m_currentPen.setStyle(Qt::DashLine);
        }
        else if (dashstyle == "dashDot") {
            m_currentPen.setStyle(Qt::DashDotLine);
        }
    }

    readNext();
    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL group
//! Vml group handler
/*
 Parent elements:
 - background (Part 1, §17.2.1);
 - group (§14.1.2.7);
 - object (Part 1, §17.3.3.19);
 - [done] pict (§9.2.2.2);
 - [done] pict (§9.5.1)

 Child elements:
 - anchorlock (Anchor Location Is Locked) §14.3.2.1
 - arc (Arc Segment) §14.1.2.1
 - borderbottom (Bottom Border) §14.3.2.2
 - borderleft (Left Border) §14.3.2.3
 - borderright (Right Border) §14.3.2.4
 - bordertop (Top Border) §14.3.2.5
 - callout (Callout) §14.2.2.2
 - ClientData (Attached Object Data) §14.4.2.12
 - clippath (Shape Clipping Path) §14.2.2.3
 - curve (Bezier Curve) §14.1.2.3
 - diagram (VML Diagram) §14.2.2.8
 - extrusion (3D Extrusion) §14.2.2.11
 - [done] fill (Shape Fill Properties) §14.1.2.5
 - formulas (Set of Formulas) §14.1.2.6
 - [done] group (Shape Group) §14.1.2.7
 - handles (Set of Handles) §14.1.2.9
 - image (Image File) §14.1.2.10
 - imagedata (Image Data) §14.1.2.11
 - line (Line) §14.1.2.12
 - lock (Shape Protections) §14.2.2.18
 - [done] oval (Oval) §14.1.2.13
 - path (Shape Path) §14.1.2.14
 - polyline (Multiple Path Line) §14.1.2.15
 - [done] rect (Rectangle) §14.1.2.16
 - [done] roundrect (Rounded Rectangle) §14.1.2.17
 - shadow (Shadow Effect) §14.1.2.18
 - [done] shape (Shape Definition) §14.1.2.19
 - [done] shapetype (Shape Template) §14.1.2.20
 - signatureline (Digital Signature Line) §14.2.2.30
 - skew (Skew Transform) §14.2.2.31
 - [done] stroke (Line Stroke Settings) §14.1.2.21
 - textbox (Text Box) §14.1.2.22
 - textdata (VML Diagram Text) §14.5.2.2
 - textpath (Text Layout Path) §14.1.2.23
 - [done] wrap (Text Wrapping) §14.3.2.6
*/
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_group()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());
    TRY_READ_ATTR_WITHOUT_NS(style)
    RETURN_IF_ERROR(parseCSS(style))

    pushCurrentDrawStyle(new KoGenStyle(KoGenStyle::GraphicAutoStyle, "graphic"));

    TRY_READ_ATTR_WITHOUT_NS(fillcolor)
    TRY_READ_ATTR_WITHOUT_NS(strokecolor)
    TRY_READ_ATTR_WITHOUT_NS(strokeweight)

    // To handle nested groups
    bool groupStartsFromMe = false;

    if (!m_insideGroup) {

        const QString width(m_vmlStyle.value("width"));
        const QString height(m_vmlStyle.value("height"));

        // These potentially cause an offset to all shapes in the group
        // Unhandled case: theoretically x_mar could be in different units
        // than width, in this case they should be added somehow intelligently
        const QString x_mar(m_vmlStyle.value("margin-left"));
        const QString y_mar(m_vmlStyle.value("margin-top"));

        m_insideGroup = true;
        groupStartsFromMe = true;

        m_groupWidthUnit = width.right(2); // pt, cm etc.
        m_groupHeightUnit = height.right(2);
        m_real_groupWidth = width.left(width.length() - 2).toDouble();
        m_real_groupHeight = height.left(height.length() - 2).toDouble();

        m_groupX = 0;
        m_groupY = 0;
        m_groupWidth = 0;
        m_groupHeight = 0;
        m_groupXOffset = 0;
        m_groupYOffset = 0;
        // Assuming that two last chars are unit (cm, pt)
        m_groupXOffset = x_mar.left(x_mar.length() - 2).toDouble();
        m_groupYOffset = y_mar.left(y_mar.length() - 2).toDouble();

        TRY_READ_ATTR_WITHOUT_NS(coordsize)

        if (!coordsize.isEmpty()) {
            m_groupWidth = coordsize.mid(0, coordsize.indexOf(',')).toInt();
            m_groupHeight = coordsize.mid(coordsize.indexOf(',') + 1).toInt();
        }

        TRY_READ_ATTR_WITHOUT_NS(coordorigin)
        if (!coordorigin.isEmpty()) {
            m_groupX = coordorigin.mid(0, coordorigin.indexOf(',')).toInt();
            m_groupY = coordorigin.mid(coordorigin.indexOf(',') + 1).toInt();
        }
    }

    MSOOXML::Utils::XmlWriteBuffer frameBuf;
    body = frameBuf.setWriter(body);

    m_wrapRead = false;

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(rect)
            ELSE_TRY_READ_IF(shapetype)
            ELSE_TRY_READ_IF(roundrect)
            ELSE_TRY_READ_IF(oval)
            ELSE_TRY_READ_IF(shape)
            ELSE_TRY_READ_IF(group)
            ELSE_TRY_READ_IF(fill)
            ELSE_TRY_READ_IF(stroke)
            else if (qualifiedName() == "w10:wrap") {
                m_wrapRead = true;
                TRY_READ(wrap)
            }
            SKIP_UNKNOWN
//! @todo add ELSE_WRONG_FORMAT
        }
    }

    if (groupStartsFromMe) {
        m_insideGroup = false;
    }

    body = frameBuf.originalWriter();

    m_strokeWidth = 1 ; // This seems to be the default
    m_shapeColor.clear();
    m_strokeColor.clear();
    m_currentPen = QPen();

    if (!strokeweight.isEmpty()) {
        m_strokeWidth = strokeweight.left(strokeweight.length() - 2).toDouble(); // -2 removes 'pt'
        m_strokeColor = "#000000"; // Black color seems to be default
    }

    if (!fillcolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(filled)
        if (filled != "f" && filled != "false") {
            m_shapeColor = MSOOXML::Utils::rgbColor(fillcolor);
        }
    }

    if (!strokecolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(stroked)
        if (stroked != "f" && stroked != "false") {
            m_strokeColor = MSOOXML::Utils::rgbColor(strokecolor);
        }
    }

    createFrameStart(GroupStart);

    (void)frameBuf.releaseWriter();

    body->endElement(); // draw:g

    popCurrentDrawStyle();

    READ_EPILOGUE
}

// Generic helper which approximates all figures to be rectangles
// use until better implementation is done
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::genericReader()
{
    const QXmlStreamAttributes attrs(attributes());
//! @todo support more attrs
    TRY_READ_ATTR_WITHOUT_NS(style)
    RETURN_IF_ERROR(parseCSS(style))

    pushCurrentDrawStyle(new KoGenStyle(KoGenStyle::GraphicAutoStyle, "graphic"));

    TRY_READ_ATTR_WITHOUT_NS(fillcolor)
    TRY_READ_ATTR_WITHOUT_NS(strokecolor)
    TRY_READ_ATTR_WITHOUT_NS(strokeweight)

    m_strokeWidth = 1 ; // This seems to be the default
    m_shapeColor.clear();
    m_strokeColor.clear();
    m_currentPen = QPen();

    if (!strokeweight.isEmpty()) {
        m_strokeWidth = strokeweight.left(strokeweight.length() - 2).toDouble(); // -2 removes 'pt'
        m_strokeColor = "#000000"; // Black color seems to be default
    }

    if (!fillcolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(filled)
        if (filled != "f" && filled != "false") {
            m_shapeColor = MSOOXML::Utils::rgbColor(fillcolor);
        }
    }

    if (!strokecolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(stroked)
        if (stroked != "f" && stroked != "false") {
            m_strokeColor = MSOOXML::Utils::rgbColor(strokecolor);
        }
    }

    MSOOXML::Utils::XmlWriteBuffer frameBuf;
    body = frameBuf.setWriter(body);

    bool textBoxOrImage = false;
    m_wrapRead = false;

    while (!atEnd()) {
        readNext();
        if (isEndElement() && qualifiedName() == m_currentEl) {
            break;
        }
        else if (isStartElement()) {
            TRY_READ_IF(fill)
            else if (qualifiedName() == "v:textbox") {
                TRY_READ(textbox)
                textBoxOrImage = true;
            }
            ELSE_TRY_READ_IF(stroke)
            else if (qualifiedName() == "w10:wrap") {
                m_wrapRead = true;
                TRY_READ(wrap)
            }
            SKIP_UNKNOWN
//! @todo add ELSE_WRONG_FORMAT
        }
    }

    body = frameBuf.originalWriter();

    if (!textBoxOrImage) {
        // This should be roundRect type at some point when it's supported
        createFrameStart(RectStart);
    }
    else {
        createFrameStart();
    }

    (void)frameBuf.releaseWriter();

    createFrameEnd();

    popCurrentDrawStyle();

    return KoFilter::OK;
}

#undef CURRENT_EL
#define CURRENT_EL oval
//! oval handler (Oval)
// For parents, children, look from rect
// Note: this is atm. simplified, should in reality make an oval
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_oval()
{
    READ_PROLOGUE

    m_currentEl = "v:oval";
    KoFilter::ConversionStatus status = genericReader();
    if (status != KoFilter::OK) {
        return status;
    }

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL roundrect
//! roundrect handler (Rouned rectangle)
// For parents, children, look from rect
// Note: this is atm. simplified, should in reality make a round rectangle
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_roundrect()
{
    READ_PROLOGUE

    m_currentEl = "v:roundrect";
    KoFilter::ConversionStatus status = genericReader();
    if (status != KoFilter::OK) {
        return status;
    }

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL fill
//! fill handler (Shape Fill Properties)
/*! ECMA-376 Part 4, 14.1.2.16, p.280.
 This element specifies how the path should be filled if something beyond
 a solid color fill is desired.

 Parent elements:
 - arc (§14.1.2.1)
 - background (Part 1, §17.2.1)
 - background (§14.1.2.2)
 - curve (§14.1.2.3)
 - group (§14.1.2.7)
 - image (§14.1.2.10)
 - line (§14.1.2.12)
 - object (Part 1, §17.3.3.19)
 - oval (§14.1.2.13)
 - pict (§9.2.2.2)
 - pict (§9.5.1)
 - polyline (§14.1.2.15)
 - [done] rect (§14.1.2.16)
 - roundrect (§14.1.2.17)
 - shape (§14.1.2.19)
 - shapedefaults (§14.2.2.28)
 - shapetype (§14.1.2.20)

 Child elements:
 - fill (Shape Fill Extended Properties) §14.2.2.13
*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_fill()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());
//! @todo support more attrs
    // Relationship ID of the relationship to the image used for this fill.
    TRY_READ_ATTR_WITH_NS(r, id)
    m_vmlStyle.insert("v:fill@r:id", r_id);
    // The kind of fill. Default is solid.
    TRY_READ_ATTR_WITHOUT_NS(type)
    // frame (Stretch Image to Fit) - The image is stretched to fill the shape.
    // gradient (Linear Gradient) - The fill colors blend together in a linear gradient from bottom to top.
    // gradientRadial (Radial Gradient) - The fill colors blend together in a radial gradient.
    // pattern (Image Pattern) - The image is used to create a pattern using the fill colors.
    // tile (Tiled Image) - The fill image is tiled.
    // solid (Solid Fill) - The fill pattern is a solid color.

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
//            TRY_READ_IF(fill)
//! @todo add ELSE_WRONG_FORMAT
        }
    }
    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL background

//! fill handler (Document Background)
/*! ECMA-376 Part 4, 14.1.2.2, p.235.
 This element describes the fill of the background of a page using vector graphics fills.

 Parent elements:
 - background (Part 1, §17.2.1)
 - object (Part 1, §17.3.3.19)
 - pict (§9.2.2.2)
 - pict (§9.5.1)

 Child elements:
 - [Done] fill (Shape Fill Properties) §14.1.2.5

 Attributes:
 - bwmode (Blackand- White Mode)
 - bwnormal (Normal Black-and-White Mode)
 - bwpure (Pure Black-and-White Mode)
 - fillcolor (Fill Color)
 - filled (Shape Fill Toggle)
 - id (Unique Identifier)
 - targetscreensize (Target Screen Size)
*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_VML_background()
{
    READ_PROLOGUE2(VML_background)
    //const QXmlStreamAttributes attrs(attributes());
    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(fill)
            ELSE_WRONG_FORMAT
        }
    }
    const QString rId(m_vmlStyle.value("v:fill@r:id"));
    if (!rId.isEmpty()) {
        const QString sourceName(m_context->relationships->target(m_context->path, m_context->file, rId));
        kDebug() << "sourceName:" << sourceName;
        if (sourceName.isEmpty()) {
            return KoFilter::FileNotFound;
        }
        QString destinationName = QLatin1String("Pictures/") + sourceName.mid(sourceName.lastIndexOf('/') + 1);;
        RETURN_IF_ERROR( m_context->import->copyFile(sourceName, destinationName, false ) )
        addManifestEntryForFile(destinationName);
        addManifestEntryForPicturesDir();
        if (m_pDocBkgImageWriter) {
            delete m_pDocBkgImageWriter->device();
            delete m_pDocBkgImageWriter;
            m_pDocBkgImageWriter = NULL;
        }
        QBuffer* buffer = new QBuffer();
        m_pDocBkgImageWriter = new KoXmlWriter(buffer);

        m_pDocBkgImageWriter->startElement("style:background-image");
        m_pDocBkgImageWriter->addAttribute("xlink:href", destinationName);
        m_pDocBkgImageWriter->addAttribute("xlink:type", "simple");
        m_pDocBkgImageWriter->addAttribute("xlink:actuate", "onLoad");
        m_pDocBkgImageWriter->endElement(); //style:background-image
    }
    READ_EPILOGUE
}

static QString getNumber(QString& source)
{
    QString number;
    int index = 0;
    bool numberOk = true;
    while (true) {
        QString(source.at(index)).toInt(&numberOk);
        if (numberOk) {
            number = number + source.at(index);
            ++index;
        }
        else {
            break;
        }
    }
    source = source.mid(index);
    return number;
}

static QString getArgument(QString& source, bool commaMeansZero, bool& wasCommand)
{
    wasCommand = false;
    if (source.at(0) == ',') {
        source = source.mid(1);
        if (commaMeansZero) {
            return "0";
        }
    }
    bool isNumber;
    QString(source.at(0)).toInt(&isNumber);
    if (isNumber) {
        return getNumber(source);
    }
    if (source.at(0) == '-') { //negative number
        source = source.mid(1);
        return QString("-%1").arg(getNumber(source));
    }
    if (source.at(0) == ',') { // case of 1,,2
        return "0";
    }
    if (source.at(0) == '#') {
        source = source.mid(1);
        return QString("$%1").arg(getNumber(source));
    }
    if (source.at(0) == '@') {
        source = source.mid(1);
        return QString("?f%1").arg(getNumber(source));
    }

    wasCommand = true;
    return "0"; // this means case 1,e
}

static QString convertToEnhancedPath(const QString& source)
{
    enum ConversionState {CommandExpected, ArgumentExpected};
    QString parsedString = source;
    QString returnedString;
    ConversionState state = CommandExpected;
    enum CommandType {MoveCommand, LineCommand, RelativeLineCommand, QuadEllipXCommand, QuadEllipYCommand,
                      CurveCommand, RelativeCurveCommand, ArcCommand, ArcToCommand, ArcCommandClock, ArcToCommandClock,
                      AngleEllipseToCommand, AngleEllipseCommand};
    CommandType lastCommand = MoveCommand;
    QString firstMoveX, firstMoveY, currentX, currentY;
    bool argumentMove;
    QChar command;
    QString first, second, third, fourth, fifth, sixth, seventh, eighth;

    while (true) {
        if (parsedString.length() == 0) {
            break;
        }
        while (parsedString.at(0) == ' ') {
            parsedString = parsedString.trimmed();
        }
        switch (state) {
        case CommandExpected:
            command = parsedString.at(0);
            parsedString = parsedString.mid(1);
            state = ArgumentExpected;
            if (command == 'm') {
                lastCommand = MoveCommand;
            }
            else if (command == 'l') {
                lastCommand = LineCommand;
            }
            else if (command == 'r') {
                lastCommand = RelativeLineCommand;
            }
            else if (command == 'x') {
                state = CommandExpected;
                returnedString += " Z";
            }
            else if (command == 'e') {
                returnedString += " N";
                state = CommandExpected;
            }
            else if (command == 'c') {
                lastCommand = CurveCommand;
            }
            else if (command == 'v') {
                lastCommand = RelativeCurveCommand;
            }
            else if (command == 'q') {
                QChar subcommand = parsedString.at(0);
                parsedString = parsedString.mid(1);
                if (subcommand == 'x') {
                    lastCommand = QuadEllipXCommand;
                    returnedString += " X";
                }
                else {
                    lastCommand = QuadEllipYCommand;
                    returnedString += " Y";
                }
            }
            else if (command == 'a') {
                QChar subcommand = parsedString.at(0);
                parsedString = parsedString.mid(1);
                if (subcommand == 'r') {
                    lastCommand = ArcCommand;
                    returnedString += " B";
                }
                else if (subcommand == 'e') {
                    lastCommand = AngleEllipseToCommand;
                    returnedString += " T";
                }
                else if (subcommand == 'l') {
                    lastCommand = AngleEllipseCommand;
                    returnedString += " U";
                }
                else {
                    lastCommand = ArcToCommand;
                    returnedString += " A";
                }
            }
            else if (command == 'w') {
                QChar subcommand = parsedString.at(0);
                parsedString = parsedString.mid(1);
                if (subcommand == 'r') {
                    lastCommand = ArcCommandClock;
                    returnedString += " V";
                }
                else {
                    lastCommand = ArcToCommandClock;
                    returnedString += " W";
                }
            }
            else if (command == 'n') {
                QChar subcommand = parsedString.at(0);
                parsedString = parsedString.mid(1);
                if (subcommand == 'f') {
                    returnedString += " F";
                }
                else {
                    returnedString += " S";
                }
                state = CommandExpected;
            }
            break;
        case ArgumentExpected:
            switch (lastCommand) {
            case MoveCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                firstMoveX = first;
                firstMoveY = second;
                currentX = first;
                currentY = second;
                returnedString += QString(" M %1 %2").arg(first).arg(second);
                state = CommandExpected;
                break;
            case RelativeCurveCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                third = getArgument(parsedString, false, argumentMove);
                fourth = getArgument(parsedString, false, argumentMove);
                fifth = getArgument(parsedString, false, argumentMove);
                sixth = getArgument(parsedString, false, argumentMove);
                first = QString("%1").arg(first.toInt() + currentX.toInt());
                second = QString("%1").arg(second.toInt() + currentY.toInt());
                third = QString("%1").arg(third.toInt() + currentX.toInt());
                fourth = QString("%1").arg(fourth.toInt() + currentY.toInt());
                currentX = QString("%1").arg(fifth.toInt() + currentX.toInt());
                currentY = QString("%1").arg(sixth.toInt() + currentY.toInt());
                returnedString += QString(" C %1 %2 %3 %4 %5 %6").arg(first).arg(second).arg(third).
                                  arg(fourth).arg(currentX).arg(currentY);
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    third = getArgument(parsedString, false, argumentMove);
                    fourth = getArgument(parsedString, false, argumentMove);
                    fifth = getArgument(parsedString, false, argumentMove);
                    sixth = getArgument(parsedString, false, argumentMove);
                    first = QString("%1").arg(first.toInt() + currentX.toInt());
                    second = QString("%1").arg(second.toInt() + currentY.toInt());
                    third = QString("%1").arg(third.toInt() + currentX.toInt());
                    fourth = QString("%1").arg(fourth.toInt() + currentY.toInt());
                    currentX = QString("%1").arg(fifth.toInt() + currentX.toInt());
                    currentY = QString("%1").arg(sixth.toInt() + currentY.toInt());
                    returnedString += QString(" %1 %2 %3 %4 %5 %6").arg(first).arg(second).arg(third).
                                      arg(fourth).arg(currentX).arg(currentY);
                }
                break;
            case CurveCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                third = getArgument(parsedString, false, argumentMove);
                fourth = getArgument(parsedString, false, argumentMove);
                fifth = getArgument(parsedString, false, argumentMove);
                sixth = getArgument(parsedString, false, argumentMove);
                returnedString += QString(" C %1 %2 %3 %4 %5 %6").arg(first).arg(second).arg(third).
                                  arg(fourth).arg(fifth).arg(sixth);
                currentX = fifth;
                currentY = sixth;
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    third = getArgument(parsedString, false, argumentMove);
                    fourth = getArgument(parsedString, false, argumentMove);
                    fifth = getArgument(parsedString, false, argumentMove);
                    sixth = getArgument(parsedString, false, argumentMove);
                    returnedString += QString(" %1 %2 %3 %4 %5 %6").arg(first).arg(second).arg(third).
                                      arg(fourth).arg(fifth).arg(sixth);
                    currentX = fifth;
                    currentY = sixth;
                }
                break;
            case LineCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                returnedString += QString(" L %1 %2").arg(first).arg(second);
                currentX = first;
                currentY = second;
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    currentX = first;
                    currentY = second;
                    returnedString += QString(" %1 %2").arg(first).arg(second);
                }
                break;
            case RelativeLineCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                currentX = QString("%1").arg(first.toInt() + currentX.toInt());
                currentY = QString("%1").arg(second.toInt() + currentY.toInt());
                returnedString += QString(" L %1 %2").arg(currentX).arg(currentY);
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    currentX = QString("%1").arg(first.toInt() + currentX.toInt());
                    currentY = QString("%1").arg(second.toInt() + currentY.toInt());
                    returnedString += QString(" %1 %2").arg(currentX).arg(currentY);
                }
                break;
            case QuadEllipXCommand:
            case QuadEllipYCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                returnedString += QString(" %1 %2").arg(first).arg(second);
                currentX = first;
                currentY = second;
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    currentX = first;
                    currentY = second;
                    returnedString += QString(" %1 %2").arg(first).arg(second);
                }
                break;
            case ArcCommand:
            case ArcToCommand:
            case ArcCommandClock:
            case ArcToCommandClock:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                third = getArgument(parsedString, false, argumentMove);
                fourth = getArgument(parsedString, false, argumentMove);
                fifth = getArgument(parsedString, false, argumentMove);
                sixth = getArgument(parsedString, false, argumentMove);
                seventh = getArgument(parsedString, false, argumentMove);
                eighth = getArgument(parsedString, false, argumentMove);
                currentX = seventh;
                currentY = eighth;
                returnedString += QString(" %1 %2 %3 %4 %5 %6 %7 %8").arg(first).arg(second).arg(third).arg(fourth).
                    arg(fifth).arg(sixth).arg(seventh).arg(eighth);
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    third = getArgument(parsedString, false, argumentMove);
                    fourth = getArgument(parsedString, false, argumentMove);
                    fifth = getArgument(parsedString, false, argumentMove);
                    sixth = getArgument(parsedString, false, argumentMove);
                    seventh = getArgument(parsedString, false, argumentMove);
                    eighth = getArgument(parsedString, false, argumentMove);
                    currentX = seventh;
                    currentY = eighth;
                    returnedString += QString(" %1 %2 %3 %4 %5 %6 %7 %8").arg(first).arg(second).arg(third).arg(fourth).
                        arg(fifth).arg(sixth).arg(seventh).arg(eighth);
                }
                break;
            case AngleEllipseToCommand:
            case AngleEllipseCommand:
                first = getArgument(parsedString, true, argumentMove);
                second = getArgument(parsedString, false, argumentMove);
                third = getArgument(parsedString, false, argumentMove);
                fourth = getArgument(parsedString, false, argumentMove);
                fifth = getArgument(parsedString, false, argumentMove);
                sixth = getArgument(parsedString, false, argumentMove);
                // These are most likely wrong, spec does not mention which units the angle uses
                // nor how the angle should be interpreted
                currentX = QString("%1").arg(first.toInt() + cos(sixth.toDouble()));
                currentY = QString("%1").arg(first.toInt() + sin(sixth.toDouble()));
                returnedString += QString(" %1 %2 %3 %4 %5 %6").arg(first).arg(second).arg(third).arg(fourth).
                    arg(fifth).arg(sixth);
                while (true) {
                    first = getArgument(parsedString, false, argumentMove);
                    if (argumentMove) {
                        state = CommandExpected;
                        break;
                    }
                    second = getArgument(parsedString, false, argumentMove);
                    third = getArgument(parsedString, false, argumentMove);
                    fourth = getArgument(parsedString, false, argumentMove);
                    fifth = getArgument(parsedString, false, argumentMove);
                    sixth = getArgument(parsedString, false, argumentMove);
                    currentX = QString("%1").arg(first.toInt() + cos(sixth.toDouble()));
                    currentY = QString("%1").arg(first.toInt() + sin(sixth.toDouble()));
                    returnedString += QString(" %1 %2 %3 %4 %5 %6").arg(first).arg(second).arg(third).arg(fourth).
                    arg(fifth).arg(sixth);
                }
                break;
            }
        }
    }

    return returnedString;
}

#undef CURRENT_EL
#define CURRENT_EL shapetype
//! shapetype handler (Shape Template)
/*! ECMA-376 Part 4, 14.1.2.20, p.539.
 This element defines a shape template that can be used to create other shapes.
 Shapetype is identical to the shape element (§14.1.2.19) except it cannot reference another
 shapetype element.

 Parent elements:
 - background (Part 1, §17.2.1)
 - group (§14.1.2.7)
 - [done] object (Part 1, §17.3.3.19)
 - pict (§9.2.2.2)
 - pict (§9.5.1)

 Child elements:
 - bordertop (Top Border) §14.3.2.5
 - callout (Callout) §14.2.2.2
 - ClientData (Attached Object Data) §14.4.2.12
 - clippath (Shape Clipping Path) §14.2.2.3
 - complex (Complex) §14.2.2.7
 - extrusion (3D Extrusion) §14.2.2.11
 - fill (Shape Fill Properties) §14.1.2.5
 - [done] formulas (Set of Formulas) §14.1.2.6
 - handles (Set of Handles) §14.1.2.9
 - imagedata (Image Data) §14.1.2.11
 - lock (Shape Protections) §14.2.2.18
 - path (Shape Path) §14.1.2.14
 - shadow (Shadow Effect) §14.1.2.18
 - signatureline (Digital Signature Line) §14.2.2.30
 - skew (Skew Transform) §14.2.2.31
 - stroke (Line Stroke Settings) §14.1.2.21
 - textbox (Text Box) §14.1.2.22
 - textdata (VML Diagram Text) §14.5.2.2
 - textpath (Text Layout Path) §14.1.2.23
 - wrap (Text Wrapping) §14.3.2.6
*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_shapetype()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());

    TRY_READ_ATTR_WITHOUT_NS(adj)
    TRY_READ_ATTR_WITHOUT_NS(coordsize)
    TRY_READ_ATTR_WITHOUT_NS(id)
    TRY_READ_ATTR_WITHOUT_NS(path)

    // Using KoXmlWriters is another opinion, but this one is used to avoid
    // hassle with deletions (koxmlwriter not deleting its device etc)
    m_shapeTypeString = "<draw:enhanced-geometry ";

    if (!adj.isEmpty()) {
        QString tempModifiers = adj;
        tempModifiers.replace(',', " ");
        m_shapeTypeString += QString("draw:modifiers=\"%1\" ").arg(tempModifiers);
    }
    if (!coordsize.isEmpty()) {
        QString tempViewBox = "0 0 " + coordsize;
        tempViewBox.replace(',', " ");
        m_shapeTypeString += QString("svg:viewBox=\"%1\" ").arg(tempViewBox);
    }
    if (!path.isEmpty()) {
        QString enPath = convertToEnhancedPath(path);
        m_shapeTypeString += QString("draw:enhanced-path=\"%1\" ").arg(enPath);
    }

    m_shapeTypeString += ">";

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(formulas)
            SKIP_UNKNOWN
        }
    }

    m_shapeTypeString += "</draw:enhanced-geometry>";
    m_shapeTypeStrings[id] = m_shapeTypeString;

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL formulas
/* formulas handlers (Set of Formulas)

 Parent elements:
 - arc (§14.1.2.1);
 - background (Part 1, §17.2.1);
 - curve (§14.1.2.3);
 - group (§14.1.2.7);
 - image (§14.1.2.10);
 - line (§14.1.2.12);
 - object (Part 1, §17.3.3.19);
 - oval (§14.1.2.13);
 - pict (§9.2.2.2);
 - pict (§9.5.1);
 - polyline (§14.1.2.15);
 - rect (§14.1.2.16);
 - roundrect (§14.1.2.17);
 - shape (§14.1.2.19);
 - [done] shapetype (§14.1.2.20)

 Child elements:
 -  [done] f (Single Formula) §14.1.2.4
*/
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_formulas()
{
    READ_PROLOGUE

    m_formulaIndex = 0;

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(f)
            ELSE_WRONG_FORMAT
        }
    }

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL f
/*! f handler (Shape Definition)
 Parent elements:
 - [done] formulas (§14.1.2.6)

 Child elements:
 - none
*/
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_f()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());
    TRY_READ_ATTR_WITHOUT_NS(eqn)
    m_shapeTypeString += "\n<draw:equation ";
    m_shapeTypeString += QString("draw:name=\"f%1\" draw:formula=\"").arg(m_formulaIndex);

    if (!eqn.isEmpty()) {
        eqn = eqn.trimmed();
        eqn.replace('#', '$'); // value reference
        eqn.replace('@', "?f"); // function reference
        eqn.replace("pixelWidth", "width");
        eqn.replace("pixelHeight", "height");
        eqn.replace("emuWidth", "width");
        eqn.replace("emuHeight", "height");
        eqn.replace("emuWidth2", "(width/2)");
        eqn.replace("emuHeight2", "(height/2)");
        eqn.replace("lineDrawn", "hasstroke");
        eqn.replace("pixelLineWidth", "1");
        eqn.replace("xcenter", "(width/2)");
        eqn.replace("ycenter", "(height/2)");
        int commandIndex = eqn.indexOf(' ');
        QString command = eqn.left(commandIndex);
        eqn = eqn.mid(commandIndex + 1);
        QList<QString> parameters;
        while (true) {
            commandIndex = eqn.indexOf(' ');
            if (commandIndex < 0) {
                parameters.append(eqn);
                break;
            }
            parameters.append(eqn.left(commandIndex));
            eqn = eqn.mid(commandIndex + 1);
        }
        if (command == "val") {
            m_shapeTypeString += parameters.at(0);
        }
        else if (command == "sum") {
            m_shapeTypeString += parameters.at(0) + "+" + parameters.at(1) + "-" + parameters.at(2);
        }
        else if (command == "prod") {
            m_shapeTypeString += parameters.at(0) + "*" + parameters.at(1) + "/" + parameters.at(2);
        }
        else if (command == "abs") {
            m_shapeTypeString += QString("abs(%1)").arg(parameters.at(0));
        }
        else if (command == "min") {
            m_shapeTypeString += QString("min(%1,%2)").arg(parameters.at(0)).arg(parameters.at(1));
        }
        else if (command == "max") {
            m_shapeTypeString += QString("max(%1,%2)").arg(parameters.at(0)).arg(parameters.at(1));
        }
        else if (command == "if") {
            m_shapeTypeString += QString("if(max(%1,0),%2,%3)").arg(parameters.at(0)).arg(parameters.at(1)).arg(parameters.at(2));
        }
        else if (command == "sqrt") {
            m_shapeTypeString += QString("sqrt(%1)").arg(parameters.at(0));
        }
        else if (command == "mid") {
            m_shapeTypeString += QString("(%1+%2)/2").arg(parameters.at(0)).arg(parameters.at(1));
        }
        else if (command == "mod") {
            m_shapeTypeString += QString("sqrt(%1*%1+%2*%2+%3*%3)").arg(parameters.at(0)).arg(parameters.at(1)).arg(parameters.at(2));
        }
        else if (command == "ellipse") {
            m_shapeTypeString += QString("%3-sqrt(1-(%1/%2)*(%1/%2))").arg(parameters.at(0)).arg(parameters.at(1)).arg(parameters.at(2));
        }
        else if (command == "atan2") {
            m_shapeTypeString += QString("atan2(%2,%1)").arg(parameters.at(0)).arg(parameters.at(1));
        }
        else if (command == "cosatan2") {
            m_shapeTypeString += QString("%1*cos(atan2(%3,%2))").arg(parameters.at(0)).arg(parameters.at(1)).arg(parameters.at(2));
        }
        else if (command == "sinatan2") {
            m_shapeTypeString += QString("%1*sin(atan2(%3,%2))").arg(parameters.at(0)).arg(parameters.at(1)).arg(parameters.at(2));
        }
        else if (command == "sumangle") {
            m_shapeTypeString += QString("%1+%2*65536-%3*65536").arg(parameters.at(0)).arg(parameters.at(1)).arg(parameters.at(2));
        }
        else if (command == "sin") {
            m_shapeTypeString += QString("%1*sin(%2*65536)").arg(parameters.at(0)).arg(parameters.at(1));
        }
        else if (command == "cos") {
            m_shapeTypeString += QString("%1*cos(%2*65536)").arg(parameters.at(0)).arg(parameters.at(1));
        }
        else if (command == "tan") {
            m_shapeTypeString += QString("%1*tan(%2*65536)").arg(parameters.at(0)).arg(parameters.at(1));
        }
    }

    m_shapeTypeString += "\" ";
    m_shapeTypeString += "/>";

    ++m_formulaIndex;
    readNext();
    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL shape
/*! shape handler (Shape Definition)
 ECMA-376 Part 4, 14.1.2.19, p.509.

 Parent elements:
 - background (Part 1, §17.2.1)
 - group (§14.1.2.7)
 - [done] object (Part 1, §17.3.3.19)
 - [done] pict (§9.2.2.2); pict (§9.5.1)

 Child elements:
 - anchorlock (Anchor Location Is Locked) §14.3.2.1
 - borderbottom (Bottom Border) §14.3.2.2
 - borderleft (Left Border) §14.3.2.3
 - borderright (Right Border) §14.3.2.4
 - bordertop (Top Border) §14.3.2.5
 - callout (Callout) §14.2.2.2
 - ClientData (Attached Object Data) §14.4.2.12
 - clippath (Shape Clipping Path) §14.2.2.3
 - equationxml (Storage for Alternate Math Content) §14.2.2.10
 - extrusion (3D Extrusion) §14.2.2.11
 - fill (Shape Fill Properties) §14.1.2.5
 - [done] formulas (Set of Formulas) §14.1.2.6
 - handles (Set of Handles) §14.1.2.9
 - [done] imagedata (Image Data) §14.1.2.11
 - ink (Ink) §14.2.2.15
 - iscomment (Ink Annotation Flag) §14.5.2.1
 - lock (Shape Protections) §14.2.2.18
 - path (Shape Path) §14.1.2.14
 - shadow (Shadow Effect) §14.1.2.18
 - signatureline (Digital Signature Line) §14.2.2.30
 - skew (Skew Transform) §14.2.2.31
 - [done] stroke (Line Stroke Settings) §14.1.2.21
 - [done] textbox (Text Box) §14.1.2.22
 - textdata (VML Diagram Text) §14.5.2.2
 - textpath (Text Layout Path) §14.1.2.23
 - [done] wrap (Text Wrapping) §14.3.2.6
*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_shape()
{
    READ_PROLOGUE
/*    e.g. <v:shape id="_x0000_i1025" type="#_x0000_t75" style="width:166.5pt;height:124.5pt" o:ole="">
             <v:imagedata r:id="rId7" o:title=""/>
           </v:shape>*/
    const QXmlStreamAttributes attrs(attributes());

    TRY_READ_ATTR_WITHOUT_NS(id)
    m_currentShapeId = id;

    // For some shapes, it seems that this is the correct id.
    TRY_READ_ATTR_WITH_NS(o, spid)
    if (!o_spid.isEmpty()) {
        m_currentShapeId = o_spid;
    }

    TRY_READ_ATTR_WITH_NS(o, connectortype)

    // CSS2 styling properties of the shape, http://www.w3.org/TR/REC-CSS2
    TRY_READ_ATTR_WITHOUT_NS(style)
    RETURN_IF_ERROR(parseCSS(style))
    kDebug() << "m_vmlStyle:" << m_vmlStyle;

    //! @todo position (can be relative...)
    TRY_READ_ATTR_WITHOUT_NS_INTO(alt, m_shapeAltText)
    TRY_READ_ATTR_WITHOUT_NS_INTO(title, m_shapeTitle)
    TRY_READ_ATTR_WITHOUT_NS(type)
    TRY_READ_ATTR_WITHOUT_NS(path)
    TRY_READ_ATTR_WITHOUT_NS(adj)
    TRY_READ_ATTR_WITHOUT_NS(coordsize)
    if (!path.isEmpty()) {
        m_shapeTypeString = "<draw:enhanced-geometry ";

        if (!adj.isEmpty()) {
            QString tempModifiers = adj;
            tempModifiers.replace(',', " ");
            m_shapeTypeString += QString("draw:modifiers=\"%1\" ").arg(tempModifiers);
        }
        if (!coordsize.isEmpty()) {
            QString tempViewBox = "0 0 " + coordsize;
            tempViewBox.replace(',', " ");
            m_shapeTypeString += QString("svg:viewBox=\"%1\" ").arg(tempViewBox);
        }
        m_shapeTypeString += QString("draw:enhanced-path=\"%1\" ").arg(convertToEnhancedPath(path));
        m_shapeTypeString += ">";
    }

    TRY_READ_ATTR_WITHOUT_NS(fillcolor)
    TRY_READ_ATTR_WITHOUT_NS(strokecolor)
    TRY_READ_ATTR_WITHOUT_NS(strokeweight)

    m_strokeWidth = 1 ; // This seems to be the default
    m_shapeColor.clear();
    m_strokeColor.clear();
    m_currentPen = QPen();

    if (!strokeweight.isEmpty()) {
        m_strokeWidth = strokeweight.left(strokeweight.length() - 2).toDouble(); // -2 removes 'pt'
        m_strokeColor = "#000000"; // Black color seems to be default
    }

    if (!fillcolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(filled)
        if (filled != "f" && filled != "false") {
            m_shapeColor = MSOOXML::Utils::rgbColor(fillcolor);
        }
    }

    if (!strokecolor.isEmpty()) {
        TRY_READ_ATTR_WITHOUT_NS(stroked)
        if (stroked != "f" && stroked != "false") {
            m_strokeColor = MSOOXML::Utils::rgbColor(strokecolor);
        }
    }

    MSOOXML::Utils::XmlWriteBuffer frameBuf;
    body = frameBuf.setWriter(body);

    pushCurrentDrawStyle(new KoGenStyle(KoGenStyle::GraphicAutoStyle, "graphic"));
    m_wrapRead = false;

    bool isCustomShape = true;

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            if (qualifiedName() == "v:imagedata") {
                isCustomShape = false;
                TRY_READ(imagedata)
            }
            else if (qualifiedName() == "v:textbox") {
                isCustomShape = false;
                TRY_READ(textbox)
            }
            ELSE_TRY_READ_IF(stroke)
            else if (qualifiedName() == "w10:wrap") {
                m_wrapRead = true;
                TRY_READ(wrap)
            }
            ELSE_TRY_READ_IF(formulas)
            SKIP_UNKNOWN
        }
    }

    // Case of a custom shape which is not defined in a shapetype
    if (!path.isEmpty()) {
        m_shapeTypeString += "</draw:enhanced-geometry>";
    }

    body = frameBuf.originalWriter();

    if (m_outputFrames) {
        if (o_connectortype.isEmpty()) {
            if (!isCustomShape) {
                createFrameStart();
            }
            else {
                createFrameStart(CustomStart);
            }
        }
        else {
            createFrameStart(StraightConnectorStart);
        }
    }

    (void)frameBuf.releaseWriter();

    if (m_outputFrames) {
        if (isCustomShape) {
            if (!type.isEmpty()) {
                type = type.mid(1); // removes extra # from the start
                body->addCompleteElement(m_shapeTypeStrings.value(type).toUtf8());
            }
            else {
                body->addCompleteElement(m_shapeTypeString.toUtf8());
            }
        }
        createFrameEnd();
    }

    popCurrentDrawStyle();

    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL imagedata
/*! imagedata handler (Image Data)
 ECMA-376 Part 4, 14.1.2.11, p.351.

 Parent elements:
 - arc (§14.1.2.1)
 - background (Part 1, §17.2.1)
 - curve (§14.1.2.3)
 - group (§14.1.2.7)
 - image (§14.1.2.10)
 - line (§14.1.2.12)
 - object (Part 1, §17.3.3.19)
 - oval (§14.1.2.13)
 - pict (§9.2.2.2)
 - pict (§9.5.1)
 - polyline (§14.1.2.15)
 - rect (§14.1.2.16)
 - roundrect (§14.1.2.17)
 - [done] shape (§14.1.2.19)
 - shapetype (§14.1.2.20)

 Child elements:
 - none

*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_imagedata()
{
    READ_PROLOGUE

/*    e.g. <v:imagedata r:id="rId7" o:title="..."/> */
    const QXmlStreamAttributes attrs(attributes());
    QString imagedata;
    TRY_READ_ATTR_WITH_NS(r, id)
    if (!r_id.isEmpty()) {
        imagedata = m_context->relationships->target(m_context->path, m_context->file, r_id);
    }
    else {
        TRY_READ_ATTR_WITH_NS(o, relid)
        if (!o_relid.isEmpty()) {
            imagedata = m_context->relationships->target(m_context->path, m_context->file, o_relid);
        }
    }

    kDebug() << "imagedata:" << imagedata;
    if (!imagedata.isEmpty()) {
//! @todo ooo saves binaries to the root dir; should we?
        m_context->import->imageSize(imagedata, m_imageSize);

        m_imagedataPath = QLatin1String("Pictures/") + imagedata.mid(imagedata.lastIndexOf('/') + 1);;
        RETURN_IF_ERROR( m_context->import->copyFile(imagedata, m_imagedataPath, false ) )
        addManifestEntryForFile(m_imagedataPath);
        m_imagedataFile = imagedata;
        addManifestEntryForPicturesDir();
    }

    readNext();
    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL textbox
/*! text box handler (Text Box)

 Parent elements:
 - [done] shape (§14.1.2.19)
 - more...

*/
//! @todo support all elements
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_textbox()
{
    READ_PROLOGUE

    const QXmlStreamAttributes attrs(attributes());

    body->startElement("draw:text-box");

    while (!atEnd()) {
        readNext();
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
#ifdef DOCXXMLDOCREADER_CPP
            TRY_READ_IF_NS(w, txbxContent)
#endif
        }
    }

    body->endElement(); // draw-textbox

    READ_EPILOGUE
}

#undef MSOOXML_CURRENT_NS
#define MSOOXML_CURRENT_NS "w10"

#undef CURRENT_EL
#define CURRENT_EL wrap
// Wrap handler, todo fully..
KoFilter::ConversionStatus MSOOXML_CURRENT_CLASS::read_wrap()
{
    READ_PROLOGUE

    const QXmlStreamAttributes attrs(attributes());

    TRY_READ_ATTR_WITHOUT_NS(type)
    TRY_READ_ATTR_WITHOUT_NS(side)

    if (type.isEmpty()) {
        if (m_vmlStyle.contains("z-index")) {
            m_currentDrawStyle->addProperty("style:wrap", "run-through");
            if (m_vmlStyle.value("z-index").toInt() > 0) {
                m_currentDrawStyle->addProperty("style:run-through", "foreground");
            }
            else {
                m_currentDrawStyle->addProperty("style:run-through", "background");
            }
        }
        else {
            m_currentDrawStyle->addProperty("style:wrap", "run-through");
            m_currentDrawStyle->addProperty("style:run-through", "foreground");
        }
    }
    else if (type == "through") {
        m_currentDrawStyle->addProperty("style:wrap", "run-through");
    }
    else if (type == "topAndBottom") {
        m_currentDrawStyle->addProperty("style:wrap", "none");
    }
    else {
        if (side.isEmpty()) { // Note doc doesn't say which one is default
            m_currentDrawStyle->addProperty("style:wrap", "biggest");
        }
        else if (side == "left") {
            m_currentDrawStyle->addProperty("style:wrap", "left");
        }
        else if (side == "largest") {
            m_currentDrawStyle->addProperty("style:wrap", "biggest");
        }
        else if (side == "right") {
            m_currentDrawStyle->addProperty("style:wrap", "right");
        }
        else if (side == "both") {
            m_currentDrawStyle->addProperty("style:wrap", "parallel");
        }
    }

    TRY_READ_ATTR_WITHOUT_NS(anchorx)
    TRY_READ_ATTR_WITHOUT_NS(anchory)

    // Documentation says default to be 'page', however because these are always in a paragraph
    // in a text run, a better default for odf purposes seems to be 'paragraph'

    if (anchory == "page") {
        m_currentDrawStyle->addProperty("style:vertical-rel", "page");
        m_anchorType = "page";
    }
    else if (anchory == "text") {
        m_anchorType = "as-char";
        m_currentDrawStyle->addProperty("style:vertical-rel", "text");
    }
    else if (anchory == "line") {
        m_anchorType = "as-char";
        m_currentDrawStyle->addProperty("style:vertical-rel", "line");
    }
    else { // margin
        m_anchorType = "paragraph";
        m_currentDrawStyle->addProperty("style:vertical-rel", "paragraph");
    }

    if (anchorx == "page") {
        m_currentDrawStyle->addProperty("style:horizontal-rel", "page");
    }
    else if (anchorx == "margin") {
        m_currentDrawStyle->addProperty("style:horizontal-rel", "page-start-margin");
    }
    else if (anchorx == "text") {
        // Empty, horizontal-rel cannot be anything
    }
    else {
        m_currentDrawStyle->addProperty("style:horizontal-rel", "paragraph");
    }

    readNext();
    READ_EPILOGUE
}

#endif // MSOOXMLVMLREADER_IMPL_H
