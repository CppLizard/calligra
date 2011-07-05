/* This file is part of the KDE project
 * Copyright (C) 2011 Casper Boemann <cbo@kogmbh.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#ifndef TOCDOCUMENTLAYOUT_H
#define TOCDOCUMENTLAYOUT_H

#include "textlayout_export.h"

#include <QAbstractTextDocumentLayout>


/**
 * Dummy TextLayouter that does nothing really, but without it the Table of Contents
 * can not be layout.TextLayouter
 * The real layout of the ToC still happens by the KoTextLayoutArea as part of
 * KoTextDocumentLayout of the main document
 *
 * You really sholdn't add anything to this class
 */

class TEXTLAYOUT_EXPORT ToCDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_OBJECT
public:
    /// constructor
    explicit ToCDocumentLayout(QTextDocument *doc);
    virtual ~ToCDocumentLayout();

    /// Returns the bounding rectangle of block.
    virtual QRectF blockBoundingRect(const QTextBlock & block) const;
    /**
     * Returns the total size of the document. This is useful to display
     * widgets since they can use to information to update their scroll bars
     * correctly
     */
    virtual QSizeF documentSize() const;

    /// Draws the layout on the given painter with the given context.
    virtual void draw(QPainter * painter, const QAbstractTextDocumentLayout::PaintContext & context);

    virtual QRectF frameBoundingRect(QTextFrame*) const;

    /// reimplemented DO NOT CALL - USE HITTEST IN THE ROOTAREAS INSTEAD
    virtual int hitTest(const QPointF & point, Qt::HitTestAccuracy accuracy) const;

    /// reimplemented to always return 1
    virtual int pageCount() const;

    /// reimplemented from QAbstractTextDocumentLayout
    virtual void documentChanged(int position, int charsRemoved, int charsAdded);
/*
protected:
    /// reimplemented
    virtual void drawInlineObject(QPainter *painter, const QRectF &rect, QTextInlineObject object, int position, const QTextFormat &format);
    /// reimplemented
    virtual void positionInlineObject(QTextInlineObject item, int position, const QTextFormat &format);
    /// reimplemented
    virtual void resizeInlineObject(QTextInlineObject item, int position, const QTextFormat &format);
*/
};

#endif
