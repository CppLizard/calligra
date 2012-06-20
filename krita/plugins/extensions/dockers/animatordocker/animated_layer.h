/*
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


#ifndef ANIMATED_LAYER_H
#define ANIMATED_LAYER_H

#include "kis_group_layer.h"
#include "kis_node_manager.h"

#include "frame_layer.h"

/**
 *  Animated layer class
 */
class AnimatedLayer : public KisGroupLayer
{
    Q_OBJECT

public:
    AnimatedLayer(KisImageWSP image, const QString& name, quint8 opacity);
    AnimatedLayer(const KisGroupLayer& source);

public:
    virtual QString animationName() const = 0;
    virtual void setAnimationName(const QString& name) = 0;

public:
    virtual FrameLayer* getUpdatedFrame(int num);
    virtual FrameLayer* getCachedFrame(int num) = 0;
    virtual void updateAllFrames();

public:
    virtual void setEnabled(bool val);
    virtual bool enabled() const;

public:
    virtual bool displayable() const = 0;
    virtual bool hasPreview() const = 0;

public:
    /**
     * @return number of first frame with some info
     */
    virtual int dataStart() const = 0;

    /**
     * @return number of last frame with some info+1 [firstFrame; lastFrame)
     */
    virtual int dataEnd() const = 0;

protected:
    virtual void updateFrame(int num);

private:
    bool m_enabled;
};

typedef KisSharedPtr<AnimatedLayer> AnimatedLayerSP;
#endif // ANIMATED_LAYER_H
