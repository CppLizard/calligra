/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006-2013
 * Copyright (C) 2015 Michael Abrahams <miabraha@gmail.com>
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

#define GL_GLEXT_PROTOTYPES

#include "opengl/kis_opengl_canvas2.h"

#ifdef HAVE_OPENGL

#include <QFile>
#include <QMenu>
#include <QWidget>
#include <QBrush>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPoint>
#include <QPainter>
#include <QMatrix>
#include <kis_debug.h>
#include <QThread>
#include <QMessageBox>
#include <QFile>
#include <QOpenGLShaderProgram>
#include <QTransform>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <KoResourcePaths.h>


#include "KoToolProxy.h"

#include "kis_config.h"
#include "kis_types.h"
#include <kis_favorite_resource_manager.h>
#include "canvas/kis_canvas2.h"
#include "kis_coordinates_converter.h"
#include "kis_image.h"
#include "opengl/kis_opengl_image_textures.h"
#include "kis_canvas_resource_provider.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_debug.h"
#include "opengl/kis_opengl_canvas2_p.h"
#include "kis_coordinates_converter.h"
#include "canvas/kis_display_filter.h"
#include "canvas/kis_display_color_converter.h"

#define NEAR_VAL -1000.0
#define FAR_VAL 1000.0

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

static bool OPENGL_SUCCESS = false;

namespace
{
const GLuint NO_PROGRAM = 0;
}


typedef void (*kis_glLogicOp)(int);
static kis_glLogicOp ptr_glLogicOp = 0;


struct KisOpenGLCanvas2::Private
{
public:
    Private()
        : canvasInitialized(false)
        , displayShader(0)
        , checkerShader(0)
        , cursorShader(0)
        , glSyncObject(0)
        , wrapAroundMode(false)

    {
    }

    ~Private() {
        delete displayShader;
        delete checkerShader;
        delete cursorShader;

        Sync::deleteSync(glSyncObject);
    }

    bool canvasInitialized;

    QVector3D vertices[6];
    QVector2D texCoords[6];

    KisOpenGLImageTexturesSP openGLImageTextures;

    QOpenGLShaderProgram *displayShader;
    int displayUniformLocationModelViewProjection;
    int displayUniformLocationTextureMatrix;
    int displayUniformLocationViewPortScale;
    int displayUniformLocationTexelSize;
    int displayUniformLocationTexture0;
    int displayUniformLocationTexture1;

    int displayUniformLocationFixedLodLevel;
    QOpenGLShaderProgram *checkerShader;

    int checkerUniformLocationModelViewProjection;
    int checkerUniformLocationTextureMatrix;

    QOpenGLShaderProgram *cursorShader;
    int cursorShaderModelViewProjectionUniform;



    KisDisplayFilter* displayFilter;
    KisTextureTile::FilterMode filterMode;

    GLsync glSyncObject;

    bool firstDrawImage;
    qreal scaleX, scaleY;

    bool wrapAroundMode;

    int xToColWithWrapCompensation(int x, const QRect &imageRect) {
        int firstImageColumn = openGLImageTextures->xToCol(imageRect.left());
        int lastImageColumn = openGLImageTextures->xToCol(imageRect.right());

        int colsPerImage = lastImageColumn - firstImageColumn + 1;
        int numWraps = floor(qreal(x) / imageRect.width());
        int remainder = x - imageRect.width() * numWraps;

        return colsPerImage * numWraps + openGLImageTextures->xToCol(remainder);
    }

    int yToRowWithWrapCompensation(int y, const QRect &imageRect) {
        int firstImageRow = openGLImageTextures->yToRow(imageRect.top());
        int lastImageRow = openGLImageTextures->yToRow(imageRect.bottom());

        int rowsPerImage = lastImageRow - firstImageRow + 1;
        int numWraps = floor(qreal(y) / imageRect.height());
        int remainder = y - imageRect.height() * numWraps;

        return rowsPerImage * numWraps + openGLImageTextures->yToRow(remainder);
    }

};

KisOpenGLCanvas2::KisOpenGLCanvas2(KisCanvas2 *canvas, KisCoordinatesConverter *coordinatesConverter, QWidget *parent, KisImageWSP image, KisDisplayColorConverter *colorConverter)
    : QOpenGLWidget(parent)
    , KisCanvasWidgetBase(canvas, coordinatesConverter)
    , d(new Private())
{

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    setFormat(format);

    KisConfig cfg;
    cfg.writeEntry("canvasState", "OPENGL_STARTED");

    d->openGLImageTextures = KisOpenGLImageTextures::getImageTextures(image,
                                                                      colorConverter->monitorProfile(),
                                                                      colorConverter->renderingIntent(),
                                                                      colorConverter->conversionFlags());
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_AcceptTouchEvents);
    setAutoFillBackground(false);

    setAttribute(Qt::WA_InputMethodEnabled, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);

    setDisplayFilterImpl(colorConverter->displayFilter(), true);

    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    slotConfigChanged();
    cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
}

KisOpenGLCanvas2::~KisOpenGLCanvas2()
{
    delete d;
}

void KisOpenGLCanvas2::setDisplayFilter(KisDisplayFilter* displayFilter) {
    setDisplayFilterImpl(displayFilter, false);
}

void KisOpenGLCanvas2::setDisplayFilterImpl(KisDisplayFilter* displayFilter, bool initializing)
{

    bool needsInternalColorManagement =
        !displayFilter || displayFilter->useInternalColorManagement();

    bool needsFullRefresh = d->openGLImageTextures->
        setInternalColorManagementActive(needsInternalColorManagement);

    d->displayFilter = displayFilter;
    if (d->canvasInitialized) {
        d->canvasInitialized = false;
        initializeDisplayShader();
        initializeCheckerShader();
        d->canvasInitialized = true;
    }

    if (!initializing && needsFullRefresh) {
        canvas()->startUpdateInPatches(canvas()->image()->bounds());
    }
    else if (!initializing)  {
        canvas()->updateCanvas();
    }
}

void KisOpenGLCanvas2::disconnectCurrentCanvas()
{
    Q_ASSERT(d->openGLImageTextures);
    d->openGLImageTextures->disconnect(canvas());
    d->openGLImageTextures->disconnect(canvas()->image());
}


void KisOpenGLCanvas2::setWrapAroundViewingMode(bool value)
{
    d->wrapAroundMode = value;
    update();
}

void KisOpenGLCanvas2::initializeGL()
{
//    KisConfig cfg;
//    if (cfg.disableVSync()) {
//        if (!VSyncWorkaround::tryDisableVSync(this)) {
//            warnUI;
//            warnUI << "WARNING: We didn't manage to switch off VSync on your graphics adapter.";
//            warnUI << "WARNING: It means either your hardware or driver doesn't support it,";
//            warnUI << "WARNING: or we just don't know about this hardware. Please report us a bug";
//            warnUI << "WARNING: with the output of \'glxinfo\' for your card.";
//            warnUI;
//            warnUI << "WARNING: Trying to workaround it by disabling Double Buffering.";
//            warnUI << "WARNING: You may see some flickering when painting with some tools. It doesn't";
//            warnUI << "WARNING: affect the quality of the final image, though.";
//            warnUI;

//            if (cfg.disableDoubleBuffering() && QOpenGLContext::currentContext()->format().swapBehavior() == QSurfaceFormat::DoubleBuffer) {
//                errUI << "CRITICAL: Failed to disable Double Buffering. Lines may look \"bended\" on your image.";
//                errUI << "CRITICAL: Your graphics card or driver does not fully support Krita's OpenGL canvas.";
//                errUI << "CRITICAL: For an optimal experience, please disable OpenGL";
//                errUI;
//            }
//        }
//    }

    KisConfig cfg;
    dbgUI << "OpenGL: Preparing to initialize OpenGL for KisCanvas";
    int glVersion = KisOpenGL::initializeContext(context());
    dbgUI << "OpenGL: Version found" << glVersion;
    initializeOpenGLFunctions();
    VSyncWorkaround::tryDisableVSync(context());

    d->openGLImageTextures->initGL(context()->functions());
    d->openGLImageTextures->generateCheckerTexture(createCheckersImage(cfg.checkSize()));
    initializeCheckerShader();
    initializeDisplayShader();

    ptr_glLogicOp = (kis_glLogicOp)(context()->getProcAddress("glLogicOp"));

    Sync::init(context());



    /**
     * Warn about Intel's broken video drivers
     */
#if defined HAVE_OPENGL && defined Q_OS_WIN
#ifndef GL_RENDERER
#  define GL_RENDERER 0x1F01
#endif
    QString renderer = QString((const char*)glGetString(GL_RENDERER));
    if (cfg.useOpenGL() && renderer.startsWith("Intel") && !cfg.readEntry("WarnedAboutIntel", false)) {
        QMessageBox::information(0,
                                 i18nc("@title:window", "Krita: Warning"),
                                 i18n("You have an Intel(R) HD Graphics video adapter.\n"
                                      "If you experience problems like a black or blank screen,"
                                      "please update your display driver to the latest version.\n\n"
                                      "You can also disable OpenGL rendering in Krita's Settings.\n"));
        cfg.writeEntry("WarnedAboutIntel", true);
    }
#endif


    d->canvasInitialized = true;
}

void KisOpenGLCanvas2::resizeGL(int width, int height)
{
    coordinatesConverter()->setCanvasWidgetSize(QSize(width, height));
    paintGL();
}

void KisOpenGLCanvas2::paintGL()
{
    if (!OPENGL_SUCCESS) {
        KisConfig cfg;
        cfg.writeEntry("canvasState", "OPENGL_PAINT_STARTED");
    }

    QPainter gc(this);
    gc.beginNativePainting();
    renderCanvasGL();

    if (d->glSyncObject) {
        Sync::deleteSync(d->glSyncObject);
    }
    d->glSyncObject = Sync::getSync();
    gc.endNativePainting();

    renderDecorations(&gc);
    gc.end();


    if (!OPENGL_SUCCESS) {
        KisConfig cfg;
        cfg.writeEntry("canvasState", "OPENGL_SUCCESS");
        OPENGL_SUCCESS = true;
    }
}

QOpenGLShaderProgram *KisOpenGLCanvas2::getCursorShader()
{
    if (d->cursorShader == 0) {
        d->cursorShader = new QOpenGLShaderProgram();
        d->cursorShader->addShaderFromSourceFile(QOpenGLShader::Vertex, KoResourcePaths::findResource("data", "krita/shaders/cursor.vert"));
        d->cursorShader->addShaderFromSourceFile(QOpenGLShader::Fragment, KoResourcePaths::findResource("data", "krita/shaders/cursor.frag"));
        d->cursorShader->bindAttributeLocation("a_vertexPosition", PROGRAM_VERTEX_ATTRIBUTE);
        if (! d->cursorShader->link()) {
            dbgUI << "OpenGL error" << glGetError();
            qFatal("Failed linking cursor shader");
        }
        Q_ASSERT(d->cursorShader->isLinked());
        d->cursorShaderModelViewProjectionUniform = d->cursorShader->uniformLocation("modelViewProjection");
    }

    return d->cursorShader;
}

void KisOpenGLCanvas2::paintToolOutline(const QPainterPath &path)
{

    QOpenGLShaderProgram *cursorShader = getCursorShader();
    cursorShader->bind();

    // setup the mvp transformation
    KisCoordinatesConverter *converter = coordinatesConverter();

    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(0, width(), height(), 0, NEAR_VAL, FAR_VAL);

    // Set view/projection matrices
    QMatrix4x4 modelMatrix(converter->flakeToWidgetTransform());
    modelMatrix.optimize();
    modelMatrix = projectionMatrix * modelMatrix;
    cursorShader->setUniformValue(d->cursorShaderModelViewProjectionUniform, modelMatrix);

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // XXX: glLogicOp not in ES 2.0 -- in that case, it would be better to use another method.
    // It is defined in 3.1 core profile onward.
    glEnable(GL_COLOR_LOGIC_OP);
    if (ptr_glLogicOp) {
        ptr_glLogicOp(GL_XOR);
    }

    // setup the array of vertices
    QVector<QVector3D> vertices;
    QList<QPolygonF> subPathPolygons = path.toSubpathPolygons();
    for (int i=0; i<subPathPolygons.size(); i++) {
        const QPolygonF& polygon = subPathPolygons.at(i);
        for (int j=0; j < polygon.count(); j++) {
            QPointF p = polygon.at(j);
            vertices << QVector3D(p.x(), p.y(), 0.f);
        }
        cursorShader->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
        cursorShader->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, vertices.constData());

        glDrawArrays(GL_LINE_STRIP, 0, vertices.size());

        vertices.clear();
    }

    glDisable(GL_COLOR_LOGIC_OP);

    cursorShader->release();

}

bool KisOpenGLCanvas2::isBusy() const
{
    return Sync::syncStatus(d->glSyncObject) == Sync::Unsignaled;
}

inline void rectToVertices(QVector3D* vertices, const QRectF &rc)
{
     vertices[0] = QVector3D(rc.left(),  rc.bottom(), 0.f);
     vertices[1] = QVector3D(rc.left(),  rc.top(),    0.f);
     vertices[2] = QVector3D(rc.right(), rc.bottom(), 0.f);
     vertices[3] = QVector3D(rc.left(),  rc.top(), 0.f);
     vertices[4] = QVector3D(rc.right(), rc.top(), 0.f);
     vertices[5] = QVector3D(rc.right(), rc.bottom(),    0.f);
}

inline void rectToTexCoords(QVector2D* texCoords, const QRectF &rc)
{
    texCoords[0] = QVector2D(rc.left(), rc.bottom());
    texCoords[1] = QVector2D(rc.left(), rc.top());
    texCoords[2] = QVector2D(rc.right(), rc.bottom());
    texCoords[3] = QVector2D(rc.left(), rc.top());
    texCoords[4] = QVector2D(rc.right(), rc.top());
    texCoords[5] = QVector2D(rc.right(), rc.bottom());
}

void KisOpenGLCanvas2::drawCheckers()
{
    if (!d->checkerShader) {
        return;
    }

    KisCoordinatesConverter *converter = coordinatesConverter();
    QTransform textureTransform;
    QTransform modelTransform;
    QRectF textureRect;
    QRectF modelRect;

    QRectF viewportRect = !d->wrapAroundMode ?
        converter->imageRectInViewportPixels() :
        converter->widgetToViewport(this->rect());

    converter->getOpenGLCheckersInfo(viewportRect,
                                     &textureTransform, &modelTransform, &textureRect, &modelRect);

    // XXX: getting a config object every time we draw the checkers is bad for performance!
    KisConfig cfg;
    GLfloat checkSizeScale = KisOpenGLImageTextures::BACKGROUND_TEXTURE_CHECK_SIZE / static_cast<GLfloat>(cfg.checkSize());

    textureTransform *= QTransform::fromScale(checkSizeScale / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE,
                                              checkSizeScale / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE);

    d->checkerShader->bind();

    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(0, width(), height(), 0, NEAR_VAL, FAR_VAL);

    // Set view/projection matrices
    QMatrix4x4 modelMatrix(modelTransform);
    modelMatrix.optimize();
    modelMatrix = projectionMatrix * modelMatrix;
    d->checkerShader->setUniformValue(d->checkerUniformLocationModelViewProjection, modelMatrix);

    QMatrix4x4 textureMatrix(textureTransform);
    d->checkerShader->setUniformValue(d->checkerUniformLocationTextureMatrix, textureMatrix);

    //Setup the geometry for rendering
    rectToVertices(d->vertices, modelRect);
    d->checkerShader->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    d->checkerShader->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, d->vertices);

    rectToTexCoords(d->texCoords, textureRect);
    d->checkerShader->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    d->checkerShader->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, d->texCoords);

     // render checkers
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->openGLImageTextures->checkerTexture());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, 0);
    d->checkerShader->release();
}

void KisOpenGLCanvas2::drawImage()
{
    if (!d->displayShader) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    KisCoordinatesConverter *converter = coordinatesConverter();

    d->displayShader->bind();

    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    projectionMatrix.ortho(0, width(), height(), 0, NEAR_VAL, FAR_VAL);

    // Set view/projection matrices
    QMatrix4x4 modelMatrix(coordinatesConverter()->imageToWidgetTransform());
    modelMatrix.optimize();
    modelMatrix = projectionMatrix * modelMatrix;
    d->displayShader->setUniformValue(d->displayUniformLocationModelViewProjection, modelMatrix);

    QMatrix4x4 textureMatrix;
    textureMatrix.setToIdentity();
    d->displayShader->setUniformValue(d->displayUniformLocationTextureMatrix, textureMatrix);

    QRectF widgetRect(0,0, width(), height());
    QRectF widgetRectInImagePixels = converter->documentToImage(converter->widgetToDocument(widgetRect));

    qreal scaleX, scaleY;
    converter->imageScale(&scaleX, &scaleY);
    d->displayShader->setUniformValue(d->displayUniformLocationViewPortScale, (GLfloat) scaleX);
    d->displayShader->setUniformValue(d->displayUniformLocationTexelSize, (GLfloat) d->openGLImageTextures->texelSize());

    QRect ir = d->openGLImageTextures->storedImageBounds();
    QRect wr = widgetRectInImagePixels.toAlignedRect();

    if (!d->wrapAroundMode) {
        // if we don't want to paint wrapping images, just limit the
        // processing area, and the code will handle all the rest
        wr &= ir;
    }

    int firstColumn = d->xToColWithWrapCompensation(wr.left(), ir);
    int lastColumn = d->xToColWithWrapCompensation(wr.right(), ir);
    int firstRow = d->yToRowWithWrapCompensation(wr.top(), ir);
    int lastRow = d->yToRowWithWrapCompensation(wr.bottom(), ir);

    int minColumn = d->openGLImageTextures->xToCol(ir.left());
    int maxColumn = d->openGLImageTextures->xToCol(ir.right());
    int minRow = d->openGLImageTextures->yToRow(ir.top());
    int maxRow = d->openGLImageTextures->yToRow(ir.bottom());

    int imageColumns = maxColumn - minColumn + 1;
    int imageRows = maxRow - minRow + 1;

    for (int col = firstColumn; col <= lastColumn; col++) {
        for (int row = firstRow; row <= lastRow; row++) {

            int effectiveCol = col;
            int effectiveRow = row;
            QPointF tileWrappingTranslation;

            if (effectiveCol > maxColumn || effectiveCol < minColumn) {
                int translationStep = floor(qreal(col) / imageColumns);
                int originCol = translationStep * imageColumns;
                effectiveCol = col - originCol;
                tileWrappingTranslation.rx() = translationStep * ir.width();
            }

            if (effectiveRow > maxRow || effectiveRow < minRow) {
                int translationStep = floor(qreal(row) / imageRows);
                int originRow = translationStep * imageRows;
                effectiveRow = row - originRow;
                tileWrappingTranslation.ry() = translationStep * ir.height();
            }

            KisTextureTile *tile =
                    d->openGLImageTextures->getTextureTileCR(effectiveCol, effectiveRow);

            if (!tile) {
                warnUI << "OpenGL: Trying to paint texture tile but it has not been created yet.";
                continue;
            }

            /*
             * We create a float rect here to workaround Qt's
             * "history reasons" in calculation of right()
             * and bottom() coordinates of integer rects.
             */
            QRectF textureRect(tile->tileRectInTexturePixels());
            QRectF modelRect(tile->tileRectInImagePixels().translated(tileWrappingTranslation.x(), tileWrappingTranslation.y()));

            //Setup the geometry for rendering
            rectToVertices(d->vertices, modelRect);
            d->displayShader->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
            d->displayShader->setAttributeArray(PROGRAM_VERTEX_ATTRIBUTE, d->vertices);

            rectToTexCoords(d->texCoords, textureRect);
            d->displayShader->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
            d->displayShader->setAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE, d->texCoords);

            if (d->displayFilter) {
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_3D, d->displayFilter->lutTexture());
                d->displayShader->setUniformValue(d->displayUniformLocationTexture1, 1);
            }

            int currentLodPlane = tile->currentLodPlane();
            if (d->displayUniformLocationFixedLodLevel >= 0) {
                d->displayShader->setUniformValue(d->displayUniformLocationFixedLodLevel,
                                                  (GLfloat) currentLodPlane);
            }

            glActiveTexture(GL_TEXTURE0);
            tile->bindToActiveTexture();

            if (currentLodPlane) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            } else if (SCALE_MORE_OR_EQUAL_TO(scaleX, scaleY, 2.0)) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            } else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                switch(d->filterMode) {
                case KisTextureTile::NearestFilterMode:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    break;
                case KisTextureTile::BilinearFilterMode:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    break;
                case KisTextureTile::TrilinearFilterMode:
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    break;
                case KisTextureTile::HighQualityFiltering:
                    if (SCALE_LESS_THAN(scaleX, scaleY, 0.5)) {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                    } else {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    }
                    break;
                }
            }

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    d->displayShader->release();
}

void KisOpenGLCanvas2::reportShaderLinkFailedAndExit(bool result, const QString &context, const QString &log)
{
    KisConfig cfg;

    if (cfg.useVerboseOpenGLDebugOutput()) {
        dbgUI << "GL-log:" << context << log;
    }

    if (result) return;

    QMessageBox::critical(this, i18nc("@title:window", "Krita"),
                          QString(i18n("Krita could not initialize the OpenGL canvas:\n\n%1\n\n%2\n\n Krita will disable OpenGL and close now.")).arg(context).arg(log),
                          QMessageBox::Close);

    cfg.setUseOpenGL(false);
    cfg.setCanvasState("OPENGL_FAILED");
}

void KisOpenGLCanvas2::initializeCheckerShader()
{
    if (d->canvasInitialized) return;

    delete d->checkerShader;
    d->checkerShader = new QOpenGLShaderProgram();

    QString vertexShaderName;
    QString fragmentShaderName;

    if (KisOpenGL::supportsGLSL13()) {
        vertexShaderName = KoResourcePaths::findResource("data", "krita/shaders/matrix_transform.vert");
        fragmentShaderName = KoResourcePaths::findResource("data", "krita/shaders/simple_texture.frag");
    } else {
        vertexShaderName = KoResourcePaths::findResource("data", "krita/shaders/matrix_transform_legacy.vert");
        fragmentShaderName = KoResourcePaths::findResource("data", "krita/shaders/simple_texture_legacy.frag");
    }

    bool result;

    result = d->checkerShader->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderName);
    reportShaderLinkFailedAndExit(result, "Checker vertex shader", d->checkerShader->log());

    result = d->checkerShader->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderName);
    reportShaderLinkFailedAndExit(result, "Checker fragment shader", d->checkerShader->log());

    d->checkerShader->bindAttributeLocation("a_vertexPosition", PROGRAM_VERTEX_ATTRIBUTE);
    d->checkerShader->bindAttributeLocation("a_textureCoordinate", PROGRAM_TEXCOORD_ATTRIBUTE);

    result = d->checkerShader->link();
    reportShaderLinkFailedAndExit(result, "Checker shader (link)", d->checkerShader->log());

    Q_ASSERT(d->checkerShader->isLinked());

    d->checkerUniformLocationModelViewProjection = d->checkerShader->uniformLocation("modelViewProjection");
    d->checkerUniformLocationTextureMatrix = d->checkerShader->uniformLocation("textureMatrix");

}

QByteArray KisOpenGLCanvas2::buildFragmentShader()
{
    QByteArray shaderText;

    bool haveDisplayFilter = d->displayFilter && !d->displayFilter->program().isEmpty();
    bool useHiQualityFiltering = d->filterMode == KisTextureTile::HighQualityFiltering;
    bool haveGLSL13 = KisOpenGL::supportsGLSL13();
    bool useDirectLodFetch = haveGLSL13;

    QString filename;

    if (haveGLSL13) {
        filename = "highq_downscale.frag";
        shaderText.append("#version 130\n");
    } else {
        filename = "simple_texture_legacy.frag";
    }

    QString fileKey = QString("krita/shaders/%1")
        .arg(filename);

    if (haveDisplayFilter) {
        shaderText.append("#define USE_OCIO\n");
        shaderText.append(d->displayFilter->program().toLatin1());
    }

    if (haveGLSL13 && useHiQualityFiltering) {
        shaderText.append("#define HIGHQ_SCALING\n");
    }

    if (haveGLSL13 && useDirectLodFetch) {
        shaderText.append("#define DIRECT_LOD_FETCH\n");
    }

    {
        QFile prefaceFile(KoResourcePaths::findResource("data", fileKey));
        prefaceFile.open(QIODevice::ReadOnly);
        shaderText.append(prefaceFile.readAll());
    }

    return shaderText;
}

void KisOpenGLCanvas2::initializeDisplayShader()
{
    if (d->canvasInitialized) return;

    delete d->displayShader;
    d->displayShader = new QOpenGLShaderProgram();

    bool result = d->displayShader->addShaderFromSourceCode(QOpenGLShader::Fragment, buildFragmentShader());
    reportShaderLinkFailedAndExit(result, "Display fragment shader", d->displayShader->log());

    if (KisOpenGL::supportsGLSL13()) {
        result = d->displayShader->addShaderFromSourceFile(QOpenGLShader::Vertex, KoResourcePaths::findResource("data", "krita/shaders/matrix_transform.vert"));
    } else {
        result = d->displayShader->addShaderFromSourceFile(QOpenGLShader::Vertex, KoResourcePaths::findResource("data", "krita/shaders/matrix_transform_legacy.vert"));
    }
    reportShaderLinkFailedAndExit(result, "Display vertex shader", d->displayShader->log());

    d->displayShader->bindAttributeLocation("a_vertexPosition", PROGRAM_VERTEX_ATTRIBUTE);
    d->displayShader->bindAttributeLocation("a_textureCoordinate", PROGRAM_TEXCOORD_ATTRIBUTE);

    result = d->displayShader->link();
    reportShaderLinkFailedAndExit(result, "Display shader (link)", d->displayShader->log());

    Q_ASSERT(d->displayShader->isLinked());

    d->displayUniformLocationModelViewProjection = d->displayShader->uniformLocation("modelViewProjection");
    d->displayUniformLocationTextureMatrix = d->displayShader->uniformLocation("textureMatrix");
    d->displayUniformLocationTexture0 = d->displayShader->uniformLocation("texture0");

    // ocio
    d->displayUniformLocationTexture1 = d->displayShader->uniformLocation("texture1");

    // highq || lod
    d->displayUniformLocationViewPortScale = d->displayShader->uniformLocation("viewportScale");

    // highq
    d->displayUniformLocationTexelSize = d->displayShader->uniformLocation("texelSize");

    // lod
    d->displayUniformLocationFixedLodLevel =
        KisOpenGL::supportsGLSL13() ?
        d->displayShader->uniformLocation("fixedLodLevel") : -1;
}

void KisOpenGLCanvas2::slotConfigChanged()
{
    KisConfig cfg;
    d->openGLImageTextures->generateCheckerTexture(createCheckersImage(cfg.checkSize()));
    d->openGLImageTextures->updateConfig(cfg.useOpenGLTextureBuffer(), cfg.numMipmapLevels());
    d->filterMode = (KisTextureTile::FilterMode) cfg.openGLFilteringMode();

    notifyConfigChanged();
}

QVariant KisOpenGLCanvas2::inputMethodQuery(Qt::InputMethodQuery query) const
{
    return processInputMethodQuery(query);
}

void KisOpenGLCanvas2::inputMethodEvent(QInputMethodEvent *event)
{
    processInputMethodEvent(event);
}

void KisOpenGLCanvas2::renderCanvasGL()
{
    // Draw the border (that is, clear the whole widget to the border color)
    QColor widgetBackgroundColor = borderColor();
    glClearColor(widgetBackgroundColor.redF(), widgetBackgroundColor.greenF(), widgetBackgroundColor.blueF(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    drawCheckers();
    drawImage();
}

void KisOpenGLCanvas2::renderDecorations(QPainter *painter)
{
    QRect boundingRect = coordinatesConverter()->imageRectInWidgetPixels().toAlignedRect();
    drawDecorations(*painter, boundingRect);
}


void KisOpenGLCanvas2::setDisplayProfile(KisDisplayColorConverter *colorConverter)
{
    d->openGLImageTextures->setMonitorProfile(colorConverter->monitorProfile(),
                                              colorConverter->renderingIntent(),
                                              colorConverter->conversionFlags());
}

void KisOpenGLCanvas2::channelSelectionChanged(QBitArray channelFlags)
{
    d->openGLImageTextures->setChannelFlags(channelFlags);
}


void KisOpenGLCanvas2::finishResizingImage(qint32 w, qint32 h)
{
    if (d->canvasInitialized) {
        d->openGLImageTextures->slotImageSizeChanged(w, h);
    }
}

KisUpdateInfoSP KisOpenGLCanvas2::startUpdateCanvasProjection(const QRect & rc, QBitArray channelFlags)
{
    d->openGLImageTextures->setChannelFlags(channelFlags);
    return d->openGLImageTextures->updateCache(rc);
}


QRect KisOpenGLCanvas2::updateCanvasProjection(KisUpdateInfoSP info)
{
    // See KisQPainterCanvas::updateCanvasProjection for more info
    bool isOpenGLUpdateInfo = dynamic_cast<KisOpenGLUpdateInfo*>(info.data());
    if (isOpenGLUpdateInfo) {
        d->openGLImageTextures->recalculateCache(info);
    }
    return QRect(); // FIXME: Implement dirty rect for OpenGL
}

bool KisOpenGLCanvas2::callFocusNextPrevChild(bool next)
{
    return focusNextPrevChild(next);
}

KisOpenGLImageTexturesSP KisOpenGLCanvas2::openGLImageTextures() const
{
    return d->openGLImageTextures;
}

#endif // HAVE_OPENGL
