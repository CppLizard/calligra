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

#include "animator_dock.h"

#include <klocale.h>

#include "kis_view2.h"
#include "kis_animation.h"
#include "kis_canvas2.h"
#include "kis_animation_doc.h"
#include "kis_animation_part.h"
#include "kis_timeline.h"

AnimatorDock::AnimatorDock() : QDockWidget(i18n("Animator")), m_canvas(0), m_animation(0)
{
    this->setMinimumHeight(120);
    m_mainWidget = new KisTimeline(this);
    this->setWidget(m_mainWidget);
}

void AnimatorDock::setCanvas(KoCanvasBase *canvas)
{
    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
    if(m_canvas && m_canvas->view() && m_canvas->view()->document() && m_canvas->view()->document()->documentPart()) {
        m_animation = dynamic_cast<KisAnimationPart*>(m_canvas->view()->document()->documentPart())->animation();
        if(m_animation) {
            m_mainWidget->setCanvas(m_canvas);
            m_mainWidget->setModel(m_animation);
        }
    }
}

#include "animator_dock.moc"
