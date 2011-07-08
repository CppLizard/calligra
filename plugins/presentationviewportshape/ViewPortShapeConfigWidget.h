/* This file is part of the KDE project
 * Copyright (C) 2011 Aakriti Gupta<aakriti.a.gupta@gmail.com>
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

//#ifndef RECTANGLESHAPECONFIGWIDGET_H
//#define RECTANGLESHAPECONFIGWIDGET_H

#include <ui_ViewPortShapeConfigWidget.h>

#include <KoShapeConfigWidgetBase.h>
#include "PresentationViewPortShape.h"
//#include "rectangle/RectangleShapeConfigWidget.h"

class ViewPortShapeConfigWidget: public KoShapeConfigWidgetBase
{
    //Q_OBJECT
public:
    ViewPortShapeConfigWidget();
    /// reimplemented
    virtual void open(KoShape *shape);
    /// reimplemented
    virtual void save();
    /// reimplemented
    virtual void setUnit(KoUnit unit);
    /// reimplemented
    virtual bool showOnShapeCreate() { return false; }
    /// reimplemented
    virtual QUndoCommand *createCommand();

private:
    Ui::ViewPortShapeConfigWidget widget;
    PresentationViewPortShape *m_rectangle;
    
};

//#endif // RECTANGLESHAPECONFIGWIDGET_H
