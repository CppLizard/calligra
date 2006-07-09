/*
 * This file is part of Krita
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GL
#include <qgl.h>
#endif

#include <qradiobutton.h>

#include <kaction.h>
#include <kdialogbase.h>
#include <klocale.h>

#include "kis_grid_manager.h"
#include "kis_grid_manager.moc"

#include "kis_config.h"
#include "kis_view.h"
#include "kis_image.h"

KisGridManager::KisGridManager(KisView * parent)
    : QObject(parent), m_view(parent)
{

}

KisGridManager::~KisGridManager()
{

}

void KisGridManager::setup(KActionCollection * collection)
{
    m_toggleGrid = new KToggleAction(i18n("Show Grid"), "", this, SLOT(toggleGrid()), collection, "view_toggle_grid");
    m_toggleGrid->setCheckedState(KGuiItem(i18n("Hide Grid")));
    m_toggleGrid->setChecked(false);

    // Fast grid config
    m_gridFastConfig1x1 = new KAction(i18n("1x1"), 0, "", this, SLOT(fastConfig1x1()), collection, "view_fast_grid_1x1");
    m_gridFastConfig2x2 = new KAction(i18n("2x2"), 0, "", this, SLOT(fastConfig2x2()), collection, "view_fast_grid_2x2");
    m_gridFastConfig5x5 = new KAction(i18n("5x5"), 0, "", this, SLOT(fastConfig5x5()), collection, "view_fast_grid_5x5");
    m_gridFastConfig10x10 = new KAction(i18n("10x10"), 0, "", this, SLOT(fastConfig10x10()), collection, "view_fast_grid_10x10");
    m_gridFastConfig20x20 = new KAction(i18n("20x20"), 0, "", this, SLOT(fastConfig20x20()), collection, "view_fast_grid_20x20");
    m_gridFastConfig40x40 = new KAction(i18n("40x40"), 0, "", this, SLOT(fastConfig40x40()), collection, "view_fast_grid_40x40");
}

void KisGridManager::updateGUI()
{

}

void KisGridManager::toggleGrid()
{
    m_view->updateCanvas();
}

void KisGridManager::fastConfig1x1()
{
    KisConfig cfg;
    cfg.setGridHSpacing(1);
    cfg.setGridVSpacing(1);
    m_view->updateCanvas();
}

void KisGridManager::fastConfig2x2()
{
    KisConfig cfg;
    cfg.setGridHSpacing(2);
    cfg.setGridVSpacing(2);
    m_view->updateCanvas();
}

void KisGridManager::fastConfig5x5()
{
    KisConfig cfg;
    cfg.setGridHSpacing(5);
    cfg.setGridVSpacing(5);
    m_view->updateCanvas();
}

void KisGridManager::fastConfig10x10()
{
    KisConfig cfg;
    cfg.setGridHSpacing(10);
    cfg.setGridVSpacing(10);
    m_view->updateCanvas();
}

void KisGridManager::fastConfig20x20()
{
    KisConfig cfg;
    cfg.setGridHSpacing(20);
    cfg.setGridVSpacing(20);
    m_view->updateCanvas();
}

void KisGridManager::fastConfig40x40()
{
    KisConfig cfg;
    cfg.setGridHSpacing(40);
    cfg.setGridVSpacing(40);
    m_view->updateCanvas();
}

Qt::PenStyle KisGridManager::GridDrawer::gs2style(Q_UINT32 s)
{
    switch(s)
    {
        case 1:
            return Qt::DashLine;
        case 2:
            return Qt::DotLine;
        case 3:
            return Qt::DashDotLine;
        case 4:
            return Qt::DashDotDotLine;
        default:
            return Qt::SolidLine;
    }
}

void KisGridManager::drawGrid(QRect wr, QPainter *p, bool openGL)
{
    KisImageSP image = m_view->canvasSubject()->currentImg();

    if (image) {
        if (m_toggleGrid->isChecked())
        {
            GridDrawer *gridDrawer = 0;

            if (openGL) {
                gridDrawer = new OpenGLGridDrawer();
            } else {
                Q_ASSERT(p);

                if (p) {
                    gridDrawer = new QPainterGridDrawer(p);
                }
            }

            Q_ASSERT(gridDrawer != 0);

            if (gridDrawer) {
                gridDrawer->drawGrid(image, wr);
                delete gridDrawer;
            }
        }
    }
}

void KisGridManager::GridDrawer::drawGrid(KisImageSP image, const QRect& wr)
{
    KisConfig cfg;
    
    Q_UINT32 offsetx = cfg.getGridOffsetX();
    Q_UINT32 offsety = cfg.getGridOffsetY();
    Q_UINT32 hspacing = cfg.getGridHSpacing();
    Q_UINT32 vspacing = cfg.getGridVSpacing();
    Q_UINT32 subdivision = cfg.getGridSubdivisions() - 1;
    //double ihspsub = hspacing / (double)subdivision;
    //double ivspsub = hspacing / (double)subdivision;

    Q_INT32 imageWidth = image->width();
    Q_INT32 imageHeight = image->height();

    // Draw vertical line
    QPen mainPen = QPen ( cfg.getGridMainColor(), 1, gs2style( cfg.getGridMainStyle() ) );
    QPen subdivisionPen = QPen ( cfg.getGridSubdivisionColor(), 1, gs2style( cfg.getGridSubdivisionStyle() ) );
    Q_UINT32 i = 0;
    for( Q_INT32 x = offsetx; x <= wr.right(); x +=hspacing)
    {
        if( i == subdivision )
        {
            setPen(mainPen);
            i = 0;
        } else {
            setPen(subdivisionPen);
            i++;
        }
        if( x >= wr.x() )
        {
            // Always draw the full line otherwise the line stippling varies
            // with the location of wr and we get glitchy patterns.
            drawLine(x, 0, x, imageHeight);
        }
    }
    // Draw horizontal line
    i = 0;
    for( Q_INT32 y = offsety; y <= wr.bottom(); y +=vspacing)
    {
        if( i == subdivision )
        {
            setPen(mainPen);
            i = 0;
        } else {
            setPen(subdivisionPen);
            i++;
        }
        if( y >= wr.y() )
        {
            drawLine(0, y, imageWidth, y);
        }
    }
}

KisGridManager::OpenGLGridDrawer::OpenGLGridDrawer()
{
#ifdef HAVE_GL
    glPushAttrib(GL_ALL_ATTRIB_BITS);
#endif
}

KisGridManager::OpenGLGridDrawer::~OpenGLGridDrawer()
{
#ifdef HAVE_GL
    glPopAttrib();
#endif
}

void KisGridManager::OpenGLGridDrawer::setPen(const QPen& pen)
{
#ifdef HAVE_GL
    Qt::PenStyle penStyle = pen.style();

    if (penStyle == Qt::SolidLine) {
        glDisable(GL_LINE_STIPPLE);
    } else {
        GLushort lineStipple;

        switch (penStyle) {
        case Qt::NoPen:
            lineStipple = 0;
            break;
        default:
        case Qt::SolidLine:
            lineStipple = 0xffff;
            break;
        case Qt::DashLine:
            lineStipple = 0x3fff;
            break;
        case Qt::DotLine:
            lineStipple = 0x3333;
            break;
        case Qt::DashDotLine:
            lineStipple = 0x33ff;
            break;
        case Qt::DashDotDotLine:
            lineStipple = 0x333f;
            break;
        }

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, lineStipple);
    }

    QColor penColor = pen.color();

    glColor3ub(penColor.red(), penColor.green(), penColor.blue());
#else
    Q_UNUSED(pen);
#endif
}

void KisGridManager::OpenGLGridDrawer::drawLine(Q_INT32 x1, Q_INT32 y1, Q_INT32 x2, Q_INT32 y2)
{
#ifdef HAVE_GL
    glBegin(GL_LINES);
    glVertex2i(x1, y1);
    glVertex2i(x2, y2);
    glEnd();
#else
    Q_UNUSED(x1);
    Q_UNUSED(y1);
    Q_UNUSED(x2);
    Q_UNUSED(y2);
#endif
}

