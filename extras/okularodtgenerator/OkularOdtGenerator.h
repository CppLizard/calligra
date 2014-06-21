/* This file is part of the KDE project
   Copyright (C) 2012 Sven Langkamp <sven.langkamp@gmail.com>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef OKULARODTGENERATOR_H
#define OKULARODTGENERATOR_H

#include <okular/core/generator.h>
#include <okular/core/document.h>

class KWDocument;

class OkularOdtGenerator : public Okular::Generator
{
public:
    OkularOdtGenerator( QObject *parent, const QVariantList &args );
    ~OkularOdtGenerator();

    bool loadDocument( const QString &fileName, QVector<Okular::Page*> &pages );

    bool canGeneratePixmap() const;
    void generatePixmap( Okular::PixmapRequest *request );

    const Okular::DocumentInfo* generateDocumentInfo();

protected:
    bool doCloseDocument();

private:
    KWDocument* m_doc;

    Okular::DocumentInfo m_documentInfo;
};

#endif
