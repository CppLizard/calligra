/*
 *  Normal animated layer: class for most usually used, universal layer
 *  Copyright (C) 2011 Torio Mlshi <mlshi@lavabit.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "normal_animated_layer.h"

NormalAnimatedLayer::NormalAnimatedLayer(KisImageWSP image, const QString& name, quint8 opacity): InterpolatedAnimatedLayer(image, name, opacity)
{
}

NormalAnimatedLayer::NormalAnimatedLayer(const KisGroupLayer& source): InterpolatedAnimatedLayer(source)
{
}

KisCloneLayerSP NormalAnimatedLayer::interpolate(KisNodeSP from, KisCloneLayerSP to, double position)
{
    KisCloneLayerSP result = MovingInterpolation::makeLayer(from, to, position);
    TransparencyInterpolation::changeLayer(result, from, to, position);
//     YetAnotherInterpolation::changeLayer(result, from, to, position);
    return result;
}

#include "moc_normal_animated_layer.cpp"
