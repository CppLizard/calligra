/*
 *
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

#include "animator_view.h"

#include <QMenu>

#include "animator_manager_factory.h"

AnimatorView::AnimatorView()
{
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activate(QModelIndex)));
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(slotCustomContextMenuRequested(QPoint)));
    m_actions = new AnimatorActions(this);
}

AnimatorView::~AnimatorView()
{
}

void AnimatorView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);
    connect(model, SIGNAL(layoutChanged()), SLOT(resizeColumnsToContent()));
    AnimatorManager* manager = AnimatorManagerFactory::instance()->getManager(amodel()->image());
    m_actions->setManager(manager);
    resizeColumnsToContent();
}

AnimatorModel* AnimatorView::amodel()
{
    return qobject_cast<AnimatorModel*>(model());
}

void AnimatorView::resizeColumnsToContent()
{
    for (int i = 0; i < model()->columnCount(); ++i)
        resizeColumnToContents(i);
}


void AnimatorView::activate(QModelIndex index)
{
    if (!amodel())
        return;
    KisNodeSP node = amodel()->nodeFromIndex(index);
    if (!node && index.column() != 0) {
        node = amodel()->nodeFromIndex(amodel()->index(index.row(), 0, index.parent()));
    }
    AnimatorManager* manager = AnimatorManagerFactory::instance()->getManager(amodel()->image());
    manager->activate(amodel()->frameNumber(index), node);
}

void AnimatorView::slotCustomContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = indexAt(pos);
    activate(index);
    
    QMenu *menu = new QMenu(this);
    menu->addActions(m_actions->actions("frames-adding"));
    menu->addSeparator();
    menu->addActions(m_actions->actions("frames-edit-one"));
    
    menu->popup(mapToGlobal(pos));
}
