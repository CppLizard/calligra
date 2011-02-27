/*
 *  Copyright (c) 2011 Lukáš Tvrdý <lukast.dev@gmail.com>
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

#ifndef _KIS_FIXED_PAINTER_
#define _KIS_FIXED_PAINTER_

#include <kis_types.h>

class QRect;
class QPainterPath;
class KoColor;

class KisFixedPainter {

public:
    KisFixedPainter();
    KisFixedPainter(KisFixedPaintDeviceSP device);
    ~KisFixedPainter();
    void setPaintColor(const KoColor& color);

    /// returns the area where the caller can bitblit the desired path
    QRect fillPainterPath(const QPainterPath &path);

private:
    void init();

private:
    struct Private;
    Private* const d;

};

#endif