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
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

DESCRIPTION

    This file implements a simplified abstraction of Microsoft Word documents.
*/

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <kdebug.h>
#include <msword.h>
#include <properties.h>
#include <qlist.h>

class Document: private MsWord
{
protected:

    // Construction. Invoke with the OLE streams that comprise the Word document.

    Document(
        const U8 *mainStream,
        const U8 *table0Stream,
        const U8 *table1Stream,
        const U8 *dataStream);
    virtual ~Document();

    // Metadata.

    QString getTitle(void) const;
    QString getSubject(void) const;
    QString getAuthor(void) const;
    QString getLastRevisedBy(void) const;

    // Call the parse() function to process the document. The callbacks return
    // the text along with any relevant attributes.

    void parse();

    // We describe the data we return to a client in terms of a "run" of
    // character data along with some properties. The properties may be
    // formatting information or, in the case of embedded objects, data
    // associated with the object. Thus, we have a base class called Run
    // and a set of derivations for each specialisation...

    class Run
    {
    public:
        U32 start;
        U32 end;

        // We need at least one virtual function to enable RTTI!

        virtual ~Run() {};
    };

    // Specialisation for text formatting.

    class Format: public Run
    {
    public:
        Properties *values;
    };

    // Specialisation for embedded images/objects/etc.

    class Image: public Run
    {
    public:
        unsigned id;
        QString type;
        unsigned length;
        const char *data;
    };

    typedef struct
    {
        PAP baseStyle;
        QList<Run> runs;
    } Attributes;

    virtual void gotError(
        const QString &text) = 0;
    virtual void gotParagraph(
        const QString &text,
        Attributes &style) = 0;
    virtual void gotHeadingParagraph(
        const QString &text,
        Attributes &style) = 0;
    virtual void gotListParagraph(
        const QString &text,
        Attributes &style) = 0;
    virtual void gotTableBegin(
        unsigned tableNumber) = 0;
    virtual void gotTableEnd(
        unsigned tableNumber) = 0;
    virtual void gotTableRow(
        unsigned tableNumber,
        unsigned rowNumber,
        const QString texts[],
        const PAP styles[],
        TAP &row) = 0;

private:

    // Error handling and reporting support.

    static const int s_area = 30513;

    // Parse context.

    unsigned m_tableNumber;
    unsigned m_tableRowNumber;
    unsigned m_characterPosition;
    unsigned m_imageNumber;

    // Character property handling.

    void createAttributes(
        const QString &text,
        const PAP &style,
        const CHPXarray &chpxs,
        Attributes &style);

    // Override the base class functions.

    void gotParagraph(
        const QString &text,
        const PAP &pap,
        const CHPXarray &chpxs);
    void gotHeadingParagraph(
        const QString &text,
        const PAP &pap,
        const CHPXarray &chpxs);
    void gotListParagraph(
        const QString &text,
        const PAP &pap,
        const CHPXarray &chpxs);
    void gotTableBegin();
    void gotTableEnd();
    void gotTableRow(
        const QString texts[],
        const PAP styles[],
        TAP &row);
};
#endif
