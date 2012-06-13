/*
 *  Copyright (c) 2005-2007 Adrian Page <adrian@pagenet.plus.com>
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

#include "opengl/kis_opengl_image_textures.h"

#ifdef HAVE_OPENGL

#include <QApplication>

#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>

#include "kis_image.h"
#include "kis_config.h"

#ifdef HAVE_OPENEXR
#include <half.h>
#endif

#ifdef HAVE_GLEW
#include "opengl/kis_opengl_hdr_exposure_program.h"
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif


KisOpenGLImageTextures::ImageTexturesMap KisOpenGLImageTextures::imageTexturesMap;

#ifdef HAVE_GLEW
KisOpenGLHDRExposureProgram *KisOpenGLImageTextures::HDRExposureProgram = 0;
#endif

KisOpenGLImageTextures::KisOpenGLImageTextures()
{
    m_image = 0;
    m_monitorProfile = 0;
    m_exposure = 0;
}

KisOpenGLImageTextures::KisOpenGLImageTextures(KisImageWSP image, KoColorProfile *monitorProfile)
{
    m_image = image;
    m_monitorProfile = monitorProfile;
    m_exposure = 0;

    KisOpenGL::makeContextCurrent();

    getTextureSize(&m_texturesInfo);

    glGenTextures(1, &m_backgroundTexture);
    createImageTextureTiles();

    KisOpenGLUpdateInfoSP info = updateCache(m_image->bounds());
    recalculateCache(info);
}

KisOpenGLImageTextures::~KisOpenGLImageTextures()
{
    ImageTexturesMap::iterator it = imageTexturesMap.find(m_image);
    if (it != imageTexturesMap.end()) {
        KisOpenGLImageTextures *textures = it.value();
        if (textures == this) {
            dbgUI << "Removing shared image context from map";
            imageTexturesMap.erase(it);
        }
    }

    destroyImageTextureTiles();
    glDeleteTextures(1, &m_backgroundTexture);
}

bool KisOpenGLImageTextures::imageCanShareTextures(KisImageWSP image)
{
    return !image->colorSpace()->hasHighDynamicRange() ||
        imageCanUseHDRExposureProgram(image);
}

KisOpenGLImageTexturesSP KisOpenGLImageTextures::getImageTextures(KisImageWSP image, KoColorProfile *monitorProfile)
{
    KisOpenGL::makeContextCurrent();
    createHDRExposureProgramIfCan();

    if (imageCanShareTextures(image)) {
        ImageTexturesMap::iterator it = imageTexturesMap.find(image);

        if (it != imageTexturesMap.end()) {
            KisOpenGLImageTexturesSP textures = it.value();
            textures->setMonitorProfile(monitorProfile);

            return textures;
        } else {
            KisOpenGLImageTextures *imageTextures = new KisOpenGLImageTextures(image, monitorProfile);
            imageTexturesMap[image] = imageTextures;
            dbgUI << "Added shareable textures to map";

            return imageTextures;
        }
    } else {
        return new KisOpenGLImageTextures(image, monitorProfile);
    }
}

QRect KisOpenGLImageTextures::calculateTileRect(int col, int row) const
{
    return m_image->bounds() &
        QRect(col * m_texturesInfo.effectiveWidth,
              row * m_texturesInfo.effectiveHeight,
              m_texturesInfo.effectiveWidth,
              m_texturesInfo.effectiveHeight);
}

void KisOpenGLImageTextures::createImageTextureTiles()
{
    KisOpenGL::makeContextCurrent();

    destroyImageTextureTiles();
    updateTextureFormat();

    m_storedImageBounds = m_image->bounds();
    const int lastCol = xToCol(m_image->width());
    const int lastRow = yToRow(m_image->height());
    m_numCols = lastCol + 1;

    // Default color is transparent black
    const int pixelSize = 4;
    QByteArray emptyTileData((m_texturesInfo.width) * (m_texturesInfo.height) * pixelSize, 0);

    KisConfig config;
    KisTextureTile::FilterMode mode = config.useOpenGLTrilinearFiltering() ? KisTextureTile::TrilinearFilterMode : KisTextureTile::BilinearFilterMode;

    for (int row = 0; row <= lastRow; row++) {
        for (int col = 0; col <= lastCol; col++) {
            QRect tileRect = calculateTileRect(col, row);

            KisTextureTile *tile = new KisTextureTile(tileRect,
                                                      &m_texturesInfo,
                                                      emptyTileData.constData(),
                                                      mode);
            m_textureTiles.append(tile);
        }
    }
}

void KisOpenGLImageTextures::destroyImageTextureTiles()
{
    if(m_textureTiles.isEmpty()) return;

    KisOpenGL::makeContextCurrent();
    foreach(KisTextureTile *tile, m_textureTiles) {
        delete tile;
    }
    m_textureTiles.clear();
    m_storedImageBounds = QRect();
}

KisOpenGLUpdateInfoSP
KisOpenGLImageTextures::updateCache(const QRect& rect)
{
    KisOpenGLUpdateInfoSP info = new KisOpenGLUpdateInfo();

    QRect updateRect = rect & m_image->bounds();
    if (updateRect.isEmpty()) return info;


    /**
     * Why the rect is artificial? That's easy!
     * It does not represent any real piece of the image. It is
     * intentionally stretched to get through the overlappping
     * stripes of neutrality and poke neighbouring tiles.
     * Thanks to the rect we get the coordinates of all the tiles
     * involved into update process
     */

    QRect artificialRect = stretchRect(updateRect, m_texturesInfo.border);

    int firstColumn = xToCol(artificialRect.left());
    int lastColumn = xToCol(artificialRect.right());
    int firstRow = yToRow(artificialRect.top());
    int lastRow = yToRow(artificialRect.bottom());


    qint32 numItems = (lastColumn - firstColumn + 1) * (lastRow - firstRow + 1);
    info->tileList.reserve(numItems);

    for (int col = firstColumn; col <= lastColumn; col++) {
        for (int row = firstRow; row <= lastRow; row++) {

            QRect tileRect = calculateTileRect(col, row);
            QRect tileTextureRect = stretchRect(tileRect, m_texturesInfo.border);

            KisTextureTileUpdateInfo tileInfo(col, row,
                                              tileTextureRect,
                                              updateRect,
                                              m_image->bounds());

            tileInfo.retrieveData(m_image);
            info->tileList.append(tileInfo);
        }
    }
    return info;
}

void KisOpenGLImageTextures::recalculateCache(KisUpdateInfoSP info)
{
    KisOpenGLUpdateInfoSP glInfo = dynamic_cast<KisOpenGLUpdateInfo*>(info.data());
    if(!glInfo) return;

    KisOpenGL::makeContextCurrent();
    KIS_OPENGL_CLEAR_ERROR();


    KisTextureTileUpdateInfo tileInfo;
    foreach(tileInfo, glInfo->tileList) {
        const KoColorSpace *dstCS = 0;

        switch(m_texturesInfo.type) {
        case GL_UNSIGNED_BYTE:
            dstCS = KoColorSpaceRegistry::instance()->rgb8(m_monitorProfile);
            break;
#if defined(HAVE_GLEW) && defined(HAVE_OPENEXR)
        case GL_UNSIGNED_SHORT:
            dstCS = KoColorSpaceRegistry::instance()->rgb16(m_monitorProfile);
            break;
        case GL_HALF_FLOAT_ARB:
            dstCS = KoColorSpaceRegistry::instance()->colorSpace("RGBA", "F16", m_monitorProfile);
            break;
        case GL_FLOAT:
            dstCS = KoColorSpaceRegistry::instance()->colorSpace("RGBA", "F32", m_monitorProfile);
            break;
#endif
        default:
            qFatal("Unknown m_imageTextureType");
        }

        tileInfo.convertTo(dstCS);
        KisTextureTile *tile = getTextureTileCR(tileInfo.tileCol(), tileInfo.tileRow());
        tile->update(tileInfo);
        tileInfo.destroy();

        KIS_OPENGL_PRINT_ERROR();
    }
}

void KisOpenGLImageTextures::generateBackgroundTexture(const QImage & checkImage)
{
    KisOpenGL::makeContextCurrent();

    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    Q_ASSERT(checkImage.width() == BACKGROUND_TEXTURE_SIZE);
    Q_ASSERT(checkImage.height() == BACKGROUND_TEXTURE_SIZE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, BACKGROUND_TEXTURE_SIZE, BACKGROUND_TEXTURE_SIZE,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, checkImage.bits());
}

GLuint KisOpenGLImageTextures::backgroundTexture() const
{
    return m_backgroundTexture;
}

void KisOpenGLImageTextures::slotImageSizeChanged(qint32 /*w*/, qint32 /*h*/)
{
    createImageTextureTiles();
}

void KisOpenGLImageTextures::setMonitorProfile(KoColorProfile *monitorProfile)
{
    if (monitorProfile != m_monitorProfile) {
        m_monitorProfile = monitorProfile;
        KisOpenGLUpdateInfoSP info = updateCache(m_image->bounds());
        recalculateCache(info);
    }
}

void KisOpenGLImageTextures::setHDRExposure(float exposure)
{
    if (exposure != m_exposure) {
        m_exposure = exposure;

        if (m_image->colorSpace()->hasHighDynamicRange()) {
#ifdef HAVE_GLEW
            if (m_usingHDRExposureProgram) {
                HDRExposureProgram->setExposure(exposure);
            } else {
#endif

#ifdef __GNUC__
#warning "FIXME: Move this setOverrideCursor to a higher level"
#else
#pragma WARNING( "FIXME: Move this setOverrideCursor to a higher level") { )
#endif
                QApplication::setOverrideCursor(Qt::WaitCursor);
                KisOpenGLUpdateInfoSP info = updateCache(m_image->bounds());
                recalculateCache(info);
                QApplication::restoreOverrideCursor();
#ifdef HAVE_GLEW
            }
#endif
        }
    }
}

void KisOpenGLImageTextures::createHDRExposureProgramIfCan()
{
    KisConfig cfg;
    if (!cfg.useOpenGLShaders()) return;

#ifdef HAVE_GLEW
    if (!HDRExposureProgram && KisOpenGL::hasShadingLanguage()) {
        dbgUI << "Creating shared HDR exposure program";
        HDRExposureProgram = new KisOpenGLHDRExposureProgram();
        Q_CHECK_PTR(HDRExposureProgram);
    }
#endif
}

bool KisOpenGLImageTextures::usingHDRExposureProgram() const
{
#ifdef HAVE_GLEW
    return m_usingHDRExposureProgram;
#else
    return false;
#endif
}

void KisOpenGLImageTextures::activateHDRExposureProgram()
{
#ifdef HAVE_GLEW
    if (m_usingHDRExposureProgram) {
        HDRExposureProgram->activate();
    }
#endif
}

void KisOpenGLImageTextures::deactivateHDRExposureProgram()
{
#ifdef HAVE_GLEW
    if (m_usingHDRExposureProgram) {
        KisOpenGLProgram::deactivate();
    }
#endif
}

void KisOpenGLImageTextures::getTextureSize(KisGLTexturesInfo *texturesInfo)
{
    // TODO: make configurable
    const GLint preferredTextureSize = 256;

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    texturesInfo->width = qMin(preferredTextureSize, maxTextureSize);
    texturesInfo->height = qMin(preferredTextureSize, maxTextureSize);

    texturesInfo->border = 1;

    texturesInfo->effectiveWidth = texturesInfo->width - 2 * texturesInfo->border;
    texturesInfo->effectiveHeight = texturesInfo->height - 2 * texturesInfo->border;
}

void KisOpenGLImageTextures::updateTextureFormat()
{
    m_texturesInfo.format = GL_RGBA8;
    m_texturesInfo.type = GL_UNSIGNED_BYTE;

    #ifdef HAVE_GLEW
    m_usingHDRExposureProgram = imageCanUseHDRExposureProgram(m_image);

    KoID colorModelId = m_image->colorSpace()->colorModelId();
    KoID colorDepthId = m_image->colorSpace()->colorDepthId();

    dbgUI << "Choosing texture format:";

    if (colorModelId == RGBAColorModelID) {
        if (colorDepthId == Float16BitsColorDepthID && imageCanUseHDRExposureProgram(m_image)) {

            if (GLEW_ARB_texture_float) {
                m_texturesInfo.format = GL_RGBA16F_ARB;
                dbgUI << "Using ARB half";
            } else {
                Q_ASSERT(GLEW_ATI_texture_float);
                m_texturesInfo.format = GL_RGBA_FLOAT16_ATI;
                dbgUI << "Using ATI half";
            }

            if (GLEW_ARB_half_float_pixel) {
                dbgUI << "Pixel type half";
                m_texturesInfo.type = GL_HALF_FLOAT_ARB;
            } else {
                dbgUI << "Pixel type float";
                m_texturesInfo.type = GL_FLOAT;
            }

            m_usingHDRExposureProgram = true;

        }
        else if (colorDepthId == Float32BitsColorDepthID && imageCanUseHDRExposureProgram(m_image)) {

            if (GLEW_ARB_texture_float) {
                m_texturesInfo.format = GL_RGBA32F_ARB;
                dbgUI << "Using ARB float";
            } else {
                Q_ASSERT(GLEW_ATI_texture_float);
                m_texturesInfo.format = GL_RGBA_FLOAT32_ATI;
                dbgUI << "Using ATI float";
            }

            m_texturesInfo.type = GL_FLOAT;
            m_usingHDRExposureProgram = true;
        }
        else if (colorDepthId != Integer8BitsColorDepthID) {
            dbgUI << "Using 16 bits rgba";
            m_texturesInfo.format = GL_RGBA16;
            m_texturesInfo.type = GL_UNSIGNED_SHORT;
        }
    }
    else {
        // We will convert the colorspace to 16 bits rgba, instead of 8 bits
        if (colorDepthId == Integer16BitsColorDepthID) {
            dbgUI << "Using conversion to 16 bits rgba";
            m_texturesInfo.format = GL_RGBA16;
            m_texturesInfo.type = GL_UNSIGNED_SHORT;
        }
    }
#endif
}

bool KisOpenGLImageTextures::imageCanUseHDRExposureProgram(KisImageWSP image)
{
#ifdef HAVE_GLEW

    KisConfig cfg;

    if (!image->colorSpace()->hasHighDynamicRange() ||
        !cfg.useOpenGLShaders() ||
        !HDRExposureProgram ||
        !HDRExposureProgram->isValid()) {

        return false;
    }

    KisOpenGL::makeContextCurrent();
    KoID colorModelId = image->colorSpace()->colorModelId();
    KoID colorDepthId = image->colorSpace()->colorDepthId();

    if (!(    colorModelId == RGBAColorModelID
              && (colorDepthId == Float16BitsColorDepthID || colorDepthId == Float32BitsColorDepthID)
              && (GLEW_ARB_texture_float || GLEW_ATI_texture_float))) {
        return false;
    }
    return true;
#else
    Q_UNUSED(image);
    return false;
#endif
}

#include "kis_opengl_image_textures.moc"

#endif // HAVE_OPENGL

