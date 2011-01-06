/* This file is part of the KDE project
 * Copyright (C) 2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2010 Boudewijn Rempt <boud@kogmbh.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/
#ifndef KOSHAPEPRIVATE_H
#define KOSHAPEPRIVATE_H

#include "KoShape.h"

#include <QPixmapCache>
#include <QPoint>
#include <QPaintDevice>

#include <KoCanvasBase.h>

/**
 * Contains the information needed for caching the contents of a shape.
 *
 * There are two possibilities: one cache made at 100% zoom at 72 dpi,
 * or a cache for every zoomlevel at the current resolution. The cache
 * is one big QPixMap for the entire shape.
 */
class KoShapeCache
{
public:
    struct DeviceData {
        DeviceData() : allExposed(true) {}

        QImage image;

        // List of logical exposed rects in document coordinates
        // These are the rects that are queued for updating, not
        // the rects that have already been painted.
        QVector<QRectF> exposed;
        // true if the whole shape has been exposed and asked to redraw
        bool allExposed;
    };

    // Map the cache to the canvas it is shown on (in QGraphicsView this is QPaintDevice)
    QMap<KoShapeManager *, DeviceData *> deviceData;

    // Empty cache
    void purge();
};

class KoShapePrivate
{
public:
    KoShapePrivate(KoShape *shape);
    virtual ~KoShapePrivate();
    /**
     * Notify the shape that a change was done. To be used by inheriting shapes.
     * @param type the change type
     */
    void shapeChanged(KoShape::ChangeType type);

    void addShapeManager(KoShapeManager *manager);
    void removeShapeManager(KoShapeManager *manager);

    /**
     * Fills the style stack and returns the value of the given style property (e.g fill, stroke).
     */
    static QString getStyleProperty(const char *property, KoShapeLoadingContext &context);

    /// Loads the shadow style
    KoShapeShadow *loadOdfShadow(KoShapeLoadingContext &context) const;

    /// calls update on the shape where the border is.
    void updateBorder();

    QSizeF size; // size in pt
    QString shapeId;
    QString name; ///< the shapes names

    QTransform localMatrix; ///< the shapes local transformation matrix

    QVector<QPointF> connectors; ///< glue points in percent of size [0..1]

    KoShapeContainer *parent;
    QSet<KoShapeManager *> shapeManagers;
    QSet<KoShape *> toolDelegates;
    KoShapeUserData *userData;
    KoShapeApplicationData *appData;
    KoShapeBackground * fill; ///< Stands for the background color / fill etc.
    KoShapeBorderModel *border; ///< points to a border, or 0 if there is no border
    KoShape *q_ptr;
    QList<KoShape*> dependees; ///< list of shape dependent on this shape
    KoShapeShadow * shadow; ///< the current shape shadow
    QMap<QString, QString> additionalAttributes;
    QMap<QByteArray, QString> additionalStyleAttributes;
    QSet<KoEventAction *> eventActions; ///< list of event actions the shape has
    KoFilterEffectStack *filterEffectStack; ///< stack of filter effects applied to the shape
    qreal transparency; ///< the shapes transparency

    static const int MaxZIndex = 32767;
    int zIndex : 16; // keep maxZIndex in sync!
    int runThrough : 16;
    int visible : 1;
    int printable : 1;
    int geometryProtected : 1;
    int keepAspect : 1;
    int selectable : 1;
    int detectCollision : 1;
    int protectContent : 1;

    KoShape::CacheMode cacheMode;

    KoShapeCache *cache;

    /**
     * @return the shape cache if there is one, else 0
     */
    KoShapeCache *maybeShapeCache() const;

    /**
     * return the shape cache if there is one, else create on
     */
    KoShapeCache *shapeCache() const;

    /**
     * purge and remove the shape cache
     */
    void removeShapeCache();

    Q_DECLARE_PUBLIC(KoShape)
};

#endif
