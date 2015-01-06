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

#ifndef __KIS_DELEGATED_TOOL_POLICIES_H
#define __KIS_DELEGATED_TOOL_POLICIES_H

#include <QtGlobal>

#include "krita_export.h"


class KoCanvasBase;

struct KRITAUI_EXPORT NoopActivationPolicy {
    static inline void onActivate(KoCanvasBase *canvas) {
        Q_UNUSED(canvas);
    }
};

struct KRITAUI_EXPORT DeselectShapesActivationPolicy {
    static void onActivate(KoCanvasBase *canvas);
};


#endif /* __KIS_DELEGATED_TOOL_POLICIES_H */