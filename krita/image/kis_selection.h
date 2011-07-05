/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef KIS_SELECTION_H_
#define KIS_SELECTION_H_

#include <QRect>

#include "kis_types.h"
#include "krita_export.h"
#include "kis_paint_device.h"

enum selectionMode {
    PIXEL_SELECTION,
    SHAPE_PROTECTION
};

enum selectionAction {
    SELECTION_REPLACE,
    SELECTION_ADD,
    SELECTION_SUBTRACT,
    SELECTION_INTERSECT
};

#include "kis_pixel_selection.h"


class KisSelectionComponent;

/**
 * KisSelection is a compisite object. It may contain an instance
 * of KisPixelSelection and a KisShapeSelection object. Both these
 * selections are merged into a projection of the KisSelection.
 *
 * Every pixel in the paint device can indicate a degree of selectedness, varying
 * between MIN_SELECTED and MAX_SELECTED.
 *
 * The projection() paint device itself is only a projection: you can
 * read from it, but not write to it. You need to keep track of
 * the need for updating the projection yourself: there is no
 * automatic updating after changing the contents of one or more
 * of the selection components. 
 */
class KRITAIMAGE_EXPORT KisSelection : public KisShared
{

public:
    /**
     * Create a new KisSelection.
     *
     * @param defaultBounds defines the bounds of the selection when
     * Select All is initiated.
     */
    KisSelection(KisDefaultBoundsSP defaultBounds = new KisDefaultBounds());

    /**
     * Copy the selection. The selection components are copied, too.
     */
    KisSelection(const KisSelection& rhs);

    /**
     * Delete the selection. The shape selection component is deleted, the
     * pixel selection component is contained in a shared pointer, so that
     * may still be valid.
     */
    virtual ~KisSelection();

    bool hasPixelSelection() const;
    bool hasShapeSelection() const;

    /**
     * return the pixel selection component of this selection or zero
     * if hasPixelSelection() returns false.
     */
    KisPixelSelectionSP pixelSelection() const;

    /**
     * return the vector selection component of this selection or zero
     * if hasShapeSelection() returns false.
     */
    KisSelectionComponent* shapeSelection() const;

    void setPixelSelection(KisPixelSelectionSP pixelSelection);
    void setShapeSelection(KisSelectionComponent* shapeSelection);

    /**
     * Return the pixel selection associated with this selection or
     * create a new one if there is currently no pixel selection
     * component in this selection.
     */
    KisPixelSelectionSP getOrCreatePixelSelection();

    /**
     * Returns the projection of the selection. It may be the same
     * as pixel selection. You must read selection data from this
     * paint device only
     */
    KisPixelSelectionSP projection() const;

    /**
     * Updates the projection of the selection. You should call this
     * method after the every change of the selection components.
     * There is no automatic updates framework present
     */
    void updateProjection(const QRect& rect);
    void updateProjection();

    void setDeselected(bool deselected);
    bool isDeselected();
    void setVisible(bool visible);
    bool isVisible();

    /**
     * Convinience functions. Just call the corresponding methods
     * of the underlying projection
     */
    bool isTotallyUnselected(const QRect & r) const;
    bool isProbablyTotallyUnselected(const QRect & r) const;
    QRect selectedRect() const;
    QRect selectedExactRect() const;
    void setX(qint32 x);
    void setY(qint32 y);
    qint32 x() const;
    qint32 y() const;

    KisDefaultBoundsSP defaultBounds() const;
    void setDefaultBounds(KisDefaultBoundsSP bounds);

    void clear();
    KisPixelSelectionSP mergedPixelSelection();

    KDE_DEPRECATED quint8 selected(qint32 x, qint32 y) const;
    KDE_DEPRECATED void setDirty(const QRect &rc = QRect());

private:

    struct Private;
    Private * const m_d;
};

#endif // KIS_SELECTION_H_
