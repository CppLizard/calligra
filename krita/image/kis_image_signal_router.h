/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_IMAGE_SIGNAL_ROUTER_H
#define __KIS_IMAGE_SIGNAL_ROUTER_H

#include <QObject>
#include <QVector>
#include "krita_export.h"
#include "kis_types.h"
#include "kis_group_layer.h"


class KoColorSpace;
class KoColorProfile;

enum KisImageSignalType {
    LayersChangedSignal,
    ModifiedSignal,
    SizeChangedSignal,
    ProfileChangedSignal,
    ColorSpaceChangedSignal,
    ResolutionChangedSignal
};

typedef QVector<KisImageSignalType> KisImageSignalVector;

class KRITAIMAGE_EXPORT KisImageSignalRouter : public QObject
{
    Q_OBJECT

public:
    KisImageSignalRouter(KisImageWSP image);
    ~KisImageSignalRouter();

    void emitNotification(KisImageSignalType type);
    void emitNotifications(KisImageSignalVector notifications);

    void emitNodeChanged(KisNode *node);
    void emitAboutToAddANode(KisNode *parent, int index);
    void emitNodeHasBeenAdded(KisNode *parent, int index);
    void emitAboutToRemoveANode(KisNode *parent, int index);
    void emitNodeHasBeenRemoved(KisNode *parent, int index);
    void emitAboutToMoveNode(KisNode *parent, int oldIndex, int newIndex);
    void emitNodeHasBeenMoved(KisNode *parent, int oldIndex, int newIndex);

private:
    bool checkSameThread();

private slots:
    void slotNotification(KisImageSignalType type);

signals:

    void sigNotification(KisImageSignalType type);

    // Notifications
    void sigLayersChanged(KisGroupLayerSP rootLayer);
    void sigPostLayersChanged(KisGroupLayerSP rootLayer);

    void sigImageModified();

    void sigSizeChanged(qint32 w, qint32 h);
    void sigProfileChanged(const KoColorProfile *  profile);
    void sigColorSpaceChanged(const KoColorSpace*  cs);
    void sigResolutionChanged(double xRes, double yRes);

    // Synchronous
    void sigNodeChanged(KisNode *node);
    void sigAboutToAddANode(KisNode *parent, int index);
    void sigNodeHasBeenAdded(KisNode *parent, int index);
    void sigAboutToRemoveANode(KisNode *parent, int index);
    void sigNodeHasBeenRemoved(KisNode *parent, int index);
    void sigAboutToMoveNode(KisNode *parent, int oldIndex, int newIndex);
    void sigNodeHasBeenMoved(KisNode *parent, int oldIndex, int newIndex);

private:
    KisImageWSP m_image;
};

#endif /* __KIS_IMAGE_SIGNAL_ROUTER_H */
