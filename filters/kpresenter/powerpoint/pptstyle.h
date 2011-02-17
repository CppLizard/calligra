/* This file is part of the KDE project
   Copyright (C) 2010 KO GmbH <jos.van.den.oever@kogmbh.com>
   Copyright (C) 2010, 2011 Matus Uzak <matus.uzak@ixonos.com>

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

#ifndef PPTSTYLE_H
#define PPTSTYLE_H

#include "generated/simpleParser.h"

enum TextTypeEnum {
    Tx_TYPE_TITLE = 0,
    Tx_TYPE_BODY,
    Tx_TYPE_NOTES,
    Tx_TYPE_OTHER = 4,
    Tx_TYPE_CENTERBODY,
    Tx_TYPE_CENTERTITLE,
    Tx_TYPE_HALFBODY,
    Tx_TYPE_QUARTERBODY
};

class PptTextPFRun {
public:

    // NOTE: Deprecated !!!
    //
    // FIXME: required by ListStyleInput but it doesn't make much sense.
    explicit PptTextPFRun();

    /**
     * Construct TextPFException and TextPFException9 hierarchy.
     * @param d DocumentContainer address
     * @param mh MasterOrSlideContainer hierarchy
     * @param texts together with @param pcd provides access to StyleTextProp9Atom
     * @param pcd provides access to additional text formatting in StyleTextProp9Atom
     * @param tc provides access to text formatting in MasterTextPropAtom
     * @param tr specifies tabbing, margins, and indentation for text
     * @param start specifies begging of the paragraph in the slide text
     */
    explicit PptTextPFRun(const MSO::DocumentContainer* d,
                          QList<const MSO::MasterOrSlideContainer*> &mh,
                          const MSO::SlideListWithTextSubContainerOrAtom* texts = 0,
                          const MSO::PptOfficeArtClientData* pcd = 0,
                          const MSO::TextContainer* tc = 0,
                          const MSO::TextRuler* tr = 0,
                          quint32 start = 0);

    quint32 count() const { return m_count; }
    quint16 level() const { return m_level; }
    bool isList() const;

    // TextPFException
    bool fHasBullet() const;
    bool fBulletHasFont() const;
    bool fBulletHasColor() const;
    bool fBulletHasSize() const;
    qint16 bulletChar() const;
    quint16 bulletFontRef() const;
    qint16 bulletSize() const;
    MSO::ColorIndexStruct bulletColor() const;
    quint16 textAlignment() const;
    qint16 lineSpacing() const;
    qint16 spaceBefore() const;
    qint16 spaceAfter() const;
    quint16 leftMargin() const;
    quint16 indent() const;
    quint16 defaultTabSize() const;
    MSO::TabStops tabStops() const;
    quint16 fontAlign() const;
    bool charWrap() const;
    bool wordWrap() const;
    bool overflow() const;
    quint16 textDirection() const;

    // TextPFException9
    qint32 bulletBlipRef() const;
    qint16 fBulletHasAutoNumber() const;
    qint16 startNum() const;

    /**
     * Check the scheme field of the TextAutoNumberScheme structure.
     *
     * Specifies the automatic numbering scheme for text paragraphs.  The
     * default value is ANM_ArabicPeriod, tested and discussed on the Office
     * File Format Forum.
     *
     * @return the scheme describing the style of the number bullets
     * corresponding to the TextAutoNumberSchemeEnum or the default value if
     * TextAutoNumberScheme is not provided.
     */
    quint16 scheme() const;

private:
    //the indentation level of the paragraph
    quint16 m_level;

    //number of characters of the corresponding text to which this paragraph
    //formatting applies
    quint32 m_count;

    //default values of bullet properties (text type specific)
    bool m_fHasBullet;

    //ident/margin for text with indentLevel == m_level (provided by TextRuler)
    QList<qint16> m_indent;
    QList<qint16> m_leftMargin;

    QList<const MSO::TextPFException*> pfs;
    const MSO::TextPFException9* pf9s[6];
};

class PptTextCFRun {
public:
    /**
     * Construct TextCFException hierarchy.
     * @param d DocumentContainer
     * @param mh MasterOrSlideContainer hierarchy
     * @param tc TextContainer
     * @param level specifies the indentation level of the paragraph
     */
    PptTextCFRun(const MSO::DocumentContainer* d,
                 QList<const MSO::MasterOrSlideContainer*> &mh,
                 const MSO::TextContainer* tc,
                 quint16 level);

    // NOTE: Deprecated !!!
    /**
     * Only default TextCFExceptions are required.
     * @param DocumentContainer
     */
    PptTextCFRun(const MSO::DocumentContainer* d);

    /**
     * Add the TextCFException structure present in the current TextContainer,
     * which specifies character-level style and formatting, font information,
     * coloring and positioninghe for the given run of text.
     *
     * @return the number of characters of the corresponding text to which this
     * character formatting applies
     */
    int addCurrentCFRun(const MSO::TextContainer* tc, quint32 start);

    bool bold() const;
    bool italic() const;
    bool underline() const;
    bool shadow() const;
    bool fehint() const;
    bool kumi() const;
    bool emboss() const;
    quint8 pp9rt() const;
    quint16 fontRef() const;
    quint16 oldEAFontRef() const;
    quint16 ansiFontRef() const;
    quint16 symbolFontRef() const;
    quint16 fontSize() const;
    MSO::ColorIndexStruct color() const;
    qint16 position() const;

private:
    QList<const MSO::TextCFException*> cfs;

    //the indentation level of the corresponding paragraph
    quint16 m_level;

    //each run of text has a separate character-level formatting provided by
    //TextCFRun which is added/replaced on top of the cfs list
    bool m_cfrun_rm;
};


template<class T>
const T*
getPP(const MSO::DocumentContainer* dc)
{
    if (dc == 0 || dc->docInfoList == 0) return 0;
    foreach (const MSO::DocInfoListSubContainerOrAtom& a,
                 dc->docInfoList->rgChildRec) {
        const MSO::DocProgTagsContainer* d
                = a.anon.get<MSO::DocProgTagsContainer>();
        if (d) {
            foreach (const MSO::DocProgTagsSubContainerOrAtom& da,
                     d->rgChildRec) {
                const MSO::DocProgBinaryTagContainer* c
                        = da.anon.get<MSO::DocProgBinaryTagContainer>();
                if (c) {
                    const T* t = c->rec.anon.get<T>();
                    if (t) return t;
                }
            }
        }
    }
    return 0;
}

template<class T>
const T*
getPP(const MSO::PptOfficeArtClientData& o)
{
    foreach (const MSO::ShapeClientRoundtripDataSubcontainerOrAtom& s,
             o.rgShapeClientRoundtripData) {
        const MSO::ShapeProgsTagContainer* p
                = s.anon.get<MSO::ShapeProgsTagContainer>();
        if (p) {
            foreach (const MSO::ShapeProgTagsSubContainerOrAtom& s,
                     p->rgChildRec) {
                const MSO::ShapeProgBinaryTagContainer* a
                        = s.anon.get<MSO::ShapeProgBinaryTagContainer>();
                if (a) {
                    const T* pp = a->rec.anon.get<T>();
                    if (pp) {
                        return pp;
                    }
                }
            }
        }
    }
    foreach (const MSO::ShapeClientRoundtripDataSubcontainerOrAtom& s,
             o.rgShapeClientRoundtripData0) {
        const MSO::ShapeProgsTagContainer* p
                = s.anon.get<MSO::ShapeProgsTagContainer>();
        if (p) {
            foreach (const MSO::ShapeProgTagsSubContainerOrAtom& s,
                     p->rgChildRec) {
                const MSO::ShapeProgBinaryTagContainer* a
                        = s.anon.get<MSO::ShapeProgBinaryTagContainer>();
                if (a) {
                    const T* pp = a->rec.anon.get<T>();
                    if (pp) {
                        return pp;
                    }
                }
            }
        }
    }
    return 0;
}

template<class T, class C>
const T*
getPP(const C* c)
{
    if (!c) return 0;
    const MSO::SlideProgTagsContainer* sc = c->slideProgTagsContainer.data();
    if (!sc) return 0;
    foreach (const MSO::SlideProgTagsSubContainerOrAtom& a , sc->rgTypeRec) {
        const MSO::SlideProgBinaryTagContainer* bt
                = a.anon.get<MSO::SlideProgBinaryTagContainer>();
        if (bt) {
            const T* t = bt->rec.anon.get<T>();
            if (t) return t;
        }
    }
    return 0;
}
template<class T>
const T*
getPP(const MSO::MasterOrSlideContainer* m) {
    if (!m) return 0;
    const MSO::MainMasterContainer* mm = m->anon.get<MSO::MainMasterContainer>();
    if (mm) return getPP<T>(mm);
    return getPP<T>(m->anon.get<MSO::SlideContainer>());
}

#endif //PPTSTYLE_H
