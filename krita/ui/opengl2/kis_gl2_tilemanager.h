/* This file is part of the KDE project
 *
 * Copyright (c) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

#ifndef KIS_GL2_TILEMANAGER_H
#define KIS_GL2_TILEMANAGER_H

#include <QObject>
#include <kis_types.h>

class QRect;
class KisGL2Canvas;

class KisGL2TileManager : public QObject
{
    Q_OBJECT
public:
    explicit KisGL2TileManager(KisGL2Canvas* parent = 0);
    virtual ~KisGL2TileManager();

    void initialize(KisImageWSP image);
    void update(const QRect &area);
    void render(uint texture);
    void resize(int width, int height);

    uint framebufferTexture() const;

private:
    class Private;
    Private * const d;
};

#endif // KIS_GL2_TILEMANAGER_H
