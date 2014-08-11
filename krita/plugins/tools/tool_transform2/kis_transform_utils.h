/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_TRANSFORM_UTILS_H
#define __KIS_TRANSFORM_UTILS_H

#include <QtGlobal>

#include "kis_coordinates_converter.h"

#include <QTransform>
#include <QMatrix4x4>

class ToolTransformArgs;

class KisTransformUtils
{
public:

    static const int rotationHandleVisualRadius;
    static const int handleVisualRadius;
    static const int handleRadius;
    static const int rotationHandleRadius;

    template <class T>
    static T flakeToImage(const KisCoordinatesConverter *converter, T object) {
        return converter->documentToImage(converter->flakeToDocument(object));
    }

    template <class T>
    static T imageToFlake(const KisCoordinatesConverter *converter, T object) {
        return converter->documentToFlake(converter->imageToDocument(object));
    }

    static QTransform imageToFlakeTransform(const KisCoordinatesConverter *converter);
    static qreal effectiveHandleGrabRadius(const KisCoordinatesConverter *converter);

    static qreal effectiveRotationHandleGrabRadius(const KisCoordinatesConverter *converter);

    static qreal scaleFromAffineMatrix(const QTransform &t);

    static QPointF clipInRect(QPointF p, QRectF r);

    struct MatricesPack
    {
        MatricesPack(const ToolTransformArgs &args);

        QTransform TS;
        QTransform SC;
        QTransform S;
        QMatrix4x4 P;
        QTransform projectedP;
        QTransform T;

        // the final transformation looks like
        // transform = TS * SC * S * projectedP * T
        QTransform finalTransform() const;
    };
};

#endif /* __KIS_TRANSFORM_UTILS_H */
