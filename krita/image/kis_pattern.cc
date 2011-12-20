/*
 *  kis_pattern.cc - part of Krayon
 *
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *                2001 John Califf <jwcaliff@compuzone.net>
 *                2004 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_pattern.h"


#include <sys/types.h>
#include <netinet/in.h>

#include <limits.h>
#include <stdlib.h>

#include <QPoint>
#include <QSize>
#include <QImage>
#include <QMap>
#include <QFile>
#include <QTextStream>

#include "KoColor.h"
#include "KoColorSpace.h"
#include <klocale.h>

#include "kis_debug.h"
#include "kis_layer.h"
#include "kis_paint_device.h"

KisPattern::KisPattern(const QString& file)
        : KoPattern(file)
{
}

KisPattern::KisPattern(const QImage &image, const QString &name)
        : KoPattern(0)
{
    setImage(image);
    setName(name);
}

KisPattern::~KisPattern()
{
}

KisPaintDeviceSP KisPattern::paintDevice(const KoColorSpace * colorSpace) const
{
    KisPaintDevice* dev = new KisPaintDevice(colorSpace, name());
    dev->convertFromQImage(image(), 0);
    return dev;
}

KisPattern* KisPattern::clone() const
{
    KisPattern* pattern = new KisPattern(filename());
    pattern->setImage(image());
    pattern->setName(name());
    return pattern;
}
