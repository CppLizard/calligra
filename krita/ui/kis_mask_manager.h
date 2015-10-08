/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
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
#ifndef KIS_MASK_MANAGER
#define KIS_MASK_MANAGER

#include <QObject>
#include <QPointer>

#include "kis_types.h"
#include "KisView.h"

class KisViewManager;
class KActionCollection;
class KisNodeCommandsAdapter;
class KisActionManager;

#include "kis_mask.h"

/**
 * Handle the gui for manipulating masks.
 */
class KisMaskManager : public QObject
{

    Q_OBJECT

public:


    KisMaskManager(KisViewManager * view);
    ~KisMaskManager() {}
    void setView(QPointer<KisView>view);

private:
    
    friend class KisNodeManager;
    
    void setup(KActionCollection * actionCollection, KisActionManager *actionManager);

    void updateGUI();
    
    /**
     * @return the paint device associated with the currently
     *         active mask, if there is one.
     */
    KisPaintDeviceSP activeDevice();

    /**
     * @return the active mask, if there is one
     */
    KisMaskSP activeMask();

    /**
     * Create an exact duplicate of the current mask.
     */
    void duplicateMask();

    /**
     * Delete the mask
     */
    void removeMask();

    /**
     * Move the mask one up in the mask stack. If we reach the top
     * of the stack, try to move the mask to the layer on top of the
     * active layer.
     */
    void raiseMask();

    /**
     * Move the mask one down in the mask stack. If we reach the bottom
     * of the stack, try to move the mask to the layer beneath the
     * active layer.
     */
    void lowerMask();

    /**
     * Move the mask to the top of the mask stack of the active layer
     */
    void maskToTop();

    /**
     * Move the mask to the bottom of the mask stack of the active
     * layer
     */
    void maskToBottom();

    /**
     * Show the mask properties dialog
     */
    void maskProperties();

    /**
     * called whenever the mask stack is updated to enable/disable all
     * menu items
     */
    void masksUpdated();

    /**
     * Activate a new mask. There can be only one mask active per
     * view; and if the mask is active, it becomes the paint device.
     */
    void activateMask(KisMaskSP mask);

    void adjustMaskPosition(KisNodeSP node, KisNodeSP activeNode, bool avoidActiveNode, KisNodeSP &parent, KisNodeSP &above);
    void createMaskCommon(KisMaskSP mask, KisNodeSP activeNode, KisPaintDeviceSP copyFrom, const KUndo2MagicString &macroName, const QString &nodeType, const QString &nodeName, bool suppressSelection, bool avoidActiveNode);

    void createSelectionMask(KisNodeSP activeNode, KisPaintDeviceSP copyFrom, bool avoidActiveNode);
    void createFilterMask(KisNodeSP activeNode, KisPaintDeviceSP copyFrom, bool quiet, bool avoidActiveNode);
    void createTransformMask(KisNodeSP activeNode);
    void createTransparencyMask(KisNodeSP activeNode, KisPaintDeviceSP copyFrom, bool avoidActiveNode);

    KisViewManager * m_view;
    QPointer<KisView>m_imageView;
    KisNodeCommandsAdapter* m_commandsAdapter;

};

#endif // KIS_MASK_MANAGER
