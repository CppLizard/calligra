/*
    Copyright (C) 2000, S.R.Haque <shaheedhaque@hotmail.com>.
    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aU32 with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

DESCRIPTION

    This is a generic parser for Microsoft Office Drawings (MSODs). The
    specification for this is the Microsoft Office 97 Drawing File Format
    published in MSDN. The output is a series of callbacks (a.k.a. virtual
    functions) which the caller can override as required.
*/

#ifndef MSOD_H
#define MSOD_H

class QString;
class QPointArray;
#include <kwmf.h>
#include <qvector.h>

class Msod :
    private KWmf
{
public:

    // Construction.

    Msod(
        unsigned dpi);
    virtual ~Msod();

    // Called to parse the given file. We extract a drawing by shapeId.
    // If the drawing is not found, the return value will be false.

    bool parse(
        unsigned shapeId,
        const QString &file,
        const char *delayStream = 0L);
    bool parse(
        unsigned shapeId,
        QDataStream &stream,
        unsigned size,
        const char *delayStream = 0L);

    typedef KWmf::DrawContext DrawContext;

    // Should be protected...

    void brushSet(
        unsigned colour,
        unsigned style);
    void penSet(
        unsigned colour,
        unsigned style,
        unsigned width);

protected:
    // Override to get results of parsing.

    virtual void gotEllipse(
        const DrawContext &dc,
        QString type,
        QPoint topLeft,
        QSize halfAxes,
        unsigned startAngle,
        unsigned stopAngle) = 0;
    virtual void gotPicture(
        unsigned id,
        QString extension,
        unsigned length,
        const char *data) = 0;
    virtual void gotPolygon(
        const DrawContext &dc,
        const QPointArray &points) = 0;
    virtual void gotPolyline(
        const DrawContext &dc,
        const QPointArray &points) = 0;
    virtual void gotRectangle(
        const DrawContext &dc,
        const QPointArray &points) = 0;

private:
    Msod(const Msod &);
    const Msod &operator=(const Msod &);

    // Debug support.

    static const int s_area = 30505;

    // Use unambiguous names for Microsoft types.

    typedef unsigned char U8;
    typedef unsigned short U16;
    typedef unsigned int U32;

    int m_dpi;
    DrawContext m_dc;
    unsigned m_dggError;
    unsigned m_requestedShapeId;
    bool m_isRequiredDrawing;
    const char *m_delayStream;

    QPoint normalisePoint(
        QDataStream &operands);
    QSize normaliseSize(
        QDataStream &operands);

    // Common Header (MSOBFH)

    typedef struct
    {
        union
        {
            U32 info;
            struct
            {
                U32 ver: 4;
                U32 inst: 12;
                U32 fbt: 16;
            } fields;
        } opcode;
        U32 cbLength;
    } Header;

    typedef enum
    {
        msoblipERROR,               // An error occured during loading.
        msoblipUNKNOWN,             // An unknown blip type.
        msoblipEMF,                 // Windows Enhanced Metafile.
        msoblipWMF,                 // Windows Metafile.
        msoblipPICT,                // Macintosh PICT.
        msoblipJPEG,                // JFIF.
        msoblipPNG,                 // PNG.
        msoblipDIB,                 // Windows DIB.
        msoblipFirstClient = 32,    // First client defined blip type.
        msoblipLastClient  = 255    // Last client defined blip type.
    } MSOBLIPTYPE;

    MSOBLIPTYPE m_blipType;
    unsigned m_imageId;
    class Image
    {
    public:
        QString extension;
        unsigned length;
        const char *data;
        Image() { data = 0L; }
        ~Image() { delete [] data; }
    };
    QVector<Image> m_images;

    // Opcode handling and painter methods.

    void walk(
        U32 bytes,
        QDataStream &operands);
    void skip(
        U32 bytes,
        QDataStream &operands);
    void invokeHandler(
        Header &op,
        U32 bytes,
        QDataStream &operands);

    void opAlignrule(Header &op, U32 bytes, QDataStream &operands);
    void opAnchor(Header &op, U32 bytes, QDataStream &operands);
    void opArcrule(Header &op, U32 bytes, QDataStream &operands);
    void opBlip(Header &op, U32 bytes, QDataStream &operands);
    void opBse(Header &op, U32 bytes, QDataStream &operands);
    void opBstorecontainer(Header &op, U32 bytes, QDataStream &operands);
    void opCalloutrule(Header &op, U32 bytes, QDataStream &operands);
    void opChildanchor(Header &op, U32 bytes, QDataStream &operands);
    void opClientanchor(Header &op, U32 bytes, QDataStream &operands);
    void opClientdata(Header &op, U32 bytes, QDataStream &operands);
    void opClientrule(Header &op, U32 bytes, QDataStream &operands);
    void opClienttextbox(Header &op, U32 bytes, QDataStream &operands);
    void opClsid(Header &op, U32 bytes, QDataStream &operands);
    void opColormru(Header &op, U32 bytes, QDataStream &operands);
    void opConnectorrule(Header &op, U32 bytes, QDataStream &operands);
    void opDeletedpspl(Header &op, U32 bytes, QDataStream &operands);
    void opDg(Header &op, U32 bytes, QDataStream &operands);
    void opDgcontainer(Header &op, U32 bytes, QDataStream &operands);
    void opDgg(Header &op, U32 bytes, QDataStream &operands);
    void opDggcontainer(Header &op, U32 bytes, QDataStream &operands);
    void opOleobject(Header &op, U32 bytes, QDataStream &operands);
    void opOpt(Header &op, U32 bytes, QDataStream &operands);
    void opRegroupitems(Header &op, U32 bytes, QDataStream &operands);
    void opSelection(Header &op, U32 bytes, QDataStream &operands);
    void opSolvercontainer(Header &op, U32 bytes, QDataStream &operands);
    void opSp(Header &op, U32 bytes, QDataStream &operands);
    void opSpcontainer(Header &op, U32 bytes, QDataStream &operands);
    void opSpgr(Header &op, U32 bytes, QDataStream &operands);
    void opSpgrcontainer(Header &op, U32 bytes, QDataStream &operands);
    void opSplitmenucolors(Header &op, U32 bytes, QDataStream &operands);
    void opTextbox(Header &op, U32 bytes, QDataStream &operands);                                                               // do nothing

    void shpLine(Header &op, U32 bytes, QDataStream &operands);                                                               // do nothing
    void shpPictureFrame(Header &op, U32 bytes, QDataStream &operands);                                                               // do nothing
    void shpRectangle(Header &op, U32 bytes, QDataStream &operands);                                                               // do nothing

    // Option handling.

    typedef struct
    {
        union
        {
            U16 info;
            struct
            {
                U16 pid: 14;
                U16 fBid: 1;
                U16 fComplex: 1;
            } fields;
        } opcode;
        U32 value;
    } Option;

    double from1616ToDouble(U32 value);
};

#endif
