/*
 *  Copyright (c) 2013 Somsubhra Bairi <somsubhra.bairi@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License, or(at you option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_kranim_saver.h"
#include "kis_kranim_tags.h"
#include "kis_animation_doc.h"
#include "kis_animation_part.h"
#include "kis_animation.h"
#include "kis_layer.h"
#include "kis_store_paintdevice_writer.h"

#include <KoStore.h>

using namespace KRANIM;

struct KisKranimSaver::Private
{
    KisAnimation *animation;
    KisAnimationDoc *doc;
    KoStore *store;
};

KisKranimSaver::KisKranimSaver(KisAnimationDoc *document)
    : m_d(new Private)
{
    m_d->doc = document;
    m_d->animation = dynamic_cast<KisAnimationPart*>(document->documentPart())->animation();
}

KisKranimSaver::~KisKranimSaver()
{
    delete m_d;
}

QDomElement KisKranimSaver::saveXML(QDomDocument &doc)
{
    QDomElement layersElement = doc.createElement("layers");
    QDomElement layer = doc.createElement("layer");
    layersElement.appendChild(layer);
    return layersElement;
}

QDomElement KisKranimSaver::saveMetaData(QDomDocument &doc, QDomNode root)
{
    QDomElement metaDataElement = doc.createElement("metadata");
    metaDataElement.setAttribute(MIME, NATIVE_MIMETYPE);
    metaDataElement.setAttribute(NAME, m_d->animation->name());
    metaDataElement.setAttribute(AUTHOR, m_d->animation->author());
    metaDataElement.setAttribute(FPS, m_d->animation->fps());
    metaDataElement.setAttribute(TIME, m_d->animation->time());
    metaDataElement.setAttribute(HEIGHT, m_d->animation->height());
    metaDataElement.setAttribute(WIDTH, m_d->animation->width());
    metaDataElement.setAttribute(RESOLUTION, m_d->animation->resolution());
    metaDataElement.setAttribute(DESCRIPTION, m_d->animation->description());
    root.appendChild(metaDataElement);
    return metaDataElement;
}

void KisKranimSaver::saveFrame(KisAnimationStore *store, KisLayerSP frame, const QRect &framePosition)
{
    if(frame) {
        this->saveFrame(store, frame->paintDevice(), framePosition);
    }
}

void KisKranimSaver::saveFrame(KisAnimationStore *store, KisPaintDeviceSP device, const QRect &framePosition)
{
    QString location = "frame" + QString::number(framePosition.x()) + "layer" + QString::number(framePosition.y());

    store->openFileWriting(location);
    m_writer = new KisAnimationStoreWriter(store);
    device->write(*m_writer);
    store->closeFile();

    store->openFileWriting(location + ".defaultpixel");
    store->writeDataToFile((char*)device->defaultPixel(), device->colorSpace()->pixelSize());
    store->closeFile();
}

void KisKranimSaver::deleteFrame(KisAnimationStore *store, int frame, int layer)
{
    QString location = "frame" + QString::number(frame) + "layer" + QString::number(layer);

    store->deleteFile(location);
    store->deleteFile(location + ".defaultpixel");
}

void KisKranimSaver::renameFrame(KisAnimationStore *store, int oldFrame, int oldLayer, int newFrame, int newLayer)
{
    QString oldLocation = "frame" + QString::number(oldFrame) + "layer" + QString::number(oldLayer);
    QString newLocation = "frame" + QString::number(newFrame) + "layer" + QString::number(newLayer);

    qDebug() << oldLocation << "->" << newLocation;

    store->renameFile(oldLocation, newLocation);
    store->renameFile(oldLocation + ".defaultpixel", newLocation + ".defaultpixel");
}
