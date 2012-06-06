/*
 * Copyright (c) 2012 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
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

#include <OpenRijn/AbstractCanvas.h>
#include <kis_types.h>

class KisPainter;

class KisRijnCanvas : public OpenRijn::AbstractCanvas
{
public:
    KisRijnCanvas(KisImageWSP _image, KisPaintDeviceSP _paintDevice);
    virtual ~KisRijnCanvas();
    KisPainter* painter();
    KisImageWSP image();
private:
    KisImageWSP       m_image;
    KisPaintDeviceSP  m_paintDevice;
    KisPainter*       m_painter;
};
