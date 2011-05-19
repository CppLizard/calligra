/* This file is part of the KDE project
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
 * Copyright (C) 2007 Sebastian Sauer <mail@dipe.org>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2009-2011 KO GmbH <cbo@kogmbh.com>
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

#ifndef KOSTYLETHUMBNAILER_H
#define KOSTYLETHUMBNAILER_H

#include "textlayout_export.h"

#include <QPixmap>

class QTextDocument;
class KoCharacterStyle;
class KoParagraphStyle;

/**
 * Helper class to create (and cache) thumbnails of styles
 */
class TEXTLAYOUT_EXPORT KoStyleThumbnailer
{
public:
    /**
     * Create a new style thumbnailer.
     */
    explicit KoStyleThumbnailer();

    /**
     * Destructor.
     */
    virtual ~KoStyleThumbnailer();

    /**
     * Set a text document that is capable of drawing the styles
     * The method only has any effect first time, and the document is just to generate
     ' preview pixmaps
     */
    void setPixmapHelperDocument(QTextDocument *pixmapHelperDocument);

    /**
     * Return a thumbnail representing the style
     */
    QPixmap thumbnail(KoParagraphStyle *style);

    /**
     * Return a thumbnail representing the style
     */
    QPixmap thumbnail(KoCharacterStyle *style);

private:
    class Private;
    Private* const d;
};

#endif
