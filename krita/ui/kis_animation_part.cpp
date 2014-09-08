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

#include "kis_animation_part.h"
#include "kis_part2.h"
#include "kis_config.h"
#include "kis_animation_selector.h"
#include "kis_animation_factory.h"
#include "kis_animation_doc.h"

KisAnimationPart::KisAnimationPart(QObject* parent)
    : KisPart2(parent), m_animation(0)
{
    //setComponentData(KisAnimationFactory::componentData(), false);
}

KisAnimationPart::~KisAnimationPart()
{
    delete m_animation;
}

QList<KoPart::CustomDocumentWidgetItem> KisAnimationPart::createCustomDocumentWidgets(QWidget *parent)
{
    KisConfig cfg;
    int w = cfg.defImageWidth();
    int h = cfg.defImageHeight();

    QList<KoPart::CustomDocumentWidgetItem> widgetlist;

    KoPart::CustomDocumentWidgetItem item;
    item.widget = new KisAnimationSelector(parent, qobject_cast<KisAnimationDoc*>(document()), w, h, cfg.defImageResolution(),cfg.defColorModel(), cfg.defaultColorDepth(), cfg.defColorProfile(),
                                           i18n("untitled-animation"));

    item.title = i18n("Animation");
    item.icon = "tool-animator";
    widgetlist << item;
    return widgetlist;
}

void KisAnimationPart::setAnimation(KisAnimation *animation)
{
    m_animation = animation;
}
