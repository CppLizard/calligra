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

#ifndef ANIMATOR_UPDATER_H
#define ANIMATOR_UPDATER_H

#include <QObject>

#include "animator_manager.h"

class AnimatorUpdater : public QObject
{
    Q_OBJECT

public:
    AnimatorUpdater(AnimatorManager* manager);
    virtual ~AnimatorUpdater();
    
public slots:
    virtual void fullUpdate();
    virtual void fullUpdateLayer(AnimatedLayerSP layer);
    virtual void update(int oldFrame, int newFrame);
    virtual void updateLayer(AnimatedLayerSP layer, int oldFrame, int newFrame);
    
public:
    virtual void playerModeOff();
    virtual void playerModeOn();
    virtual bool playerMode();
    
protected:
    virtual void frameVisible(KisNodeSP frame, bool visible, int opacity);
    virtual void frameVisible(KisNodeSP frame, int opacity);
    virtual void frameUnvisible(KisNodeSP frame);
    
protected:
    AnimatorManager* m_manager;

private:
    bool m_playerMode;
};

#endif // ANIMATOR_UPDATER_H
