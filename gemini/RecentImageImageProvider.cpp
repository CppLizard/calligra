/* This file is part of the KDE project
 * Copyright (C) 2012 Boudewijn Rempt <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "RecentImageImageProvider.h"
#include <QFile>
#include <QImage>
#include <QImageReader>

#include <KoStore.h>
#include <KoDocument.h>
#include <KMimeTypeTrader>
#include <KMimeType>
#include <kio/previewjob.h>

RecentImageImageProvider::RecentImageImageProvider()
    : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

QImage RecentImageImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    int width = 128;
    int height = 128;
    if(id.toLower().endsWith("odt") || id.toLower().endsWith("doc") || id.toLower().endsWith("docx"))
        width *= 0.72413793;
    else
        height *= 0.72413793;

    if (size) {
        *size = QSize(width, height);
    }

    QSize sz(requestedSize.width() > 0 ? requestedSize.width() : width,
             requestedSize.height() > 0 ? requestedSize.height() : height);

    QFile f(id);
    QImage thumbnail(sz, QImage::Format_ARGB32_Premultiplied);
    thumbnail.fill(Qt::white);

    if (f.exists()) {
        // try to use any embedded thumbnail
        KoStore *store = KoStore::createStore(id, KoStore::Read);

        if (store &&
                (store->open(QLatin1String("Thumbnails/thumbnail.png")) ||
                    store->open(QLatin1String("preview.png")))) {
            // Hooray! No long delay for the user...
            const QByteArray thumbnailData = store->read(store->size());

            if (thumbnail.loadFromData(thumbnailData) &&
                    (thumbnail.width() >= width || thumbnail.height() >= height)) {
                thumbnail = thumbnail.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
        delete store;
    }
    return thumbnail;
}
