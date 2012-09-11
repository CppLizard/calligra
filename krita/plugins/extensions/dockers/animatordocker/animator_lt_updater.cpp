/*
 *
 *  Copyright (C) 2011 Torio Mlshi <mlshi@lavabit.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "animator_lt_updater.h"

#include "animator_switcher.h"

#include "filtered_frame_layer.h"
#include <kis_adjustment_layer.h>

AnimatorLTUpdater::AnimatorLTUpdater(AnimatorManager* manager) : AnimatorUpdater(manager)
{
    m_LT = new AnimatorFilteredLT(manager->image());
    setMode(AnimatorLTUpdater::Disabled);
    connect(m_LT, SIGNAL(opacityChanged(int)), this, SLOT(updateRelFrame(int)));
    connect(m_LT, SIGNAL(visibilityChanged(int)), this, SLOT(updateRelFrame(int)));
    connect(m_LT, SIGNAL(fullUpdate()), this, SLOT(updateRelFrames()));
}

AnimatorLTUpdater::~AnimatorLTUpdater()
{
}

void AnimatorLTUpdater::updateLayer(AnimatedLayerSP layer, int oldFrame, int newFrame)
{
    if (! layer->displayable())
        return;

    if (mode() == AnimatorLTUpdater::Disabled) {
        AnimatorUpdater::updateLayer(layer, oldFrame, newFrame);
        return;
    }

    if (mode() == AnimatorLTUpdater::KeyFramed) {
        warnKrita << "KeyFramed mode is not supported yet";
        AnimatorUpdater::updateLayer(layer, oldFrame, newFrame);
        return;
    }

    if (playerMode()) {
        AnimatorUpdater::updateLayer(layer, oldFrame, newFrame);
        return;
    }

    FramedAnimatedLayerSP framedLayer = qobject_cast<FramedAnimatedLayer*>(layer.data());
    if (!framedLayer) {
        AnimatorUpdater::updateLayer(layer, oldFrame, newFrame);
        return;
    }

    for (int i = -getLT()->getNear(); i <= getLT()->getNear(); ++i) {
        frameUnvisible(framedLayer->frameAt(i+oldFrame));
    }

    for (int i = -getLT()->getNear(); i <= getLT()->getNear(); ++i) {
        if (getLT()->getVisibility(i)) {
            setupFilter(framedLayer->frameAt(i+newFrame), i);
            frameVisible(framedLayer->frameAt(i+newFrame), getLT()->getOpacityU8(i));
        }
    }
}

void AnimatorLTUpdater::updateRelFrames()
{
    // TODO: don't use switcher
    int frame = m_manager->getSwitcher()->currentFrame();
    update(frame, frame);
}

void AnimatorLTUpdater::updateRelFrame(int relFrame)
{
    if (mode() == AnimatorLTUpdater::Disabled)
        return;

    if (mode() == AnimatorLTUpdater::KeyFramed) {
        warnKrita << "KeyFramed mode is not supported yet";
        return;
    }

    int cFrame = m_manager->getSwitcher()->currentFrame();
    QList<AnimatedLayerSP> layers = m_manager->layers();
    AnimatedLayerSP layer;
    foreach (layer, layers) {
        FramedAnimatedLayerSP framedLayer = qobject_cast<FramedAnimatedLayer*>(layer.data());
        if (framedLayer) {
            SimpleFrameLayer *frame = qobject_cast<SimpleFrameLayer*>(framedLayer->frameAt(cFrame+relFrame));
            if (frame) {
                frameVisible(frame, getLT()->getVisibility(relFrame), getLT()->getOpacityU8(relFrame));
            }
        }
    }
}

void AnimatorLTUpdater::setupFilter(FrameLayer* frame, int rel)
{
    FilteredFrameLayer *fframe = qobject_cast<FilteredFrameLayer*>(frame);
    if (fframe) {
        if (filter(rel)) {
            fframe->setFilter(qobject_cast<KisAdjustmentLayer*>(filter(rel)->clone().data()));
        } else if (!filter(rel) && fframe->filter()) {
            fframe->setFilter(0);
        }
    }
}


AnimatorFilteredLT* AnimatorLTUpdater::getLT()
{
    return m_LT;
}


AnimatorLTUpdater::LTUpdaterMode AnimatorLTUpdater::mode() const
{
    return m_mode;
}

void AnimatorLTUpdater::setMode(AnimatorLTUpdater::LTUpdaterMode mode)
{
    m_mode = mode;

    // TODO: don't do full update
    fullUpdate();
    int frame = m_manager->getSwitcher()->currentFrame();
    update(frame, frame);
}


KisAdjustmentLayerSP AnimatorLTUpdater::filter(int relFrame)
{
    AnimatorFilteredLT *flt = qobject_cast<AnimatorFilteredLT*>(getLT());
    if (flt) {
        return flt->filter(relFrame);
    }
    return 0;
}
