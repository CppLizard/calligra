/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_NODE_DUMMIES_GRAPH_H
#define __KIS_NODE_DUMMIES_GRAPH_H

#include <QList>

#include "krita_export.h"

class KisNodeShape;

/**
 * KisNodeDummy is a simplified representation of a node
 * in the node stack. It stores all the hierarchy information
 * about the node, so you needn't access from the node
 * directly (actually, you cannot do it usually, because UI
 * works in a different thread and race conditions are possible).
 *
 * The dummy stores a KisNodeShape which can store a pointer to
 * to node. You can access to the node data through it, but take
 * care -- not all the information is accessible in
 * multithreading environment.
 *
 * The ownership on the KisNodeShape is taken by the dummy.
 * The ownership on the children of the dummy is taken as well.
 */

class KRITAUI_EXPORT KisNodeDummy
{
public:
    KisNodeDummy(KisNodeShape *nodeShape);
    ~KisNodeDummy();

    KisNodeDummy* firstChild() const;
    KisNodeDummy* lastChild() const;
    KisNodeDummy* nextSibling() const;
    KisNodeDummy* prevSibling() const;
    KisNodeDummy* parent() const;

    KisNodeShape* nodeShape() const;

private:
    friend class KisNodeDummiesGraph;

    KisNodeDummy *m_parent;
    QList<KisNodeDummy*> m_children;

    KisNodeShape *m_nodeShape;
};

/**
 * KisNodeDummiesGraph manages the hierarchy of dummy objects
 * representing nodes in the UI environment.
 */

class KRITAUI_EXPORT KisNodeDummiesGraph
{
public:
    /**
     * Adds a dummy \p node to the position specified
     * by \p parent and \p aboveThis.
     *
     * It is not expected that you would add a dummy twice.
     */
    void addNode(KisNodeDummy *node, KisNodeDummy *parent, KisNodeDummy *aboveThis);

    /**
     * Moves a dummy \p node from its current position to
     * the position specified by \p parent and \p aboveThis.
     *
     * It is expected that the dummy \p node has been added
     * to the graph with addNode() before calling this function.
     */
    void moveNode(KisNodeDummy *node, KisNodeDummy *parent, KisNodeDummy *aboveThis);

    /**
     * Removes the dummy \p node from the graph.
     *
     * WARNING: The dummy is only "unlinked" from the graph. Neither
     * deletion of the node nor deletion of its children happens.
     * The dummy keeps maintaining its children so after unlinking
     * it from the graph you can just type to free memory recursively:
     * \code
     * graph.removeNode(node);
     * delete node;
     * \endcode
     */
    void removeNode(KisNodeDummy *node);
};

#endif /* __KIS_NODE_DUMMIES_GRAPH_H */
