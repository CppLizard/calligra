/* This file is part of the KDE project
 * Copyright 2017 Dag Andersen <danders@get2net.dk>
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

#include "AddRemoveAxisCommand.h"

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
#include "Axis.h"
#include "ChartTextShapeCommand.h"
#include "ChartDebug.h"

using namespace KoChart;

AddRemoveAxisCommand::AddRemoveAxisCommand(Axis *axis, ChartShape *chart, bool add, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_axis(axis)
    , m_chart(chart)
    , m_add(add)
    , mine(add)
{
    if (add) {
        setText(kundo2_i18n("Add Axis"));
        initAdd();
    } else {
        setText(kundo2_i18n("Remove Axis"));
        initRemove();
    }
}

AddRemoveAxisCommand::~AddRemoveAxisCommand()
{
    if (mine) {
        m_chart->layout()->remove(m_axis->title()); // to be safe; this should be handled elsewhere
        delete m_axis;
    }
}

void AddRemoveAxisCommand::redo()
{
    mine = !mine;
    if (m_add) {
        m_axis->plotArea()->addAxis(m_axis);
    } else {
        m_axis->plotArea()->takeAxis(m_axis);
    }
    KUndo2Command::redo();
    m_chart->update();
}

void AddRemoveAxisCommand::undo()
{
    mine = !mine;
    if (m_add) {
        m_axis->plotArea()->takeAxis(m_axis);
    } else {
        m_axis->plotArea()->addAxis(m_axis);
    }
    KUndo2Command::undo();
    m_chart->update();
}

void AddRemoveAxisCommand::initAdd()
{
    m_axis->title()->setVisible(false);
    if (!m_axis->titleText().isEmpty()) {
        new ChartTextShapeCommand(m_axis->title(), m_chart, true, this);
    }
}

void AddRemoveAxisCommand::initRemove()
{
    if (m_axis->title()->isVisible()) {
        new ChartTextShapeCommand(m_axis->title(), m_chart, false, this);
    }
}
