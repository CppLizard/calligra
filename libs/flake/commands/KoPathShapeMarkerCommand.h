/* This file is part of the KDE project
 * Copyright (C) 2010 Jeremy Lugagne <lugagne.jeremy@gmail.com>
 * Copyright (C) 2011 Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
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

#ifndef KoPathShapeMarkerCommand_H
#define KoPathShapeMarkerCommand_H

#include "flake_export.h"

#include "KoPathShape.h"
#include <kundo2command.h>
#include <QList>


class KoMarker;

/// The undo / redo command for setting the shape marker
class FLAKE_EXPORT KoPathShapeMarkerCommand : public KUndo2Command
{
public:
    /**
     * Command to set a new shape marker.
     * @param shapes a set of all the shapes that should get the new marker.
     * @param marker the new marker, the same for all given shapes
     * @param position the position - begin or end - of the marker on the shape
     * @param parent the parent command used for macro commands
     */
    KoPathShapeMarkerCommand(const QList<KoPathShape*> &shapes, KoMarker *marker, KoPathShape::MarkerPosition position, KUndo2Command *parent = 0);
    
    virtual ~KoPathShapeMarkerCommand();
    /// redo the command
    void redo();
    /// revert the actions done in redo
    void undo();
    
private:
    class Private;
    Private * const d;
};

#endif // KoPathShapeMarkerCommand_H
