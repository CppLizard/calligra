/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>

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

#include <handler.h>
#include <koView.h>
#include <koFrame.h>
#include <math.h>
#include <qwmatrix.h>
#include <kdebug.h>

EventHandler::EventHandler( QObject* target )
{
    m_target = target;

    m_target->installEventFilter( this );
}

EventHandler::~EventHandler()
{
}

QObject* EventHandler::target()
{
    return m_target;
}

// ------------------------------------------------------

class PartResizeHandlerPrivate {
public:
    PartResizeHandlerPrivate( const QWMatrix& matrix, KoView *view, KoChild* child,
			      KoChild::Gadget gadget, const QPoint& point ) :
	m_gadget(gadget), m_view(view), m_child(child), m_parentMatrix(matrix) {
	
	m_geometryStart = child->geometry();
	m_matrix = child->matrix() * matrix;
	m_invertParentMatrix = matrix.invert();
	
	bool ok = true;
	m_invert = m_matrix.invert( &ok );
	ASSERT( ok );
	m_mouseStart = m_invert.map( m_invertParentMatrix.map( point ) );
    }
    ~PartResizeHandlerPrivate() {}

    KoChild::Gadget m_gadget;
    QPoint m_mouseStart;
    QRect m_geometryStart;
    KoView* m_view;
    KoChild* m_child;
    QWMatrix m_invert;
    QWMatrix m_matrix;
    QWMatrix m_parentMatrix;
    QWMatrix m_invertParentMatrix;
};

PartResizeHandler::PartResizeHandler( QWidget* widget, const QWMatrix& matrix, KoView* view, KoChild* child,
				      KoChild::Gadget gadget, const QPoint& point )
    : EventHandler( widget )
{
    child->lock();
    d=new PartResizeHandlerPrivate(matrix, view, child, gadget, point);
}

PartResizeHandler::~PartResizeHandler()
{
    d->m_child->unlock();
    delete d;
    d=0L;
}

void PartResizeHandler::repaint(QRegion &rgn)
{
  rgn = rgn.unite( d->m_child->frameRegion( d->m_parentMatrix, true ) );
  rgn.translate(- d->m_view->canvasXOffset(), - d->m_view->canvasYOffset());
  ((QWidget*)target())->repaint( rgn );
}

bool PartResizeHandler::eventFilter( QObject*, QEvent* ev )
{
    if ( ev->type() == QEvent::MouseButtonRelease )
    {
	delete this;
	return true;
    }
    else if ( ev->type() == QEvent::MouseMove )
    {
	QMouseEvent* e = (QMouseEvent*)ev;
	QPoint p = d->m_invert.map( d->m_invertParentMatrix.map( e->pos() + QPoint(d->m_view->canvasXOffset(), d->m_view->canvasYOffset()) ) );
	QRegion rgn( d->m_child->frameRegion( d->m_parentMatrix, true ) );

	double x1_x, x1_y, x2_x, x2_y;
	d->m_matrix.map( double( p.x() ), 0.0, &x1_x, &x1_y );
	d->m_matrix.map( double( d->m_mouseStart.x() ), 0.0, &x2_x, &x2_y );		
	double y1_x, y1_y, y2_x, y2_y;
	d->m_matrix.map( 0.0, double( p.y() ), &y1_x, &y1_y );
	d->m_matrix.map( 0.0, double( d->m_mouseStart.y() ), &y2_x, &y2_y );		
		
	double dx = x2_x - x1_x;
	double dy = x2_y - x1_y;
	int x = int( std::sqrt( dx * dx + dy * dy ) * ( d->m_mouseStart.x() < p.x() ? 1.0 : -1.0 ) );
		
	dx = y2_x - y1_x;
	dy = y2_y - y1_y;
	int y = int( std::sqrt( dx * dx + dy * dy ) * ( d->m_mouseStart.y() < p.y() ? 1.0 : -1.0 ) );

	switch( d->m_gadget )
        {
	case KoChild::TopLeft:
	    {
		x = QMIN( d->m_geometryStart.width() - 1, x );
		y = QMIN( d->m_geometryStart.height() - 1, y );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x() + x, d->m_geometryStart.y() + y,
					     d->m_geometryStart.width() - x, d->m_geometryStart.height() - y ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::TopMid:
	    {
		y = QMIN( d->m_geometryStart.height() - 1, y );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x(), d->m_geometryStart.y() + y,
					     d->m_geometryStart.width(), d->m_geometryStart.height() - y ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::TopRight:
	    {
		x = QMAX( -d->m_geometryStart.width() + 1, x );
		y = QMIN( d->m_geometryStart.height() - 1, y );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x(), d->m_geometryStart.y() + y,
					     d->m_geometryStart.width() + x, d->m_geometryStart.height() - y ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::MidLeft:
	    {
		x = QMIN( d->m_geometryStart.width() - 1, x );
		
		d->m_child->setGeometry( QRect( d->m_geometryStart.x() + x, d->m_geometryStart.y(),
					     d->m_geometryStart.width() - x, d->m_geometryStart.height() ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::MidRight:
	    {
		x = QMAX( -d->m_geometryStart.width() + 1, x );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x(), d->m_geometryStart.y(),
					     d->m_geometryStart.width() + x, d->m_geometryStart.height() ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::BottomLeft:
	    {
		x = QMIN( d->m_geometryStart.width() - 1, x );
		y = QMAX( -d->m_geometryStart.height() + 1, y );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x() + x, d->m_geometryStart.y(),
					     d->m_geometryStart.width() - x, d->m_geometryStart.height() + y ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::BottomMid:
	    {
		y = QMAX( -d->m_geometryStart.height() + 1, y );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x(), d->m_geometryStart.y(),
					     d->m_geometryStart.width(), d->m_geometryStart.height() + y ) );
		repaint(rgn);
	    }
	    break;
	case KoChild::BottomRight:
	    {
		x = QMAX( -d->m_geometryStart.width() + 1, x );
		y = QMAX( -d->m_geometryStart.height() + 1, y );

		d->m_child->setGeometry( QRect( d->m_geometryStart.x(), d->m_geometryStart.y(),
					     d->m_geometryStart.width() + x, d->m_geometryStart.height() + y ) );
		repaint(rgn);
	    }
	    break;
	default:
	    ASSERT( 0 );
	}
	return true;
    }
    return false;
}

// --------------------------------------------------------------

class PartMoveHandlerPrivate {
public:
    PartMoveHandlerPrivate( const QWMatrix& matrix, KoView* view, KoChild* child,
			    const QPoint& point) : m_view(view), m_dragChild(child),
						   m_parentMatrix(matrix) {					
	m_invertParentMatrix = matrix.invert();
	m_mouseDragStart = m_invertParentMatrix.map( point );
	m_geometryDragStart = m_dragChild->geometry();
	m_rotationDragStart = m_dragChild->rotationPoint();
    }
    ~PartMoveHandlerPrivate() {}

    KoView* m_view;
    KoChild* m_dragChild;
    QPoint m_mouseDragStart;
    QRect m_geometryDragStart;
    QPoint m_rotationDragStart;
    QWMatrix m_invertParentMatrix;
    QWMatrix m_parentMatrix;
};

PartMoveHandler::PartMoveHandler( QWidget* widget, const QWMatrix& matrix, KoView* view, KoChild* child,
				  const QPoint& point )
    : EventHandler( widget )
{
    child->lock();
    d=new PartMoveHandlerPrivate(matrix, view, child, point);
}

PartMoveHandler::~PartMoveHandler()
{
    d->m_dragChild->unlock();
    delete d;
    d=0L;
}

bool PartMoveHandler::eventFilter( QObject*, QEvent* ev )
{
    if ( ev->type() == QEvent::MouseButtonRelease )
    {
	delete this;
	return true;
    }
    else if ( ev->type() == QEvent::MouseMove )
    {
	QMouseEvent* e = (QMouseEvent*)ev;
	
	QRegion bound = d->m_dragChild->frameRegion( d->m_parentMatrix, true );
	QPoint pos = d->m_invertParentMatrix.map( e->pos()  + QPoint(d->m_view->canvasXOffset(), d->m_view->canvasYOffset()) );
	d->m_dragChild->setGeometry( QRect( d->m_geometryDragStart.x() + pos.x() - d->m_mouseDragStart.x(),
					     d->m_geometryDragStart.y() + pos.y() - d->m_mouseDragStart.y(),
					     d->m_geometryDragStart.width(), d->m_geometryDragStart.height() ) );
	d->m_dragChild->setRotationPoint( QPoint( d->m_rotationDragStart.x() + pos.x() - d->m_mouseDragStart.x(),
					       d->m_rotationDragStart.y() + pos.y() - d->m_mouseDragStart.y() ) );
        bound = bound.unite( d->m_dragChild->frameRegion( d->m_parentMatrix, false ) );
	bound.translate(- d->m_view->canvasXOffset(), - d->m_view->canvasYOffset());
	((QWidget*)target())->repaint( bound );

	return true;
    }

    return false;
}

// -------------------------------------------------------

ContainerHandler::ContainerHandler( KoView* view, QWidget* widget )
    : EventHandler( widget )
{
    m_view = view;
}

ContainerHandler::~ContainerHandler()
{
}

bool ContainerHandler::eventFilter( QObject*, QEvent* ev )
{
    if ( ev->type() == QEvent::MouseButtonPress )
    {
	QMouseEvent* e = (QMouseEvent*)ev;
        QPoint pos = e->pos() + QPoint(m_view->canvasXOffset(), m_view->canvasYOffset());

	KoChild *child = 0;
	KoChild::Gadget gadget = KoChild::NoGadget;
	KoDocumentChild* docChild = m_view->selectedChild();
	if ( docChild )
	{
	    KoViewChild *viewChild = m_view->child( docChild->document() );
	
	    if ( viewChild )
	      child = viewChild;
	    else
	      child = docChild;
	
	    gadget = child->gadgetHitTest( pos, m_view->matrix() );
	}
	if ( gadget == KoChild::NoGadget )
        {
	    docChild = m_view->activeChild();
	    if ( docChild )
	    {
	        KoViewChild *viewChild = m_view->child( docChild->document() );
		
		if ( viewChild )
		  child = viewChild;
		else
		  child = docChild;
		
  		gadget = child->gadgetHitTest( pos, m_view->matrix() );
	    }
	}
	
	if ( e->button() == LeftButton && gadget == KoChild::Move )
        {
	    (void)new PartMoveHandler( (QWidget*)target(), m_view->matrix(), m_view, child, pos );
	    return true;
	}
	else if ( e->button() == LeftButton && gadget != KoChild::NoGadget )
        {
	    (void)new PartResizeHandler( (QWidget*)target(), m_view->matrix(), m_view, child, gadget, pos );
	    return true;
	}

	return false;
    }
    else if ( ev->type() == QEvent::MouseMove )
    {
        QWidget *targetWidget = static_cast<QWidget *>( target() );
	QMouseEvent* e = (QMouseEvent*)ev;
        QPoint pos = e->pos() + QPoint(m_view->canvasXOffset(), m_view->canvasYOffset());
	
	bool retval = true;

	KoChild *child = 0;
	KoDocumentChild* docChild = m_view->selectedChild();
	KoChild::Gadget gadget = KoChild::NoGadget;
	if ( docChild )
	{
	    KoViewChild *viewChild = m_view->child( docChild->document() );
	
	    if ( viewChild )
	      child = viewChild;
	    else
	      child = docChild;
	
	    gadget = child->gadgetHitTest( pos, m_view->matrix() );
	}
	if ( gadget == KoChild::NoGadget )
        {
	    docChild = m_view->activeChild();
	    if ( docChild )
	    {
  	      KoViewChild *viewChild = m_view->child( docChild->document() );
	
	      if ( viewChild )
	        child = viewChild;
	      else
	        child = docChild;
		
	      gadget = child->gadgetHitTest( pos, m_view->matrix() );
	    }
	    retval = false;
	}

	if ( gadget == KoChild::TopLeft || gadget == KoChild::BottomRight )
	    targetWidget->setCursor( sizeFDiagCursor );
	else if ( gadget == KoChild::TopRight || gadget == KoChild::BottomLeft )
	    targetWidget->setCursor( sizeBDiagCursor );	
	else if ( gadget == KoChild::TopMid || gadget == KoChild::BottomMid )
	    targetWidget->setCursor( sizeVerCursor );
	else if ( gadget == KoChild::MidLeft || gadget == KoChild::MidRight )
	    targetWidget->setCursor( sizeHorCursor );
	else if ( gadget == KoChild::Move )
	    targetWidget->setCursor( pointingHandCursor );
	else
        {
	    targetWidget->setCursor( arrowCursor );
	    return false;
	}
	//	return true;
	return retval;
    }

    return false;
}

#include <handler.moc>
