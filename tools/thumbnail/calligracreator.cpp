/*  This file is part of the KDE libraries
    Copyright (C) 2002 Simon MacMullen <calligra@babysimon.co.uk>

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
#include "calligracreator.h"
#include <time.h>

#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QTimerEvent>

#include <kapplication.h>
#include <kfileitem.h>
#include <klibloader.h>
#include <kparts/part.h>
#include <kparts/componentfactory.h>

#include <KoPart.h>
#include <KoStore.h>
#include <KoDocument.h>
#include <QAbstractEventDispatcher>
#include <kmimetype.h>
#include <kdemacros.h>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new CalligraCreator;
    }
}

CalligraCreator::CalligraCreator()
    : m_part(0)
    , m_doc(0)
{
}

CalligraCreator::~CalligraCreator()
{
    delete m_part;
    delete m_doc;
}

bool CalligraCreator::create(const QString &path, int width, int height, QImage &img)
{
    KoStore* store = KoStore::createStore(path, KoStore::Read);

    if ( store && ( store->open( QString("Thumbnails/thumbnail.png") ) || store->open( QString("preview.png") ) ) )
    {
        // Hooray! No long delay for the user...
        QByteArray bytes = store->read(store->size());
        store->close();
        delete store;
        return img.loadFromData(bytes);
    }
    delete store;

    QString mimetype = KMimeType::findByPath( path )->name();

    m_part = KMimeTypeTrader::self()->createPartInstanceFromQuery<KoPart>( mimetype );

    if (!m_part) return false;

    m_doc = m_part->document();

    connect(m_part, SIGNAL(completed()), SLOT(slotCompleted()));

    KUrl url;
    url.setPath( path );
    m_doc->setCheckAutoSaveFile( false );
    m_doc->setAutoErrorHandlingEnabled( false ); // don't show message boxes
    if ( !m_doc->openUrl( url ) )
        return false;
    m_completed = false;
    startTimer(5000);
//     while (!m_completed)
//         kapp->processOneEvent();
    QAbstractEventDispatcher::instance()->unregisterTimers(this);

    // render the page on a bigger pixmap and use smoothScale,
    // looks better than directly scaling with the QPainter (malte)
    QPixmap pix;
    if (width > 400)
    {
        pix = m_doc->generatePreview(QSize(width, height));
    }
    else
    {
        pix = m_doc->generatePreview(QSize(400, 400));
    }

    img = pix.toImage();
    return true;
}

void CalligraCreator::timerEvent(QTimerEvent *)
{
    m_part->closeUrl();
    m_completed = true;
}

void CalligraCreator::slotCompleted()
{
    m_completed = true;
}

ThumbCreator::Flags CalligraCreator::flags() const
{
    if (m_doc && (m_doc->mimeType() == "image/openraster" || m_doc->mimeType() == "application/x-krita")) {
        return (Flags)(DrawFrame);
    }
    else {
        return (Flags)(DrawFrame | BlendIcon);
    }
}

#include "calligracreator.moc"

