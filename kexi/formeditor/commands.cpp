/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <qdom.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>

#include "formIO.h"
#include "container.h"
#include "objecttree.h"
#include "formmanager.h"
#include "form.h"
#include "objpropbuffer.h"
#include "kexiproperty.h"
#include "widgetlibrary.h"
#include "events.h"
#include "utils.h"

#include "commands.h"

namespace KFormDesigner {

// PropertyCommand

PropertyCommand::PropertyCommand(ObjectPropertyBuffer *buf, const QString &wname, const QVariant &oldValue, const QVariant &value, const QCString &property)
  : KCommand(), m_buffer(buf), m_value(value), m_property(property)
{
	m_oldvalues.insert(wname, oldValue);
}

PropertyCommand::PropertyCommand(ObjectPropertyBuffer *buf, const QMap<QString, QVariant> &oldvalues, const QVariant &value, const QCString &property)
  : KCommand(), m_buffer(buf), m_value(value), m_oldvalues(oldvalues), m_property(property)
{}

void
PropertyCommand::setValue(const QVariant &value)
{
	m_value = value;
	emit m_buffer->manager()->dirty(m_buffer->manager()->activeForm());
}

void
PropertyCommand::execute()
{
	m_buffer->m_undoing = true;
	m_buffer->m_manager->activeForm()->resetSelection();

	QMap<QString, QVariant>::ConstIterator endIt = m_oldvalues.constEnd();
	for(QMap<QString, QVariant>::ConstIterator it = m_oldvalues.constBegin(); it != endIt; ++it)
	{
		QWidget *widg = m_buffer->m_manager->activeForm()->objectTree()->lookup(it.key())->widget();
		m_buffer->m_manager->activeForm()->setSelectedWidget(widg, true);
		//m_buffer->setSelectedWidget(widg, true);
	}

	(*m_buffer)[m_property] = m_value;
	m_buffer->m_undoing = false;
}

void
PropertyCommand::unexecute()
{
	m_buffer->m_undoing = true;
	m_buffer->m_manager->activeForm()->resetSelection();

	(*m_buffer)[m_property] = m_oldvalues.begin().data();
	m_buffer->m_undoing = false;

	QMap<QString, QVariant>::ConstIterator endIt = m_oldvalues.constEnd();
	for(QMap<QString, QVariant>::ConstIterator it = m_oldvalues.constBegin(); it != endIt; ++it)
	{
		QWidget *widg = m_buffer->m_manager->activeForm()->objectTree()->lookup(it.key())->widget();
		m_buffer->m_manager->activeForm()->setSelectedWidget(widg, true);
		//m_buffer->setSelectedWidget(widg, true);
		widg->setProperty(m_property, it.data());
	}
}

QString
PropertyCommand::name() const
{
	if(m_oldvalues.count() >= 2)
		return i18n("Change %1 of multiple widgets" ).arg(m_property);
	else
		return i18n("Change %1 of widget %2" ).arg(m_property).arg(m_oldvalues.begin().key());
}

// GeometryPropertyCommand (for multiples widgets)

GeometryPropertyCommand::GeometryPropertyCommand(ObjectPropertyBuffer *buf, const QStringList &names, QPoint oldPos)
 : KCommand(), m_buffer(buf), m_names(names), m_oldPos(oldPos)
{}

void
GeometryPropertyCommand::execute()
{
	m_buffer->m_undoing = true;
	int dx = m_pos.x() - m_oldPos.x();
	int dy = m_pos.y() - m_oldPos.y();

	QStringList::ConstIterator endIt = m_names.constEnd();
	// We move every widget in our list by (dx, dy)
	for(QStringList::ConstIterator it = m_names.constBegin(); it != endIt; ++it)
	{
		QWidget *w = m_buffer->m_manager->activeForm()->objectTree()->lookup(*it)->widget();
		w->move(w->x() + dx, w->y() + dy);
	}
	m_buffer->m_undoing = false;
}

void
GeometryPropertyCommand::unexecute()
{
	m_buffer->m_undoing = true;
	int dx = m_pos.x() - m_oldPos.x();
	int dy = m_pos.y() - m_oldPos.y();

	QStringList::ConstIterator endIt = m_names.constEnd();
	// We move every widget in our list by (-dx, -dy) to undo the move
	for(QStringList::ConstIterator it = m_names.constBegin(); it != endIt; ++it)
	{
		QWidget *w = m_buffer->m_manager->activeForm()->objectTree()->lookup(*it)->widget();
		w->move(w->x() - dx, w->y() - dy);
	}
	m_buffer->m_undoing = false;
}

void
GeometryPropertyCommand::setPos(QPoint pos)
{
	m_pos = pos;
	emit m_buffer->manager()->dirty(m_buffer->manager()->activeForm());
}

QString
GeometryPropertyCommand::name() const
{
	return i18n("Move multiple widgets");
}

/////////////////  AlignWidgetsCommand  ////////

AlignWidgetsCommand::AlignWidgetsCommand(int type, WidgetList &list, Form *form)
: KCommand(), m_form(form), m_type(type)
{
	for(QWidget *w = list.first(); w; w = list.next())
		m_pos.insert(w->name(), w->pos());
}

void
AlignWidgetsCommand::execute()
{
	// To avoid creation of GeometryPropertyCommand
	m_form->resetSelection();

	int gridX = m_form->gridX();
	int gridY = m_form->gridY();
	QWidget *parentWidget = m_form->selectedWidgets()->first()->parentWidget();
	int tmpx, tmpy;

	WidgetList list;
	QMap<QString, QPoint>::ConstIterator endIt = m_pos.constEnd();
	for(QMap<QString, QPoint>::ConstIterator it = m_pos.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if(item && item->widget())
			list.append(item->widget());
	}

	switch(m_type)
	{
		case AlignToGrid:
		{
			for(QWidget *w = list.first(); w; w = list.next())
			{
				tmpx = int( (float)w->x() / ((float)gridX) + 0.5 ) * gridX;
				tmpy = int( (float)w->y() / ((float)gridY) + 0.5 ) * gridY;

				if((tmpx != w->x()) || (tmpy != w->y()))
					w->move(tmpx, tmpy);
			}
			break;
		}

		case AlignToLeft:
		{
			tmpx = parentWidget->width();
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(w->x() < tmpx)
					tmpx = w->x();
			}

			for(QWidget *w = list.first(); w; w = list.next())
				w->move(tmpx, w->y());
			break;
		}

		case AlignToRight:
		{
			tmpx = 0;
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(w->x() + w->width() > tmpx)
					tmpx = w->x() + w->width();
			}

			for(QWidget *w = list.first(); w; w = list.next())
				w->move(tmpx - w->width(), w->y());
			break;
		}

		case AlignToTop:
		{
			tmpy = parentWidget->height();
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(w->y() < tmpy)
					tmpy = w->y();
			}

			for(QWidget *w = list.first(); w; w = list.next())
				w->move(w->x(), tmpy);
			break;
		}

		case AlignToBottom:
		{
			tmpy = 0;
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(w->y() + w->height() > tmpy)
					tmpy = w->y() + w->height();
			}

			for(QWidget *w = list.first(); w; w = list.next())
				w->move(w->x(), tmpy - w->height());
			break;
		}

		default:
			return;
	}

	// We restore selection
	for(QWidget *w = list.first(); w; w = list.next())
		m_form->setSelectedWidget(w, true);
}

void
AlignWidgetsCommand::unexecute()
{
	// To avoid creation of GeometryPropertyCommand
	m_form->resetSelection();
	// We move widgets to their original pos
	QMap<QString, QPoint>::ConstIterator endIt = m_pos.constEnd();
	for(QMap<QString, QPoint>::ConstIterator it = m_pos.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if(item && item->widget())
			item->widget()->move( m_pos[item->widget()->name()] );
		m_form->setSelectedWidget(item->widget(), true); // We restore selection
	}
}

QString
AlignWidgetsCommand::name() const
{
	switch(m_type)
	{
		case AlignToGrid:
			return i18n("Align Widgets To Grid");
		case AlignToLeft:
			return i18n("Align Widgets To Left");
		case AlignToRight:
			return i18n("Align Widgets To Right");
		case AlignToTop:
			return i18n("Align Widgets To Top");
		case AlignToBottom:
			return i18n("Align Widgets To Bottom");
		default:
			return QString::null;
	}
}

///// AdjustSizeCommand ///////////

AdjustSizeCommand::AdjustSizeCommand(int type, WidgetList &list, Form *form)
: KCommand(), m_form(form), m_type(type)
{
	for(QWidget *w = list.first(); w; w = list.next())
	{
		if(w->parentWidget() && w->parentWidget()->isA("QWidgetStack"))
		{
			w = w->parentWidget(); // widget is WidgetStack page
			if(w->parentWidget() && w->parentWidget()->inherits("QTabWidget")) // widget is tabwidget page
				w = w->parentWidget();
		}

		m_sizes.insert(w->name(), w->size());
		if(m_type == SizeToGrid) // SizeToGrid also move widgets
			m_pos.insert(w->name(), w->pos());
	}
}

void
AdjustSizeCommand::execute()
{
	// To avoid creation of GeometryPropertyCommand
	m_form->resetSelection();

	int gridX = m_form->gridX();
	int gridY = m_form->gridY();
	int tmpw=0, tmph=0;

	WidgetList list;
	QMap<QString, QSize>::ConstIterator endIt = m_sizes.constEnd();
	for(QMap<QString, QSize>::ConstIterator it = m_sizes.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if(item && item->widget())
			list.append(item->widget());
	}

	switch(m_type)
	{
		case SizeToGrid:
		{
			int tmpx=0, tmpy=0;
			// same as in 'Align to Grid' + for the size
			for(QWidget *w = list.first(); w; w = list.next())
			{
				tmpx = int( (float)w->x() / ((float)gridX) + 0.5 ) * gridX;
				tmpy = int( (float)w->y() / ((float)gridY) + 0.5 ) * gridY;
				tmpw = int( (float)w->width() / ((float)gridX) + 0.5 ) * gridX;
				tmph = int( (float)w->height() / ((float)gridY) + 0.5 ) * gridY;

				if((tmpx != w->x()) || (tmpy != w->y()))
					w->move(tmpx, tmpy);
				if((tmpw != w->width()) || (tmph != w->height()))
					w->resize(tmpw, tmph);
			}
			break;
		}

		case SizeToFit:
		{
			for(QWidget *w = list.first(); w; w = list.next()) {
				ObjectTreeItem *item = m_form->objectTree()->lookup(w->name());
				if(item && !item->children()->isEmpty()) { // container
					w->resize(getSizeFromChildren(item));
					w->resize(w->sizeHint()); // eg tabwidget adds the size of tabbar
				}
				else if(item && item->container()) // empty container
					w->resize(item->container()->form()->gridX() * 5, item->container()->form()->gridY() * 5); // basic size
				else
					w->resize(w->sizeHint());
			}
			break;
		}

		case SizeToSmallWidth:
		{
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if((tmpw == 0) || (w->width() < tmpw))
					tmpw = w->width();
			}

			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(tmpw != w->width())
					w->resize(tmpw, w->height());
			}
			break;
		}

		case SizeToBigWidth:
		{
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(w->width() > tmpw)
					tmpw = w->width();
			}

			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(tmpw != w->width())
					w->resize(tmpw, w->height());
			}
			break;
		}

		case SizeToSmallHeight:
		{
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if((tmph == 0) || (w->height() < tmph))
					tmph = w->height();
			}

			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(tmph != w->height())
					w->resize(w->width(), tmph);
			}
			break;
		}

		case SizeToBigHeight:
		{
			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(w->height() > tmph)
					tmph = w->height();
			}

			for(QWidget *w = list.first(); w; w = list.next())
			{
				if(tmph != w->height())
					w->resize(w->width(), tmph);
			}
			break;
		}

		default:
			break;
	}

	// We restore selection
	for(QWidget *w = list.first(); w; w = list.next())
		m_form->setSelectedWidget(w, true);
}

QSize
AdjustSizeCommand::getSizeFromChildren(ObjectTreeItem *item)
{
	if(!item->container()) // multi pages containers (eg tabwidget)
	{
		QSize s;
		// get size for each container, and keep the biggest one
		for(ObjectTreeItem *tree = item->children()->first(); tree; tree = item->children()->next())
			s = s.expandedTo(getSizeFromChildren(tree));
		return s;
	}

	int tmpw = 0, tmph = 0;
	for(ObjectTreeItem *tree = item->children()->first(); tree; tree = item->children()->next()) {
		if(!tree->widget())
			continue;
		tmpw = QMAX(tmpw, tree->widget()->geometry().right());
		tmph = QMAX(tmph, tree->widget()->geometry().bottom());
	}

	return QSize(tmpw, tmph) + QSize(10, 10);
}

void
AdjustSizeCommand::unexecute()
{
	// To avoid creation of GeometryPropertyCommand
	m_form->resetSelection();
	// We resize widgets to their original size
	QMap<QString, QSize>::ConstIterator endIt = m_sizes.constEnd();
	for(QMap<QString, QSize>::ConstIterator it = m_sizes.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if(item && item->widget())
		{
			item->widget()->resize(  m_sizes[item->widget()->name()] );
			if(m_type == SizeToGrid)
				item->widget()->move( m_pos[item->widget()->name()] );
			m_form->setSelectedWidget(item->widget(), true); // We restore selection
		}
	}
}

QString
AdjustSizeCommand::name() const
{
	switch(m_type)
	{
		case SizeToGrid:
			return i18n("Resize Widgets To Grid");
		case SizeToFit:
			return i18n("Resize Widgets To Fit Contents");
		case SizeToSmallWidth:
			return i18n("Resize Widgets To Narrowest");
		case SizeToBigWidth:
			return i18n("Resize Widgets To Widest");
		case SizeToSmallHeight:
			return i18n("Resize Widgets To Shortest");
		case SizeToBigHeight:
			return i18n("Resize Widgets To Tallest");
		default:
			return QString::null;
	}
}

// LayoutPropertyCommand

LayoutPropertyCommand::LayoutPropertyCommand(ObjectPropertyBuffer *buf, const QString &name, const QVariant &oldValue, const QVariant &value)
 : PropertyCommand(buf, name, oldValue, value, "layout")
{
	m_form = buf->m_manager->activeForm();
	Container *m_container = m_form->objectTree()->lookup(name)->container();
	// We save the geometry of each wigdet
	for(ObjectTreeItem *it = m_container->objectTree()->children()->first(); it; it = m_container->objectTree()->children()->next())
		m_geometries.insert(it->name(), it->widget()->geometry());
}

void
LayoutPropertyCommand::execute()
{
	PropertyCommand::execute();
}

void
LayoutPropertyCommand::unexecute()
{
	Container *m_container = m_form->objectTree()->lookup(m_oldvalues.begin().key())->container();
	m_container->setLayout(Container::NoLayout);
	// We put every widget back in its old location
	QMap<QString,QRect>::ConstIterator endIt = m_geometries.constEnd();
	for(QMap<QString,QRect>::ConstIterator it = m_geometries.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *tree = m_container->form()->objectTree()->lookup(it.key());
		if(tree)
			tree->widget()->setGeometry(it.data());
	}

	PropertyCommand::unexecute();
}

QString
LayoutPropertyCommand::name() const
{
	return i18n("Change layout of %1").arg(m_oldvalues.begin().key());
}


// InsertWidgetCommand

InsertWidgetCommand::InsertWidgetCommand(Container *container/*, QPoint p*/)
  : KCommand()//, m_point(p)
{
	m_containername = container->widget()->name();
	m_form = container->form();
	m_class = container->form()->manager()->insertClass();
	m_insertRect = container->m_insertRect;
	m_point = container->m_insertBegin;
	m_name = container->form()->objectTree()->genName(container->form()->manager()->lib()->namePrefix(m_class));
}

void
InsertWidgetCommand::execute()
{
	if (!m_form->objectTree())
		return;
	Container *m_container = m_form->objectTree()->lookup(m_containername)->container();
	QWidget *w = m_container->form()->manager()->lib()->createWidget(m_class, m_container->m_container, m_name.latin1(), m_container);

	if(!w)
	{
		kdDebug() << "InsertWidgetCommand::execute() ERROR: widget creation failed" << endl;
		return;
	}

	// if the insertRect is invalid (ie only one point), we use widget' size hint
	if(( (m_insertRect.width() < 21) && (m_insertRect.height() < 21)))
	{
		QSize s = w->sizeHint();

		if(s.isEmpty())
			s = QSize(20, 20); // Minimum size to avoid creating a (0,0) widget
		int x, y;
		if(m_insertRect.isValid())
		{
			x = m_insertRect.x();
			y = m_insertRect.y();
		}
		else
		{
			x = m_point.x();
			y = m_point.y();
		}
		m_insertRect = QRect(x, y, s.width(), s.height());
	}
	w->move(m_insertRect.x(), m_insertRect.y());
	w->resize(m_insertRect.width()-1, m_insertRect.height()-1); // -1 is not to hide dots
	w->setStyle(&(m_container->widget()->style()));
	w->setBackgroundOrigin(QWidget::ParentOrigin);
	w->show();

	m_container->form()->manager()->stopInsert();

	if (!m_container->form()->objectTree()->lookup(m_name))
	{
		m_container->form()->objectTree()->addItem(m_container->m_tree,
		   new ObjectTreeItem(m_container->form()->manager()->lib()->displayName(m_class), m_name, w, m_container));
	}

	// We add the autoSaveProperties in the modifProp list of the ObjectTreeItem, so that they are saved later
	ObjectTreeItem *item = m_container->form()->objectTree()->lookup(m_name);
	QStringList list(m_container->form()->manager()->lib()->autoSaveProperties(w->className()));

	QStringList::ConstIterator endIt = list.constEnd();
	for(QStringList::ConstIterator it = list.constBegin(); it != endIt; ++it)
		item->addModifiedProperty(*it, w->property((*it).latin1()));

	m_container->reloadLayout(); // reload the layout to take the new wigdet into account

	m_container->setSelectedWidget(w, false);
	m_form->manager()->lib()->startEditing(w->className(), w, item->container() ? item->container() : m_container); // we edit the widget on creation
	kdDebug() << "Container::eventFilter(): widget added " << this << endl;
}

void
InsertWidgetCommand::unexecute()
{
	QWidget *m_widget = m_form->objectTree()->lookup(m_name)->widget();
	Container *m_container = m_form->objectTree()->lookup(m_containername)->container();
	m_container->deleteWidget(m_widget);
}

QString
InsertWidgetCommand::name() const
{
	if(!m_name.isEmpty())
		return i18n("Insert %1").arg(m_name);
	else
		return i18n("Insert widget");
}

/// CreateLayoutCommand ///////////////

CreateLayoutCommand::CreateLayoutCommand(int layoutType, WidgetList &list, Form *form)
 : m_form(form), m_type(layoutType)
{
	WidgetList *m_list=0;
	switch(layoutType)
	{
		case Container::HBox:
		case Container::Grid:
			m_list = new HorWidgetList(); break;
		case Container::VBox:
			m_list = new VerWidgetList(); break;
	}
	for(QWidget *w = list.first(); w; w = list.next())
		m_list->append(w);
	m_list->sort(); // we sort them now, before creating the layout

	for(QWidget *w = m_list->first(); w; w = m_list->next())
		m_pos.insert(w->name(), w->geometry());
	ObjectTreeItem *item = form->objectTree()->lookup(m_list->first()->name());
	if(item && item->parent()->container())
		m_containername = item->parent()->name();
	delete m_list;
}

void
CreateLayoutCommand::execute()
{
	WidgetLibrary *lib = m_form->manager()->lib();
	if(!lib)  return;
	Container *container = m_form->objectTree()->lookup(m_containername)->container();
	if(!container)
		container = m_form->toplevelContainer(); // use toplevelContainer by default

	QString classname;
	switch(m_type)
	{
		case Container::HBox:
			classname = "HBox"; break;
		case Container::VBox:
			classname = "VBox"; break;
		case Container::Grid:
			classname = "Grid"; break;
		default: break;
	}

	if(m_name.isEmpty())// the name must be generated only once
		m_name = m_form->objectTree()->genName(classname);
	QWidget *w = lib->createWidget(classname, container->widget(), m_name.latin1(), container);
	ObjectTreeItem *tree = m_form->objectTree()->lookup(w->name());
	if(!tree)
		return;

	container->setSelectedWidget(0, false);
	w->move(m_pos.begin().data().topLeft()); // we move the layout at the position of the topleft widget
	w->show();

	// We reparent every widget to the Layout and insert them into it
	QMap<QString,QRect>::ConstIterator endIt = m_pos.constEnd();
	for(QMap<QString,QRect>::ConstIterator it = m_pos.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if(item && item->widget())
		{
			item->widget()->reparent(w, item->widget()->pos(), true);
			item->eventEater()->setContainer(tree->container());
			m_form->objectTree()->reparent(item->name(), m_name);
		}
	}

	tree->container()->setLayout((Container::LayoutType)m_type);
	tree->widget()->resize(tree->container()->layout()->sizeHint()); // the layout doesn't have its own size
	container->setSelectedWidget(w, false);
	m_form->manager()->windowChanged(m_form->widget()); // to reload the ObjectTreeView
}

void
CreateLayoutCommand::unexecute()
{
	ObjectTreeItem *parent = m_form->objectTree()->lookup(m_containername);
	if(!parent)
		parent = m_form->objectTree();

	// We reparent every widget to the Container and take them out of the layout
	QMap<QString,QRect>::ConstIterator endIt = m_pos.constEnd();
	for(QMap<QString,QRect>::ConstIterator it = m_pos.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if(item && item->widget())
		{
			item->widget()->reparent(parent->widget(), QPoint(0,0), true);
			item->eventEater()->setContainer(parent->container());
			if(m_pos[it.key()].isValid())
				item->widget()->setGeometry(m_pos[it.key()]);
			m_form->objectTree()->reparent(item->name(), m_containername);
		}
	}

	if(!parent->container())
		return;
	QWidget *w = m_form->objectTree()->lookup(m_name)->widget();
	parent->container()->deleteWidget(w); // delete the layout widget
	m_form->manager()->windowChanged(m_form->widget()); // to reload ObjectTreeView
}

QString
CreateLayoutCommand::name() const
{
	switch(m_type)
	{
		case Container::HBox:
			return i18n("Lay out widgets horizontally");
		case Container::VBox:
			return i18n("Lay out widgets vertically");
		case Container::Grid:
			return i18n("Lay out widgets in a grid");
		default:
			return i18n("Create Layout");
	}
}

/// BreakLayoutCommand ///////////////

BreakLayoutCommand::BreakLayoutCommand(Container *container)
 : CreateLayoutCommand()
{
	m_containername = container->toplevel()->widget()->name();
	m_name = container->widget()->name();
	m_form = container->form();
	m_type = container->layoutType();

	for(ObjectTreeItem *tree = container->objectTree()->children()->first(); tree; tree = container->objectTree()->children()->next())
	{
		QRect r(container->widget()->mapTo(container->widget()->parentWidget(), tree->widget()->pos()), tree->widget()->size());
		m_pos.insert(tree->widget()->name(), r);
	}
}

void
BreakLayoutCommand::execute()
{
	CreateLayoutCommand::unexecute();
}

void
BreakLayoutCommand::unexecute()
{
	CreateLayoutCommand::execute();
}

QString
BreakLayoutCommand::name() const
{
	return i18n("Break Layout: %1").arg(m_name);
}

// PasteWidgetCommand

PasteWidgetCommand::PasteWidgetCommand(QDomDocument &domDoc, Container *container, QPoint p)
 : m_point(p)
{
	m_data = domDoc.toCString();
	m_containername = container->widget()->name();
	m_form = container->form();

	if(domDoc.namedItem("UI").firstChild().nextSibling().toElement().tagName() != "widget")
		return;

	QRect boundingRect;
	for(QDomNode n = domDoc.namedItem("UI").firstChild(); !n.isNull(); n = n.nextSibling()) // more than one widget
	{
		if(n.toElement().tagName() != "widget")
			continue;
		QDomElement el = n.toElement();

		QDomElement rect;
		for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "geometry"))
				rect = n.firstChild().toElement();
		}
	
		QDomElement x = rect.namedItem("x").toElement();
		QDomElement y = rect.namedItem("y").toElement();
		QDomElement wi = rect.namedItem("width").toElement();
		QDomElement h = rect.namedItem("height").toElement();
	
		int rx = x.text().toInt();
		int ry = y.text().toInt();
		int rw = wi.text().toInt();
		int rh = h.text().toInt();
		QRect r(rx, ry, rw, rh);
		boundingRect = boundingRect.unite(r);
	}

	m_point = m_point - boundingRect.topLeft();
}

void
PasteWidgetCommand::execute()
{
	Container *m_container = m_form->objectTree()->lookup(m_containername)->container();
	QString errMsg;
	int errLine;
	int errCol;
	QDomDocument domDoc("UI");
	bool parsed = domDoc.setContent(m_data, false, &errMsg, &errLine, &errCol);

	if(!parsed)
	{
		kdDebug() << "WidgetWatcher::load(): " << errMsg << endl;
		kdDebug() << "WidgetWatcher::load(): line: " << errLine << " col: " << errCol << endl;
		return;
	}

	//FormIO::setCurrentForm(m_container->form());

	kdDebug() << domDoc.toString() << endl;
	if(!domDoc.namedItem("UI").hasChildNodes()) // nothing in the doc
		return;
	if(domDoc.namedItem("UI").firstChild().nextSibling().toElement().tagName() != "widget") // only one widget, so we can paste it at cursor pos
	{
		QDomElement el = domDoc.namedItem("UI").firstChild().toElement();
		fixNames(el);
		if(m_point.isNull())
			fixPos(el, m_container);
		else
			changePos(el, m_point);

		m_form->setInteractiveMode(false);
		FormIO::loadWidget(m_container, m_form->manager()->lib(), el);
		m_form->setInteractiveMode(true);
	}
	else for(QDomNode n = domDoc.namedItem("UI").firstChild(); !n.isNull(); n = n.nextSibling()) // more than one widget
	{
		if(n.toElement().tagName() != "widget")
			continue;
		QDomElement el = n.toElement();
		fixNames(el);
		if(!m_point.isNull())
			moveWidgetBy(el, m_container, m_point);
		else {
			fixPos(el, m_container);
			kdDebug() << "jdkjfldfksmfkdfjmqdsklfjdkkfmsqfksdfsm" << endl;
		}

		m_form->setInteractiveMode(false);
		FormIO::loadWidget(m_container, m_form->manager()->lib(), el);
		m_form->setInteractiveMode(true);
	}

	//FormIO::setCurrentForm(0);
	m_names.clear();
	// We store the names of all the created widgets, to delete them later
	for(QDomNode n = domDoc.namedItem("UI").firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.toElement().tagName() != "widget")
			continue;
		for(QDomNode m = n.firstChild(); !m.isNull(); n = m.nextSibling())
		{
			if((m.toElement().tagName() == "property") && (m.toElement().attribute("name") == "name"))
			{
				m_names.append(m.toElement().text());
				break;
			}
		}
	}

	m_container->form()->resetSelection();
	QStringList::ConstIterator endIt = m_names.constEnd();
	for(QStringList::ConstIterator it = m_names.constBegin(); it != endIt; ++it) // We select all the pasted widgets
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(*it);
		if(item)
			m_container->setSelectedWidget(item->widget(), true);
	}
}

void
PasteWidgetCommand::unexecute()
{
	Container *m_container = m_form->objectTree()->lookup(m_containername)->container();
	// We just delete all the widgets we have created
	QStringList::ConstIterator endIt = m_names.constEnd();
	for(QStringList::ConstIterator it = m_names.constBegin(); it != endIt; ++it)
	{
		QWidget *w = m_container->form()->objectTree()->lookup(*it)->widget();
		m_container->deleteWidget(w);
	}
}

QString
PasteWidgetCommand::name() const
{
	return i18n("Paste");
}

void
//QDomElement
PasteWidgetCommand::changePos(QDomElement &el, const QPoint &newpos)
{
	//QDomElement el = widg.cloneNode(true).toElement();
	QDomElement rect;
	// Find the widget geometry if there is one
	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "geometry"))
			rect = n.firstChild().toElement();
	}

	QDomElement x = rect.namedItem("x").toElement();
	x.removeChild(x.firstChild());
	QDomText valueX = el.ownerDocument().createTextNode(QString::number(newpos.x()));
	x.appendChild(valueX);

	QDomElement y = rect.namedItem("y").toElement();
	y.removeChild(y.firstChild());
	QDomText valueY = el.ownerDocument().createTextNode(QString::number(newpos.y()));
	y.appendChild(valueY);

	//return el;
}

void
PasteWidgetCommand::fixPos(QDomElement &el, Container *container)
{
/*	QDomElement rect;
	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "geometry"))
			rect = n.firstChild().toElement();
	}

	QDomElement x = rect.namedItem("x").toElement();
	QDomElement y = rect.namedItem("y").toElement();
	QDomElement wi = rect.namedItem("width").toElement();
	QDomElement h = rect.namedItem("height").toElement();

	int rx = x.text().toInt();
	int ry = y.text().toInt();
	int rw = wi.text().toInt();
	int rh = h.text().toInt();
	QRect r(rx, ry, rw, rh);

	QWidget *w = m_form->widget()->childAt(r.x() + 6, r.y() + 6, false);
	if(!w)
		return;

	while((w->geometry() == r) && (w != 0))// there is already a widget there, with the same size
	{
		w = m_form->widget()->childAt(w->x() + 16, w->y() + 16, false);
		r.moveBy(10,10);
	}

	// the pasted wigdet should stay inside container's boudaries
	if(r.x() < 0)
		r.setX(0);
	else if(r.right() > container->widget()->width())
		r.setX(container->widget()->width() - r.width());

	if(r.y() < 0)
		r.setY(0);
	else if(r.bottom() > container->widget()->height())
		r.setY(container->widget()->height() - r.height());

	if(r != QRect(rx, ry, rw, rh))
		//return el;
	//else
		changePos(el, QPoint(r.x(), r.y()));
*/
	moveWidgetBy(el, container, QPoint(0, 0));
}

void
PasteWidgetCommand::moveWidgetBy(QDomElement &el, Container *container, const QPoint &p)
{
	QDomElement rect;
	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "geometry"))
			rect = n.firstChild().toElement();
	}

	QDomElement x = rect.namedItem("x").toElement();
	QDomElement y = rect.namedItem("y").toElement();
	QDomElement wi = rect.namedItem("width").toElement();
	QDomElement h = rect.namedItem("height").toElement();

	int rx = x.text().toInt();
	int ry = y.text().toInt();
	int rw = wi.text().toInt();
	int rh = h.text().toInt();
	QRect r(rx + p.x(), ry + p.y(), rw, rh);
	kdDebug() << "Moving widget by " << p << " from " << rx << "  " << ry << " to " << r.topLeft() << endl; 

	QWidget *w = m_form->widget()->childAt(r.x() + 6, r.y() + 6, false);

	while(w && (w->geometry() == r))// there is already a widget there, with the same size
	{
		w = m_form->widget()->childAt(w->x() + 16, w->y() + 16, false);
		r.moveBy(10,10);
	}

	// the pasted wigdet should stay inside container's boudaries
	if(r.x() < 0)
		r.setX(0);
	else if(r.right() > container->widget()->width())
		r.setX(container->widget()->width() - r.width());

	if(r.y() < 0)
		r.setY(0);
	else if(r.bottom() > container->widget()->height())
		r.setY(container->widget()->height() - r.height());

	if(r != QRect(rx, ry, rw, rh))
		//return el;
	//else
		changePos(el, QPoint(r.x(), r.y()));
}

void
PasteWidgetCommand::fixNames(QDomElement &el)
{
	QString wname;
	for(QDomNode n = el.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if((n.toElement().tagName() == "property") && (n.toElement().attribute("name") == "name"))
		{
			wname = n.toElement().text();
			while(m_form->objectTree()->lookup(wname)) // name already exists
			{
				bool ok;
				int num = wname.right(1).toInt(&ok, 10);
				if(ok)
					wname = wname.left(wname.length()-1) + QString::number(num+1);
				else
					wname += "2";
			}
			if(wname != n.toElement().text()) // we change the name, so we recreate the element
			{
				n.removeChild(n.firstChild());
				QDomElement type = el.ownerDocument().createElement("string");
				QDomText valueE = el.ownerDocument().createTextNode(wname);
				type.appendChild(valueE);
				n.toElement().appendChild(type);
			}

		}
		if(n.toElement().tagName() == "widget") // fix child widgets names
		{
			QDomElement child = n.toElement();
			fixNames(child);
		}
	}

}

// DeleteWidgetCommand

DeleteWidgetCommand::DeleteWidgetCommand(WidgetList &list, Form *form)
 : KCommand(), m_form(form)
{
	m_domDoc = QDomDocument("UI");
	m_domDoc.appendChild(m_domDoc.createElement("UI"));

	QDomElement parent = m_domDoc.namedItem("UI").toElement();

	//for(QWidget *w = list.first(); w; w = list.next())
	/*for(WidgetListIterator it(list); it.current() != 0; ++it)
	{
		QWidget *w = it.current();
		// Don't delete tabwidget or widgetstack pages
		if(w->parentWidget()->inherits("QWidgetStack"))
		{
			list.remove(w);
			continue;
		}
	}*/
	removeChildrenFromList(list);

	for(WidgetListIterator it(list); it.current() != 0; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.current()->name());
		if (!item)
			return;

		// We need to store both parentContainer and parentWidget as they may be different (eg for TabWidget page)
		m_containers.insert(item->name(), m_form->parentContainer(item->widget())->widget()->name());
		m_parents.insert(item->name(), item->parent()->name());
		FormIO::saveWidget(item, parent, m_domDoc);
		form->connectionBuffer()->saveAllConnectionsForWidget(item->widget()->name(), m_domDoc);
	}

	FormIO::cleanClipboard(parent);
}

void
DeleteWidgetCommand::execute()
{
	QMap<QString,QString>::ConstIterator endIt = m_containers.constEnd();
	for(QMap<QString,QString>::ConstIterator  it = m_containers.constBegin(); it != endIt; ++it)
	{
		ObjectTreeItem *item = m_form->objectTree()->lookup(it.key());
		if (!item || !item->widget())
			continue;

		Container *cont = m_form->parentContainer(item->widget());
		cont->deleteWidget(item->widget());
	}
}

void
DeleteWidgetCommand::unexecute()
{
	QString wname;
	m_form->setInteractiveMode(false);
	for(QDomNode n = m_domDoc.namedItem("UI").firstChild(); !n.isNull(); n = n.nextSibling())
	{
		if(n.toElement().tagName() == "connections") // restore the widget connections
			m_form->connectionBuffer()->load(n);
		if(n.toElement().tagName() != "widget")
			continue;
		// We need first to know the name of the widget
		for(QDomNode m = n.firstChild(); !m.isNull(); n = m.nextSibling())
		{
			if((m.toElement().tagName() == "property") && (m.toElement().attribute("name") == "name"))
			{
				wname = m.toElement().text();
				break;
			}
		}

		Container *cont = m_form->objectTree()->lookup(m_containers[wname])->container();
		ObjectTreeItem *parent = m_form->objectTree()->lookup(m_parents[wname]);
		QDomElement widg = n.toElement();
		if(parent)
			FormIO::loadWidget(cont, m_form->manager()->lib(), widg, parent->widget());
		else
			FormIO::loadWidget(cont, m_form->manager()->lib(), widg);
	}
	m_form->setInteractiveMode(true);
}

QString
DeleteWidgetCommand::name() const
{
	return i18n("Delete widget");
}

// CutWidgetCommand

CutWidgetCommand::CutWidgetCommand(WidgetList &list, Form *form)
 : DeleteWidgetCommand(list, form)
{}

void
CutWidgetCommand::execute()
{
	DeleteWidgetCommand::execute();
	m_data = m_form->manager()->m_domDoc.toCString();
	m_form->manager()->m_domDoc.setContent(m_domDoc.toCString());
}

void
CutWidgetCommand::unexecute()
{
	DeleteWidgetCommand::unexecute();
	m_form->manager()->m_domDoc.setContent(m_data);
}

QString
CutWidgetCommand::name() const
{
	return i18n("Cut");
}

}

