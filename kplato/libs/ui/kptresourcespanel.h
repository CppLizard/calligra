/* This file is part of the KDE project
   Copyright (C) 2003 Thomas Zander <zander@kde.org>
   Copyright (C) 2007 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPTPRESOURCESPANEL_H
#define KPTPRESOURCESPANEL_H

#include "kplatoui_export.h"

#include "kptresource.h"
#include "ui_resourcespanelbase.h"


namespace KPlato
{

class Project;
class GroupItem;
class ResourcesPanelGroupLVItem;
class MacroCommand;

class ResourcesPanelBase : public QWidget, public Ui::ResourcesPanelBase
{
public:
  explicit ResourcesPanelBase( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class ResourcesPanel : public ResourcesPanelBase {
    Q_OBJECT
public:
    ResourcesPanel (QWidget *parent, Project *project);
    ~ResourcesPanel();
    
    bool ok();
    MacroCommand *buildCommand();

protected slots:
    void slotAddGroup();
    void slotDeleteGroup();

    void slotAddResource();
    void slotEditResource();
    void slotDeleteResource();

    void slotGroupSelectionChanged(QTreeWidgetItem *item);
    void slotGroupSelectionChanged();
    void slotGroupChanged(QTreeWidgetItem *ci, int col);
    void slotResourceChanged();
    void slotCurrentChanged(QListWidgetItem *curr, QListWidgetItem* prev);

signals:
    void changed();
    void selectionChanged();
    
private:
    Project *project;
    ResourcesPanelGroupLVItem *m_groupItem;

    QList<GroupItem*> m_groupItems;
    QList<GroupItem*> m_deletedGroupItems;

    bool m_blockResourceRename;
};

} //KPlato namespace

#endif // PRESOURCESPANEL_H
