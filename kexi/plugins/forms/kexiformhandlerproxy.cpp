/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kexiformhandlerproxy.h"
#include "kexiformhandler.h"
#include "kexiformhandleritem.h"
#include "kexiprojecthandleritem.h"
#include "kexidatasourcedlg.h"

#include <qpixmap.h>

#include <kdebug.h>
#include <klocale.h>
#include <klineeditdlg.h>
#include <kaction.h>


KexiFormHandlerProxy::KexiFormHandlerProxy(KexiFormHandler *handler, KexiView *view)
 : KexiProjectHandlerProxy(handler, view)
 ,KXMLGUIClient()
 ,m_formHandler(handler)
{
	m_createAction = new KAction(i18n("Create &Form..."), "form", "",
		this, SLOT(slotCreate()), actionCollection(), "formpart_create");

	m_openAction = new KexiPartItemAction(i18n("Open Form"), "", "",
		this,SLOT(slotOpen(const QString &)), actionCollection(), "formpart_open");
	m_editAction = new KexiPartItemAction(i18n("Design Form"), "edit", "",
		this,SLOT(slotAlter(const QString &)), actionCollection(), "formpart_edit");
	m_deleteAction = new KexiPartItemAction(i18n("Delete Form..."), "button_cancel", "",
		this,SLOT(slotDelete(const QString &)), actionCollection(), "formpart_delete");

	// actions in group menu
	m_createAction->plug(m_group_pmenu);

	// actions in item menu
	m_openAction->plug(m_item_pmenu);
	m_editAction->plug(m_item_pmenu);
	m_deleteAction->plug(m_item_pmenu);
	m_item_pmenu->insertSeparator();
	m_createAction->plug(m_item_pmenu);
					   
    setXMLFile("kexiformpartui.rc");

    view->insertChildClient(this);
}

/*
KexiPartPopupMenu *
KexiFormHandlerProxy::groupContext()
{
	kdDebug() << "KexiFormHandlerProxy::groupContext()" << endl;
	KexiPartPopupMenu *m = new KexiPartPopupMenu(this);
	m->insertAction(i18n("Create Form..."), SLOT(slotCreate()));

	return m;
}

KexiPartPopupMenu *
KexiFormHandlerProxy::itemContext(const QString &identifier)
{
	KexiPartPopupMenu *m = new KexiPartPopupMenu(this);
	m->insertAction(i18n("Open Form"), SLOT(slotOpen(const QString &)));
	m->insertAction(i18n("Design Form"), SLOT(slotAlter(const QString &)));
	m->insertAction(i18n("Delete Form"), SLOT(slotDelete(const QString &)));
	m->insertSeparator();
	m->insertAction(i18n("Create Form..."), SLOT(slotCreate()));

	return m;
}*/

void
KexiFormHandlerProxy::slotCreate()
{
	bool ok = false;
	KexiDataSourceDlg *d = new KexiDataSourceDlg(m_view->project(), kexiView());
	if(d->exec() == QDialog::Accepted)
	{
		QString name = d->name();
		KexiFormHandlerItem *i = new KexiFormHandlerItem(m_formHandler, name, name + ".ui");
		part()->items()->insert("kexi/form/" + name + ".ui", i);
		emit m_formHandler->itemListChanged(part());
		kexiView()->project()->addFileReference(FileReference("Forms",name,"/form/" + name + ".ui"));
		i->designForm(m_view);
	}
}

void
KexiFormHandlerProxy::slotOpen(const QString &identifier)
{
	if(kexiView()->activateWindow(identifier))
		return;
}

void
KexiFormHandlerProxy::slotAlter(const QString &identifier)
{
}

void
KexiFormHandlerProxy::slotDelete(const QString &identifier)
{
}

void
KexiFormHandlerProxy::executeItem(const QString &identifier)
{
	slotOpen(identifier);
}

#include "kexiformhandlerproxy.moc"
