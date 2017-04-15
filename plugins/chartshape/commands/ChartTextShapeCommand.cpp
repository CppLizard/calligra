/* This file is part of the KDE project
 * Copyright 2017 Dag Andersen <danders@get2net.dk>
 * Copyright 2012 Brijesh Patel <brijesh3105@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "ChartTextShapeCommand.h"

// KF5
#include <klocalizedstring.h>

// Calligra
#include "KoShape.h"
#include "KoShapeMoveCommand.h"
#include "KoShapeSizeCommand.h"

// KoChart
#include "ChartShape.h"
#include "ChartLayout.h"
#include "PlotArea.h"
#include "ChartDebug.h"

using namespace KoChart;

ChartTextShapeCommand::ChartTextShapeCommand(KoShape* textShape, ChartShape *chart, bool isVisible, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_textShape(textShape)
    , m_chart(chart)
    , m_oldIsVisible(textShape->isVisible())
    , m_newIsVisible(isVisible)
{
    Q_ASSERT(m_oldIsVisible != m_newIsVisible);

    init();

    if (m_newIsVisible) {
        setText(kundo2_i18n("Show Text Shape"));
    } else {
        setText(kundo2_i18n("Hide Text Shape"));
    }
}

ChartTextShapeCommand::~ChartTextShapeCommand()
{
}

void ChartTextShapeCommand::redo()
{
    if (m_oldIsVisible == m_newIsVisible) {
        return;
    }
    // Actually do the work
    KUndo2Command::redo();
    m_textShape->setVisible(m_newIsVisible); // after redo()
    m_chart->update();
}

void ChartTextShapeCommand::undo()
{
    if (m_oldIsVisible == m_newIsVisible) {
        return;
    }
    KUndo2Command::undo();
    m_textShape->setVisible(m_oldIsVisible); // after undo()
    m_chart->update();
}

void ChartTextShapeCommand::init()
{
    const QMap<KoShape*, QRectF> map = m_chart->layout()->calculateLayout(m_textShape, m_newIsVisible);
    QVector<QPointF> oldpositions;
    QVector<QPointF> newpositions;
    QVector<QSizeF> oldsizes;
    QVector<QSizeF> newsizes;
    QList<KoShape*> positionshapes;
    QList<KoShape*> sizeshapes;
    QMap<KoShape*, QRectF>::const_iterator it = map.constBegin();
    for (; it != map.constEnd(); ++it) {
        if (it.key()->position() != it.value().topLeft()) {
            positionshapes << it.key();
            oldpositions << it.key()->position();
            newpositions << it.value().topLeft();
        }
        if (it.key()->size() != it.value().size()) {
            sizeshapes << it.key();
            oldsizes << it.key()->size();
            newsizes << it.value().size();
        }
    }
    if (!positionshapes.isEmpty()) {
        new KoShapeMoveCommand(positionshapes, oldpositions, newpositions, this);
    }
    if (!sizeshapes.isEmpty()) {
        new KoShapeSizeCommand(sizeshapes, oldsizes, newsizes, this);
    }
}
