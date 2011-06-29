/*
 *  Copyright (c) 2005 Adrian Page <adrian@pagenet.plus.com>
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

#include "kis_tool_shape.h"

#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>

#include <KoCanvasBase.h>
#include <KoShapeController.h>
#include <KoColorBackground.h>
#include <KoPatternBackground.h>

#include <kis_debug.h>
#include <klocale.h>

#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <recorder/kis_recorded_paint_action.h>
#include <KoShape.h>

KisToolShape::KisToolShape(KoCanvasBase * canvas, const QCursor & cursor)
        : KisToolPaint(canvas, cursor)
{
    m_shapeOptionsWidget = 0;
}

KisToolShape::~KisToolShape()
{
}

int KisToolShape::flags() const
{
    return KisTool::FLAG_USES_CUSTOM_COMPOSITEOP|KisTool::FLAG_USES_CUSTOM_PRESET;
}

QWidget * KisToolShape::createOptionWidget()
{
    QWidget * optionWidget = KisToolPaint::createOptionWidget();

    m_shapeOptionsWidget = new WdgGeometryOptions(0);
    Q_CHECK_PTR(m_shapeOptionsWidget);

    m_shapeOptionsWidget->cmbFill->setParent(optionWidget);
    m_shapeOptionsWidget->cmbFill->move(QPoint(0, 0));
    m_shapeOptionsWidget->cmbFill->show();
    m_shapeOptionsWidget->textLabelFill->setParent(optionWidget);
    m_shapeOptionsWidget->textLabelFill->move(QPoint(0, 0));
    m_shapeOptionsWidget->textLabelFill->show();
    addOptionWidgetOption(m_shapeOptionsWidget->cmbFill, m_shapeOptionsWidget->textLabelFill);

    m_shapeOptionsWidget->cmbFill->setCurrentIndex(KisPainter::FillStyleNone);

    m_shapeOptionsWidget->cmbOutline->setParent(optionWidget);
    m_shapeOptionsWidget->cmbOutline->move(QPoint(0, 0));
    m_shapeOptionsWidget->cmbOutline->show();
    m_shapeOptionsWidget->textLabelOutline->setParent(optionWidget);
    m_shapeOptionsWidget->textLabelOutline->move(QPoint(0, 0));
    m_shapeOptionsWidget->textLabelOutline->show();
    addOptionWidgetOption(m_shapeOptionsWidget->cmbOutline, m_shapeOptionsWidget->textLabelOutline);

    m_shapeOptionsWidget->cmbOutline->setCurrentIndex(KisPainter::StrokeStyleBrush);

    return optionWidget;
}

KisPainter::FillStyle KisToolShape::fillStyle(void)
{
    if (m_shapeOptionsWidget) {
        return static_cast<KisPainter::FillStyle>(m_shapeOptionsWidget->cmbFill->currentIndex());
    } else {
        return KisPainter::FillStyleNone;
    }
}

KisPainter::StrokeStyle KisToolShape::strokeStyle(void)
{
    if (m_shapeOptionsWidget) {
        return static_cast<KisPainter::StrokeStyle>(m_shapeOptionsWidget->cmbOutline->currentIndex());
    } else {
        return KisPainter::StrokeStyleNone;
    }
}

void KisToolShape::setupPainter(KisPainter * painter)
{
    KisToolPaint::setupPainter(painter);
    painter->setFillStyle(fillStyle());
    painter->setStrokeStyle(strokeStyle());
}

void KisToolShape::setupPaintAction(KisRecordedPaintAction* action)
{
    KisToolPaint::setupPaintAction(action);
    action->setFillStyle(fillStyle());
    action->setStrokeStyle(strokeStyle());
    action->setGenerator(currentGenerator());
    action->setPattern(currentPattern());
    action->setGradient(currentGradient());
}

void KisToolShape::addShape(KoShape* shape)
{
    KoImageCollection* imageCollection = canvas()->shapeController()->resourceManager()->imageCollection();
    switch(fillStyle()) {
        case KisPainter::FillStyleForegroundColor:
            shape->setBackground(new KoColorBackground(currentFgColor().toQColor()));
            break;
        case KisPainter::FillStyleBackgroundColor:
            shape->setBackground(new KoColorBackground(currentBgColor().toQColor()));
            break;
        case KisPainter::FillStylePattern:
            if (imageCollection) {
                KoPatternBackground* fill = new KoPatternBackground(imageCollection);
                fill->setPattern(currentPattern()->image());
                shape->setBackground(fill);
            } else {
                shape->setBackground(0);
            }
            break;
        case KisPainter::FillStyleNone:
        default:
            shape->setBackground(0);
            break;
    }
    KUndo2Command * cmd = canvas()->shapeController()->addShape(shape);
    canvas()->addCommand(cmd);
}

#include "kis_tool_shape.moc"
