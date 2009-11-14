/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_xcf_import.h"

#include <ctype.h>

#include <QApplication>
#include <QFile>

#include <KGenericFactory>

#include <kio/netaccess.h>
#include <kio/deletejob.h>

#include <KoColorSpaceRegistry.h>
#include <KoFilterChain.h>

#include <kis_debug.h>
#include <kis_doc2.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_iterator.h>
#include <KoColorSpaceTraits.h>
#include <kis_paint_device.h>
#include <KoColorSpace.h>
#include <qendian.h>
#include "xcftools.h"
#include <KoCompositeOp.h>

typedef KGenericFactory<KisXCFImport> XCFImportFactory;
K_EXPORT_COMPONENT_FACTORY(libkritaxcfimport, XCFImportFactory("kofficefilters"))

KisXCFImport::KisXCFImport(QObject* parent, const QStringList&) : KoFilter(parent)
{
}

KisXCFImport::~KisXCFImport()
{
}

KoFilter::ConversionStatus KisXCFImport::convert(const QByteArray& from, const QByteArray& to)
{
    dbgFile << "Importing using XCFImport!";

    if (to != "application/x-krita")
        return KoFilter::BadMimeType;

    KisDoc2 * doc = dynamic_cast<KisDoc2*>(m_chain -> outputDocument());

    if (!doc)
        return KoFilter::CreationError;

    QString filename = m_chain -> inputFile();

    if (filename.isEmpty()) {
        return KoFilter::FileNotFound;
    }

    KUrl url;
    url.setPath(filename);


    dbgFile << "Import: " << url;
    if (url.isEmpty())
        return KoFilter::FileNotFound;

    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, qApp -> activeWindow())) {
        dbgFile << "Inexistant file";
        return KoFilter::FileNotFound;
    }

    // We're not set up to handle asynchronous loading at the moment.
    QString tmpFile;
    KoFilter::ConversionStatus result;
    if (KIO::NetAccess::download(url, tmpFile, QApplication::activeWindow())) {
        KUrl uriTF;
        uriTF.setPath(tmpFile);

        // open the file
        QFile *fp = new QFile(uriTF.path());
        if (fp->exists()) {
            doc->prepareForImport();
            result = loadFromDevice(fp, doc);
        } else {
            result = KoFilter::CreationError;
        }

        KIO::NetAccess::removeTempFile(tmpFile);
        return result;
    }
    dbgFile << "Download failed";
    return KoFilter::CreationError;
}

QString layerModeG2K(GimpLayerModeEffects mode)
{
    switch (mode) {
    case GIMP_NORMAL_MODE:
        return COMPOSITE_OVER;
    case GIMP_DISSOLVE_MODE:
        return COMPOSITE_DISSOLVE;
    case GIMP_MULTIPLY_MODE:
        return COMPOSITE_MULT;
    case GIMP_SCREEN_MODE:
        return COMPOSITE_SCREEN;
    case GIMP_OVERLAY_MODE:
        return COMPOSITE_OVERLAY;
    case GIMP_DIFFERENCE_MODE:
        return COMPOSITE_DIFF;
    case GIMP_ADDITION_MODE:
        return COMPOSITE_ADD;
    case GIMP_SUBTRACT_MODE:
        return COMPOSITE_SUBTRACT;
    case GIMP_DARKEN_ONLY_MODE:
        return COMPOSITE_DARKEN;
    case GIMP_LIGHTEN_ONLY_MODE:
        return COMPOSITE_LIGHTEN;
    case GIMP_HUE_MODE:
        return COMPOSITE_HUE;
    case GIMP_SATURATION_MODE:
        return COMPOSITE_SATURATION;
    case GIMP_COLOR_MODE:
        return COMPOSITE_COLOR;
    case GIMP_VALUE_MODE:
        return COMPOSITE_VALUE;
    case GIMP_DIVIDE_MODE:
        return COMPOSITE_DIVIDE;
    case GIMP_DODGE_MODE:
        return COMPOSITE_DODGE;
    case GIMP_BURN_MODE:
        return COMPOSITE_BURN;
    case GIMP_COLOR_ERASE_MODE:
        return COMPOSITE_ERASE;

    case GIMP_NORMAL_NOPARTIAL_MODE:
    case GIMP_ANTI_ERASE_MODE:
    case GIMP_REPLACE_MODE:
    case GIMP_GRAIN_EXTRACT_MODE:
    case GIMP_GRAIN_MERGE_MODE:
    case GIMP_HARDLIGHT_MODE:
    case GIMP_SOFTLIGHT_MODE:
    case GIMP_BEHIND_MODE:
        break;
    }
    dbgFile << "Unknown mode: " << mode;
    return COMPOSITE_OVER;
}

KoFilter::ConversionStatus KisXCFImport::loadFromDevice(QIODevice* device, KisDoc2* doc)
{
    dbgFile << "Start decoding file";
    // Read the file into memory
    device->open(QIODevice::ReadOnly);
    QByteArray data = device->readAll();
    xcf_file = (uint8_t*)data.data();
    xcf_length = data.size();
    device->close();

    // Decode the data
    getBasicXcfInfo() ;

    dbgFile << XCF.version << "width = " << XCF.width << "height = " << XCF.height << "layers = " << XCF.numLayers;

    KisImageSP image = new KisImage(doc->undoAdapter(), XCF.width, XCF.height, KoColorSpaceRegistry::instance()->rgb8(), "built image");
    image->lock();

    for (int i = 0; i < XCF.numLayers; ++i) {
        xcfLayer& xcflayer = XCF.layers[i];
        dbgFile << i << " name = " << xcflayer.name << " opacity = " << xcflayer.opacity;

        const KoColorSpace* colorSpace = 0;
        switch (xcflayer.type) {
        case GIMP_INDEXED_IMAGE:
        case GIMP_INDEXEDA_IMAGE:
        case GIMP_RGB_IMAGE:
        case GIMP_RGBA_IMAGE:
            colorSpace = KoColorSpaceRegistry::instance()->rgb8();
            break;
        case GIMP_GRAY_IMAGE:
        case GIMP_GRAYA_IMAGE:
            colorSpace = KoColorSpaceRegistry::instance()->colorSpace("GRAYA", "");
            break;
        }

        KisPaintLayerSP layer = new KisPaintLayer(image, xcflayer.name, xcflayer.opacity, colorSpace);

        layer->setCompositeOp(layerModeG2K(xcflayer.mode));
        layer->setVisible(xcflayer.isVisible);

        image->addNode(layer.data(), image->rootLayer().data());
        
        dbgFile << xcflayer.pixels.tileptrs;

    }
    image->unlock();
    doc->setCurrentImage(image);
    return KoFilter::OK;
}
