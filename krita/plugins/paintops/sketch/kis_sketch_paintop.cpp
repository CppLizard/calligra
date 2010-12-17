/*
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (c) 2010 Ricardo Cabello <hello@mrdoob.com>
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

#include "kis_sketch_paintop.h"
#include "kis_sketch_paintop_settings.h"

#include <cmath>
#include <QRect>

#include <KoColor.h>
#include <KoColorSpace.h>

#include <kis_image.h>
#include <kis_debug.h>

#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_types.h>
#include <kis_paintop.h>
#include <kis_paint_information.h>

#include <kis_pressure_opacity_option.h>

/*
* Based on Harmony project http://github.com/mrdoob/harmony/
*/
// chrome : diff 0.2, sketchy : 0.3, fur: 0.5
// fur : distance / thresholdDistance

// shaded: opacity per line :/
// ((1 - (d / 1000)) * 0.1 * BRUSH_PRESSURE), offset == 0
// chrome: color per line :/
//this.context.strokeStyle = "rgba(" + Math.floor(Math.random() * COLOR[0]) + ", " + Math.floor(Math.random() * COLOR[1]) + ", " + Math.floor(Math.random() * COLOR[2]) + ", " + 0.1 * BRUSH_PRESSURE + " )";

// long fur
// from: count + offset * -random
// to: i point - (offset * -random)  + random * 2
// probability distance / thresholdDistnace

// shaded: probabity : paint always - 0.0 density

KisSketchPaintOp::KisSketchPaintOp(const KisSketchPaintOpSettings *settings, KisPainter * painter, KisImageWSP image)
        : KisPaintOp(painter)
{
    Q_UNUSED(image);
    m_opacityOption.readOptionSetting(settings);
    m_sizeOption.readOptionSetting(settings);
    m_rotationOption.readOptionSetting(settings);
    m_sketchProperties.readOptionSetting(settings);
    m_brushOption.readOptionSetting(settings);

    m_brush = m_brushOption.brush();
    
    m_opacityOption.sensor()->reset();
    m_sizeOption.sensor()->reset();
    m_rotationOption.sensor()->reset();

    m_painter = 0;
    m_count = 0;
}

KisSketchPaintOp::~KisSketchPaintOp()
{
    delete m_painter;
}

void KisSketchPaintOp::drawConnection(const QPointF& start, const QPointF& end)
{
    if (m_sketchProperties.lineWidth == 1){
        m_painter->drawThickLine(start, end, m_sketchProperties.lineWidth,m_sketchProperties.lineWidth);
    }else{
        m_painter->drawLine(start, end, m_sketchProperties.lineWidth, true);
    }
   
}

void KisSketchPaintOp::updateBrushMask(const KisPaintInformation& info, qreal scale, qreal rotation){
    m_maskDab = cachedDab(m_dab->colorSpace());
    
    if (m_brush->brushType() == IMAGE || m_brush->brushType() == PIPE_IMAGE) {
        m_maskDab = m_brush->paintDevice(m_dab->colorSpace(), scale, rotation, info, 0.0, 0.0);
    } else {
        KoColor color = painter()->paintColor();
        color.convertTo(m_maskDab->colorSpace());
        m_brush->mask(m_maskDab, color, scale, scale, rotation, info, 0.0, 0.0);
    }

    // update bounding box
    m_brushBoundingBox = m_maskDab->bounds();
    m_hotSpot = m_brush->hotSpot(scale,scale,rotation);
    m_brushBoundingBox.translate(info.pos() - m_hotSpot);
}

KisDistanceInformation KisSketchPaintOp::paintLine(const KisPaintInformation& pi1, const KisPaintInformation& pi2, const KisDistanceInformation& savedDist)
{
    Q_UNUSED(savedDist);
    if (!painter()) return KisDistanceInformation();

    if (!m_dab) {
        m_dab = new KisPaintDevice(painter()->device()->colorSpace());
        m_painter = new KisPainter(m_dab);
    } else {
        m_dab->clear();
    }

    QPointF prevMouse = pi1.pos();
    QPointF mousePosition = pi2.pos();
    m_points.append(mousePosition);


    // shaded: does not draw this line, chrome does, fur does
    if (m_sketchProperties.makeConnection){
        drawConnection(prevMouse,mousePosition);
    }

    double scale = m_sizeOption.apply(pi2);
    double rotation = m_rotationOption.apply(pi2);
    
    setCurrentScale(scale);
    setCurrentRotation(rotation);
    
    qreal thresholdDistance;
    
    // update the mask for simple mode only once
    // determine the radius
    if (m_count == 0 && m_sketchProperties.simpleMode){
        updateBrushMask(pi2,1.0,0.0);
        //m_radius = qMax(m_maskDab->bounds().width(),m_maskDab->bounds().height()) * 0.5;
        m_radius = 0.5 * qMax(m_brush->width(), m_brush->height());
    }
    
    if (!m_sketchProperties.simpleMode){
        updateBrushMask(pi2,scale,rotation);
        m_radius = qMax(m_maskDab->bounds().width(),m_maskDab->bounds().height()) * 0.5;
        thresholdDistance = pow(m_radius,2);
    }

    if (m_sketchProperties.simpleMode){
        // update the radius according scale in simple mode
        thresholdDistance = pow(m_radius * scale,2);
    }    

    // determine density
    qreal density = thresholdDistance * m_sketchProperties.probability;
    
    // probability behaviour
    // TODO: make this option
    qreal probability = 1.0 - m_sketchProperties.probability;

    QColor painterColor = painter()->paintColor().toQColor();
    QColor randomColor;
    KoColor color(m_dab->colorSpace());
    
    int w = m_maskDab->bounds().width();
    quint8 opacityU8 = 0;
    quint8 * pixel;
    qreal distance;
    QPoint  positionInMask;
    QPointF diff;

    m_painter->setPaintColor( painter()->paintColor() );
    int size = m_points.size();
    // MAIN LOOP
    for (int i = 0; i < size; i++) {
        diff = m_points.at(i) - mousePosition;
        distance = diff.x() * diff.x() + diff.y() * diff.y();
        
        // circle test
        bool makeConnection = false;
        if (m_sketchProperties.simpleMode){
            if (distance < thresholdDistance){
                makeConnection = true;
            }
        // mask test
        }else{
            
            if ( m_brushBoundingBox.contains( m_points.at(i) ) ){
                positionInMask = (diff + m_hotSpot).toPoint();
                pixel = m_maskDab->data() + ((positionInMask.y() * w + positionInMask.x()) * m_maskDab->pixelSize());
                opacityU8 = m_maskDab->colorSpace()->opacityU8( pixel );
                if (opacityU8 != 0){
                    makeConnection = true;
                }
            }
            
        }
        
        if (!makeConnection){
            // check next point
            continue;
        }

        if (m_sketchProperties.distanceDensity){
            probability =  distance / density;
        }

        // density check
        if (drand48() >= probability) {
            QPointF offsetPt = diff * m_sketchProperties.offset;
            
            if (m_sketchProperties.randomRGB){
                // some color transformation per line goes here
                randomColor.setRgbF(drand48() * painterColor.redF(),
                                    drand48() * painterColor.greenF(),
                                    drand48() * painterColor.blueF()
                                    );
                color.fromQColor(randomColor);
                m_painter->setPaintColor(color);
            }
            
            // distance based opacity
            quint8 opacity = OPACITY_OPAQUE_U8;
            if (m_sketchProperties.distanceOpacity){ 
                opacity *= qRound((1.0 - (distance / thresholdDistance)));
            }
            
            if (m_sketchProperties.randomOpacity){
                opacity *= drand48();
            }
            
            m_painter->setOpacity(opacity);
            
            if (m_sketchProperties.magnetify) {
                drawConnection(mousePosition + offsetPt, m_points.at(i) - offsetPt);
            }else{
                drawConnection(mousePosition + offsetPt, mousePosition - offsetPt);
            }

            
            
        }
    }// end of MAIN LOOP

    m_count++;

    QRect rc = m_dab->extent();
    quint8 origOpacity = m_opacityOption.apply(painter(), pi2);

    painter()->bitBlt(rc.x(), rc.y(), m_dab, rc.x(), rc.y(), rc.width(), rc.height());
    renderMirrorMask(rc, m_dab);
    painter()->setOpacity(origOpacity);

    KisVector2D end = toKisVector2D(pi2.pos());
    KisVector2D start = toKisVector2D(pi1.pos());
    KisVector2D dragVec = end - start;

    return KisDistanceInformation(0, dragVec.norm());
}



qreal KisSketchPaintOp::paintAt(const KisPaintInformation& info)
{
    return paintLine(info, info).spacing;
}
