/* This file is part of the KDE project
   Copyright 2009 Vera Lukman <shicmap@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KIS_FAVORITE_RESOURCE_MANAGER_H
#define KIS_FAVORITE_RESOURCE_MANAGER_H

#include <QObject>
#include <kis_types.h>
#include <QQueue>
#include <QList>
#include "KoResourceServerObserver.h"

#include <KoColor.h>

class QString;
class QStringList;
class QToolButton;
class QPoint;
class KoID;
class KisPaintopBox;
class KisPaletteManager;
class KisView2;
class KisPaintOpPreset;

class KisFavoriteResourceManager : public QObject, public KoResourceServerObserver<KisPaintOpPreset>
{
    Q_OBJECT

public:

    KisFavoriteResourceManager(KisPaintopBox *paintopBox);
    ~KisFavoriteResourceManager();

    void setPopupPalette(QWidget *palette);

    virtual void unsetResourceServer();

    QList<QImage> favoritePresetImages();

    void setCurrentTag(const QString& tagName);

    int numFavoritePresets();

    QVector<KisPaintOpPreset*> favoritePresetList();

    int recentColorsTotal();
    const KoColor& recentColorAt(int pos);

    // Reimplemented from KoResourceServerObserver
    virtual void removingResource(KisPaintOpPreset* resource);
    virtual void resourceAdded(KisPaintOpPreset* resource);
    virtual void resourceChanged(KisPaintOpPreset* resource);
    virtual void syncTaggedResourceView();
    virtual void syncTagAddition(const QString& tag);
    virtual void syncTagRemoval(const QString& tag);

    //BgColor;
    KoColor bgColor() const;

    /**
     * Set palette to block updates, paintops won't be deleted when they are deleted from server
     * Used when overwriting a resource
     */
    void setBlockUpdates(bool block);

signals:

    void sigSetFGColor(const KoColor& c);
    void sigSetBGColor(const KoColor& c);

    // This is a flag to handle a bug:
    // If pop up palette is visible and a new colour is selected, the new colour
    // will be added when the user clicks on the canvas to hide the palette
    // In general, we want to be able to store recent colours if the pop up palette
    // is not visible
    void sigEnableChangeColor(bool b);

    void sigChangeFGColorSelector(const KoColor&);

    void setSelectedColor(int);

    void updatePalettes();

    void hidePalettes();

public slots:

    void slotChangeActivePaintop(int);

    /*update the priority of a colour in m_colorList, used only by m_popupPalette*/
    void slotUpdateRecentColor(int);

    /*add a colour to m_colorList, used by KisCanvasResourceProvider*/
    void slotAddRecentColor(const KoColor&);

    void slotChangeFGColorSelector(KoColor c);

    void slotSetBGColor(const KoColor c);

private slots:
    void updateFavoritePresets();

private:
    KisPaintopBox *m_paintopBox;

    QVector<KisPaintOpPreset*> m_favoritePresetsList;

    class ColorDataList;
    ColorDataList *m_colorList;

    bool m_blockUpdates;

    void saveFavoritePresets();

    KoColor m_bgColor;
    QString m_currentTag;
};

#endif
