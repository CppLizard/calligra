/* This file is part of the KDE project
 * Copyright (C) 2011 Aakriti Gupta <aakriti.a.gupta@gmail.com>
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

#include "SvgWriter.h"
#include "SvgWriter_generic.h"
//#include "SvgAnimationData.h"
//#include "KoShape.h"

SvgWriter::SvgWriter(const QList<KoShapeLayer*> &layers, const QSizeF& pageSize): SvgWriter_generic(layers, pageSize)
{
}

SvgWriter::SvgWriter(const QList< KoShape* >& toplevelShapes, const QSizeF& pageSize): SvgWriter_generic(toplevelShapes, pageSize)
{
}

SvgWriter::~SvgWriter()
{
}

void SvgWriter::saveAppData(KoShape *shape)
{
  //No extra app data to be written
}
    

      