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


#ifndef KIS_ANIMATION_DOC_H
#define KIS_ANIMATION_DOC_H

#include "kis_doc2.h"
#include "kis_animation_part.h"
#include "kranimstore/kis_animation_store.h"
#include "kis_animation.h"

#include <QHash>

class KisOnionSkinLoader;
class KisKranimLoader;

#define KIS_ANIM_MIME_TYPE "application/x-krita-animation"

/**
 * This class represents te animation document.
 *
 * It handles all the frame and layer events from
 * coming from the timeline and onion skin dockers.
 *
 * It also manages a state of the animation and the
 * animation file on disk
 */
class KRITAUI_EXPORT KisAnimationDoc : public KisDoc2
{
    Q_OBJECT
public:
    KisAnimationDoc();
    virtual ~KisAnimationDoc();
    void frameSelectionChanged(QRect frame, bool savePreviousFrame=true);

    void addKeyFrame(QRect frame);
    void addBlankFrame(QRect frame);
    void breakFrame(QRect frame, bool blank=false);

    void removeFrame(QRect frame);
    void removeLayer(int layer);

    void moveLayerUp(int layer);
    void moveLayerDown(int layer);

    void addPaintLayer();
    void addVectorLayer();

    KisAnimationStore* getStore();
    KisAnimation* getAnimation();

    void loadAnimationFile(KisAnimation* animation, KisAnimationStore* store, QDomDocument doc);

    QRect getParentFramePosition(int frame, int layer);
    QRect getPreviousKeyFramePosition(int frame, int layer);
    QRect getNextKeyFramePosition(int frame, int layer);

    QString getFrameFile(int frame, int layer);
    QString getPreviousKeyFrameFile(int frame, int layer);
    QString getNextKeyFrameFile(int frame, int layer);

    QRect currentFramePosition();
    KisNodeSP currentFrame();

    int numberOfLayers();

    KisKranimLoader* kranimLoader();

    void play();
    void pause();
    void stop();

    void onionSkinStateChanged();

    void addCurrentLoadedLayer(KisLayerSP layer);

    void removePreviousLayers();

    bool storeHasFile(QString location);

    void loadFrame(KisLayerSP layer, QString location);

    QList<int> keyFramePositions();

    void onionSkinStateToggled(QHash<int, bool> states);
    void visibilityStateToggled(QHash<int, bool> states);
    void lockStateToggled(QHash<int, bool> states);

public slots:
    void setImageModified();

    void playbackStateChanged();

private:
    void preSaveAnimation();

    void addFrameToXML();

    void deleteFrameFromXML(int frame, int layer);

    void deleteLayerFromXML(int layer);

    void saveXMLToDisk();

    QDomNode getFrameElementFromXML(int frame, int layer);

    void updateActiveFrame();

    void loadOnionSkins();

    void applyLayerStates(int layerNumber, KisLayerSP layer);

private:
    class KisAnimationDocPrivate;
    KisAnimationDocPrivate* const d;

signals:
    void sigFrameModified();
    void sigImportFinished(QHash<int, QList<QRect> >);
};

#endif // KIS_ANIMATION_DOC_H
