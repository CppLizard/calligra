/* This file is part of the KDE project
   Copyright 2012 Brijesh Patel <brijesh3105@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "AxisCommand.h"

// KF5
#include <klocalizedstring.h>

// KoChart
#include "Axis.h"
#include "ChartDebug.h"
#include "ChartTextShapeCommand.h"

using namespace KoChart;
using namespace KChart;


AxisCommand::AxisCommand(Axis* axis, ChartShape* chart)
    : m_chart(chart)
    , m_axis(axis)
{
    m_newShowMajorGridLines = m_axis->showMajorGrid();
    m_newShowMinorGridLines = m_axis->showMinorGrid();
    m_newUseLogarithmicScaling = m_axis->scalingIsLogarithmic();
    m_newLabelsFont = m_axis->font();
    m_newShowAxis = m_axis->isVisible();
}

AxisCommand::~AxisCommand()
{
}

void AxisCommand::redo()
{
    // save the old type
    m_oldShowMajorGridLines = m_axis->showMajorGrid();
    m_oldShowMinorGridLines = m_axis->showMinorGrid();
    m_oldUseLogarithmicScaling = m_axis->scalingIsLogarithmic();
    m_oldLabelsFont = m_axis->font();
    m_oldShowAxis = m_axis->isVisible();
    /*m_oldStepWidth = m_axis->majorInterval();
    m_oldSubStepWidth = m_axis->minorInterval();
    m_oldUseAutomaticStepWidth = m_axis->useAutomaticMajorInterval();
    m_oldUseAutomaticSubStepWidth = m_axis->useAutomaticMinorInterval();*/

    KUndo2Command::redo();

    if (m_oldShowMajorGridLines == m_newShowMajorGridLines && m_oldShowMinorGridLines == m_newShowMinorGridLines
        && m_oldUseLogarithmicScaling == m_newUseLogarithmicScaling && m_oldLabelsFont == m_newLabelsFont
        && m_oldShowAxis == m_newShowAxis)
        {
            return;
        }

    // Actually do the work
    m_axis->setShowMajorGrid(m_newShowMajorGridLines);
    m_axis->setShowMinorGrid(m_newShowMinorGridLines);
    m_axis->setScalingLogarithmic(m_oldUseLogarithmicScaling);
    m_axis->setFont(m_newLabelsFont);
    m_axis->setFontSize(m_newLabelsFont.pointSize());
    m_axis->setVisible(m_newShowAxis);
    /*m_axis->setMajorInterval(m_newStepWidth);
    m_axis->setMinorInterval(m_newSubStepWidth);
    m_axis->setUseAutomaticMajorInterval(m_newUseAutomaticStepWidth);
    m_axis->setUseAutomaticMinorInterval(m_newUseAutomaticSubStepWidth);*/

    m_chart->update();
}

void AxisCommand::undo()
{
    KUndo2Command::undo();
    if (m_oldShowMajorGridLines == m_newShowMajorGridLines && m_oldShowMinorGridLines == m_newShowMinorGridLines
        && m_oldUseLogarithmicScaling == m_newUseLogarithmicScaling && m_oldLabelsFont == m_newLabelsFont
        && m_oldShowAxis == m_newShowAxis)
    {
        return;
    }
    m_axis->setShowMajorGrid(m_oldShowMajorGridLines);
    m_axis->setShowMinorGrid(m_oldShowMinorGridLines);
    m_axis->setScalingLogarithmic(m_oldUseLogarithmicScaling);
    m_axis->setFont(m_oldLabelsFont);
    m_axis->setFontSize(m_oldLabelsFont.pointSize());
    m_axis->setVisible(m_oldShowAxis);
    /*m_axis->setMajorInterval(m_oldStepWidth);
    m_axis->setMinorInterval(m_oldSubStepWidth);
    m_axis->setUseAutomaticMajorInterval(m_oldUseAutomaticStepWidth);
    m_axis->setUseAutomaticMinorInterval(m_oldUseAutomaticSubStepWidth);*/

    m_chart->update();
}

void AxisCommand::setAxisShowTitle(bool show)
{
    if (show) {
        setText(kundo2_i18n("Show Axis Title"));
    } else {
        setText(kundo2_i18n("Hide Axis Title"));
    }
    new ChartTextShapeCommand(m_axis->title(), m_chart, show, this);
}

void AxisCommand::setShowAxis(bool show)
{
    m_newShowAxis = show;

    if (show) {
        setText(kundo2_i18n("Show Axis"));
    } else {
        setText(kundo2_i18n("Hide Axis"));
    }
}

void AxisCommand::setAxisShowMajorGridLines(bool show)
{
    m_newShowMajorGridLines = show;

    if (show) {
        setText(kundo2_i18n("Show Axis Major Gridlines"));
    } else {
        setText(kundo2_i18n("Hide Axis Major Gridlines"));
    }
}

void AxisCommand::setAxisShowMinorGridLines(bool show)
{
    m_newShowMinorGridLines = show;

    if (show) {
        setText(kundo2_i18n("Show Axis Minor Gridlines"));
    } else {
        setText(kundo2_i18n("Hide Axis Minor Gridlines"));
    }
}

void AxisCommand::setAxisUseLogarithmicScaling(bool b)
{
    m_newUseLogarithmicScaling = b;

    if (b) {
        setText(kundo2_i18n("Logarithmic Scaling"));
    } else {
        setText(kundo2_i18n("Linear Scaling"));
    }
}

void AxisCommand::setAxisStepWidth(qreal width)
{
    m_newStepWidth = width;

    setText(kundo2_i18n("Set Axis Step Width"));
}

void AxisCommand::setAxisSubStepWidth(qreal width)
{
    m_newSubStepWidth = width;

    setText(kundo2_i18n("Set Axis Substep Width"));
}

void AxisCommand::setAxisUseAutomaticStepWidth(bool automatic)
{
    m_newShowMajorGridLines = automatic;

    if (automatic) {
        setText(kundo2_i18n("Set Automatic Step Width"));
    } else {
        setText(kundo2_i18n("Set Manual Step Width"));
    }
}

void AxisCommand::setAxisUseAutomaticSubStepWidth(bool automatic)
{
    m_newShowMinorGridLines = automatic;

    if (automatic) {
        setText(kundo2_i18n("Automatic Substep Width"));
    } else {
        setText(kundo2_i18n("Manual Substep Width"));
    }
}

void AxisCommand::setAxisLabelsFont(const QFont &font)
{
    m_newLabelsFont = font;

    setText(kundo2_i18n("Set Axis Label Font"));
}
