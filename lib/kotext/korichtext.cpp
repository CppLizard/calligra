/****************************************************************************
** Implementation of the internal Qt classes dealing with rich text
**
** Created : 990101
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "korichtext.h"
#include "qclipboard.h"
#include "qdragobject.h"
#include "qpaintdevicemetrics.h"
#include "qdrawutil.h"

#include <stdlib.h>
#include "koparagcounter.h"
#include "kotextdocument.h"
#include <kdebug.h>
#include <kdeversion.h>
#if ! KDE_IS_VERSION(3,1,90)
#include <kdebugclasses.h>
#endif
#include <kglobal.h>
#include <klocale.h>

//#define PARSER_DEBUG
//#define DEBUG_COLLECTION
//#define DEBUG_TABLE_RENDERING

static KoTextFormatCollection *qFormatCollection = 0;

#if defined(PARSER_DEBUG)
static QString debug_indent;
#endif

static bool is_printer( QPainter *p )
{
    return p && p->device() && p->device()->devType() == QInternal::Printer;
}

static inline int scale( int value, QPainter *painter )
{
    if ( is_printer( painter ) ) {
	QPaintDeviceMetrics metrics( painter->device() );
#if defined(Q_WS_X11)
	value = value * metrics.logicalDpiY() / QPaintDevice::x11AppDpiY();
#elif defined (Q_WS_WIN)
	int gdc = GetDeviceCaps( GetDC( 0 ), LOGPIXELSY );
	if ( gdc )
	    value = value * metrics.logicalDpiY() / gdc;
#elif defined (Q_WS_MAC)
	value = value * metrics.logicalDpiY() / 75; // ##### FIXME
#elif defined (Q_WS_QWS)
	value = value * metrics.logicalDpiY() / 75;
#endif
    }
    return value;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void KoTextDocCommandHistory::addCommand( KoTextDocCommand *cmd )
{
    if ( current < (int)history.count() - 1 ) {
	QPtrList<KoTextDocCommand> commands;
	commands.setAutoDelete( FALSE );

	for( int i = 0; i <= current; ++i ) {
	    commands.insert( i, history.at( 0 ) );
	    history.take( 0 );
	}

	commands.append( cmd );
	history.clear();
	history = commands;
	history.setAutoDelete( TRUE );
    } else {
	history.append( cmd );
    }

    if ( (int)history.count() > steps )
	history.removeFirst();
    else
	++current;
}

KoTextCursor *KoTextDocCommandHistory::undo( KoTextCursor *c )
{
    if ( current > -1 ) {
	KoTextCursor *c2 = history.at( current )->unexecute( c );
	--current;
	return c2;
    }
    return 0;
}

KoTextCursor *KoTextDocCommandHistory::redo( KoTextCursor *c )
{
    if ( current > -1 ) {
	if ( current < (int)history.count() - 1 ) {
	    ++current;
	    return history.at( current )->execute( c );
	}
    } else {
	if ( history.count() > 0 ) {
	    ++current;
	    return history.at( current )->execute( c );
	}
    }
    return 0;
}

bool KoTextDocCommandHistory::isUndoAvailable()
{
    return current > -1;
}

bool KoTextDocCommandHistory::isRedoAvailable()
{
   return current > -1 && current < (int)history.count() - 1 || current == -1 && history.count() > 0;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextDocDeleteCommand::KoTextDocDeleteCommand( KoTextDocument *d, int i, int idx, const QMemArray<KoTextStringChar> &str )
    : KoTextDocCommand( d ), id( i ), index( idx ), parag( 0 ), text( str )
{
    for ( int j = 0; j < (int)text.size(); ++j ) {
	if ( text[ j ].format() )
	    text[ j ].format()->addRef();
    }
}

/*KoTextDocDeleteCommand::KoTextDocDeleteCommand( KoTextParag *p, int idx, const QMemArray<KoTextStringChar> &str )
    : KoTextDocCommand( 0 ), id( -1 ), index( idx ), parag( p ), text( str )
{
    for ( int i = 0; i < (int)text.size(); ++i ) {
	if ( text[ i ].format() )
	    text[ i ].format()->addRef();
    }
}*/

KoTextDocDeleteCommand::~KoTextDocDeleteCommand()
{
    for ( int i = 0; i < (int)text.size(); ++i ) {
	if ( text[ i ].format() )
	    text[ i ].format()->removeRef();
    }
    text.resize( 0 );
}

KoTextCursor *KoTextDocDeleteCommand::execute( KoTextCursor *c )
{
    KoTextParag *s = doc ? doc->paragAt( id ) : parag;
    if ( !s ) {
	qWarning( "can't locate parag at %d, last parag: %d", id, doc->lastParag()->paragId() );
	return 0;
    }

    cursor.setParag( s );
    cursor.setIndex( index );
    int len = text.size();
    if ( c )
	*c = cursor;
    if ( doc ) {
	doc->setSelectionStart( KoTextDocument::Temp, &cursor );
	for ( int i = 0; i < len; ++i )
	    cursor.gotoNextLetter();
	doc->setSelectionEnd( KoTextDocument::Temp, &cursor );
	doc->removeSelectedText( KoTextDocument::Temp, &cursor );
	if ( c )
	    *c = cursor;
    } else {
	s->remove( index, len );
    }

    return c;
}

KoTextCursor *KoTextDocDeleteCommand::unexecute( KoTextCursor *c )
{
    KoTextParag *s = doc ? doc->paragAt( id ) : parag;
    if ( !s ) {
	qWarning( "can't locate parag at %d, last parag: %d", id, doc->lastParag()->paragId() );
	return 0;
    }

    cursor.setParag( s );
    cursor.setIndex( index );
    QString str = KoTextString::toString( text );
    cursor.insert( str, TRUE, &text );
    cursor.setParag( s );
    cursor.setIndex( index );
    if ( c ) {
	c->setParag( s );
	c->setIndex( index );
	for ( int i = 0; i < (int)text.size(); ++i )
	    c->gotoNextLetter();
    }

    s = cursor.parag();
    while ( s ) {
	s->format();
	s->setChanged( TRUE );
	if ( s == c->parag() )
	    break;
	s = s->next();
    }

    return &cursor;
}

KoTextDocFormatCommand::KoTextDocFormatCommand( KoTextDocument *d, int sid, int sidx, int eid, int eidx,
					const QMemArray<KoTextStringChar> &old, KoTextFormat *f, int fl )
    : KoTextDocCommand( d ), startId( sid ), startIndex( sidx ), endId( eid ), endIndex( eidx ), format( f ), oldFormats( old ), flags( fl )
{
    format = d->formatCollection()->format( f );
    for ( int j = 0; j < (int)oldFormats.size(); ++j ) {
	if ( oldFormats[ j ].format() )
	    oldFormats[ j ].format()->addRef();
    }
}

KoTextDocFormatCommand::~KoTextDocFormatCommand()
{
    format->removeRef();
    for ( int j = 0; j < (int)oldFormats.size(); ++j ) {
	if ( oldFormats[ j ].format() )
	    oldFormats[ j ].format()->removeRef();
    }
}

KoTextCursor *KoTextDocFormatCommand::execute( KoTextCursor *c )
{
    KoTextParag *sp = doc->paragAt( startId );
    KoTextParag *ep = doc->paragAt( endId );
    if ( !sp || !ep )
	return c;

    KoTextCursor start( doc );
    start.setParag( sp );
    start.setIndex( startIndex );
    KoTextCursor end( doc );
    end.setParag( ep );
    end.setIndex( endIndex );

    doc->setSelectionStart( KoTextDocument::Temp, &start );
    doc->setSelectionEnd( KoTextDocument::Temp, &end );
    doc->setFormat( KoTextDocument::Temp, format, flags );
    doc->removeSelection( KoTextDocument::Temp );
    if ( endIndex == ep->length() ) // ### Not in QRT - report sent. Description at http://bugs.kde.org/db/34/34556.html
        end.gotoLeft();
    *c = end;
    return c;
}

KoTextCursor *KoTextDocFormatCommand::unexecute( KoTextCursor *c )
{
    KoTextParag *sp = doc->paragAt( startId );
    KoTextParag *ep = doc->paragAt( endId );
    if ( !sp || !ep )
	return 0;

    int idx = startIndex;
    int fIndex = 0;
    if( !oldFormats.isEmpty()) // ## not in QRT. Not sure how it can happen.
    {
    for ( ;; ) {
	if ( oldFormats.at( fIndex ).c == '\n' ) {
	    if ( idx > 0 ) {
		if ( idx < sp->length() && fIndex > 0 )
		    sp->setFormat( idx, 1, oldFormats.at( fIndex - 1 ).format() );
		if ( sp == ep )
		    break;
		sp = sp->next();
		idx = 0;
	    }
	    fIndex++;
	}
	if ( oldFormats.at( fIndex ).format() )
	    sp->setFormat( idx, 1, oldFormats.at( fIndex ).format() );
	idx++;
	fIndex++;
	if ( fIndex >= (int)oldFormats.size() )
	    break;
	if ( idx >= sp->length() ) {
	    if ( sp == ep )
		break;
	    sp = sp->next();
	    idx = 0;
	}
    }
    }
    KoTextCursor end( doc );
    end.setParag( ep );
    end.setIndex( endIndex );
    if ( endIndex == ep->length() )
        end.gotoLeft();
    *c = end;
    return c;
}

KoTextAlignmentCommand::KoTextAlignmentCommand( KoTextDocument *d, int fParag, int lParag, int na, const QMemArray<int> &oa )
    : KoTextDocCommand( d ), firstParag( fParag ), lastParag( lParag ), newAlign( na ), oldAligns( oa )
{
}

KoTextCursor *KoTextAlignmentCommand::execute( KoTextCursor *c )
{
    KoTextParag *p = doc->paragAt( firstParag );
    if ( !p )
	return c;
    while ( p ) {
	p->setAlignment( newAlign );
	if ( p->paragId() == lastParag )
	    break;
	p = p->next();
    }
    return c;
}

KoTextCursor *KoTextAlignmentCommand::unexecute( KoTextCursor *c )
{
    KoTextParag *p = doc->paragAt( firstParag );
    if ( !p )
	return c;
    int i = 0;
    while ( p ) {
	if ( i < (int)oldAligns.size() )
	    p->setAlignment( oldAligns.at( i ) );
	if ( p->paragId() == lastParag )
	    break;
	p = p->next();
	++i;
    }
    return c;
}


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextCursor::KoTextCursor( KoTextDocument *d )
    : doc( d ), ox( 0 ), oy( 0 )
{
    nested = FALSE;
    idx = 0;
    string = doc ? doc->firstParag() : 0;
    tmpIndex = -1;
}

KoTextCursor::KoTextCursor()
{
}

KoTextCursor::KoTextCursor( const KoTextCursor &c )
{
    doc = c.doc;
    ox = c.ox;
    oy = c.oy;
    nested = c.nested;
    idx = c.idx;
    string = c.string;
    tmpIndex = c.tmpIndex;
    indices = c.indices;
    parags = c.parags;
    xOffsets = c.xOffsets;
    yOffsets = c.yOffsets;
}

KoTextCursor &KoTextCursor::operator=( const KoTextCursor &c )
{
    doc = c.doc;
    ox = c.ox;
    oy = c.oy;
    nested = c.nested;
    idx = c.idx;
    string = c.string;
    tmpIndex = c.tmpIndex;
    indices = c.indices;
    parags = c.parags;
    xOffsets = c.xOffsets;
    yOffsets = c.yOffsets;

    return *this;
}

bool KoTextCursor::operator==( const KoTextCursor &c ) const
{
    return doc == c.doc && string == c.string && idx == c.idx;
}

int KoTextCursor::totalOffsetX() const
{
    if ( !nested )
	return 0;
    QValueStack<int>::ConstIterator xit = xOffsets.begin();
    int xoff = ox;
    for ( ; xit != xOffsets.end(); ++xit )
	xoff += *xit;
    return xoff;
}

int KoTextCursor::totalOffsetY() const
{
    if ( !nested )
	return 0;
    QValueStack<int>::ConstIterator yit = yOffsets.begin();
    int yoff = oy;
    for ( ; yit != yOffsets.end(); ++yit )
	yoff += *yit;
    return yoff;
}

void KoTextCursor::gotoIntoNested( const QPoint &globalPos )
{
    if ( !doc )
	return;
    push();
    ox = 0;
    int bl, y;
    string->lineHeightOfChar( idx, &bl, &y );
    oy = y + string->rect().y();
    nested = TRUE;
    QPoint p( globalPos.x() - offsetX(), globalPos.y() - offsetY() );
    Q_ASSERT( string->at( idx )->isCustom() );
    ox = string->at( idx )->x;
    string->at( idx )->customItem()->enterAt( this, doc, string, idx, ox, oy, p );
}

void KoTextCursor::invalidateNested()
{
    if ( nested ) {
	QValueStack<KoTextParag*>::Iterator it = parags.begin();
	QValueStack<int>::Iterator it2 = indices.begin();
	for ( ; it != parags.end(); ++it, ++it2 ) {
	    if ( *it == string )
		continue;
	    (*it)->invalidate( 0 );
	    if ( (*it)->at( *it2 )->isCustom() )
		(*it)->at( *it2 )->customItem()->invalidate();
	}
    }
}

void KoTextCursor::insert( const QString &str, bool checkNewLine, QMemArray<KoTextStringChar> *formatting )
{
    string->invalidate( idx );
    tmpIndex = -1;
    bool justInsert = TRUE;
    QString s( str );
#if defined(Q_WS_WIN)
    if ( checkNewLine )
	s = s.replace( QRegExp( "\\r" ), "" );
#endif
    if ( checkNewLine )
	justInsert = s.find( '\n' ) == -1;
    if ( justInsert ) {
	string->insert( idx, s );
	if ( formatting ) {
	    for ( int i = 0; i < (int)s.length(); ++i ) {
		if ( formatting->at( i ).format() ) {
		    formatting->at( i ).format()->addRef();
		    string->string()->setFormat( idx + i, formatting->at( i ).format(), TRUE );
		}
	    }
	}
	idx += s.length();
    } else {
	QStringList lst = QStringList::split( '\n', s, TRUE );
	QStringList::Iterator it = lst.begin();
	int y = string->rect().y() + string->rect().height();
	int lastIndex = 0;
	KoTextFormat *lastFormat = 0;
	for ( ; it != lst.end(); ) {
	    if ( it != lst.begin() ) {
		splitAndInsertEmptyParag( FALSE, TRUE );
		//string->setEndState( -1 );
		string->prev()->format( -1, FALSE );
		if ( lastFormat && formatting && string->prev() ) {
		    lastFormat->addRef();
		    string->prev()->string()->setFormat( string->prev()->length() - 1, lastFormat, TRUE );
		}
	    }
	    lastFormat = 0;
	    QString s = *it;
	    ++it;
	    if ( !s.isEmpty() )
		string->insert( idx, s );
            else
                string->invalidate( 0 );

	    if ( formatting ) {
		int len = s.length();
		for ( int i = 0; i < len; ++i ) {
		    if ( formatting->at( i + lastIndex ).format() ) {
			formatting->at( i + lastIndex ).format()->addRef();
			string->string()->setFormat( i + idx, formatting->at( i + lastIndex ).format(), TRUE );
		    }
		}
		if ( it != lst.end() )
		    lastFormat = formatting->at( len + lastIndex ).format();
		++len;
		lastIndex += len;
	    }

	    idx += s.length();
	}
	string->format( -1, FALSE );
	int dy = string->rect().y() + string->rect().height() - y;
	KoTextParag *p = string;
	p->setParagId( p->prev()->paragId() + 1 );
	p = p->next();
	while ( p ) {
	    p->setParagId( p->prev()->paragId() + 1 );
	    p->move( dy );
	    p->invalidate( 0 );
	    //p->setEndState( -1 );
	    p = p->next();
	}
    }

#if 0  //// useless and slow
    int h = string->rect().height();
    string->format( -1, TRUE );
    if ( h != string->rect().height() )
	invalidateNested();
    else if ( doc && doc->parent() )
	doc->nextDoubleBuffered = TRUE;
#endif
}

void KoTextCursor::gotoLeft()
{
    if ( string->string()->isRightToLeft() )
	gotoNextLetter();
    else
	gotoPreviousLetter();
}

void KoTextCursor::gotoPreviousLetter()
{
    tmpIndex = -1;

    if ( idx > 0 ) {
	idx--;
    } else if ( string->prev() ) {
	string = string->prev();
	while ( !string->isVisible() )
	    string = string->prev();
	idx = string->length() - 1;
    } else {
	if ( nested ) {
	    pop();
	    processNesting( Prev );
	    if ( idx == -1 ) {
		pop();
		if ( idx > 0 ) {
		    idx--;
		} else if ( string->prev() ) {
		    string = string->prev();
		    idx = string->length() - 1;
		}
	    }
	}
    }

    const KoTextStringChar *tsc = string->at( idx );
    if ( tsc && tsc->isCustom() && tsc->customItem()->isNested() ) {
	processNesting( EnterEnd );
    }
}

void KoTextCursor::push()
{
    indices.push( idx );
    parags.push( string );
    xOffsets.push( ox );
    yOffsets.push( oy );
    nestedStack.push( nested );
}

void KoTextCursor::pop()
{
    if ( !doc )
	return;
    idx = indices.pop();
    string = parags.pop();
    ox = xOffsets.pop();
    oy = yOffsets.pop();
    //if ( doc->parent() )
    //doc = doc->parent();
    nested = nestedStack.pop();
}

void KoTextCursor::restoreState()
{
    while ( !indices.isEmpty() )
	pop();
}

bool KoTextCursor::place( const QPoint &p, KoTextParag *s, bool link, int *customItemIndex )
{
    if ( customItemIndex )
        *customItemIndex = -1;
    QPoint pos( p );
    QRect r;
    if ( pos.y() < s->rect().y() )
	pos.setY( s->rect().y() );
    while ( s ) {
	r = s->rect();
	r.setWidth( doc ? doc->width() : QWIDGETSIZE_MAX );
	if ( !s->next() || ( pos.y() >= r.y() && pos.y() < s->next()->rect().y() ) )
	    break;
	s = s->next();
    }

    if ( !s )
	return FALSE;

    setParag( s, FALSE );
    int y = s->rect().y();
    int lines = s->lines();
    KoTextStringChar *chr = 0;
    int index = 0;
    int i = 0;
    int cy = 0;
    //int ch = 0;
    for ( ; i < lines; ++i ) {
	chr = s->lineStartOfLine( i, &index );
	cy = s->lineY( i );
	//ch = s->lineHeight( i );
	if ( !chr )
	    return FALSE;
	if ( i < lines - 1 && pos.y() >= y + cy && pos.y() <= y + s->lineY( i+1 ) )
	    break;
    }
    int nextLine;
    if ( i < lines - 1 )
	s->lineStartOfLine( i+1, &nextLine );
    else
	nextLine = s->length();
    i = index;
    int x = s->rect().x();
    if ( pos.x() < x )
	pos.setX( x + 1 );
    int cw;
    int curpos = s->length()-1;
    int dist = 10000000;
    bool inCustom = FALSE;
    while ( i < nextLine ) {
	chr = s->at(i);
	int cpos = x + chr->x;
	cw = chr->width; //s->string()->width( i );
	if ( chr->isCustom() ) {
             if ( pos.x() >= cpos && pos.x() <= cpos + cw &&
                  pos.y() >= y + cy && pos.y() <= y + cy + chr->height() ) {
                if ( customItemIndex )
                    *customItemIndex = i;
                if ( chr->customItem()->isNested() )
                {
                    curpos = i;
                    inCustom = TRUE;
                    break;
                }
	    }
	}
        if( chr->rightToLeft )
            cpos += cw;
        int d = cpos - pos.x();
        bool dm = d < 0 ? !chr->rightToLeft : chr->rightToLeft;
        if ( QABS( d ) < dist || (dist == d && dm == TRUE ) ) {
            dist = QABS( d );
            if ( !link || pos.x() >= x + chr->x ) {
                curpos = i;
            }
        }
	i++;
    }
    setIndex( curpos, FALSE );

    if ( inCustom && doc && parag()->at( curpos )->isCustom() && parag()->at( curpos )->customItem()->isNested() ) {
	KoTextDocument *oldDoc = doc;
	pos.setX( pos.x() - parag()->at( curpos )->x );
	gotoIntoNested( pos );
	if ( oldDoc == doc )
	    return TRUE;
	QPoint p( pos.x() - offsetX(), pos.y() - offsetY() );
	if ( !place( p, document()->firstParag() ) )
	    pop();
    }
    return TRUE;
}

void KoTextCursor::processNesting( Operation op )
{
    if ( !doc )
	return;
    push();
    ox = string->at( idx )->x;
    int bl, y;
    string->lineHeightOfChar( idx, &bl, &y );
    oy = y + string->rect().y();
    nested = TRUE;
    bool ok = FALSE;

    switch ( op ) {
    case EnterBegin:
	ok = string->at( idx )->customItem()->enter( this, doc, string, idx, ox, oy );
	break;
    case EnterEnd:
	ok = string->at( idx )->customItem()->enter( this, doc, string, idx, ox, oy, TRUE );
	break;
    case Next:
	ok = string->at( idx )->customItem()->next( this, doc, string, idx, ox, oy );
	break;
    case Prev:
	ok = string->at( idx )->customItem()->prev( this, doc, string, idx, ox, oy );
	break;
    case Down:
	ok = string->at( idx )->customItem()->down( this, doc, string, idx, ox, oy );
	break;
    case Up:
	ok = string->at( idx )->customItem()->up( this, doc, string, idx, ox, oy );
	break;
    }
    if ( !ok )
	pop();
}

void KoTextCursor::gotoRight()
{
    if ( string->string()->isRightToLeft() )
	gotoPreviousLetter();
    else
	gotoNextLetter();
}

void KoTextCursor::gotoNextLetter()
{
    tmpIndex = -1;

    const KoTextStringChar *tsc = string->at( idx );
    if ( tsc && tsc->isCustom() && tsc->customItem()->isNested() ) {
	processNesting( EnterBegin );
	return;
    }

    if ( idx < string->length() - 1 ) {
	idx++;
    } else if ( string->next() ) {
	string = string->next();
	while ( !string->isVisible() )
	    string = string->next();
	idx = 0;
    } else {
	if ( nested ) {
	    pop();
	    processNesting( Next );
	    if ( idx == -1 ) {
		pop();
		if ( idx < string->length() - 1 ) {
		    idx++;
		} else if ( string->next() ) {
		    string = string->next();
		    idx = 0;
		}
	    }
	}
    }
}

void KoTextCursor::gotoUp()
{
    int indexOfLineStart;
    int line;
    KoTextStringChar *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    tmpIndex = QMAX( tmpIndex, idx - indexOfLineStart );
    if ( indexOfLineStart == 0 ) {
	if ( !string->prev() ) {
	    if ( !nested )
		return;
	    pop();
	    processNesting( Up );
	    if ( idx == -1 ) {
		pop();
		if ( !string->prev() )
		    return;
		idx = tmpIndex = 0;
	    } else {
		tmpIndex = -1;
		return;
	    }
	}
	string = string->prev();
	while ( !string->isVisible() )
	    string = string->prev();
	int lastLine = string->lines() - 1;
	if ( !string->lineStartOfLine( lastLine, &indexOfLineStart ) )
	    return;
	if ( indexOfLineStart + tmpIndex < string->length() )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = string->length() - 1;
    } else {
	--line;
	int oldIndexOfLineStart = indexOfLineStart;
	if ( !string->lineStartOfLine( line, &indexOfLineStart ) )
	    return;
	if ( indexOfLineStart + tmpIndex < oldIndexOfLineStart )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = oldIndexOfLineStart - 1;
    }
}

void KoTextCursor::gotoDown()
{
    int indexOfLineStart;
    int line;
    KoTextStringChar *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    tmpIndex = QMAX( tmpIndex, idx - indexOfLineStart );
    if ( line == string->lines() - 1 ) {
	if ( !string->next() ) {
	    if ( !nested )
		return;
	    pop();
	    processNesting( Down );
	    if ( idx == -1 ) {
		pop();
		if ( !string->next() )
		    return;
		idx = tmpIndex = 0;
	    } else {
		tmpIndex = -1;
		return;
	    }
	}
	string = string->next();
	while ( !string->isVisible() )
	    string = string->next();
	if ( !string->lineStartOfLine( 0, &indexOfLineStart ) )
	    return;
	int end;
	if ( string->lines() == 1 )
	    end = string->length();
	else
	    string->lineStartOfLine( 1, &end );
	if ( indexOfLineStart + tmpIndex < end )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = end - 1;
    } else {
	++line;
	int end;
	if ( line == string->lines() - 1 )
	    end = string->length();
	else
	    string->lineStartOfLine( line + 1, &end );
	if ( !string->lineStartOfLine( line, &indexOfLineStart ) )
	    return;
	if ( indexOfLineStart + tmpIndex < end )
	    idx = indexOfLineStart + tmpIndex;
	else
	    idx = end - 1;
    }
}

void KoTextCursor::gotoLineEnd()
{
    tmpIndex = -1;
    int indexOfLineStart;
    int line;
    KoTextStringChar *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    if ( line == string->lines() - 1 ) {
	idx = string->length() - 1;
    } else {
	c = string->lineStartOfLine( ++line, &indexOfLineStart );
	indexOfLineStart--;
	idx = indexOfLineStart;
    }
}

void KoTextCursor::gotoLineStart()
{
    tmpIndex = -1;
    int indexOfLineStart;
    int line;
    KoTextStringChar *c = string->lineStartOfChar( idx, &indexOfLineStart, &line );
    if ( !c )
	return;

    idx = indexOfLineStart;
}

void KoTextCursor::gotoHome()
{
    tmpIndex = -1;
    if ( doc )
	string = doc->firstParag();
    idx = 0;
}

void KoTextCursor::gotoEnd()
{
    if ( doc && !doc->lastParag()->isValid() )
    {
	qDebug("Last parag, %d, is invalid - aborting gotoEnd() !",doc->lastParag()->paragId());
	return;
    }

    tmpIndex = -1;
    if ( doc )
	string = doc->lastParag();
    idx = string->length() - 1;
}

void KoTextCursor::gotoPageUp( int visibleHeight )
{
    tmpIndex = -1;
    KoTextParag *s = string;
    int h = visibleHeight;
    int y = s->rect().y();
    while ( s ) {
	if ( y - s->rect().y() >= h )
	    break;
	s = s->prev();
    }

    if ( !s && doc )
	s = doc->firstParag();

    string = s;
    idx = 0;
}

void KoTextCursor::gotoPageDown( int visibleHeight )
{
    tmpIndex = -1;
    KoTextParag *s = string;
    int h = visibleHeight;
    int y = s->rect().y();
    while ( s ) {
	if ( s->rect().y() - y >= h )
	    break;
	s = s->next();
    }

    if ( !s && doc ) {
	s = doc->lastParag();
	string = s;
	idx = string->length() - 1;
	return;
    }

    if ( !s->isValid() )
	return;

    string = s;
    idx = 0;
}

void KoTextCursor::gotoWordRight()
{
    if ( string->string()->isRightToLeft() )
	gotoPreviousWord();
    else
	gotoNextWord();
}

void KoTextCursor::gotoWordLeft()
{
    if ( string->string()->isRightToLeft() )
	gotoNextWord();
    else
	gotoPreviousWord();
}

void KoTextCursor::gotoPreviousWord()
{
    gotoPreviousLetter();
    tmpIndex = -1;
    KoTextString *s = string->string();
    bool allowSame = FALSE;
    if ( idx == ( (int)s->length()-1 ) )
        return;
    for ( int i = idx; i >= 0; --i ) {
	if ( s->at( i ).c.isSpace() || s->at( i ).c == '\t' || s->at( i ).c == '.' ||
	     s->at( i ).c == ',' || s->at( i ).c == ':' || s->at( i ).c == ';' ) {
	    if ( !allowSame )
		continue;
	    idx = i + 1;
	    return;
	}
	if ( !allowSame && !( s->at( i ).c.isSpace() || s->at( i ).c == '\t' || s->at( i ).c == '.' ||
			      s->at( i ).c == ',' || s->at( i ).c == ':' || s->at( i ).c == ';'  ) )
	    allowSame = TRUE;
    }
    idx = 0;
}

void KoTextCursor::gotoNextWord()
{
    tmpIndex = -1;
    KoTextString *s = string->string();
    bool allowSame = FALSE;
    for ( int i = idx; i < (int)s->length(); ++i ) {
	if ( ! ( s->at( i ).c.isSpace() || s->at( i ).c == '\t' || s->at( i ).c == '.' ||
	     s->at( i ).c == ',' || s->at( i ).c == ':' || s->at( i ).c == ';' ) ) {
	    if ( !allowSame )
		continue;
	    idx = i;
	    return;
	}
	if ( !allowSame && ( s->at( i ).c.isSpace() || s->at( i ).c == '\t' || s->at( i ).c == '.' ||
			      s->at( i ).c == ',' || s->at( i ).c == ':' || s->at( i ).c == ';'  ) )
	    allowSame = TRUE;
    }

    if ( idx < ((int)s->length()-1) ) {
        gotoLineEnd();
    } else if ( string->next() ) {
	string = string->next();
	while ( !string->isVisible() )
	    string = string->next();
	idx = 0;
    } else {
	gotoLineEnd();
    }
}

bool KoTextCursor::atParagStart() const
{
    return idx == 0;
}

bool KoTextCursor::atParagEnd() const
{
    return idx == string->length() - 1;
}

void KoTextCursor::splitAndInsertEmptyParag( bool ind, bool updateIds )
{
    if ( !doc )
	return;
    tmpIndex = -1;
    KoTextFormat *f = 0;
    if ( doc->useFormatCollection() ) {
	f = string->at( idx )->format();
	if ( idx == string->length() - 1 && idx > 0 )
	    f = string->at( idx - 1 )->format();
	if ( f->isMisspelled() ) {
            KoTextFormat fNoMisspelled( *f );
            fNoMisspelled.setMisspelled( false );
	    f->removeRef();
	    f = doc->formatCollection()->format( &fNoMisspelled );
	}
    }

    if ( atParagEnd() ) {
	KoTextParag *n = string->next();
	KoTextParag *s = doc->createParag( doc, string, n, updateIds );
	if ( f )
	    s->setFormat( 0, 1, f, TRUE );
	s->copyParagData( string );
	if ( ind ) {
	    int oi, ni;
	    s->indent( &oi, &ni );
	    string = s;
	    idx = ni;
	} else {
	    string = s;
	    idx = 0;
	}
    } else if ( atParagStart() ) {
	KoTextParag *p = string->prev();
	KoTextParag *s = doc->createParag( doc, p, string, updateIds );
	if ( f )
	    s->setFormat( 0, 1, f, TRUE );
	s->copyParagData( string );
	if ( ind ) {
	    s->indent();
	    s->format();
	    indent();
	    string->format();
	}
    } else {
	QString str = string->string()->toString().mid( idx, 0xFFFFFF );
	KoTextParag *n = string->next();
	KoTextParag *s = doc->createParag( doc, string, n, updateIds );
	s->copyParagData( string );
	s->remove( 0, 1 );
	s->append( str, TRUE );
	for ( uint i = 0; i < str.length(); ++i ) {
            KoTextStringChar* tsc = string->at( idx + i );
	    s->setFormat( i, 1, tsc->format(), TRUE );
	    if ( tsc->isCustom() ) {
		KoTextCustomItem * item = tsc->customItem();
		s->at( i )->setCustomItem( item );
		tsc->loseCustomItem();
#if 0
		s->addCustomItem();
		string->removeCustomItem();
#endif
		doc->unregisterCustomItem( item, string );
		doc->registerCustomItem( item, s );
	    }
	}
	string->truncate( idx );
	if ( ind ) {
	    int oi, ni;
	    s->indent( &oi, &ni );
	    string = s;
	    idx = ni;
	} else {
	    string = s;
	    idx = 0;
	}
    }

    invalidateNested();
}

bool KoTextCursor::remove()
{
    tmpIndex = -1;
    if ( !atParagEnd() ) {
	string->remove( idx, 1 );
	int h = string->rect().height();
	string->format( -1, TRUE );
	if ( h != string->rect().height() )
	    invalidateNested();
	//else if ( doc && doc->parent() )
	//    doc->nextDoubleBuffered = TRUE;
	return FALSE;
    } else if ( string->next() ) {
	if ( string->length() == 1 ) {
	    string->next()->setPrev( string->prev() );
	    if ( string->prev() )
		string->prev()->setNext( string->next() );
	    KoTextParag *p = string->next();
	    delete string;
	    string = p;
	    string->invalidate( 0 );
            //// kotext
            string->invalidateCounters();
            ////
	    KoTextParag *s = string;
	    while ( s ) {
		s->id = s->p ? s->p->id + 1 : 0;
		//s->state = -1;
		//s->needPreProcess = TRUE;
		s->changed = TRUE;
		s = s->n;
	    }
	    string->format();
	} else {
	    string->join( string->next() );
	}
	invalidateNested();
	return TRUE;
    }
    return FALSE;
}

void KoTextCursor::killLine()
{
    if ( atParagEnd() )
	return;
    string->remove( idx, string->length() - idx - 1 );
    int h = string->rect().height();
    string->format( -1, TRUE );
    if ( h != string->rect().height() )
	invalidateNested();
    //else if ( doc && doc->parent() )
    //doc->nextDoubleBuffered = TRUE;
}

void KoTextCursor::indent()
{
    int oi = 0, ni = 0;
    string->indent( &oi, &ni );
    if ( oi == ni )
	return;

    if ( idx >= oi )
	idx += ni - oi;
    else
	idx = ni;
}

void KoTextCursor::setDocument( KoTextDocument *d )
{
    doc = d;
    string = d->firstParag();
    idx = 0;
    nested = FALSE;
    restoreState();
    tmpIndex = -1;
}


int KoTextCursor::x() const
{
    KoTextStringChar *c = string->at( idx );
    int curx = c->x;
    if ( c->rightToLeft )
        curx += c->width; //string->string()->width( idx );
    return curx;
}

int KoTextCursor::y() const
{
    int dummy, line;
    string->lineStartOfChar( idx, &dummy, &line );
    return string->lineY( line );
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if 0
KoTextDocument::KoTextDocument( KoTextDocument *p )
    : par( p ), parParag( 0 ),
#ifdef QTEXTTABLE_AVAILABLE
      tc( 0 ),
#endif
      tArray( 0 ), tStopWidth( 0 )
{
    fCollection = new KoTextFormatCollection;
    init();
}

KoTextDocument::KoTextDocument( KoTextDocument *p, KoTextFormatCollection *f )
    : par( p ), parParag( 0 ),
#ifdef QTEXTTABLE_AVAILABLE
      tc( 0 ),
#endif
      tArray( 0 ), tStopWidth( 0 )
{
    fCollection = f;
    init();
}
#endif

void KoTextDocument::init()
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new KoTextDocument (%p)", this );
#endif
    //oTextValid = TRUE;
    //if ( par )
//	par->insertChild( this );
    //pProcessor = 0;
    useFC = TRUE;
    pFormatter = 0;
    indenter = 0;
    fParag = 0;
    m_pageBreakEnabled = false;
    minw = 0;
    align = Qt::AlignAuto;
    nSelections = 1;
    addMargs = FALSE;

#if 0
    preferRichText = FALSE;
    txtFormat = Qt::AutoText;
    focusIndicator.parag = 0;
    minwParag = 0;
    sheet_ = QStyleSheet::defaultSheet();
    factory_ = QMimeSourceFactory::defaultFactory();
    contxt = QString::null;
    fCollection->setStyleSheet( sheet_ );
#endif

    underlLinks = TRUE;
    backBrush = 0;
    buf_pixmap = 0;
    //nextDoubleBuffered = FALSE;

    //if ( par )
//	withoutDoubleBuffer = par->withoutDoubleBuffer;
//    else
	withoutDoubleBuffer = FALSE;

    lParag = fParag = createParag( this, 0, 0 );
    tmpCursor = 0;

    //cx = 0;
    //cy = 2;
    //if ( par )
	cx = cy = 0;
    cw = 600;
    vw = 0;
    flow_ = new KoTextFlow;
    flow_->setWidth( cw );

    leftmargin = 0; // 4 in QRT
    rightmargin = 0; // 4 in QRT

    selectionColors[ Standard ] = QApplication::palette().color( QPalette::Active, QColorGroup::Highlight );
    selectionText[ Standard ] = TRUE;
    commandHistory = new KoTextDocCommandHistory( 100 );
    tStopWidth = formatCollection()->defaultFormat()->width( 'x' ) * 8;
}

KoTextDocument::~KoTextDocument()
{
    //if ( par )
//	par->removeChild( this );
    //// kotext
    m_bDestroying = true;
    clear( false );
    ////
    delete commandHistory;
    delete flow_;
    //if ( !par )
	delete pFormatter;
    delete fCollection;
    //delete pProcessor;
    delete buf_pixmap;
    delete indenter;
    delete backBrush;
    if ( tArray )
	delete [] tArray;
}

void KoTextDocument::clear( bool createEmptyParag )
{
    if ( flow_ )
	flow_->clear();
    while ( fParag ) {
	KoTextParag *p = fParag->next();
	delete fParag;
	fParag = p;
    }
    fParag = lParag = 0;
    if ( createEmptyParag )
	fParag = lParag = createParag( this );
    selections.clear();
}

int KoTextDocument::widthUsed() const
{
    KoTextParag *p = fParag;
    int w = 0;
    while ( p ) {
	int a = p->alignment();
	p->setAlignment( Qt::AlignLeft );
	p->invalidate( 0 );
	p->format();
	w = QMAX( w, p->rect().width() );
	p->setAlignment( a );
	p->invalidate( 0 );
	p = p->next();
    }
    return w;
}

int KoTextDocument::height() const
{
    int h = 0;
    if ( lParag )
	h = lParag->rect().top() + lParag->rect().height() + 1;
    int fh = flow_->boundingRect().height();
    return QMAX( h, fh );
}



KoTextParag *KoTextDocument::createParag( KoTextDocument *d, KoTextParag *pr, KoTextParag *nx, bool updateIds )
{
    return new KoTextParag( d, pr, nx, updateIds );
}

bool KoTextDocument::setMinimumWidth( int w, KoTextParag *p )
{
    if ( w == -1 ) {
	minw = 0;
	p = 0;
    }
#if 0
    if ( p == minwParag ) {
	minw = w;
	emit minimumWidthChanged( minw );
    } else if ( w > minw ) {
	minw = w;
	minwParag = p;
	emit minimumWidthChanged( minw );
    }
#endif
    cw = QMAX( minw, cw );
    return TRUE;
}

void KoTextDocument::setPlainText( const QString &text )
{
    clear();
    //preferRichText = FALSE;
    //oTextValid = TRUE;
    //oText = text;

    int lastNl = 0;
    int nl = text.find( '\n' );
    if ( nl == -1 ) {
	lParag = createParag( this, lParag, 0 );
	if ( !fParag )
	    fParag = lParag;
	QString s = text;
	if ( !s.isEmpty() ) {
	    if ( s[ (int)s.length() - 1 ] == '\r' )
		s.remove( s.length() - 1, 1 );
	    lParag->append( s );
	}
    } else {
	for (;;) {
	    lParag = createParag( this, lParag, 0 );
	    if ( !fParag )
		fParag = lParag;
	    QString s = text.mid( lastNl, nl - lastNl );
	    if ( !s.isEmpty() ) {
		if ( s[ (int)s.length() - 1 ] == '\r' )
		    s.remove( s.length() - 1, 1 );
		lParag->append( s );
	    }
	    if ( nl == 0xffffff )
		break;
	    lastNl = nl + 1;
	    nl = text.find( '\n', nl + 1 );
	    if ( nl == -1 )
		nl = 0xffffff;
	}
    }
    if ( !lParag )
	lParag = fParag = createParag( this, 0, 0 );
}

void KoTextDocument::setText( const QString &text, const QString & /*context*/ )
{
    //focusIndicator.parag = 0;
    selections.clear();
#if 0
    if ( txtFormat == Qt::AutoText && QStyleSheet::mightBeRichText( text ) ||
	 txtFormat == Qt::RichText )
	setRichText( text, context );
    else
#endif
	setPlainText( text );
}

QString KoTextDocument::plainText( KoTextParag *p ) const
{
    if ( !p ) {
	QString buffer;
	QString s;
	KoTextParag *p = fParag;
	while ( p ) {
	    s = p->string()->toString();
	    s.remove( s.length() - 1, 1 );
	    if ( p->next() )
		s += "\n";
	    buffer += s;
	    p = p->next();
	}
	return buffer;
    } else {
	return p->string()->toString();
    }
}

QString KoTextDocument::richText( KoTextParag * ) const
{
    QString s;
    // TODO update from QRT if this code is needed
    return s;
}

QString KoTextDocument::text() const
{
    if ( plainText().simplifyWhiteSpace().isEmpty() )
	return QString("");
    //if ( txtFormat == Qt::AutoText && preferRichText || txtFormat == Qt::RichText )
    //    return richText();
    return plainText( 0 );
}

QString KoTextDocument::text( int parag ) const
{
    KoTextParag *p = paragAt( parag );
    if ( !p )
	return QString::null;

    //if ( txtFormat == Qt::AutoText && preferRichText || txtFormat == Qt::RichText )
    //    return richText( p );
    //else
	return plainText( p );
}

void KoTextDocument::invalidate()
{
    KoTextParag *s = fParag;
    while ( s ) {
	s->invalidate( 0 );
	s = s->next();
    }
}

void KoTextDocument::informParagraphDeleted( KoTextParag* parag )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.begin();
    for ( ; it != selections.end(); ++it )
    {
        if ( (*it).startCursor.parag() == parag ) {
            if ( parag->prev() ) {
                KoTextParag* prevP = parag->prev();
                (*it).startCursor.setParag( prevP );
                (*it).startCursor.setIndex( prevP->length()-1 );
            } else
                (*it).startCursor.setParag( parag->next() ); // sets index to 0
        }
        if ( (*it).endCursor.parag() == parag ) {
            if ( parag->prev() ) {
                KoTextParag* prevP = parag->prev();
                (*it).endCursor.setParag( prevP );
                (*it).endCursor.setIndex( prevP->length()-1 );
            } else
                (*it).endCursor.setParag( parag->next() ); // sets index to 0
        }
    }
    emit paragraphDeleted( parag );
}

void KoTextDocument::selectionStart( int id, int &paragId, int &index )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;
    KoTextDocumentSelection &sel = *it;
    paragId = !sel.swapped ? sel.startCursor.parag()->paragId() : sel.endCursor.parag()->paragId();
    index = !sel.swapped ? sel.startCursor.index() : sel.endCursor.index();
}

KoTextCursor KoTextDocument::selectionStartCursor( int id)
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return KoTextCursor( this );
    KoTextDocumentSelection &sel = *it;
    if ( sel.swapped )
	return sel.endCursor;
    return sel.startCursor;
}

KoTextCursor KoTextDocument::selectionEndCursor( int id)
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return KoTextCursor( this );
    KoTextDocumentSelection &sel = *it;
    if ( !sel.swapped )
	return sel.endCursor;
    return sel.startCursor;
}

void KoTextDocument::selectionEnd( int id, int &paragId, int &index )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;
    KoTextDocumentSelection &sel = *it;
    paragId = sel.swapped ? sel.startCursor.parag()->paragId() : sel.endCursor.parag()->paragId();
    index = sel.swapped ? sel.startCursor.index() : sel.endCursor.index();
}

bool KoTextDocument::isSelectionSwapped( int id )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return false;
    KoTextDocumentSelection &sel = *it;
    return sel.swapped;
}

KoTextParag *KoTextDocument::selectionStart( int id )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return 0;
    KoTextDocumentSelection &sel = *it;
    if ( sel.startCursor.parag()->paragId() <  sel.endCursor.parag()->paragId() )
	return sel.startCursor.parag();
    return sel.endCursor.parag();
}

KoTextParag *KoTextDocument::selectionEnd( int id )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return 0;
    KoTextDocumentSelection &sel = *it;
    if ( sel.startCursor.parag()->paragId() >  sel.endCursor.parag()->paragId() )
	return sel.startCursor.parag();
    return sel.endCursor.parag();
}

void KoTextDocument::addSelection( int id )
{
    nSelections = QMAX( nSelections, id + 1 );
}

static void setSelectionEndHelper( int id, KoTextDocumentSelection &sel, KoTextCursor &start, KoTextCursor &end )
{
    KoTextCursor c1 = start;
    KoTextCursor c2 = end;
    if ( sel.swapped ) {
	c1 = end;
	c2 = start;
    }

    c1.parag()->removeSelection( id );
    c2.parag()->removeSelection( id );
    if ( c1.parag() != c2.parag() ) {
	c1.parag()->setSelection( id, c1.index(), c1.parag()->length() - 1 );
	c2.parag()->setSelection( id, 0, c2.index() );
    } else {
	c1.parag()->setSelection( id, QMIN( c1.index(), c2.index() ), QMAX( c1.index(), c2.index() ) );
    }

    sel.startCursor = start;
    sel.endCursor = end;
    if ( sel.startCursor.parag() == sel.endCursor.parag() )
	sel.swapped = sel.startCursor.index() > sel.endCursor.index();
}

bool KoTextDocument::setSelectionEnd( int id, KoTextCursor *cursor )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return FALSE;
    KoTextDocumentSelection &sel = *it;

    KoTextCursor start = sel.startCursor;
    KoTextCursor end = *cursor;

    if ( start == end ) {
	removeSelection( id );
	setSelectionStart( id, cursor );
	return TRUE;
    }

    if ( sel.endCursor.parag() == end.parag() ) {
	setSelectionEndHelper( id, sel, start, end );
	return TRUE;
    }

    bool inSelection = FALSE;
    KoTextCursor c( this );
    KoTextCursor tmp = sel.startCursor;
    if ( sel.swapped )
	tmp = sel.endCursor;
    tmp.restoreState();
    KoTextCursor tmp2 = *cursor;
    tmp2.restoreState();
    c.setParag( tmp.parag()->paragId() < tmp2.parag()->paragId() ? tmp.parag() : tmp2.parag() );
    KoTextCursor old;
    bool hadStart = FALSE;
    bool hadEnd = FALSE;
    bool hadStartParag = FALSE;
    bool hadEndParag = FALSE;
    bool hadOldStart = FALSE;
    bool hadOldEnd = FALSE;
    bool leftSelection = FALSE;
    sel.swapped = FALSE;
    for ( ;; ) {
	if ( c == start )
	    hadStart = TRUE;
	if ( c == end )
	    hadEnd = TRUE;
	if ( c.parag() == start.parag() )
	    hadStartParag = TRUE;
	if ( c.parag() == end.parag() )
	    hadEndParag = TRUE;
	if ( c == sel.startCursor )
	    hadOldStart = TRUE;
	if ( c == sel.endCursor )
	    hadOldEnd = TRUE;

	if ( !sel.swapped &&
	     ( hadEnd && !hadStart ||
	       hadEnd && hadStart && start.parag() == end.parag() && start.index() > end.index() ) )
	    sel.swapped = TRUE;

	if ( c == end && hadStartParag ||
	     c == start && hadEndParag ) {
	    KoTextCursor tmp = c;
	    tmp.restoreState();
	    if ( tmp.parag() != c.parag() ) {
		int sstart = tmp.parag()->selectionStart( id );
		tmp.parag()->removeSelection( id );
		tmp.parag()->setSelection( id, sstart, tmp.index() );
	    }
	}

	if ( inSelection &&
	     ( c == end && hadStart || c == start && hadEnd ) )
	     leftSelection = TRUE;
	else if ( !leftSelection && !inSelection && ( hadStart || hadEnd ) )
	    inSelection = TRUE;

	bool noSelectionAnymore = hadOldStart && hadOldEnd && leftSelection && !inSelection && !c.parag()->hasSelection( id ) && c.atParagEnd();
	c.parag()->removeSelection( id );
	if ( inSelection ) {
	    if ( c.parag() == start.parag() && start.parag() == end.parag() ) {
		c.parag()->setSelection( id, QMIN( start.index(), end.index() ), QMAX( start.index(), end.index() ) );
	    } else if ( c.parag() == start.parag() && !hadEndParag ) {
		c.parag()->setSelection( id, start.index(), c.parag()->length() - 1 );
	    } else if ( c.parag() == end.parag() && !hadStartParag ) {
		c.parag()->setSelection( id, end.index(), c.parag()->length() - 1 );
	    } else if ( c.parag() == end.parag() && hadEndParag ) {
		c.parag()->setSelection( id, 0, end.index() );
	    } else if ( c.parag() == start.parag() && hadStartParag ) {
		c.parag()->setSelection( id, 0, start.index() );
	    } else {
		c.parag()->setSelection( id, 0, c.parag()->length() - 1 );
	    }
	}

	if ( leftSelection )
	    inSelection = FALSE;

	old = c;
	c.gotoNextLetter();
	if ( old == c || noSelectionAnymore )
	    break;
    }

    if ( !sel.swapped )
	sel.startCursor.parag()->setSelection( id, sel.startCursor.index(), sel.startCursor.parag()->length() - 1 );

    sel.startCursor = start;
    sel.endCursor = end;
    if ( sel.startCursor.parag() == sel.endCursor.parag() )
	sel.swapped = sel.startCursor.index() > sel.endCursor.index();

    setSelectionEndHelper( id, sel, start, end );

    return TRUE;
}

void KoTextDocument::selectAll( int id )
{
    removeSelection( id );

    KoTextDocumentSelection sel;
    sel.swapped = FALSE;
    KoTextCursor c( this );

    c.setParag( fParag );
    c.setIndex( 0 );
    sel.startCursor = c;

    c.setParag( lParag );
    c.setIndex( lParag->length() - 1 );
    sel.endCursor = c;

    KoTextParag *p = fParag;
    while ( p ) {
	p->setSelection( id, 0, p->length() - 1 );
#ifdef QTEXTTABLE_AVAILABLE
	for ( int i = 0; i < (int)p->length(); ++i ) {
	    if ( p->at( i )->isCustom() && p->at( i )->customItem()->isNested() ) {
		KoTextTable *t = (KoTextTable*)p->at( i )->customItem();
		QPtrList<KoTextTableCell> tableCells = t->tableCells();
		for ( KoTextTableCell *c = tableCells.first(); c; c = tableCells.next() )
		    c->richText()->selectAll( id );
	    }
	}
#endif
	p = p->next();
    }

    selections.insert( id, sel );
}

bool KoTextDocument::removeSelection( int id )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return FALSE;

    KoTextDocumentSelection &sel = *it;

    KoTextCursor c( this );
    KoTextCursor tmp = sel.startCursor;
    if ( sel.swapped )
	tmp = sel.endCursor;
    tmp.restoreState();
    c.setParag( tmp.parag() );
    KoTextCursor old;
    bool hadStart = FALSE;
    bool hadEnd = FALSE;
    KoTextParag *lastParag = 0;
    bool leftSelection = FALSE;
    bool inSelection = FALSE;
    sel.swapped = FALSE;
    for ( ;; ) {
	if ( !hadStart && c.parag() == sel.startCursor.parag() )
	    hadStart = TRUE;
	if ( !hadEnd && c.parag() == sel.endCursor.parag() )
	    hadEnd = TRUE;

        if ( !leftSelection && !inSelection && ( c.parag() == sel.startCursor.parag() || c.parag() == sel.endCursor.parag() ) )
	    inSelection = TRUE;

	if ( inSelection &&
	     ( c == sel.endCursor && hadStart || c == sel.startCursor && hadEnd ) ) {
	     leftSelection = TRUE;
             inSelection = FALSE;
        }

	bool noSelectionAnymore = leftSelection && !inSelection && !c.parag()->hasSelection( id ) && c.atParagEnd();

	if ( lastParag != c.parag() )
	    c.parag()->removeSelection( id );

	old = c;
	lastParag = c.parag();
	c.gotoNextLetter();
	if ( old == c || noSelectionAnymore )
	    break;
    }

    selections.remove( id );
    return TRUE;
}

QString KoTextDocument::selectedText( int id, bool withCustom ) const
{
    // ######## TODO: look at textFormat() and return rich text or plain text (like the text() method!)
    QMap<int, KoTextDocumentSelection>::ConstIterator it = selections.find( id );
    if ( it == selections.end() )
	return QString::null;

    KoTextDocumentSelection sel = *it;


    KoTextCursor c1 = sel.startCursor;
    KoTextCursor c2 = sel.endCursor;
    if ( sel.swapped ) {
	c2 = sel.startCursor;
	c1 = sel.endCursor;
    }

    c2.restoreState();
    c1.restoreState();

    if ( c1.parag() == c2.parag() ) {
	QString s;
	KoTextParag *p = c1.parag();
	int end = c2.index();
	if ( p->at( QMAX( 0, end - 1 ) )->isCustom() )
	    ++end;
	if ( !withCustom || !p->customItems() ) {
	    s += p->string()->toString().mid( c1.index(), end - c1.index() );
	} else {
	    for ( int i = c1.index(); i < end; ++i ) {
		if ( p->at( i )->isCustom() ) {
#ifdef QTEXTTABLE_AVAILABLE
		    if ( p->at( i )->customItem()->isNested() ) {
			s += "\n";
			KoTextTable *t = (KoTextTable*)p->at( i )->customItem();
			QPtrList<KoTextTableCell> cells = t->tableCells();
			for ( KoTextTableCell *c = cells.first(); c; c = cells.next() )
			    s += c->richText()->plainText() + "\n";
			s += "\n";
		    }
#endif
		} else {
		    s += p->at( i )->c;
		}
		s += "\n";
	    }
	}
	return s;
    }

    QString s;
    KoTextParag *p = c1.parag();
    int start = c1.index();
    while ( p ) {
	int end = p == c2.parag() ? c2.index() : p->length() - 1;
	if ( p == c2.parag() && p->at( QMAX( 0, end - 1 ) )->isCustom() )
	    ++end;
	if ( !withCustom || !p->customItems() ) {
	    s += p->string()->toString().mid( start, end - start );
	    if ( p != c2.parag() )
		s += "\n";
	} else {
	    for ( int i = start; i < end; ++i ) {
		if ( p->at( i )->isCustom() ) {
#ifdef QTEXTTABLE_AVAILABLE
		    if ( p->at( i )->customItem()->isNested() ) {
			s += "\n";
			KoTextTable *t = (KoTextTable*)p->at( i )->customItem();
			QPtrList<KoTextTableCell> cells = t->tableCells();
			for ( KoTextTableCell *c = cells.first(); c; c = cells.next() )
			    s += c->richText()->plainText() + "\n";
			s += "\n";
		    }
#endif
		} else {
		    s += p->at( i )->c;
		}
		s += "\n";
	    }
	}
	start = 0;
	if ( p == c2.parag() )
	    break;
	p = p->next();
    }
    return s;
}

void KoTextDocument::setFormat( int id, KoTextFormat *f, int flags )
{
    QMap<int, KoTextDocumentSelection>::ConstIterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    KoTextDocumentSelection sel = *it;

    KoTextCursor c1 = sel.startCursor;
    KoTextCursor c2 = sel.endCursor;
    if ( sel.swapped ) {
	c2 = sel.startCursor;
	c1 = sel.endCursor;
    }

    c2.restoreState();
    c1.restoreState();

    if ( c1.parag() == c2.parag() ) {
	c1.parag()->setFormat( c1.index(), c2.index() - c1.index(), f, TRUE, flags );
	return;
    }

    c1.parag()->setFormat( c1.index(), c1.parag()->length() - c1.index(), f, TRUE, flags );
    KoTextParag *p = c1.parag()->next();
    while ( p && p != c2.parag() ) {
	p->setFormat( 0, p->length(), f, TRUE, flags );
	p = p->next();
    }
    c2.parag()->setFormat( 0, c2.index(), f, TRUE, flags );
}

void KoTextDocument::copySelectedText( int id )
{
#ifndef QT_NO_CLIPBOARD
    if ( !hasSelection( id ) )
	return;

    QApplication::clipboard()->setText( selectedText( id ) );
#endif
}

void KoTextDocument::removeSelectedText( int id, KoTextCursor *cursor )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    KoTextDocumentSelection sel = *it;

    KoTextCursor c1 = sel.startCursor;
    KoTextCursor c2 = sel.endCursor;
    if ( sel.swapped ) {
	c2 = sel.startCursor;
	c1 = sel.endCursor;
    }

    // ### no support for editing tables yet
    if ( c1.nestedDepth() || c2.nestedDepth() )
	return;

    c2.restoreState();
    c1.restoreState();

    *cursor = c1;
    removeSelection( id );

    if ( c1.parag() == c2.parag() ) {
	c1.parag()->remove( c1.index(), c2.index() - c1.index() );
	return;
    }

    // ## Qt has a strange setValid/isValid on QTextCursor, only used in the few lines below !?!?
    bool valid = true;
    if ( c1.parag() == fParag && c1.index() == 0 &&
         c2.parag() == lParag && c2.index() == lParag->length() - 1 )
        valid = FALSE;

    bool didGoLeft = FALSE;
    if (  c1.index() == 0 && c1.parag() != fParag ) {
	cursor->gotoPreviousLetter();
        if ( valid )
            didGoLeft = TRUE;
    }

    c1.parag()->remove( c1.index(), c1.parag()->length() - 1 - c1.index() );
    KoTextParag *p = c1.parag()->next();
    int dy = 0;
    KoTextParag *tmp;
    while ( p && p != c2.parag() ) {
	tmp = p->next();
	dy -= p->rect().height();
	delete p;
	p = tmp;
    }
    c2.parag()->remove( 0, c2.index() );
    while ( p ) {
	p->move( dy );
        //// kotext
        if ( p->paragLayout().counter )
            p->paragLayout().counter->invalidate();
        ////
	p->invalidate( 0 );
	//p->setEndState( -1 );
	p = p->next();
    }

    c1.parag()->join( c2.parag() );

    if ( didGoLeft )
	cursor->gotoNextLetter();
}

void KoTextDocument::indentSelection( int id )
{
    QMap<int, KoTextDocumentSelection>::Iterator it = selections.find( id );
    if ( it == selections.end() )
	return;

    KoTextDocumentSelection sel = *it;
    KoTextParag *startParag = sel.startCursor.parag();
    KoTextParag *endParag = sel.endCursor.parag();
    if ( sel.endCursor.parag()->paragId() < sel.startCursor.parag()->paragId() ) {
	endParag = sel.startCursor.parag();
	startParag = sel.endCursor.parag();
    }

    KoTextParag *p = startParag;
    while ( p && p != endParag ) {
	p->indent();
	p = p->next();
    }
}

void KoTextDocument::addCommand( KoTextDocCommand *cmd )
{
    commandHistory->addCommand( cmd );
}

KoTextCursor *KoTextDocument::undo( KoTextCursor *c )
{
    return commandHistory->undo( c );
}

KoTextCursor *KoTextDocument::redo( KoTextCursor *c )
{
    return commandHistory->redo( c );
}

bool KoTextDocument::find( const QString &expr, bool cs, bool wo, bool forward,
			  int *parag, int *index, KoTextCursor *cursor )
{
    KoTextParag *p = forward ? fParag : lParag;
    if ( parag )
	p = paragAt( *parag );
    else if ( cursor )
	p = cursor->parag();
    bool first = TRUE;

    while ( p ) {
	QString s = p->string()->toString();
	s.remove( s.length() - 1, 1 ); // get rid of trailing space
	int start = forward ? 0 : s.length() - 1;
	if ( first && index )
	    start = *index;
	else if ( first )
	    start = cursor->index();
	if ( !forward && first ) {
	    start -= expr.length() + 1;
	    if ( start < 0 ) {
		first = FALSE;
		p = p->prev();
		continue;
	    }
	}
	first = FALSE;

	for ( ;; ) {
	    int res = forward ? s.find( expr, start, cs ) : s.findRev( expr, start, cs );
	    if ( res == -1 )
		break;

	    bool ok = TRUE;
	    if ( wo ) {
		int end = res + expr.length();
		if ( ( res == 0 || s[ res - 1 ].isSpace() || s[ res - 1 ].isPunct() ) &&
		     ( end == (int)s.length() || s[ end ].isSpace() || s[ end ].isPunct() ) )
		    ok = TRUE;
		else
		    ok = FALSE;
	    }
	    if ( ok ) {
		cursor->setParag( p );
		cursor->setIndex( res );
		setSelectionStart( Standard, cursor );
		cursor->setIndex( res + expr.length() );
		setSelectionEnd( Standard, cursor );
		if ( parag )
		    *parag = p->paragId();
		if ( index )
		    *index = res;
		return TRUE;
	    }
	    if ( forward ) {
		start = res + 1;
	    } else {
		if ( res == 0 )
		    break;
		start = res - 1;
	    }
	}
	p = forward ? p->next() : p->prev();
    }

    return FALSE;
}

#if 0
void KoTextDocument::setTextFormat( Qt::TextFormat f )
{
    txtFormat = f;
}

Qt::TextFormat KoTextDocument::textFormat() const
{
    return txtFormat;
}
#endif

bool KoTextDocument::inSelection( int selId, const QPoint &pos ) const
{
    QMap<int, KoTextDocumentSelection>::ConstIterator it = selections.find( selId );
    if ( it == selections.end() )
	return FALSE;

    KoTextDocumentSelection sel = *it;
    KoTextParag *startParag = sel.startCursor.parag();
    KoTextParag *endParag = sel.endCursor.parag();
    if ( sel.startCursor.parag() == sel.endCursor.parag() &&
	 sel.startCursor.parag()->selectionStart( selId ) == sel.endCursor.parag()->selectionEnd( selId ) )
	return FALSE;
    if ( sel.endCursor.parag()->paragId() < sel.startCursor.parag()->paragId() ) {
	endParag = sel.startCursor.parag();
	startParag = sel.endCursor.parag();
    }

    KoTextParag *p = startParag;
    while ( p ) {
	if ( p->rect().contains( pos ) ) {
	    bool inSel = FALSE;
	    int selStart = p->selectionStart( selId );
	    int selEnd = p->selectionEnd( selId );
	    int y = 0;
	    int h = 0;
	    for ( int i = 0; i < p->length(); ++i ) {
		if ( i == selStart )
		    inSel = TRUE;
		if ( i == selEnd )
		    break;
		if ( p->at( i )->lineStart ) {
		    y = (*p->lineStarts.find( i ))->y;
		    h = (*p->lineStarts.find( i ))->h;
		}
		if ( pos.y() - p->rect().y() >= y && pos.y() - p->rect().y() <= y + h ) {
		    if ( inSel && pos.x() >= p->at( i )->x &&
			 pos.x() <= p->at( i )->x + p->at( i )->width /*p->at( i )->format()->width( p->at( i )->c )*/ )
			return TRUE;
		}
	    }
	}
	if ( pos.y() < p->rect().y() )
	    break;
	if ( p == endParag )
	    break;
	p = p->next();
    }

    return FALSE;
}

#if 0
void KoTextDocument::doLayout( QPainter *p, int w )
{
    if ( !is_printer( p ) )
	p = 0;
    withoutDoubleBuffer = ( p != 0 );
    flow_->setWidth( w );
    cw = w;
    vw = w;
    fCollection->setPainter( p );
    KoTextParag *parag = fParag;
    while ( parag ) {
	parag->invalidate( 0 );
	parag->setPainter( p, TRUE );
	parag->format();
	parag = parag->next();
    }

    fCollection->setPainter( 0 );
    parag = fParag;
    while ( parag ) {
	parag->setPainter( 0, FALSE );
	parag = parag->next();
    }
}
#endif

QPixmap *KoTextDocument::bufferPixmap( const QSize &s )
{
    if ( !buf_pixmap ) {
	int w = QABS( s.width() );
	int h = QABS( s.height() );
	buf_pixmap = new QPixmap( w, h );
    } else {
	if ( buf_pixmap->width() < s.width() ||
	     buf_pixmap->height() < s.height() ) {
	    buf_pixmap->resize( QMAX( s.width(), buf_pixmap->width() ),
				QMAX( s.height(), buf_pixmap->height() ) );
	}
    }

    return buf_pixmap;
}

#if 0
void KoTextDocument::draw( QPainter *p, const QRect &rect, const QColorGroup &cg, const QBrush *paper )
{
    if ( !firstParag() )
	return;

    QBrush bgBrush = paper ? *paper : cg.brush( QColorGroup::Base ); // ## QRT doesn't use cg.brush(Base)
    {
	p->setBrushOrigin( -int( p->translationX() ),
			   -int( p->translationY() ) );
	p->fillRect( rect, bgBrush );
    }

#if 0 // strange code found in QRT - I don't want all my colors to go away !
    if ( formatCollection()->defaultFormat()->color() != cg.text() ) {
	QDict<KoTextFormat> formats = formatCollection()->dict();
	QDictIterator<KoTextFormat> it( formats );
	while ( it.current() ) {
	    if ( it.current() == formatCollection()->defaultFormat() ) {
		++it;
		continue;
	    }
	    it.current()->setColor( cg.text() );
	    ++it;
	}
	formatCollection()->defaultFormat()->setColor( cg.text() );
    }
#endif

    KoTextParag *parag = firstParag();
    while ( parag ) {
	if ( !parag->isValid() )
	    parag->format();
	int y = parag->rect().y();
	QRect pr( parag->rect() );
	pr.setX( 0 );
	pr.setWidth( QWIDGETSIZE_MAX );
	if ( !rect.isNull() && !rect.intersects( pr ) ) {
	    parag = parag->next();
	    continue;
	}
	p->translate( 0, y );
	if ( rect.isValid() )
	    parag->paint( *p, cg, 0, FALSE, rect.x(), rect.y(), rect.width(), rect.height() );
	else
	    parag->paint( *p, cg, 0, FALSE );
	p->translate( 0, -y );
	parag = parag->next();
	if ( !flow()->isEmpty() )
	    flow()->drawFloatingItems( p, rect.x(), rect.y(), rect.width(), rect.height(), cg, FALSE );
    }
}

void KoTextDocument::drawParag( QPainter *p, KoTextParag *parag, int cx, int cy, int cw, int ch,
			       QPixmap *&doubleBuffer, const QColorGroup &cg,
			       bool drawCursor, KoTextCursor *cursor, bool resetChanged )
{
    QPainter *painter = 0;
    if ( resetChanged )
	parag->setChanged( FALSE );
    QRect ir( parag->rect() );
    bool useDoubleBuffer = true;
    //bool useDoubleBuffer = !parag->document()->parent();
    //if ( !useDoubleBuffer && parag->document()->nextDoubleBuffered )
    //useDoubleBuffer = TRUE;
    if ( p->device()->devType() == QInternal::Printer )
	useDoubleBuffer = FALSE;

    if ( useDoubleBuffer  ) {
	if ( cx >= 0 && cy >= 0 )
        {
	    ir = ir.intersect( QRect( cx, cy, cw, ch ) );
            if (ir.isEmpty())
                useDoubleBuffer = FALSE;
        }
    }

    if ( useDoubleBuffer  ) {
	painter = new QPainter;
	if ( !doubleBuffer ||
	     ir.width() > doubleBuffer->width() ||
	     ir.height() > doubleBuffer->height() ) {
	    doubleBuffer = bufferPixmap( ir.size() );
	    painter->begin( doubleBuffer );
	} else {
	    painter->begin( doubleBuffer );
	}
    } else {
	painter = p;
	painter->translate( ir.x(), ir.y() );
    }

    painter->setBrushOrigin( -ir.x(), -ir.y() );

    if ( useDoubleBuffer || is_printer( painter ) ) {
	if ( !parag->backgroundColor() )
	    painter->fillRect( QRect( 0, 0, ir.width(), ir.height() ),
			       cg.brush( QColorGroup::Base ) );
	else
	    painter->fillRect( QRect( 0, 0, ir.width(), ir.height() ),
			       *parag->backgroundColor() );
    } else {
	if ( cursor && cursor->parag() == parag ) {
	    if ( !parag->backgroundColor() )
		painter->fillRect( QRect( parag->at( cursor->index() )->x, 0, 2, ir.height() ),
				   cg.brush( QColorGroup::Base ) );
	    else
		painter->fillRect( QRect( parag->at( cursor->index() )->x, 0, 2, ir.height() ),
				   *parag->backgroundColor() );
	}
    }

    painter->translate( -( ir.x() - parag->rect().x() ),
		       -( ir.y() - parag->rect().y() ) );
    parag->paint( *painter, cg, drawCursor ? cursor : 0, TRUE, cx, cy, cw, ch );
    if ( !flow()->isEmpty() ) {
	painter->translate( 0, -parag->rect().y() );
	QRect cr( cx, cy, cw, ch );
	cr = cr.intersect( QRect( 0, parag->rect().y(), parag->rect().width(), parag->rect().height() ) );
	flow()->drawFloatingItems( painter, cr.x(), cr.y(), cr.width(), cr.height(), cg, FALSE );
	painter->translate( 0, +parag->rect().y() );
    }

    if ( useDoubleBuffer ) {
	delete painter;
	painter = 0;
	p->drawPixmap( ir.topLeft(), *doubleBuffer, QRect( QPoint( 0, 0 ), ir.size() ) );
    } else {
	painter->translate( -ir.x(), -ir.y() );
    }

    if ( useDoubleBuffer ) {
        QRect rect = parag->rect();
        if ( rect.x() + rect.width() < parag->document()->x() + parag->document()->width() ) {
            p->fillRect( rect.x() + rect.width(), rect.y(),
                         ( parag->document()->x() + parag->document()->width() ) -
                         ( rect.x() + rect.width() ),
                         rect.height(), cg.brush( QColorGroup::Base ) );
    }

    //parag->document()->nextDoubleBuffered = FALSE;
}

KoTextParag *KoTextDocument::draw( QPainter *p, int cx, int cy, int cw, int ch, const QColorGroup &cg,
				 bool onlyChanged, bool drawCursor, KoTextCursor *cursor, bool resetChanged )
{
    if ( withoutDoubleBuffer || par && par->withoutDoubleBuffer ) {
	withoutDoubleBuffer = TRUE;
	QRect crect( cx, cy, cw, ch );
	draw( p, crect, cg );
	return 0;
    }
    withoutDoubleBuffer = FALSE;

    if ( !firstParag() )
	return 0;

    if ( drawCursor && cursor )
	tmpCursor = cursor;
    if ( cx < 0 && cy < 0 ) {
	cx = 0;
	cy = 0;
	cw = width();
	ch = height();
    }

    KoTextParag *lastFormatted = 0;
    KoTextParag *parag = firstParag();

    QPixmap *doubleBuffer = 0;
    QPainter painter;
    QRect crect( cx, cy, cw, ch );

    // Space above first parag
    if ( isPageBreakEnabled() && parag && cy <= parag->rect().y() && parag->rect().y() > 0 ) {
	QRect r( 0, 0,
		 parag->document()->x() + parag->document()->width(),
		 parag->rect().y() );
	r &= crect;
	if ( !r.isEmpty() )
	    p->fillRect( r, cg.brush( QColorGroup::Base ) );
    }

    while ( parag ) {
	lastFormatted = parag;
	if ( !parag->isValid() )
	    parag->format();

	QRect ir = parag->rect();
	if ( isPageBreakEnabled() && parag->next() )
	    if ( ir.y() + ir.height() < parag->next()->rect().y() ) {
		QRect r( 0, ir.y() + ir.height(),
			 parag->document()->x() + parag->document()->width(),
			 parag->next()->rect().y() - ( ir.y() + ir.height() ) );
		r &= crect;
		if ( !r.isEmpty() )
		    p->fillRect( r, cg.brush( QColorGroup::Base ) );
	    }

	if ( !ir.intersects( crect ) ) {
	    ir.setWidth( parag->document()->width() );
	    if ( ir.intersects( crect ) )
		p->fillRect( ir.intersect( crect ), cg.brush( QColorGroup::Base ) );
	    if ( ir.y() > cy + ch ) {
		tmpCursor = 0;
		if ( buf_pixmap && buf_pixmap->height() > 300 ) {
		    delete buf_pixmap;
		    buf_pixmap = 0;
		}
		return lastFormatted;
	    }
	    parag = parag->next();
	    continue;
	}

	if ( !parag->hasChanged() && onlyChanged ) {
	    parag = parag->next();
	    continue;
	}

	drawParag( p, parag, cx, cy, cw, ch, doubleBuffer, cg, drawCursor, cursor, resetChanged );
	parag = parag->next();
    }

    parag = lastParag();
    if ( parag->rect().y() + parag->rect().height() < parag->document()->height() ) {
	if ( !parag->document()->parent() ) { // !useDoubleBuffer
	    p->fillRect( 0, parag->rect().y() + parag->rect().height(), parag->document()->width(),
			 parag->document()->height() - ( parag->rect().y() + parag->rect().height() ),
			 cg.brush( QColorGroup::Base ) );
	}
	if ( !flow()->isEmpty() ) {
	    QRect cr( cx, cy, cw, ch );
	    cr = cr.intersect( QRect( 0, parag->rect().y() + parag->rect().height(), parag->document()->width(),
				      parag->document()->height() - ( parag->rect().y() + parag->rect().height() ) ) );
	    flow()->drawFloatingItems( p, cr.x(), cr.y(), cr.width(), cr.height(), cg, FALSE );
	}
    }

    if ( buf_pixmap && buf_pixmap->height() > 300 ) {
	delete buf_pixmap;
	buf_pixmap = 0;
    }

    tmpCursor = 0;
    return lastFormatted;
}
#endif

#if 0
void KoTextDocument::setDefaultFont( const QFont &f )
{
    updateFontSizes( f.pointSize() );
}
#endif

void KoTextDocument::registerCustomItem( KoTextCustomItem *i, KoTextParag *p )
{
    if ( i && i->placement() != KoTextCustomItem::PlaceInline )
	flow_->registerFloatingItem( i );
    p->registerFloatingItem( i );
    i->setParagraph( p );
    //qDebug("KoTextDocument::registerCustomItem %p",(void*)i);
    customItems.append( i );
}

void KoTextDocument::unregisterCustomItem( KoTextCustomItem *i, KoTextParag *p )
{
    flow_->unregisterFloatingItem( i );
    p->unregisterFloatingItem( i );
    i->setParagraph( 0 );
    customItems.removeRef( i );
}

// unused in kotext, and needs KoTextStringChar::isAnchor
#if 0
bool KoTextDocument::hasFocusParagraph() const
{
    return !!focusIndicator.parag;
}

QString KoTextDocument::focusHref() const
{
    return focusIndicator.href;
}

bool KoTextDocument::focusNextPrevChild( bool next )
{
    if ( !focusIndicator.parag ) {
	if ( next ) {
	    focusIndicator.parag = fParag;
	    focusIndicator.start = 0;
	    focusIndicator.len = 0;
	} else {
	    focusIndicator.parag = lParag;
	    focusIndicator.start = lParag->length();
	    focusIndicator.len = 0;
	}
    } else {
	focusIndicator.parag->setChanged( TRUE );
    }
    focusIndicator.href = QString::null;

    if ( next ) {
	KoTextParag *p = focusIndicator.parag;
	int index = focusIndicator.start + focusIndicator.len;
	while ( p ) {
	    for ( int i = index; i < p->length(); ++i ) {
		if ( p->at( i )->isAnchor() ) {
		    p->setChanged( TRUE );
		    focusIndicator.parag = p;
		    focusIndicator.start = i;
		    focusIndicator.len = 0;
		    focusIndicator.href = p->at( i )->format()->anchorHref();
		    while ( i < p->length() ) {
			if ( !p->at( i )->format()->isAnchor() )
			    return TRUE;
			focusIndicator.len++;
			i++;
		    }
		} else if ( p->at( i )->isCustom() ) {
#ifdef QTEXTTABLE_AVAILABLE
		    if ( p->at( i )->customItem()->isNested() ) {
			KoTextTable *t = (KoTextTable*)p->at( i )->customItem();
			QPtrList<KoTextTableCell> cells = t->tableCells();
			// first try to continue
			KoTextTableCell *c;
			bool resetCells = TRUE;
			for ( c = cells.first(); c; c = cells.next() ) {
			    if ( c->richText()->hasFocusParagraph() ) {
				if ( c->richText()->focusNextPrevChild( next ) ) {
				    p->setChanged( TRUE );
				    focusIndicator.parag = p;
				    focusIndicator.start = i;
				    focusIndicator.len = 0;
				    focusIndicator.href = c->richText()->focusHref();
				    return TRUE;
				} else {
				    resetCells = FALSE;
				    c = cells.next();
				    break;
				}
			    }
			}
			// now really try
			if ( resetCells )
			    c = cells.first();
			for ( ; c; c = cells.next() ) {
			    if ( c->richText()->focusNextPrevChild( next ) ) {
				p->setChanged( TRUE );
				focusIndicator.parag = p;
				focusIndicator.start = i;
				focusIndicator.len = 0;
				focusIndicator.href = c->richText()->focusHref();
				return TRUE;
			    }
			}
		    }
#endif
		}
	    }
	    index = 0;
	    p = p->next();
	}
    } else {
	KoTextParag *p = focusIndicator.parag;
	int index = focusIndicator.start - 1;
	if ( focusIndicator.len == 0 && index < focusIndicator.parag->length() - 1 )
	    index++;
	while ( p ) {
	    for ( int i = index; i >= 0; --i ) {
		if ( p->at( i )->format()->isAnchor() ) {
		    p->setChanged( TRUE );
		    focusIndicator.parag = p;
		    focusIndicator.start = i;
		    focusIndicator.len = 0;
		    focusIndicator.href = p->at( i )->format()->anchorHref();
		    while ( i >= -1 ) {
			if ( i < 0 || !p->at( i )->format()->isAnchor() ) {
			    focusIndicator.start++;
			    return TRUE;
			}
			if ( i < 0 )
			    break;
			focusIndicator.len++;
			focusIndicator.start--;
			i--;
		    }
		} else if ( p->at( i )->isCustom() ) {
#ifdef QTEXTTABLE_AVAILABLE
		    if ( p->at( i )->customItem()->isNested() ) {
			KoTextTable *t = (KoTextTable*)p->at( i )->customItem();
			QPtrList<KoTextTableCell> cells = t->tableCells();
			// first try to continue
			KoTextTableCell *c;
			bool resetCells = TRUE;
			for ( c = cells.last(); c; c = cells.prev() ) {
			    if ( c->richText()->hasFocusParagraph() ) {
				if ( c->richText()->focusNextPrevChild( next ) ) {
				    p->setChanged( TRUE );
				    focusIndicator.parag = p;
				    focusIndicator.start = i;
				    focusIndicator.len = 0;
				    focusIndicator.href = c->richText()->focusHref();
				    return TRUE;
				} else {
				    resetCells = FALSE;
				    c = cells.prev();
				    break;
				}
			    }
			    if ( cells.at() == 0 )
				break;
			}
			// now really try
			if ( resetCells )
			    c = cells.last();
			for ( ; c; c = cells.prev() ) {
			    if ( c->richText()->focusNextPrevChild( next ) ) {
				p->setChanged( TRUE );
				focusIndicator.parag = p;
				focusIndicator.start = i;
				focusIndicator.len = 0;
				focusIndicator.href = c->richText()->focusHref();
				return TRUE;
			    }
			    if ( cells.at() == 0 )
				break;
			}
		    }
#endif
		}
	    }
	    p = p->prev();
	    if ( p )
		index = p->length() - 1;
	}
    }

    focusIndicator.parag = 0;

    return FALSE;
}
#endif

int KoTextDocument::length() const
{
    int l = 0;
    KoTextParag *p = fParag;
    while ( p ) {
	l += p->length() - 1; // don't count trailing space
	p = p->next();
    }
    return l;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextString::KoTextString()
{
    bidiDirty = FALSE;
    bNeedsSpellCheck = true;
    bidi = FALSE;
    rightToLeft = FALSE;
    dir = QChar::DirON;
}

KoTextString::KoTextString( const KoTextString &s )
{
    bidiDirty = s.bidiDirty;
    bNeedsSpellCheck = s.bNeedsSpellCheck;
    bidi = s.bidi;
    rightToLeft = s.rightToLeft;
    dir = s.dir;
    data = s.subString();
}

void KoTextString::insert( int index, const QString &s, KoTextFormat *f )
{
    int os = data.size();
    data.resize( data.size() + s.length() );
    if ( index < os ) {
	memmove( data.data() + index + s.length(), data.data() + index,
		 sizeof( KoTextStringChar ) * ( os - index ) );
    }
    for ( int i = 0; i < (int)s.length(); ++i ) {
	data[ (int)index + i ].x = 0;
	data[ (int)index + i ].pixelxadj = 0;
	data[ (int)index + i ].pixelwidth = 0;
	data[ (int)index + i ].width = 0;
	data[ (int)index + i ].lineStart = 0;
	data[ (int)index + i ].d.format = 0;
	data[ (int)index + i ].type = KoTextStringChar::Regular;
	data[ (int)index + i ].rightToLeft = 0;
	data[ (int)index + i ].startOfRun = 0;
        data[ (int)index + i ].c = s[ i ];
#ifdef DEBUG_COLLECTION
	qDebug("KoTextString::insert setting format %p to character %d",f,(int)index+i);
#endif
	data[ (int)index + i ].setFormat( f );
    }
    bidiDirty = TRUE;
    bNeedsSpellCheck = true;
}

KoTextString::~KoTextString()
{
    clear();
}

void KoTextString::insert( int index, KoTextStringChar *c )
{
    int os = data.size();
    data.resize( data.size() + 1 );
    if ( index < os ) {
	memmove( data.data() + index + 1, data.data() + index,
		 sizeof( KoTextStringChar ) * ( os - index ) );
    }
    data[ (int)index ].c = c->c;
    data[ (int)index ].x = 0;
    data[ (int)index ].pixelxadj = 0;
    data[ (int)index ].pixelwidth = 0;
    data[ (int)index ].width = 0;
    data[ (int)index ].lineStart = 0;
    data[ (int)index ].rightToLeft = 0;
    data[ (int)index ].d.format = 0;
    data[ (int)index ].type = KoTextStringChar::Regular;
    data[ (int)index ].setFormat( c->format() );
    bidiDirty = TRUE;
    bNeedsSpellCheck = true;
}

void KoTextString::truncate( int index )
{
    index = QMAX( index, 0 );
    index = QMIN( index, (int)data.size() - 1 );
    if ( index < (int)data.size() ) {
	for ( int i = index + 1; i < (int)data.size(); ++i ) {
	    if ( data[ i ].isCustom() ) {
		delete data[ i ].customItem();
		if ( data[ i ].d.custom->format )
		    data[ i ].d.custom->format->removeRef();
		delete data[ i ].d.custom;
		data[ i ].d.custom = 0;
	    } else if ( data[ i ].format() ) {
		data[ i ].format()->removeRef();
	    }
	}
    }
    data.truncate( index );
    bidiDirty = TRUE;
    bNeedsSpellCheck = true;
}

void KoTextString::remove( int index, int len )
{
    for ( int i = index; i < (int)data.size() && i - index < len; ++i ) {
	if ( data[ i ].isCustom() ) {
	    delete data[ i ].customItem();
	    if ( data[ i ].d.custom->format )
		data[ i ].d.custom->format->removeRef();
            delete data[ i ].d.custom;
	    data[ i ].d.custom = 0;
	} else if ( data[ i ].format() ) {
	    data[ i ].format()->removeRef();
	}
    }
    memmove( data.data() + index, data.data() + index + len,
	     sizeof( KoTextStringChar ) * ( data.size() - index - len ) );
    data.resize( data.size() - len );
    bidiDirty = TRUE;
    bNeedsSpellCheck = true;
}

void KoTextString::clear()
{
    for ( int i = 0; i < (int)data.count(); ++i ) {
	if ( data[ i ].isCustom() ) {
	    delete data[ i ].customItem();
	    if ( data[ i ].d.custom->format )
		data[ i ].d.custom->format->removeRef();
	    delete data[ i ].d.custom;
	    data[ i ].d.custom = 0;
	} else if ( data[ i ].format() ) {
	    data[ i ].format()->removeRef();
	}
    }
    data.resize( 0 );
}

void KoTextString::setFormat( int index, KoTextFormat *f, bool useCollection )
{
//    qDebug("KoTextString::setFormat index=%d f=%p",index,f);
    if ( useCollection && data[ index ].format() )
    {
	//qDebug("KoTextString::setFormat removing ref on old format %p",data[ index ].format());
	data[ index ].format()->removeRef();
    }
    data[ index ].setFormat( f );
}

void KoTextString::checkBidi() const
{
    bool rtlKnown = FALSE;
    if ( dir == QChar::DirR ) {
	((KoTextString *)this)->bidi = TRUE;
	((KoTextString *)this)->rightToLeft = TRUE;
	((KoTextString *)this)->bidiDirty = FALSE;
	return;
    } else if ( dir == QChar::DirL ) {
	((KoTextString *)this)->rightToLeft = FALSE;
	rtlKnown = TRUE;
    } else {
	((KoTextString *)this)->rightToLeft = FALSE;
    }

    int len = data.size();
    const KoTextStringChar *c = data.data();
    ((KoTextString *)this)->bidi = FALSE;
    while( len ) {
	if ( !rtlKnown ) {
	    switch( c->c.direction() )
	    {
		case QChar::DirL:
		case QChar::DirLRO:
		case QChar::DirLRE:
		    ((KoTextString *)this)->rightToLeft = FALSE;
		    rtlKnown = TRUE;
		    break;
		case QChar::DirR:
		case QChar::DirAL:
		case QChar::DirRLO:
		case QChar::DirRLE:
		    ((KoTextString *)this)->rightToLeft = TRUE;
		    rtlKnown = TRUE;
		    break;
		default:
		    break;
	    }
	}
	uchar row = c->c.row();
	if( (row > 0x04 && row < 0x09) || ( row > 0xfa && row < 0xff ) ) {
	    ((KoTextString *)this)->bidi = TRUE;
            if ( rtlKnown )
                break;
	}
	len--;
	++c;
    }
    ((KoTextString *)this)->bidiDirty = FALSE;
}

QMemArray<KoTextStringChar> KoTextString::subString( int start, int len ) const
{
    if ( len == 0xFFFFFF )
	len = data.size();
    QMemArray<KoTextStringChar> a;
    a.resize( len );
    for ( int i = 0; i < len; ++i ) {
	KoTextStringChar *c = &data[ i + start ];
	a[ i ].c = c->c;
	a[ i ].x = 0;
	a[ i ].pixelxadj = 0;
	a[ i ].pixelwidth = 0;
	a[ i ].width = 0;
	a[ i ].lineStart = 0;
	a[ i ].rightToLeft = 0;
	a[ i ].d.format = 0;
	a[ i ].type = KoTextStringChar::Regular;
	a[ i ].setFormat( c->format() );
	if ( c->format() )
	    c->format()->addRef();
    }
    return a;
}

QString KoTextString::mid( int start, int len ) const
{
    if ( len == 0xFFFFFF )
	len = data.size();
    QString res;
    res.setLength( len );
    for ( int i = 0; i < len; ++i ) {
	KoTextStringChar *c = &data[ i + start ];
	res[ i ] = c->c;
    }
    return res;
}

QString KoTextString::toString( const QMemArray<KoTextStringChar> &data )
{
    QString s;
    int l = data.size();
    s.setUnicode( 0, l );
    KoTextStringChar *c = data.data();
    QChar *uc = (QChar *)s.unicode();
    while ( l-- ) {
	*uc = c->c;
	uc++;
	c++;
    }

    return s;
}

QString KoTextString::toReverseString() const
{
    QString s;
    int l = length();
    s.setUnicode(0, l);
    KoTextStringChar *c = data.data() + (l-1);
    QChar *uc = (QChar *)s.unicode();
    while ( l-- ) {
	*uc = c->c;
	uc++;
	c--;
    }

    return s;
}


void KoTextStringChar::setFormat( KoTextFormat *f )
{
    if ( type == Regular ) {
	d.format = f;
    } else {
	if ( !d.custom ) {
	    d.custom = new CustomData;
	    d.custom->custom = 0;
	}
	d.custom->format = f;
        if ( d.custom->custom )
            d.custom->custom->setFormat( f );
    }
}

void KoTextStringChar::setCustomItem( KoTextCustomItem *i )
{
    if ( type == Regular ) {
	KoTextFormat *f = format();
	d.custom = new CustomData;
	d.custom->format = f;
	type = Custom;
    } else {
	delete d.custom->custom;
    }
    d.custom->custom = i;
}

void KoTextStringChar::loseCustomItem() // setRegular() might be a better name
{
    if ( isCustom() ) {
	KoTextFormat *f = d.custom->format;
	d.custom->custom = 0;
	delete d.custom;
	type = Regular;
	d.format = f;
    }
}

KoTextStringChar::~KoTextStringChar()
{
    if ( format() )
	format()->removeRef();
    switch ( type ) {
	case Custom:
	    delete d.custom; break;
	default:
	    break;
    }
}

KoTextStringChar *KoTextStringChar::clone() const
{
    KoTextStringChar *chr = new KoTextStringChar;
    chr->c = c;
    chr->x = 0;
    chr->pixelxadj = 0;
    chr->pixelwidth = 0;
    chr->width = 0;
    chr->lineStart = 0;
    chr->rightToLeft = 0;
    chr->d.format = 0;
    chr->type = KoTextStringChar::Regular;
    chr->setFormat( format() );
    if ( chr->format() )
	chr->format()->addRef();
    return chr;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextParag::KoTextParag( KoTextDocument *d, KoTextParag *pr, KoTextParag *nx, bool updateIds )
    : invalid( 0 ), p( pr ), n( nx ), doc( d ), align( 0 ), mSelections( 0 ),
      mFloatingItems( 0 ),
      //tm( -1 ), bm( -1 ), lm( -1 ), rm( -1 ), flm( -1 ),
#ifdef QTEXTTABLE_AVAILABLE
      tc( 0 ),
#endif
      //pFormatter( 0 ),
      tArray( 0 )
      //tabStopWidth( 0 ),
      //eData( 0 ), pntr( 0 ), commandHistory( 0 )
{
    //bgcol = 0;
    breakable = TRUE;
    isBr = FALSE;
    movedDown = FALSE;
    visible = TRUE;
    //list_val = -1;
    newLinesAllowed = FALSE;
    //lastInFrame = FALSE;
    defFormat = formatCollection()->defaultFormat();
    /*if ( !doc ) {
	tabStopWidth = defFormat->width( 'x' ) * 8;
	commandHistory = new KoTextDocCommandHistory( 100 );
    }*/
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new KoTextParag" );
#endif
    fullWidth = TRUE;

    if ( p ) {
	p->n = this;
#ifdef QTEXTTABLE_AVAILABLE
	if ( p->tc )
	    tc = p->tc;
#endif
    }
    if ( n ) {
	n->p = this;
#ifdef QTEXTTABLE_AVAILABLE
	if ( n->tc )
	    tc = n->tc;
#endif
    }

#ifdef QTEXTTABLE_AVAILABLE
    if ( !tc && d && d->tableCell() )
	tc = d->tableCell();
#endif

    if ( !p && doc )
	doc->setFirstParag( this );
    if ( !n && doc )
	doc->setLastParag( this );

    changed = FALSE;
    m_lineChanged = -1;
    //firstFormat = TRUE; //// unused
    //state = -1;
    //needPreProcess = FALSE;

    if ( p )
	id = p->id + 1;
    else
	id = 0;
    if ( n && updateIds ) {
	KoTextParag *s = n;
	while ( s ) {
	    s->id = s->p->id + 1;
	    //s->lm = s->rm = s->tm = s->bm = -1, s->flm = -1;
	    s = s->n;
	}
    }
    //firstPProcess = TRUE;

    str = new KoTextString();
    str->insert( 0, " ", formatCollection()->defaultFormat() );
    //// kotext
    setNewLinesAllowed(TRUE);
    ////
}

KoTextParag::~KoTextParag()
{
    //qDebug("KoTextParag::~KoTextParag %p id=%d",this,paragId());
    delete str;
//    if ( doc && p == doc->minwParag ) {
//	doc->minwParag = 0;
//	doc->minw = 0;
//    }
    if ( !doc ) {
	//delete pFormatter;
	//delete commandHistory;
    }
    delete [] tArray;
    //delete eData;
    QMap<int, KoTextParagLineStart*>::Iterator it = lineStarts.begin();
    for ( ; it != lineStarts.end(); ++it )
	delete *it;
    if ( mSelections ) delete mSelections;
    if ( mFloatingItems ) delete mFloatingItems;

    if (p)
       p->setNext(n);
    if (n)
       n->setPrev(p);

    //// kotext
    if ( doc && !doc->isDestroying() )
    {
        doc->informParagraphDeleted( this );
    }
    //kdDebug(32500) << "KoTextParag::~KoTextParag " << this << endl;
    ////
}

void KoTextParag::setNext( KoTextParag *s )
{
    n = s;
    if ( !n && doc )
	doc->setLastParag( this );
}

void KoTextParag::setPrev( KoTextParag *s )
{
    p = s;
    if ( !p && doc )
	doc->setFirstParag( this );
}

void KoTextParag::invalidate( int chr )
{
    if ( invalid < 0 )
	invalid = chr;
    else
	invalid = QMIN( invalid, chr );
#if 0 /// strange code!
    if ( mFloatingItems ) {
	for ( KoTextCustomItem *i = mFloatingItems->first(); i; i = mFloatingItems->next() )
	    i->move( 0, -1 );
    }
#endif
    //lm = rm = bm = tm = flm = -1;
}

void KoTextParag::setChanged( bool b, bool /*recursive*/ )
{
    changed = b;
    m_lineChanged = -1; // all
    //if ( recursive ) {
//	if ( doc && doc->parentParag() )
//	    doc->parentParag()->setChanged( b, recursive );
//    }
}

void KoTextParag::setLineChanged( short int line )
{
    if ( m_lineChanged == -1 ) {
        if ( !changed ) // only if the whole parag wasn't "changed" already
            m_lineChanged = line;
    }
    else
        m_lineChanged = QMIN( m_lineChanged, line ); // also works if line=-1
    changed = true;
    //qDebug( "KoTextParag::setLineChanged line=%d -> m_lineChanged=%d", line, m_lineChanged );
}

void KoTextParag::insert( int index, const QString &s )
{
#if 0
    if ( doc && !doc->useFormatCollection() && doc->preProcessor() )
	str->insert( index, s,
		     doc->preProcessor()->format( KoTextPreProcessor::Standard ) );
    else
#endif
	str->insert( index, s, formatCollection()->defaultFormat() );
    invalidate( index );
    //needPreProcess = TRUE;
}

void KoTextParag::truncate( int index )
{
    str->truncate( index );
    insert( length(), " " );
    //needPreProcess = TRUE;
}

void KoTextParag::remove( int index, int len )
{
    if ( index + len - str->length() > 0 )
	return;
    for ( int i = index; i < index + len; ++i ) {
	KoTextStringChar *c = at( i );
	if ( doc && c->isCustom() ) {
	    doc->unregisterCustomItem( c->customItem(), this );
	    //removeCustomItem();
	}
    }
    str->remove( index, len );
    invalidate( 0 );
    //needPreProcess = TRUE;
}

void KoTextParag::join( KoTextParag *s )
{
    //qDebug("KoTextParag::join this=%d (length %d) with %d (length %d)",paragId(),length(),s->paragId(),s->length());
    int oh = r.height() + s->r.height();
    n = s->n;
    if ( n )
	n->p = this;
    else if ( doc )
	doc->setLastParag( this );

    int start = str->length();
    if ( length() > 0 && at( length() - 1 )->c == ' ' ) {
	remove( length() - 1, 1 );
	--start;
    }
    append( s->str->toString(), TRUE );

    for ( int i = 0; i < s->length(); ++i ) {
	if ( !doc || doc->useFormatCollection() ) {
	    s->str->at( i ).format()->addRef();
	    str->setFormat( i + start, s->str->at( i ).format(), TRUE );
	}
	if ( s->str->at( i ).isCustom() ) {
	    KoTextCustomItem * item = s->str->at( i ).customItem();
	    str->at( i + start ).setCustomItem( item );
	    s->str->at( i ).loseCustomItem();
	    doc->unregisterCustomItem( item, s ); // ### missing in QRT
	    doc->registerCustomItem( item, this );
	}
    }
    Q_ASSERT(str->at(str->length()-1).c == ' ');

    /*if ( !extraData() && s->extraData() ) {
	setExtraData( s->extraData() );
	s->setExtraData( 0 );
    } else if ( extraData() && s->extraData() ) {
	extraData()->join( s->extraData() );
        }*/
    delete s;
    invalidate( 0 );
    //// kotext
    invalidateCounters();
    ////
    r.setHeight( oh );
    //needPreProcess = TRUE;
    if ( n ) {
	KoTextParag *s = n;
	while ( s ) {
	    s->id = s->p->id + 1;
	    //s->state = -1;
	    //s->needPreProcess = TRUE;
	    s->changed = TRUE;
	    s = s->n;
	}
    }
    format();
    //state = -1;
}

void KoTextParag::move( int &dy )
{
    //qDebug("KoTextParag::move paragId=%d dy=%d",paragId(),dy);
    if ( dy == 0 )
	return;
    changed = TRUE;
    r.moveBy( 0, dy );
    if ( mFloatingItems ) {
	for ( KoTextCustomItem *i = mFloatingItems->first(); i; i = mFloatingItems->next() ) {
		i->finalize();
	}
    }
    //if ( p )
    //    p->lastInFrame = TRUE; // Qt does this, but the loop at the end of format() calls move a lot!

    movedDown = FALSE;

    // do page breaks if required
    if ( doc && doc->isPageBreakEnabled() ) {
	int shift;
	if ( ( shift = doc->formatter()->formatVertically(  doc, this ) ) ) {
	    if ( p )
		p->setChanged( TRUE );
	    dy += shift;
	}
    }
}

void KoTextParag::format( int start, bool doMove )
{
    if ( !str || str->length() == 0 || !formatter() )
	return;

#if 0
    if ( doc &&
	 doc->preProcessor() &&
	 ( needPreProcess || state == -1 ) )
	doc->preProcessor()->process( doc, this, invalid <= 0 ? 0 : invalid );
    needPreProcess = FALSE;
#endif

    if ( invalid == -1 )
	return;

    //kdDebug() << "KoTextParag::format " << this << " id:" << paragId() << endl;

    r.moveTopLeft( QPoint( documentX(), p ? p->r.y() + p->r.height() : documentY() ) );
    //if ( p )
    //    p->lastInFrame = FALSE;

    movedDown = FALSE;
    bool formattedAgain = FALSE;

 formatAgain:
    r.setWidth( documentWidth() );

    // Not really useful....
    if ( doc && mFloatingItems ) {
	for ( KoTextCustomItem *i = mFloatingItems->first(); i; i = mFloatingItems->next() ) {
	    if ( i->placement() == KoTextCustomItem::PlaceRight )
		i->move( r.x() + r.width() - i->width, r.y() );
	    else
		i->move( i->x(), r.y() );
	}
    }
    QMap<int, KoTextParagLineStart*> oldLineStarts = lineStarts;
    lineStarts.clear();
    int y = formatter()->format( doc, this, start, oldLineStarts );

    r.setWidth( QMAX( r.width(), formatter()->minimumWidth() ) );

    QMap<int, KoTextParagLineStart*>::Iterator it = oldLineStarts.begin();

    for ( ; it != oldLineStarts.end(); ++it )
	delete *it;

    KoTextStringChar *c = 0;
    if ( hasBorder() || string()->isRightToLeft() )
        ////kotext: border extends to doc width
        ////        and, bidi parags might have a counter, which will be right-aligned...
    {
        setWidth( textDocument()->width() - 1 );
    }
    else
    {
        if ( lineStarts.count() == 1 ) { //&& ( !doc || doc->flow()->isEmpty() ) ) {
            if ( !string()->isBidi() ) {
                c = &str->at( str->length() - 1 );
                r.setWidth( c->x + c->width );
            } else {
                r.setWidth( lineStarts[0]->w );
            }
        }
        if ( newLinesAllowed ) {
            it = lineStarts.begin();
            int usedw = 0; int lineid = 0;
            for ( ; it != lineStarts.end(); ++it, ++lineid ) {
                usedw = QMAX( usedw, (*it)->w );
            }
            if ( r.width() <= 0 ) {
                // if the user specifies an invalid rect, this means that the
                // bounding box should grow to the width that the text actually
                // needs
                r.setWidth( usedw );
            } else {
                r.setWidth( QMIN( usedw, r.width() ) );
            }
        }
    }

    if ( y != r.height() )
	r.setHeight( y );

    if ( !visible )
	r.setHeight( 0 );

    // do page breaks if required
    if ( doc && doc->isPageBreakEnabled() ) {
        int shift = doc->formatter()->formatVertically( doc, this );
        //qDebug("formatVertically returned shift=%d",shift);
        if ( shift && !formattedAgain ) {
            formattedAgain = TRUE;
            goto formatAgain;
        }
    }

    if ( n && doMove && n->invalid == -1 && r.y() + r.height() != n->r.y() ) {
        //kdDebug(32500) << "r=" << r << " n->r=" << n->r << endl;
	int dy = ( r.y() + r.height() ) - n->r.y();
	KoTextParag *s = n;
	bool makeInvalid = false; //p && p->lastInFrame;
	//qDebug("might move of dy=%d. previous's lastInFrame (=makeInvalid): %d", dy, makeInvalid);
	while ( s && dy ) {
            if ( s->movedDown ) { // (not in QRT) : moved down -> invalidate and stop moving down
                s->invalidate( 0 ); // (there is no point in moving down a parag that has a frame break...)
                break;
            }
	    if ( !s->isFullWidth() )
		makeInvalid = TRUE;
	    if ( makeInvalid )
		s->invalidate( 0 );
	    s->move( dy );
	    //if ( s->lastInFrame )
            //    makeInvalid = TRUE;
  	    s = s->n;
	}
    }

//#define DEBUG_CI_PLACEMENT
    if ( mFloatingItems ) {
#ifdef DEBUG_CI_PLACEMENT
        qDebug("%d lines", lineStarts.count());
#endif
        // Place custom items - after the formatting is finished
        int len = length();
        int line = -1;
        int lineY = 0; // the one called "cy" in other algos
        int baseLine = 0;
        QMap<int, KoTextParagLineStart*>::Iterator it = lineStarts.begin();
        for ( int i = 0 ; i < len; ++i ) {
            KoTextStringChar *chr = &str->at( i );
            if ( chr->lineStart ) {
                ++line;
                if ( line > 0 )
                    ++it;
                lineY = (*it)->y;
                baseLine = (*it)->baseLine;
#ifdef DEBUG_CI_PLACEMENT
                qDebug("New line (%d): lineStart=%p lineY=%d baseLine=%d height=%d", line, (*it), lineY, baseLine, (*it)->h);
#endif
            }
            if ( chr->isCustom() ) {
                int x = chr->x;
                KoTextCustomItem* item = chr->customItem();
                Q_ASSERT( baseLine >= item->ascent() ); // something went wrong in KoTextFormatter if this isn't the case
                int y = lineY + baseLine - item->ascent();
#ifdef DEBUG_CI_PLACEMENT
                qDebug("Custom item: i=%d x=%d lineY=%d baseLine=%d ascent=%d -> y=%d", i, x, lineY, baseLine, item->ascent(), y);
#endif
                item->move( x, y );
                item->finalize();
            }
        }
    }

    //firstFormat = FALSE; //// unused
    changed = TRUE;
    invalid = -1;
    //####   string()->setTextChanged( FALSE );
}

int KoTextParag::lineHeightOfChar( int i, int *bl, int *y ) const
{
    if ( !isValid() )
	( (KoTextParag*)this )->format();

    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.end();
    --it;
    for ( ;; ) {
	if ( i >= it.key() ) {
	    if ( bl )
		*bl = ( *it )->baseLine;
	    if ( y )
		*y = ( *it )->y;
	    return ( *it )->h;
	}
	if ( it == lineStarts.begin() )
	    break;
	--it;
    }

    qWarning( "KoTextParag::lineHeightOfChar: couldn't find lh for %d", i );
    return 15;
}

KoTextStringChar *KoTextParag::lineStartOfChar( int i, int *index, int *line ) const
{
    if ( !isValid() )
	( (KoTextParag*)this )->format();

    int l = (int)lineStarts.count() - 1;
    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.end();
    --it;
    for ( ;; ) {
	if ( i >= it.key() ) {
	    if ( index )
		*index = it.key();
	    if ( line )
		*line = l;
	    return &str->at( it.key() );
	}
	if ( it == lineStarts.begin() )
	    break;
	--it;
	--l;
    }

    qWarning( "KoTextParag::lineStartOfChar: couldn't find %d", i );
    return 0;
}

int KoTextParag::lines() const
{
    if ( !isValid() )
	( (KoTextParag*)this )->format();

    return (int)lineStarts.count();
}

KoTextStringChar *KoTextParag::lineStartOfLine( int line, int *index ) const
{
    if ( !isValid() )
	( (KoTextParag*)this )->format();

    if ( line >= 0 && line < (int)lineStarts.count() ) {
	QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.begin();
	while ( line-- > 0 )
	    ++it;
	int i = it.key();
	if ( index )
	    *index = i;
	return &str->at( i );
    }

    qWarning( "KoTextParag::lineStartOfLine: couldn't find %d", line );
    return 0;
}

int KoTextParag::leftGap() const
{
    if ( !isValid() )
	( (KoTextParag*)this )->format();

    int line = 0;
    int x = str->at(0).x;  /* set x to x of first char */
    if ( str->isBidi() ) {
	for ( int i = 1; i < str->length(); ++i )
	    x = QMIN(x, str->at(i).x);
	return x;
    }

    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.begin();
    while (line < (int)lineStarts.count()) {
	int i = it.key(); /* char index */
	x = QMIN(x, str->at(i).x);
	++it;
	++line;
    }
    return x;
}

void KoTextParag::setFormat( int index, int len, KoTextFormat *f, bool useCollection, int flags )
{
    if ( index < 0 )
	index = 0;
    if ( index > str->length() - 1 )
	index = str->length() - 1;
    if ( index + len >= str->length() )
	len = str->length() - index;

    KoTextFormatCollection *fc = 0;
    if ( useCollection )
	fc = formatCollection();
    KoTextFormat *of;
    for ( int i = 0; i < len; ++i ) {
	of = str->at( i + index ).format();
	if ( !changed && f->key() != of->key() )
	    changed = TRUE;
        // ######## Is this test exhaustive?
	if ( invalid == -1 &&
	     ( f->font().family() != of->font().family() ||
	       f->pointSize() != of->pointSize() ||
	       f->font().weight() != of->font().weight() ||
	       f->font().italic() != of->font().italic() ||
	       f->vAlign() != of->vAlign() ||
               f->relativeTextSize() != of->relativeTextSize() ||
               f->offsetFromBaseLine() != of->offsetFromBaseLine() ||
               f->wordByWord() != of->wordByWord()  ||
               f->attributeFont() != of->attributeFont() ||
               f->language() != of->language()) ||
               f->hyphenation() != of->hyphenation()) {
	    invalidate( 0 );
	}
	if ( flags == -1 || flags == KoTextFormat::Format || !fc ) {
#ifdef DEBUG_COLLECTION
	    qDebug(" KoTextParag::setFormat, will use format(f) %p %s", f, f->key().latin1());
#endif
	    if ( fc )
		f = fc->format( f );
	    str->setFormat( i + index, f, useCollection );
	} else {
#ifdef DEBUG_COLLECTION
	    qDebug(" KoTextParag::setFormat, will use format(of,f,flags) of=%p %s, f=%p %s", of, of->key().latin1(), f, f->key().latin1() );
#endif
	    KoTextFormat *fm = fc->format( of, f, flags );
#ifdef DEBUG_COLLECTION
	    qDebug(" KoTextParag::setFormat, format(of,f,flags) returned %p %s ", fm,fm->key().latin1());
#endif
	    str->setFormat( i + index, fm, useCollection );
	}
    }
}

void KoTextParag::indent( int *oldIndent, int *newIndent )
{
    if ( !doc || !doc->indent() /*|| isListItem() TODO*/ ) {
	if ( oldIndent )
	    *oldIndent = 0;
	if ( newIndent )
	    *newIndent = 0;
	if ( oldIndent && newIndent )
	    *newIndent = *oldIndent;
	return;
    }
    doc->indent()->indent( doc, this, oldIndent, newIndent );
}

void KoTextParag::drawCursorDefault( QPainter &painter, KoTextCursor *cursor, int curx, int cury, int curh, const QColorGroup &cg )
{
    painter.fillRect( QRect( curx, cury, 1, curh ), cg.color( QColorGroup::Text ) );
    painter.save();
    if ( string()->isBidi() ) {
        const int d = 4;
        if ( at( cursor->index() )->rightToLeft ) {
            painter.setPen( Qt::black );
            painter.drawLine( curx, cury, curx - d / 2, cury + d / 2 );
            painter.drawLine( curx, cury + d, curx - d / 2, cury + d / 2 );
        } else {
            painter.setPen( Qt::black );
            painter.drawLine( curx, cury, curx + d / 2, cury + d / 2 );
            painter.drawLine( curx, cury + d, curx + d / 2, cury + d / 2 );
        }
    }
    painter.restore();
}

int *KoTextParag::tabArray() const
{
    int *ta = tArray;
    if ( !ta && doc )
	ta = doc->tabArray();
    return ta;
}

int KoTextParag::nextTabDefault( int, int x )
{
    int *ta = tArray;
    //if ( doc ) {
	if ( !ta )
	    ta = doc->tabArray();
	int tabStopWidth = doc->tabStopWidth();
    //}
    if ( tabStopWidth != 0 )
	return tabStopWidth*(x/tabStopWidth+1);
    else
        return x;
}

/*void KoTextParag::setPainter( QPainter *p, bool adjust  )
{
    pntr = p;
    for ( int i = 0; i < length(); ++i ) {
	if ( at( i )->isCustom() )
	    at( i )->customItem()->setPainter( p, adjust  );
    }
}*/

KoTextFormatCollection *KoTextParag::formatCollection() const
{
    if ( doc )
	return doc->formatCollection();
    if ( !qFormatCollection )
	qFormatCollection = new KoTextFormatCollection;
    return qFormatCollection;
}

QString KoTextParag::richText() const
{
    QString s;
#if 0
    KoTextStringChar *formatChar = 0;
    QString spaces;
    for ( int i = 0; i < length()-1; ++i ) {
	KoTextStringChar *c = &str->at( i );
#endif
#if 0
        if ( c->isAnchor() && !c->anchorName().isEmpty() ) {
            if ( c->anchorName().contains( '#' ) ) {
                QStringList l = QStringList::split( '#', c->anchorName() );
                for ( QStringList::ConstIterator it = l.begin(); it != l.end(); ++it )
                    s += "<a name=\"" + *it + "\"></a>";
            } else {
                s += "<a name=\"" + c->anchorName() + "\"></a>";
            }
        }
#endif
#if 0
        if ( !formatChar ) {
            s += c->format()->makeFormatChangeTags( 0, QString::null, QString::null /*c->anchorHref()*/ );
            formatChar = c;
        } else if ( ( formatChar->format()->key() != c->format()->key() && c->c != ' ' ) /* ||
                  (formatChar->isAnchor() != c->isAnchor() &&
                  (!c->anchorHref().isEmpty() || !formatChar->anchorHref().isEmpty() ) ) */ )  {// lisp was here
            s += c->format()->makeFormatChangeTags( formatChar->format(), QString::null /*formatChar->anchorHref()*/,
                                                    QString::null /*c->anchorHref()*/ );
            formatChar = c;
        }

        if ( c->c == ' ' || c->c == '\t' ) {
            spaces += c->c;
            continue;
        } else if ( !spaces.isEmpty() ) {
            if ( spaces.length() > 1 || spaces[0] == '\t' )
                s += "<wsp>" + spaces + "</wsp>";
            else
                s += spaces;
            spaces = QString::null;
        }

        if ( c->c == '<' ) {
            s += "&lt;";
        } else if ( c->c == '>' ) {
            s += "&gt;";
        } else if ( c->isCustom() ) {
            s += c->customItem()->richText();
        } else {
            s += c->c;
        }
    }
    if ( !spaces.isEmpty() ) {
        if ( spaces.length() > 1 || spaces[0] == '\t' )
                s += "<wsp>" + spaces + "</wsp>";
        else
            s += spaces;
    }

    if ( formatChar )
        s += formatChar->format()->makeFormatEndTags( QString::null /*formatChar->anchorHref()*/ );
#endif
    return s;
}

/*void KoTextParag::addCommand( KoTextDocCommand *cmd )
{
    if ( !doc )
	commandHistory->addCommand( cmd );
    else
	doc->commands()->addCommand( cmd );
}

KoTextCursor *KoTextParag::undo( KoTextCursor *c )
{
    if ( !doc )
	return commandHistory->undo( c );
    return doc->commands()->undo( c );
}

KoTextCursor *KoTextParag::redo( KoTextCursor *c )
{
    if ( !doc )
	return commandHistory->redo( c );
    return doc->commands()->redo( c );
}*/

void KoTextParag::show()
{
    if ( visible || !doc )
	return;
    visible = TRUE;
}

void KoTextParag::hide()
{
    if ( !visible || !doc )
	return;
    visible = FALSE;
}

void KoTextParag::setDirection( QChar::Direction d )
{
    if ( str && str->direction() != d ) {
	str->setDirection( d );
	invalidate( 0 );
        //// kotext
        m_layout.direction = d;
        ////
    }
}

QChar::Direction KoTextParag::direction() const
{
    return (str ? str->direction() : QChar::DirON );
}

void KoTextParag::setSelection( int id, int start, int end )
{
    QMap<int, KoTextParagSelection>::ConstIterator it = selections().find( id );
    if ( it != mSelections->end() ) {
	if ( start == ( *it ).start && end == ( *it ).end )
	    return;
    }

    KoTextParagSelection sel;
    sel.start = start;
    sel.end = end;
    (*mSelections)[ id ] = sel;
    setChanged( TRUE, TRUE );
}

void KoTextParag::removeSelection( int id )
{
    if ( !hasSelection( id ) )
	return;
    if ( mSelections )
	mSelections->remove( id );
    setChanged( TRUE, TRUE );
}

int KoTextParag::selectionStart( int id ) const
{
    if ( !mSelections )
	return -1;
    QMap<int, KoTextParagSelection>::ConstIterator it = mSelections->find( id );
    if ( it == mSelections->end() )
	return -1;
    return ( *it ).start;
}

int KoTextParag::selectionEnd( int id ) const
{
    if ( !mSelections )
	return -1;
    QMap<int, KoTextParagSelection>::ConstIterator it = mSelections->find( id );
    if ( it == mSelections->end() )
	return -1;
    return ( *it ).end;
}

bool KoTextParag::hasSelection( int id ) const
{
    if ( !mSelections )
	return FALSE;
    QMap<int, KoTextParagSelection>::ConstIterator it = mSelections->find( id );
    if ( it == mSelections->end() )
	return FALSE;
    return ( *it ).start != ( *it ).end || length() == 1;
}

bool KoTextParag::fullSelected( int id ) const
{
    if ( !mSelections )
	return FALSE;
    QMap<int, KoTextParagSelection>::ConstIterator it = mSelections->find( id );
    if ( it == mSelections->end() )
	return FALSE;
    return ( *it ).start == 0 && ( *it ).end == str->length() - 1;
}

int KoTextParag::lineY( int l ) const
{
    if ( l > (int)lineStarts.count() - 1 ) {
	qWarning( "KoTextParag::lineY: line %d out of range!", l );
	return 0;
    }

    if ( !isValid() )
	( (KoTextParag*)this )->format();

    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.begin();
    while ( l-- > 0 )
	++it;
    return ( *it )->y;
}

int KoTextParag::lineBaseLine( int l ) const
{
    if ( l > (int)lineStarts.count() - 1 ) {
	qWarning( "KoTextParag::lineBaseLine: line %d out of range!", l );
	return 10;
    }

    if ( !isValid() )
	( (KoTextParag*)this )->format();

    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.begin();
    while ( l-- > 0 )
	++it;
    return ( *it )->baseLine;
}

int KoTextParag::lineHeight( int l ) const
{
    if ( l > (int)lineStarts.count() - 1 ) {
	qWarning( "KoTextParag::lineHeight: line %d out of range!", l );
	return 15;
    }

    if ( !isValid() )
	( (KoTextParag*)this )->format();

    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.begin();
    while ( l-- > 0 )
	++it;
    return ( *it )->h;
}

void KoTextParag::lineInfo( int l, int &y, int &h, int &bl ) const
{
    if ( l > (int)lineStarts.count() - 1 ) {
	qWarning( "KoTextParag::lineInfo: line %d out of range!", l );
	qDebug( "%d %d", (int)lineStarts.count() - 1, l );
	y = 0;
	h = 15;
	bl = 10;
	return;
    }

    if ( !isValid() )
	( (KoTextParag*)this )->format();

    QMap<int, KoTextParagLineStart*>::ConstIterator it = lineStarts.begin();
    while ( l-- > 0 )
	++it;
    y = ( *it )->y;
    h = ( *it )->h;
    bl = ( *it )->baseLine;
}

uint KoTextParag::alignment() const
{
    return align;
}

void KoTextParag::setFormat( KoTextFormat *fm )
{
#if 0
    bool doUpdate = FALSE;
    if (defFormat && (defFormat != formatCollection()->defaultFormat()))
       doUpdate = TRUE;
#endif
    defFormat = formatCollection()->format( fm );
#if 0
    if ( !doUpdate )
	return;
    for ( int i = 0; i < length(); ++i ) {
	if ( at( i )->format()->styleName() == defFormat->styleName() )
	    at( i )->format()->updateStyle();
    }
#endif
}

KoTextFormatterBase *KoTextParag::formatter() const
{
    if ( doc )
	return doc->formatter();
    //if ( pFormatter )
    //    return pFormatter;
    //return ( ( (KoTextParag*)this )->pFormatter = new KoTextFormatterBaseBreakWords );
    return 0L;
}

/*void KoTextParag::setFormatter( KoTextFormatterBase *f )
{
    if ( doc ) return;
    if ( pFormatter ) delete pFormatter;
    pFormatter = f;
}*/

int KoTextParag::minimumWidth() const
{
    return doc ? doc->minimumWidth() : 0;
}

void KoTextParag::setTabArray( int *a )
{
    delete [] tArray;
    tArray = a;
}

void KoTextParag::setTabStops( int tw )
{
    if ( doc )
	doc->setTabStops( tw );
    //else
    //    tabStopWidth = tw;
}

QMap<int, KoTextParagSelection> &KoTextParag::selections() const
{
    if ( !mSelections )
	((KoTextParag *)this)->mSelections = new QMap<int, KoTextParagSelection>;
    return *mSelections;
}

QPtrList<KoTextCustomItem> &KoTextParag::floatingItems() const
{
    if ( !mFloatingItems )
	((KoTextParag *)this)->mFloatingItems = new QPtrList<KoTextCustomItem>;
    return *mFloatingItems;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextFormatterBase::KoTextFormatterBase()
    : thisminw(0), thiswused(0), wrapEnabled( TRUE ), wrapColumn( -1 ), biw( TRUE /*default in kotext*/ )
{
}

// See KoTextFormatter
#if 0
KoTextParagLineStart *KoTextFormatterBase::formatLine( KoTextParag *parag, KoTextString *string, KoTextParagLineStart *line,
						   KoTextStringChar *startChar, KoTextStringChar *lastChar, int align, int space )
{
#ifndef QT_NO_COMPLEXTEXT
    if( string->isBidi() )
	return bidiReorderLine( parag, string, line, startChar, lastChar, align, space );
#endif
    space = QMAX( space, 0 ); // #### with nested tables this gets negative because of a bug I didn't find yet, so workaround for now. This also means non-left aligned nested tables do not work at the moment
    int start = (startChar - &string->at(0));
    int last = (lastChar - &string->at(0) );
    // do alignment Auto == Left in this case
    if ( align & Qt::AlignHCenter || align & Qt::AlignRight ) {
	if ( align & Qt::AlignHCenter )
	    space /= 2;
	for ( int j = start; j <= last; ++j )
	    string->at( j ).x += space;
    } else if ( align & AlignJustify ) {
	int numSpaces = 0;
	for ( int j = start; j < last; ++j ) {
	    if( isBreakable( string, j ) ) {
		numSpaces++;
	    }
	}
	int toAdd = 0;
	for ( int k = start + 1; k <= last; ++k ) {
	    if( isBreakable( string, k ) && numSpaces ) {
		int s = space / numSpaces;
		toAdd += s;
		space -= s;
		numSpaces--;
	    }
	    string->at( k ).x += toAdd;
	}
    }

    if ( last >= 0 && last < string->length() )
	line->w = string->at( last ).x + string->width( last );
    else
	line->w = 0;

    return new KoTextParagLineStart();
}
#endif

#ifdef BIDI_DEBUG
#include <iostream>
#endif

// collects one line of the paragraph and transforms it to visual order
KoTextParagLineStart *KoTextFormatterBase::bidiReorderLine( KoTextParag * /*parag*/, KoTextString *text, KoTextParagLineStart *line,
							KoTextStringChar *startChar, KoTextStringChar *lastChar, int align, int space )
{
    int start = (startChar - &text->at(0));
    int last = (lastChar - &text->at(0) );
    //qDebug("doing BiDi reordering from %d to %d!", start, last);

    KoBidiControl *control = new KoBidiControl( line->context(), line->status );
    QString str;
    str.setUnicode( 0, last - start + 1 );
    // fill string with logically ordered chars.
    KoTextStringChar *ch = startChar;
    QChar *qch = (QChar *)str.unicode();
    while ( ch <= lastChar ) {
	*qch = ch->c;
	qch++;
	ch++;
    }
    int x = startChar->x;

    QPtrList<KoTextRun> *runs;
    runs = KoComplexText::bidiReorderLine(control, str, 0, last - start + 1,
					 (text->isRightToLeft() ? QChar::DirR : QChar::DirL) );

    // now construct the reordered string out of the runs...

    int numSpaces = 0;
    // set the correct alignment. This is a bit messy....
    if( align == Qt::AlignAuto ) {
	// align according to directionality of the paragraph...
	if ( text->isRightToLeft() )
	    align = Qt::AlignRight;
    }

    if ( align & Qt::AlignHCenter )
	x += space/2;
    else if ( align & Qt::AlignRight )
	x += space;
    else if ( align & Qt::AlignJustify ) {
	for ( int j = start; j < last; ++j ) {
	    if( isBreakable( text, j ) ) {
		numSpaces++;
	    }
	}
    }
    int toAdd = 0;
    bool first = TRUE;
    KoTextRun *r = runs->first();
    int xmax = -0xffffff;
    while ( r ) {
	if(r->level %2) {
	    // odd level, need to reverse the string
	    int pos = r->stop + start;
	    while(pos >= r->start + start) {
		KoTextStringChar *c = &text->at(pos);
		if( numSpaces && !first && isBreakable( text, pos ) ) {
		    int s = space / numSpaces;
		    toAdd += s;
		    space -= s;
		    numSpaces--;
		} else if ( first ) {
		    first = FALSE;
		    if ( c->c == ' ' )
			x -= c->format()->width( ' ' );
		}
		c->x = x + toAdd;
		c->rightToLeft = TRUE;
		c->startOfRun = FALSE;
		int ww = 0;
		if ( c->c.unicode() >= 32 || c->c == '\t' || c->c == '\n' || c->isCustom() ) {
		    ww = c->width;
		} else {
		    ww = c->format()->width( ' ' );
		}
		if ( xmax < x + toAdd + ww ) xmax = x + toAdd + ww;
		x += ww;
		pos--;
	    }
	} else {
	    int pos = r->start + start;
	    while(pos <= r->stop + start) {
		KoTextStringChar* c = &text->at(pos);
		if( numSpaces && !first && isBreakable( text, pos ) ) {
		    int s = space / numSpaces;
		    toAdd += s;
		    space -= s;
		    numSpaces--;
		} else if ( first ) {
		    first = FALSE;
		    if ( c->c == ' ' )
			x -= c->format()->width( ' ' );
		}
		c->x = x + toAdd;
		c->rightToLeft = FALSE;
		c->startOfRun = FALSE;
		int ww = 0;
		if ( c->c.unicode() >= 32 || c->c == '\t' || c->isCustom() ) {
		    ww = c->width;
		} else {
		    ww = c->format()->width( ' ' );
		}
		//qDebug("setting char %d at pos %d", pos, x);
		if ( xmax < x + toAdd + ww ) xmax = x + toAdd + ww;
		x += ww;
		pos++;
	    }
	}
	text->at( r->start + start ).startOfRun = TRUE;
	r = runs->next();
    }

    line->w = xmax + 10;
    KoTextParagLineStart *ls = new KoTextParagLineStart( control->context, control->status );
    delete control;
    delete runs;
    return ls;
}

bool KoTextFormatterBase::isStretchable( KoTextString *string, int pos ) const
{
    if ( string->at( pos ).c == QChar(160) ) //non-breaking space
	return true;
    return isBreakable( string, pos );
}

bool KoTextFormatterBase::isBreakable( KoTextString *string, int pos ) const
{
    const QChar &c = string->at( pos ).c;
    char ch = c.latin1();
    if ( c.isSpace() && ch != '\n' && c.unicode() != 0x00a0U )
	return TRUE;
    if ( c == '-' || c.unicode() == 0xad ) // hyphen or soft hyphen
	return TRUE;
    if ( !ch ) {
	// not latin1, need to do more sophisticated checks for other scripts
	uchar row = c.row();
	if ( row == 0x0e ) {
	    // 0e00 - 0e7f == Thai
	    if ( c.cell() < 0x80 ) {
#ifdef HAVE_THAI_BREAKS
		// check for thai
		if( string != cachedString ) {
		    // build up string of thai chars
		    QTextCodec *thaiCodec = QTextCodec::codecForMib(2259);
		    if ( !thaiCache )
			thaiCache = new QCString;
		    if ( !thaiIt )
			thaiIt = ThBreakIterator::createWordInstance();
		    *thaiCache = thaiCodec->fromUnicode( s->string() );
		}
		thaiIt->setText(thaiCache->data());
		for(int i = thaiIt->first(); i != thaiIt->DONE; i = thaiIt->next() ) {
		    if( i == pos )
			return TRUE;
		    if( i > pos )
			return FALSE;
		}
		return FALSE;
#else
		// if we don't have a thai line breaking lib, allow
		// breaks everywhere except directly before punctuation.
		return TRUE;
#endif
	    } else
		return FALSE;
	}
	if ( row < 0x11 ) // no asian font
	    return FALSE;
	if ( row > 0x2d && row < 0xfb || row == 0x11 )
	    // asian line breaking. Everywhere allowed except directly
	    // in front of a punctuation character.
	    return TRUE;
    }
    return FALSE;
}

void KoTextFormatterBase::insertLineStart( KoTextParag *parag, int index, KoTextParagLineStart *ls )
{
    // This tests if we break at the same character in more than one line,
    // i.e. there no space even for _one_ char in a given line.
    // However this shouldn't happen, KoTextFormatter prevents it, otherwise
    // we could loop forever (e.g. if one char is wider than the page...)
#ifdef NDEBUG
    QMap<int, KoTextParagLineStart*>::Iterator it;
    if ( ( it = parag->lineStartList().find( index ) ) == parag->lineStartList().end() ) {
	parag->lineStartList().insert( index, ls );
    } else {
        kdWarning(32500) << "insertLineStart: there's already a line for char index=" << index << endl;
	delete *it;
	parag->lineStartList().remove( it );
	parag->lineStartList().insert( index, ls );
    }
#else // non-debug code, take the fast route
    parag->lineStartList().insert( index, ls );
#endif
}


/* Standard pagebreak algorithm using KoTextFlow::adjustFlow. Returns
 the shift of the paragraphs bottom line.
 */
int KoTextFormatterBase::formatVertically( KoTextDocument* doc, KoTextParag* parag )
{
    int oldHeight = parag->rect().height();
    QMap<int, KoTextParagLineStart*>& lineStarts = parag->lineStartList();
    QMap<int, KoTextParagLineStart*>::Iterator it = lineStarts.begin();
    int h = doc->addMargins() ? parag->topMargin() : 0;
    for ( ; it != lineStarts.end() ; ++it  ) {
	KoTextParagLineStart * ls = it.data();
	ls->y = h;
	KoTextStringChar *c = &parag->string()->at(it.key());
	if ( c && c->customItem() && c->customItem()->ownLine() ) {
	    int h = c->customItem()->height;
	    c->customItem()->pageBreak( parag->rect().y() + ls->y + ls->baseLine - h, doc->flow() );
	    int delta = c->customItem()->height - h;
	    ls->h += delta;
	    if ( delta )
		parag->setMovedDown( TRUE );
	} else {
	    int shift = doc->flow()->adjustFlow( parag->rect().y() + ls->y, ls->w, ls->h );
	    ls->y += shift;
	    if ( shift )
		parag->setMovedDown( TRUE );
	}
	h = ls->y + ls->h;
    }
    int m = parag->bottomMargin();
    if ( parag->next() && doc && !doc->addMargins() )
	m = QMAX( m, parag->next()->topMargin() );
    if ( parag->next() && parag->next()->isLineBreak() )
	m = 0;
    h += m;
    parag->setHeight( h );
    return h - oldHeight;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextIndent::KoTextIndent()
{
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

KoTextFormatCollection::KoTextFormatCollection()
    : cKey( 307 ), sheet( 0 )
{
#ifdef DEBUG_COLLECTION
    qDebug("KoTextFormatCollection::KoTextFormatCollection %p", this);
#endif
    defFormat = new KoTextFormat( QApplication::font(), QColor(), KGlobal::locale()->language(), false, 1.0 ); //// kotext: need to use default QColor here
    lastFormat = cres = 0;
    cflags = -1;
    cKey.setAutoDelete( TRUE );
    cachedFormat = 0;
}

KoTextFormatCollection::KoTextFormatCollection( const QFont& defaultFont, const QColor& defaultColor, const QString & defaultLanguage, bool hyphen, double ulw )
    : cKey( 307 ), sheet( 0 )
{
#ifdef DEBUG_COLLECTION
    qDebug("KoTextFormatCollection::KoTextFormatCollection %p", this);
#endif
    defFormat = new KoTextFormat( defaultFont, defaultColor, defaultLanguage, hyphen, ulw );
    lastFormat = cres = 0;
    cflags = -1;
    cKey.setAutoDelete( TRUE );
    cachedFormat = 0;
}

KoTextFormatCollection::~KoTextFormatCollection()
{
#ifdef DEBUG_COLLECTION
    qDebug("KoTextFormatCollection::~KoTextFormatCollection %p", this);
#endif
    delete defFormat;
    defFormat = 0;
}

KoTextFormat *KoTextFormatCollection::format( const KoTextFormat *f )
{
    if ( f->parent() == this || f == defFormat ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format(f) need '%s', best case!", f->key().latin1() );
#endif
	lastFormat = const_cast<KoTextFormat*>(f);
	lastFormat->addRef();
	return lastFormat;
    }

    if ( f == lastFormat || ( lastFormat && f->key() == lastFormat->key() ) ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format(f) need '%s', good case!", f->key().latin1() );
#endif
	lastFormat->addRef();
	return lastFormat;
    }

#if 0 // #### disabled, because if this format is not in the
 // formatcollection, it doesn't get the painter through
 // KoTextFormatCollection::setPainter() which breaks printing on
 // windows
    if ( f->isAnchor() ) {
	lastFormat = createFormat( *f );
	lastFormat->collection = 0;
	return lastFormat;
    }
#endif

    KoTextFormat *fm = cKey.find( f->key() );
    if ( fm ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format(f) need '%s', normal case!", f->key().latin1() );
#endif
	lastFormat = fm;
	lastFormat->addRef();
	return lastFormat;
    }

    if ( f->key() == defFormat->key() )
	return defFormat;

#ifdef DEBUG_COLLECTION
    qDebug( " format(f) need '%s', worst case!", f->key().latin1() );
#endif
    lastFormat = createFormat( *f );
    lastFormat->collection = this;
    cKey.insert( lastFormat->key(), lastFormat );
    Q_ASSERT( f->key() == lastFormat->key() );
    return lastFormat;
}

KoTextFormat *KoTextFormatCollection::format( KoTextFormat *of, KoTextFormat *nf, int flags )
{
    if ( cres && kof == of->key() && knf == nf->key() && cflags == flags ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format(of,nf,flags) mix of '%s' and '%s, best case!", of->key().latin1(), nf->key().latin1() );
#endif
	cres->addRef();
	return cres;
    }

#ifdef DEBUG_COLLECTION
    qDebug(" format(of,nf,%d) calling createFormat(of=%p %s)",flags,of,of->key().latin1());
#endif
    cres = createFormat( *of );
    kof = of->key();
    knf = nf->key();
    cflags = flags;

#ifdef DEBUG_COLLECTION
    qDebug(" format(of,nf,%d) calling copyFormat(nf=%p %s)",flags,nf,nf->key().latin1());
#endif
    cres->copyFormat( *nf, flags );

    KoTextFormat *fm = cKey.find( cres->key() );
    if ( !fm ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format(of,nf,flags) mix of '%s' and '%s, worst case!", of->key().latin1(), nf->key().latin1() );
#endif
	cres->collection = this;
	cKey.insert( cres->key(), cres );
    } else {
#ifdef DEBUG_COLLECTION
	qDebug( " format(of,nf,flags) mix of '%s' and '%s, good case!", of->key().latin1(), nf->key().latin1() );
#endif
	delete cres;
	cres = fm;
	cres->addRef();
    }

    return cres;
}

KoTextFormat *KoTextFormatCollection::format( const QFont &f, const QColor &c, const QString & language, bool hyphen, double ulw )
{
    if ( cachedFormat && cfont == f && ccol == c ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format of font and col '%s' - best case", cachedFormat->key().latin1() );
#endif
	cachedFormat->addRef();
	return cachedFormat;
    }

    QString key = KoTextFormat::getKey( f, c, FALSE, KoTextFormat::AlignNormal );
    cachedFormat = cKey.find( key );
    cfont = f;
    ccol = c;

    if ( cachedFormat ) {
#ifdef DEBUG_COLLECTION
	qDebug( " format of font and col '%s' - good case", cachedFormat->key().latin1() );
#endif
	cachedFormat->addRef();
	return cachedFormat;
    }

    if ( key == defFormat->key() )
	return defFormat;

    cachedFormat = createFormat( f, c,language, hyphen, ulw );
    cachedFormat->collection = this;
    cKey.insert( cachedFormat->key(), cachedFormat );
    if ( cachedFormat->key() != key )
	qWarning("ASSERT: keys for format not identical: '%s '%s'", cachedFormat->key().latin1(), key.latin1() );
#ifdef DEBUG_COLLECTION
    qDebug( " format of font and col '%s' - worst case", cachedFormat->key().latin1() );
#endif
    return cachedFormat;
}

void KoTextFormatCollection::remove( KoTextFormat *f )
{
    if ( lastFormat == f )
	lastFormat = 0;
    if ( cres == f )
	cres = 0;
    if ( cachedFormat == f )
	cachedFormat = 0;
    cKey.remove( f->key() );
}

#if 0
void KoTextFormatCollection::setPainter( QPainter *p )
{
    QDictIterator<KoTextFormat> it( cKey );
    KoTextFormat *f;
    while ( ( f = it.current() ) ) {
	++it;
	f->setPainter( p );
    }
}
#endif

#ifndef NDEBUG
void KoTextFormatCollection::debug()
{
    qDebug( "------------ KoTextFormatCollection: debug --------------- BEGIN" );
    qDebug( "Default Format: '%s' (%p): realfont: %s",
            defFormat->key().latin1(), (void*)defFormat, QFontInfo( defFormat->font() ).family().latin1() );
    QDictIterator<KoTextFormat> it( cKey );
    for ( ; it.current(); ++it ) {
         Q_ASSERT(it.currentKey() == it.current()->key());
         if(it.currentKey() != it.current()->key())
             qDebug("**** MISMATCH key=%s (see line below for format)", it.currentKey().latin1());
	 it.current()->printDebug();
    }
    qDebug( "------------ KoTextFormatCollection: debug --------------- END" );
}
#endif

#if 0
void KoTextFormatCollection::updateStyles()
{
    QDictIterator<KoTextFormat> it( cKey );
    KoTextFormat *f;
    while ( ( f = it.current() ) ) {
	++it;
	f->updateStyle();
    }
}

void KoTextFormatCollection::updateFontSizes( int base )
{
    QDictIterator<KoTextFormat> it( cKey );
    KoTextFormat *f;
    while ( ( f = it.current() ) ) {
	++it;
	f->stdPointSize = base;
	f->fn.setPointSize( f->stdPointSize );
	styleSheet()->scaleFont( f->fn, f->logicalFontSize );
	f->update();
    }
    f = defFormat;
    f->stdPointSize = base;
    f->fn.setPointSize( f->stdPointSize );
    styleSheet()->scaleFont( f->fn, f->logicalFontSize );
    f->update();
}

void KoTextFormatCollection::updateFontAttributes( const QFont &f, const QFont &old )
{
    QDictIterator<KoTextFormat> it( cKey );
    KoTextFormat *fm;
    while ( ( fm = it.current() ) ) {
	++it;
	if ( fm->fn.family() == old.family() &&
	     fm->fn.weight() == old.weight() &&
	     fm->fn.italic() == old.italic() &&
	     fm->fn.underline() == old.underline() ) {
	    fm->fn.setFamily( f.family() );
	    fm->fn.setWeight( f.weight() );
	    fm->fn.setItalic( f.italic() );
	    fm->fn.setUnderline( f.underline() );
	    fm->update();
	}
    }
    fm = defFormat;
    if ( fm->fn.family() == old.family() &&
	 fm->fn.weight() == old.weight() &&
	 fm->fn.italic() == old.italic() &&
	 fm->fn.underline() == old.underline() ) {
	fm->fn.setFamily( f.family() );
	fm->fn.setWeight( f.weight() );
	fm->fn.setItalic( f.italic() );
	fm->fn.setUnderline( f.underline() );
	fm->update();
    }
}
#endif

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if 0
void KoTextFormat::setPainter( QPainter *p )
{
    painter = p;
    update();
}

static int makeLogicFontSize( int s )
{
    int defSize = QApplication::font().pointSize();
    if ( s < defSize - 4 )
	return 1;
    if ( s < defSize )
	return 2;
    if ( s < defSize + 4 )
	return 3;
    if ( s < defSize + 8 )
	return 4;
    if ( s < defSize + 12 )
	return 5;
    if (s < defSize + 16 )
	return 6;
    return 7;
}

QString KoTextFormat::makeFormatChangeTags( KoTextFormat *f, const QString& oldAnchorHref, const QString& anchorHref  ) const
{
    if ( !defaultFormat ) // #### wrong, use the document's default format instead
        defaultFormat = new KoTextFormat( QApplication::font(), QColor(), KGlobal::locale()->language() );
    QString tag;
    if ( f ) {
        if ( f->font() != defaultFormat->font() ) {
            if ( f->font().family() != defaultFormat->font().family()
                 || f->pointSize() != defaultFormat->pointSize()
                 || f->color().rgb() != defaultFormat->color().rgb() )
                tag += "</font>";
            if ( f->font().underline() && f->font().underline() != defaultFormat->font().underline() )
                tag += "</u>";
            if ( f->font().italic() && f->font().italic() != defaultFormat->font().italic() )
                tag += "</i>";
            if ( f->font().bold() && f->font().bold() != defaultFormat->font().bold() )
                tag += "</b>";
        }
        if ( !oldAnchorHref.isEmpty() )
            tag += "</a>";
    }

    if ( !anchorHref.isEmpty() )
        tag += "<a href=\"" + anchorHref + "\">";

    if ( font() != defaultFormat->font() ) {
        if ( font().bold() && font().bold() != defaultFormat->font().bold() )
            tag += "<b>";
        if ( font().italic() && font().italic() != defaultFormat->font().italic() )
            tag += "<i>";
        if ( font().underline() && font().underline() != defaultFormat->font().underline() )
            tag += "<u>";
    }
    if ( font() != defaultFormat->font()
         || color().rgb() != defaultFormat->color().rgb() ) {
        QString f;
        if ( font().family() != defaultFormat->font().family() )
            f +=" face=\"" + fn.family() + "\"";
        if ( pointSize() != defaultFormat->pointSize() ) {
            f +=" size=\"" + QString::number( makeLogicFontSize( fn.pointSize() ) ) + "\"";
            f +=" style=\"font-size:" + QString::number( fn.pointSize() ) + "pt\"";
        }
        if ( color().rgb() != defaultFormat->color().rgb() )
            f +=" color=\"" + col.name() + "\"";
        if ( !f.isEmpty() )
            tag += "<font" + f + ">";
    }

    return tag;
}

QString KoTextFormat::makeFormatEndTags( const QString& anchorHref ) const
{
    if ( !defaultFormat )
        defaultFormat = new KoTextFormat( QApplication::font(), QColor(),KGlobal::locale()->language() );
    QString tag;
    if ( font() != defaultFormat->font() ) {
        if ( font().family() != defaultFormat->font().family()
             || pointSize() != defaultFormat->pointSize()
             || color().rgb() != defaultFormat->color().rgb() )
            tag += "</font>";
        if ( font().underline() && font().underline() != defaultFormat->font().underline() )
            tag += "</u>";
        if ( font().italic() && font().italic() != defaultFormat->font().italic() )
            tag += "</i>";
        if ( font().bold() && font().bold() != defaultFormat->font().bold() )
            tag += "</b>";
    }
    if ( !anchorHref.isEmpty() )
        tag += "</a>";
    return tag;
}

KoTextFormat KoTextFormat::makeTextFormat( const QStyleSheetItem *style, const QMap<QString,QString>& attr ) const
{
    KoTextFormat format(*this);
    if ( style ) {
        format.style = style->name();
        if ( style->name() == "font") {
            if ( attr.contains("color") ) {
                QString s = attr["color"];
                if ( !s.isEmpty() ) {
                    format.col.setNamedColor( s );
                    format.linkColor = FALSE;
                }
            }
            if ( attr.contains("size") ) {
                QString a = attr["size"];
                int n = a.toInt();
                if ( a[0] == '+' || a[0] == '-' )
                    n += format.logicalFontSize;
                format.logicalFontSize = n;
                format.fn.setPointSize( format.stdPointSize );
                style->styleSheet()->scaleFont( format.fn, format.logicalFontSize );
            }
            if ( attr.contains("style" ) ) {
                QString a = attr["style"];
                if ( a.startsWith( "font-size:" ) ) {
                    QString s = a.mid( a.find( ':' ) + 1 );
                    int n = s.left( s.length() - 2 ).toInt();
                    format.logicalFontSize = 0;
                    format.fn.setPointSize( n );
                }
            }
            if ( attr.contains("face") ) {
                QString a = attr["face"];
                if ( a.contains(',') )
                    a = a.left( a.find(',') );
                format.fn.setFamily( a );
            }
        } else {

            switch ( style->verticalAlignment() ) {
            case QStyleSheetItem::VAlignBaseline:
                format.setVAlign( KoTextFormat::AlignNormal );
                break;
            case QStyleSheetItem::VAlignSuper:
                format.setVAlign( KoTextFormat::AlignSuperScript );
                break;
            case QStyleSheetItem::VAlignSub:
                format.setVAlign( KoTextFormat::AlignSubScript );
                break;
            }

            if ( style->fontWeight() != QStyleSheetItem::Undefined )
                format.fn.setWeight( style->fontWeight() );
            if ( style->fontSize() != QStyleSheetItem::Undefined ) {
                format.fn.setPointSize( style->fontSize() );
            } else if ( style->logicalFontSize() != QStyleSheetItem::Undefined ) {
                format.logicalFontSize = style->logicalFontSize();
                format.fn.setPointSize( format.stdPointSize );
                style->styleSheet()->scaleFont( format.fn, format.logicalFontSize );
            } else if ( style->logicalFontSizeStep() ) {
                format.logicalFontSize += style->logicalFontSizeStep();
                format.fn.setPointSize( format.stdPointSize );
                style->styleSheet()->scaleFont( format.fn, format.logicalFontSize );
            }
            if ( !style->fontFamily().isEmpty() )
                format.fn.setFamily( style->fontFamily() );
            if ( style->color().isValid() )
                format.col = style->color();
            if ( style->definesFontItalic() )
                format.fn.setItalic( style->fontItalic() );
            if ( style->definesFontUnderline() )
                format.fn.setUnderline( style->fontUnderline() );
        }
    }
    format.update();
    return format;
}
#endif

KoTextCustomItem::KoTextCustomItem( KoTextDocument *p )
      :  width(-1), height(0), parent(p), xpos(0), ypos(-1), parag(0)
{
    m_deleted = false; // added for kotext
}

KoTextCustomItem::~KoTextCustomItem()
{
}

struct QPixmapInt
{
    QPixmapInt() : ref( 0 ) {}
    QPixmap pm;
    int	    ref;
};

static QMap<QString, QPixmapInt> *pixmap_map = 0;

KoTextImage::KoTextImage( KoTextDocument *p, const QMap<QString, QString> &attr, const QString& context,
			QMimeSourceFactory &factory )
    : KoTextCustomItem( p )
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new KoTextImage (pappi: %p)", p );
#endif

    width = height = 0;
    if ( attr.contains("width") )
	width = attr["width"].toInt();
    if ( attr.contains("height") )
	height = attr["height"].toInt();

    reg = 0;
    QString imageName = attr["src"];

    if (!imageName)
	imageName = attr["source"];

#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "    .." + imageName );
#endif

    if ( !imageName.isEmpty() ) {
	imgId = QString( "%1,%2,%3,%4" ).arg( imageName ).arg( width ).arg( height ).arg( (ulong)&factory );
	if ( !pixmap_map )
	    pixmap_map = new QMap<QString, QPixmapInt>;
	if ( pixmap_map->contains( imgId ) ) {
	    QPixmapInt& pmi = pixmap_map->operator[](imgId);
	    pm = pmi.pm;
	    pmi.ref++;
	    width = pm.width();
	    height = pm.height();
	} else {
	    QImage img;
	    const QMimeSource* m =
		factory.data( imageName, context );
	    if ( !m ) {
		qWarning("KoTextImage: no mimesource for %s", imageName.latin1() );
	    }
	    else {
		if ( !QImageDrag::decode( m, img ) ) {
		    qWarning("KoTextImage: cannot decode %s", imageName.latin1() );
		}
	    }

	    if ( !img.isNull() ) {
		if ( width == 0 ) {
		    width = img.width();
		    if ( height != 0 ) {
			width = img.width() * height / img.height();
		    }
		}
		if ( height == 0 ) {
		    height = img.height();
		    if ( width != img.width() ) {
			height = img.height() * width / img.width();
		    }
		}
		if ( img.width() != width || img.height() != height ){
#ifndef QT_NO_IMAGE_SMOOTHSCALE
		    img = img.smoothScale(width, height);
#endif
		    width = img.width();
		    height = img.height();
		}
		pm.convertFromImage( img );
	    }
	    if ( !pm.isNull() ) {
		QPixmapInt& pmi = pixmap_map->operator[](imgId);
		pmi.pm = pm;
		pmi.ref++;
	    }
	}
	if ( pm.mask() ) {
	    QRegion mask( *pm.mask() );
	    QRegion all( 0, 0, pm.width(), pm.height() );
	    reg = new QRegion( all.subtract( mask ) );
	}
    }

    if ( pm.isNull() && (width*height)==0 )
	width = height = 50;

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;

    tmpwidth = width;
    tmpheight = height;

    attributes = attr;
}

KoTextImage::~KoTextImage()
{
    if ( pixmap_map && pixmap_map->contains( imgId ) ) {
	QPixmapInt& pmi = pixmap_map->operator[](imgId);
	pmi.ref--;
	if ( !pmi.ref ) {
	    pixmap_map->remove( imgId );
	    if ( pixmap_map->isEmpty() ) {
		delete pixmap_map;
		pixmap_map = 0;
	    }
	}
    }
}

QString KoTextImage::richText() const
{
    QString s;
    s += "<img ";
    QMap<QString, QString>::ConstIterator it = attributes.begin();
    for ( ; it != attributes.end(); ++it )
	s += it.key() + "=" + *it + " ";
    s += ">";
    return s;
}

/*void KoTextImage::setPainter( QPainter* p, bool adjust  )
{
    if ( adjust ) {
	width = scale( tmpwidth, p );
	height = scale( tmpheight, p );
    }
}*/

#if !defined(Q_WS_X11)
#include <qbitmap.h>
#include <qcleanuphandler.h>
static QPixmap *qrt_selection = 0;
static QCleanupHandler<QPixmap> qrt_cleanup_pixmap;
static void qrt_createSelectionPixmap( const QColorGroup &cg )
{
    qrt_selection = new QPixmap( 2, 2 );
    qrt_cleanup_pixmap.add( &qrt_selection );
    qrt_selection->fill( Qt::color0 );
    QBitmap m( 2, 2 );
    m.fill( Qt::color1 );
    QPainter p( &m );
    p.setPen( Qt::color0 );
    for ( int j = 0; j < 2; ++j ) {
	p.drawPoint( j % 2, j );
    }
    p.end();
    qrt_selection->setMask( m );
    qrt_selection->fill( cg.highlight() );
}
#endif

void KoTextImage::draw( QPainter* p, int x, int y, int cx, int cy, int cw, int ch, const QColorGroup& cg, bool selected )
{
    if ( placement() != PlaceInline ) {
	x = xpos;
	y = ypos;
    }

    if ( pm.isNull() ) {
	p->fillRect( x , y, width, height,  cg.dark() );
	return;
    }

    if ( is_printer( p ) ) {
	p->drawPixmap( QRect( x, y, width, height ), pm );
	return;
    }

    if ( placement() != PlaceInline && !QRect( xpos, ypos, width, height ).intersects( QRect( cx, cy, cw, ch ) ) )
	return;

    if ( placement() == PlaceInline )
	p->drawPixmap( x , y, pm );
    else
	p->drawPixmap( cx , cy, pm, cx - x, cy - y, cw, ch );

    if ( selected && placement() == PlaceInline && p->device()->devType() != QInternal::Printer ) {
#if defined(Q_WS_X11)
	p->fillRect( QRect( QPoint( x, y ), pm.size() ), QBrush( cg.highlight(), QBrush::Dense4Pattern) );
#else // in WIN32 Dense4Pattern doesn't work correctly (transparency problem), so work around it
	if ( !qrt_selection )
	    qrt_createSelectionPixmap( cg );
	p->drawTiledPixmap( x, y, pm.width(), pm.height(), *qrt_selection );
#endif
    }
}

/*void KoTextHorizontalLine::setPainter( QPainter* p, bool adjust  )
{
    if ( adjust )
	height = scale( tmpheight, p );
}*/


KoTextHorizontalLine::KoTextHorizontalLine( KoTextDocument *p, const QMap<QString, QString> &attr,
					  const QString &,
					  QMimeSourceFactory & )
    : KoTextCustomItem( p )
{
    height = tmpheight = 8;
    if ( attr.find( "color" ) != attr.end() )
	color = QColor( *attr.find( "color" ) );
    else if ( attr.find( "COLOR" ) != attr.end() )
	color = QColor( *attr.find( "COLOR" ) );
}

KoTextHorizontalLine::~KoTextHorizontalLine()
{
}

QString KoTextHorizontalLine::richText() const
{
    return "<hr>";
}

void KoTextHorizontalLine::draw( QPainter* p, int x, int y, int , int , int , int , const QColorGroup& cg, bool selected )
{
    QRect r( x, y, width, height);
    if ( is_printer( p ) ) {
	QPen oldPen = p->pen();
	if ( !color.isValid() )
	    p->setPen( QPen( cg.text(), height/8 ) );
	else
	    p->setPen( QPen( color, height/8 ) );
	p->drawLine( r.left()-1, y + height / 2, r.right() + 1, y + height / 2 );
	p->setPen( oldPen );
    } else {
	QColorGroup g( cg );
	if ( color.isValid() )
	    g.setColor( QColorGroup::Dark, color );
	if ( selected )
	    p->fillRect( r.left(), y, r.right(), y + height, g.highlight() );
	qDrawShadeLine( p, r.left() - 1, y + height / 2, r.right() + 1, y + height / 2, g, TRUE, height / 8 );
    }
}


/*****************************************************************/
// Small set of utility functions to make the parser a bit simpler
//

bool KoTextDocument::hasPrefix(const QString& doc, int pos, QChar c)
{
    if ( pos >= (int)doc.length() )
	return FALSE;
    return ( doc.unicode() )[ pos ].lower() == c.lower();
}

bool KoTextDocument::hasPrefix( const QString& doc, int pos, const QString& s )
{
    if ( pos + s.length() >= doc.length() )
	return FALSE;
    for ( int i = 0; i < (int)s.length(); i++ ) {
	if ( ( doc.unicode() )[ pos + i ].lower() != s[ i ].lower() )
	    return FALSE;
    }
    return TRUE;
}

#ifdef QTEXTTABLE_AVAILABLE
static bool qt_is_cell_in_use( QPtrList<KoTextTableCell>& cells, int row, int col )
{
    for ( KoTextTableCell* c = cells.first(); c; c = cells.next() ) {
	if ( row >= c->row() && row < c->row() + c->rowspan()
	     && col >= c->column() && col < c->column() + c->colspan() )
	    return TRUE;
    }
    return FALSE;
}

KoTextCustomItem* KoTextDocument::parseTable( const QMap<QString, QString> &attr, const KoTextFormat &fmt,
					    const QString &doc, int& pos, KoTextParag *curpar )
{

    KoTextTable* table = new KoTextTable( this, attr );
    int row = -1;
    int col = -1;

    QString rowbgcolor;
    QString rowalign;
    QString tablebgcolor = attr["bgcolor"];

    QPtrList<KoTextTableCell> multicells;

    QString tagname;
    (void) eatSpace(doc, pos);
    while ( pos < int(doc.length() )) {
	if (hasPrefix(doc, pos, QChar('<')) ){
	    if (hasPrefix(doc, pos+1, QChar('/'))) {
		tagname = parseCloseTag( doc, pos );
		if ( tagname == "table" ) {
#if defined(PARSER_DEBUG)
		    debug_indent.remove( debug_indent.length() - 3, 2 );
#endif
		    return table;
		}
	    } else {
		QMap<QString, QString> attr2;
		bool emptyTag = FALSE;
		tagname = parseOpenTag( doc, pos, attr2, emptyTag );
		if ( tagname == "tr" ) {
		    rowbgcolor = attr2["bgcolor"];
		    rowalign = attr2["align"];
		    row++;
		    col = -1;
		}
		else if ( tagname == "td" || tagname == "th" ) {
		    col++;
		    while ( qt_is_cell_in_use( multicells, row, col ) ) {
			col++;
		    }

		    if ( row >= 0 && col >= 0 ) {
			const QStyleSheetItem* s = sheet_->item(tagname);
			if ( !attr2.contains("bgcolor") ) {
			    if (!rowbgcolor.isEmpty() )
				attr2["bgcolor"] = rowbgcolor;
			    else if (!tablebgcolor.isEmpty() )
				attr2["bgcolor"] = tablebgcolor;
			}
			if ( !attr2.contains("align") ) {
			    if (!rowalign.isEmpty() )
				attr2["align"] = rowalign;
			}

			// extract the cell contents
			int end = pos;
			while ( end < (int) doc.length()
				&& !hasPrefix( doc, end, "</td")
				&& !hasPrefix( doc, end, "<td")
				&& !hasPrefix( doc, end, "</th")
				&& !hasPrefix( doc, end, "<th")
				&& !hasPrefix( doc, end, "<td")
				&& !hasPrefix( doc, end, "</tr")
				&& !hasPrefix( doc, end, "<tr")
				&& !hasPrefix( doc, end, "</table") ) {
			    if ( hasPrefix( doc, end, "<table" ) ) { // nested table
				int nested = 1;
				++end;
				while ( end < (int)doc.length() && nested != 0 ) {
				    if ( hasPrefix( doc, end, "</table" ) )
					nested--;
				    if ( hasPrefix( doc, end, "<table" ) )
					nested++;
				    end++;
				}
			    }
			    end++;
			}
			KoTextTableCell* cell  = new KoTextTableCell( table, row, col,
								    attr2, s, fmt.makeTextFormat( s, attr2 ),
								    contxt, *factory_, sheet_, doc.mid( pos, end - pos ) );
			cell->richText()->parParag = curpar;
			if ( cell->colspan() > 1 || cell->rowspan() > 1 )
			    multicells.append( cell );
			col += cell->colspan()-1;
			pos = end;
		    }
		}
	    }

	} else {
	    ++pos;
	}
    }
#if defined(PARSER_DEBUG)
    debug_indent.remove( debug_indent.length() - 3, 2 );
#endif
    return table;
}
#endif

bool KoTextDocument::eatSpace(const QString& doc, int& pos, bool includeNbsp )
{
    int old_pos = pos;
    while (pos < int(doc.length()) && (doc.unicode())[pos].isSpace() && ( includeNbsp || (doc.unicode())[pos] != QChar::nbsp ) )
	pos++;
    return old_pos < pos;
}

bool KoTextDocument::eat(const QString& doc, int& pos, QChar c)
{
    bool ok = pos < int(doc.length()) && ((doc.unicode())[pos] == c);
    if ( ok )
	pos++;
    return ok;
}
/*****************************************************************/

struct Entity {
    const char * name;
    Q_UINT16 code;
};

static const Entity entitylist [] = {
    { "AElig", 0x00c6 },
    { "Aacute", 0x00c1 },
    { "Acirc", 0x00c2 },
    { "Agrave", 0x00c0 },
    { "Alpha", 0x0391 },
    { "AMP", 38 },
    { "Aring", 0x00c5 },
    { "Atilde", 0x00c3 },
    { "Auml", 0x00c4 },
    { "Beta", 0x0392 },
    { "Ccedil", 0x00c7 },
    { "Chi", 0x03a7 },
    { "Dagger", 0x2021 },
    { "Delta", 0x0394 },
    { "ETH", 0x00d0 },
    { "Eacute", 0x00c9 },
    { "Ecirc", 0x00ca },
    { "Egrave", 0x00c8 },
    { "Epsilon", 0x0395 },
    { "Eta", 0x0397 },
    { "Euml", 0x00cb },
    { "Gamma", 0x0393 },
    { "GT", 62 },
    { "Iacute", 0x00cd },
    { "Icirc", 0x00ce },
    { "Igrave", 0x00cc },
    { "Iota", 0x0399 },
    { "Iuml", 0x00cf },
    { "Kappa", 0x039a },
    { "Lambda", 0x039b },
    { "LT", 60 },
    { "Mu", 0x039c },
    { "Ntilde", 0x00d1 },
    { "Nu", 0x039d },
    { "OElig", 0x0152 },
    { "Oacute", 0x00d3 },
    { "Ocirc", 0x00d4 },
    { "Ograve", 0x00d2 },
    { "Omega", 0x03a9 },
    { "Omicron", 0x039f },
    { "Oslash", 0x00d8 },
    { "Otilde", 0x00d5 },
    { "Ouml", 0x00d6 },
    { "Phi", 0x03a6 },
    { "Pi", 0x03a0 },
    { "Prime", 0x2033 },
    { "Psi", 0x03a8 },
    { "QUOT", 34 },
    { "Rho", 0x03a1 },
    { "Scaron", 0x0160 },
    { "Sigma", 0x03a3 },
    { "THORN", 0x00de },
    { "Tau", 0x03a4 },
    { "Theta", 0x0398 },
    { "Uacute", 0x00da },
    { "Ucirc", 0x00db },
    { "Ugrave", 0x00d9 },
    { "Upsilon", 0x03a5 },
    { "Uuml", 0x00dc },
    { "Xi", 0x039e },
    { "Yacute", 0x00dd },
    { "Yuml", 0x0178 },
    { "Zeta", 0x0396 },
    { "aacute", 0x00e1 },
    { "acirc", 0x00e2 },
    { "acute", 0x00b4 },
    { "aelig", 0x00e6 },
    { "agrave", 0x00e0 },
    { "alefsym", 0x2135 },
    { "alpha", 0x03b1 },
    { "amp", 38 },
    { "and", 0x22a5 },
    { "ang", 0x2220 },
    { "apos", 0x0027 },
    { "aring", 0x00e5 },
    { "asymp", 0x2248 },
    { "atilde", 0x00e3 },
    { "auml", 0x00e4 },
    { "bdquo", 0x201e },
    { "beta", 0x03b2 },
    { "brvbar", 0x00a6 },
    { "bull", 0x2022 },
    { "cap", 0x2229 },
    { "ccedil", 0x00e7 },
    { "cedil", 0x00b8 },
    { "cent", 0x00a2 },
    { "chi", 0x03c7 },
    { "circ", 0x02c6 },
    { "clubs", 0x2663 },
    { "cong", 0x2245 },
    { "copy", 0x00a9 },
    { "crarr", 0x21b5 },
    { "cup", 0x222a },
    { "curren", 0x00a4 },
    { "dArr", 0x21d3 },
    { "dagger", 0x2020 },
    { "darr", 0x2193 },
    { "deg", 0x00b0 },
    { "delta", 0x03b4 },
    { "diams", 0x2666 },
    { "divide", 0x00f7 },
    { "eacute", 0x00e9 },
    { "ecirc", 0x00ea },
    { "egrave", 0x00e8 },
    { "empty", 0x2205 },
    { "emsp", 0x2003 },
    { "ensp", 0x2002 },
    { "epsilon", 0x03b5 },
    { "equiv", 0x2261 },
    { "eta", 0x03b7 },
    { "eth", 0x00f0 },
    { "euml", 0x00eb },
    { "euro", 0x20ac },
    { "exist", 0x2203 },
    { "fnof", 0x0192 },
    { "forall", 0x2200 },
    { "frac12", 0x00bd },
    { "frac14", 0x00bc },
    { "frac34", 0x00be },
    { "frasl", 0x2044 },
    { "gamma", 0x03b3 },
    { "ge", 0x2265 },
    { "gt", 62 },
    { "hArr", 0x21d4 },
    { "harr", 0x2194 },
    { "hearts", 0x2665 },
    { "hellip", 0x2026 },
    { "iacute", 0x00ed },
    { "icirc", 0x00ee },
    { "iexcl", 0x00a1 },
    { "igrave", 0x00ec },
    { "image", 0x2111 },
    { "infin", 0x221e },
    { "int", 0x222b },
    { "iota", 0x03b9 },
    { "iquest", 0x00bf },
    { "isin", 0x2208 },
    { "iuml", 0x00ef },
    { "kappa", 0x03ba },
    { "lArr", 0x21d0 },
    { "lambda", 0x03bb },
    { "lang", 0x2329 },
    { "laquo", 0x00ab },
    { "larr", 0x2190 },
    { "lceil", 0x2308 },
    { "ldquo", 0x201c },
    { "le", 0x2264 },
    { "lfloor", 0x230a },
    { "lowast", 0x2217 },
    { "loz", 0x25ca },
    { "lrm", 0x200e },
    { "lsaquo", 0x2039 },
    { "lsquo", 0x2018 },
    { "lt", 60 },
    { "macr", 0x00af },
    { "mdash", 0x2014 },
    { "micro", 0x00b5 },
    { "middot", 0x00b7 },
    { "minus", 0x2212 },
    { "mu", 0x03bc },
    { "nabla", 0x2207 },
    { "nbsp", 0x00a0 },
    { "ndash", 0x2013 },
    { "ne", 0x2260 },
    { "ni", 0x220b },
    { "not", 0x00ac },
    { "notin", 0x2209 },
    { "nsub", 0x2284 },
    { "ntilde", 0x00f1 },
    { "nu", 0x03bd },
    { "oacute", 0x00f3 },
    { "ocirc", 0x00f4 },
    { "oelig", 0x0153 },
    { "ograve", 0x00f2 },
    { "oline", 0x203e },
    { "omega", 0x03c9 },
    { "omicron", 0x03bf },
    { "oplus", 0x2295 },
    { "or", 0x22a6 },
    { "ordf", 0x00aa },
    { "ordm", 0x00ba },
    { "oslash", 0x00f8 },
    { "otilde", 0x00f5 },
    { "otimes", 0x2297 },
    { "ouml", 0x00f6 },
    { "para", 0x00b6 },
    { "part", 0x2202 },
    { "percnt", 0x0025 },
    { "permil", 0x2030 },
    { "perp", 0x22a5 },
    { "phi", 0x03c6 },
    { "pi", 0x03c0 },
    { "piv", 0x03d6 },
    { "plusmn", 0x00b1 },
    { "pound", 0x00a3 },
    { "prime", 0x2032 },
    { "prod", 0x220f },
    { "prop", 0x221d },
    { "psi", 0x03c8 },
    { "quot", 34 },
    { "rArr", 0x21d2 },
    { "radic", 0x221a },
    { "rang", 0x232a },
    { "raquo", 0x00bb },
    { "rarr", 0x2192 },
    { "rceil", 0x2309 },
    { "rdquo", 0x201d },
    { "real", 0x211c },
    { "reg", 0x00ae },
    { "rfloor", 0x230b },
    { "rho", 0x03c1 },
    { "rlm", 0x200f },
    { "rsaquo", 0x203a },
    { "rsquo", 0x2019 },
    { "sbquo", 0x201a },
    { "scaron", 0x0161 },
    { "sdot", 0x22c5 },
    { "sect", 0x00a7 },
    { "shy", 0x00ad },
    { "sigma", 0x03c3 },
    { "sigmaf", 0x03c2 },
    { "sim", 0x223c },
    { "spades", 0x2660 },
    { "sub", 0x2282 },
    { "sube", 0x2286 },
    { "sum", 0x2211 },
    { "sup1", 0x00b9 },
    { "sup2", 0x00b2 },
    { "sup3", 0x00b3 },
    { "sup", 0x2283 },
    { "supe", 0x2287 },
    { "szlig", 0x00df },
    { "tau", 0x03c4 },
    { "there4", 0x2234 },
    { "theta", 0x03b8 },
    { "thetasym", 0x03d1 },
    { "thinsp", 0x2009 },
    { "thorn", 0x00fe },
    { "tilde", 0x02dc },
    { "times", 0x00d7 },
    { "trade", 0x2122 },
    { "uArr", 0x21d1 },
    { "uacute", 0x00fa },
    { "uarr", 0x2191 },
    { "ucirc", 0x00fb },
    { "ugrave", 0x00f9 },
    { "uml", 0x00a8 },
    { "upsih", 0x03d2 },
    { "upsilon", 0x03c5 },
    { "uuml", 0x00fc },
    { "weierp", 0x2118 },
    { "xi", 0x03be },
    { "yacute", 0x00fd },
    { "yen", 0x00a5 },
    { "yuml", 0x00ff },
    { "zeta", 0x03b6 },
    { "zwj", 0x200d },
    { "zwnj", 0x200c },
    { "", 0x0000 }
};





static QMap<QCString, QChar> *html_map = 0;
static void qt_cleanup_html_map()
{
    delete html_map;
    html_map = 0;
}

static QMap<QCString, QChar> *htmlMap()
{
    if ( !html_map ) {
	html_map = new QMap<QCString, QChar>;
	qAddPostRoutine( qt_cleanup_html_map );

	const Entity *ent = entitylist;
	while( ent->code ) {
	    html_map->insert( ent->name, QChar(ent->code) );
	    ent++;
	}
    }
    return html_map;
}

QChar KoTextDocument::parseHTMLSpecialChar(const QString& doc, int& pos)
{
    QCString s;
    pos++;
    int recoverpos = pos;
    while ( pos < int(doc.length()) && (doc.unicode())[pos] != ';' && !(doc.unicode())[pos].isSpace() && pos < recoverpos + 6) {
	s += (doc.unicode())[pos];
	pos++;
    }
    if ((doc.unicode())[pos] != ';' && !(doc.unicode())[pos].isSpace() ) {
	pos = recoverpos;
	return '&';
    }
    pos++;

    if ( s.length() > 1 && s[0] == '#') {
	int num = s.mid(1).toInt();
	if ( num == 151 ) // ### hack for designer manual
	    return '-';
	return num;
    }

    QMap<QCString, QChar>::Iterator it = htmlMap()->find(s);
    if ( it != htmlMap()->end() ) {
	return *it;
    }

    pos = recoverpos;
    return '&';
}

QString KoTextDocument::parseWord(const QString& doc, int& pos, bool lower)
{
    QString s;

    if ((doc.unicode())[pos] == '"') {
	pos++;
	while ( pos < int(doc.length()) && (doc.unicode())[pos] != '"' ) {
	    s += (doc.unicode())[pos];
	    pos++;
	}
	eat(doc, pos, '"');
    } else {
	static QString term = QString::fromLatin1("/>");
	while( pos < int(doc.length()) &&
	       ((doc.unicode())[pos] != '>' && !hasPrefix( doc, pos, term))
	       && (doc.unicode())[pos] != '<'
	       && (doc.unicode())[pos] != '='
	       && !(doc.unicode())[pos].isSpace())
	{
	    if ( (doc.unicode())[pos] == '&')
		s += parseHTMLSpecialChar( doc, pos );
	    else {
		s += (doc.unicode())[pos];
		pos++;
	    }
	}
	if (lower)
	    s = s.lower();
    }
    return s;
}

QChar KoTextDocument::parseChar(const QString& doc, int& pos, QStyleSheetItem::WhiteSpaceMode wsm )
{
    if ( pos >=  int(doc.length() ) )
	return QChar::null;

    QChar c = (doc.unicode())[pos++];

    if (c == '<' )
	return QChar::null;

    if ( c.isSpace() && c != QChar::nbsp ) {
	if ( wsm == QStyleSheetItem::WhiteSpacePre ) {
	    if ( c == ' ' )
		return QChar::nbsp;
	    else
		return c;
        //For the next Qt
        //} else if ( wsm ==  QStyleSheetItem_WhiteSpaceNoCompression ) {
        //return c;
	} else { // non-pre mode: collapse whitespace except nbsp
	    while ( pos< int(doc.length() ) &&
		    (doc.unicode())[pos].isSpace()  && (doc.unicode())[pos] != QChar::nbsp )
		pos++;
	    if ( wsm == QStyleSheetItem::WhiteSpaceNoWrap )
		return QChar::nbsp;
	    else
		return ' ';
	}
    }
    else if ( c == '&' )
	return parseHTMLSpecialChar( doc, --pos );
    else
	return c;
}

QString KoTextDocument::parseOpenTag(const QString& doc, int& pos,
				  QMap<QString, QString> &attr, bool& emptyTag)
{
    emptyTag = FALSE;
    pos++;
    if ( hasPrefix(doc, pos, '!') ) {
	if ( hasPrefix( doc, pos+1, "--")) {
	    pos += 3;
	    // eat comments
	    QString pref = QString::fromLatin1("-->");
	    while ( !hasPrefix(doc, pos, pref ) && pos < int(doc.length()) )
		pos++;
	    if ( hasPrefix(doc, pos, pref ) ) {
		pos += 3;
		eatSpace(doc, pos, TRUE);
	    }
	    emptyTag = TRUE;
	    return QString::null;
	}
	else {
	    // eat strange internal tags
	    while ( !hasPrefix(doc, pos, '>') && pos < int(doc.length()) )
		pos++;
	    if ( hasPrefix(doc, pos, '>') ) {
		pos++;
		eatSpace(doc, pos, TRUE);
	    }
	    return QString::null;
	}
    }

    QString tag = parseWord(doc, pos );
    eatSpace(doc, pos, TRUE);
    static QString term = QString::fromLatin1("/>");
    static QString s_TRUE = QString::fromLatin1("TRUE");

    while ((doc.unicode())[pos] != '>' && ! (emptyTag = hasPrefix(doc, pos, term) )) {
	QString key = parseWord(doc, pos );
	eatSpace(doc, pos, TRUE);
	if ( key.isEmpty()) {
	    // error recovery
	    while ( pos < int(doc.length()) && (doc.unicode())[pos] != '>' )
		pos++;
	    break;
	}
	QString value;
	if (hasPrefix(doc, pos, '=') ){
	    pos++;
	    eatSpace(doc, pos);
	    value = parseWord(doc, pos, FALSE);
	}
	else
	    value = s_TRUE;
	attr.insert(key, value );
	eatSpace(doc, pos, TRUE);
    }

    if (emptyTag) {
	eat(doc, pos, '/');
	eat(doc, pos, '>');
    }
    else
	eat(doc, pos, '>');

    return tag;
}

QString KoTextDocument::parseCloseTag( const QString& doc, int& pos )
{
    pos++;
    pos++;
    QString tag = parseWord(doc, pos );
    eatSpace(doc, pos, TRUE);
    eat(doc, pos, '>');
    return tag;
}

KoTextFlow::KoTextFlow()
{
    w = pagesize = 0;
    leftItems.setAutoDelete( FALSE );
    rightItems.setAutoDelete( FALSE );
}

KoTextFlow::~KoTextFlow()
{
}

void KoTextFlow::clear()
{
    leftItems.clear();
    rightItems.clear();
}

void KoTextFlow::setWidth( int width )
{
    w = width;
}

int KoTextFlow::adjustLMargin( int yp, int, int margin, int space, KoTextParag* )
{
    for ( KoTextCustomItem* item = leftItems.first(); item; item = leftItems.next() ) {
	if ( item->y() == -1 )
	    continue;
	if ( yp >= item->y() && yp < item->y() + item->height )
	    margin = QMAX( margin, item->x() + item->width + space );
    }
    return margin;
}

int KoTextFlow::adjustRMargin( int yp, int, int margin, int space, KoTextParag* )
{
    for ( KoTextCustomItem* item = rightItems.first(); item; item = rightItems.next() ) {
	if ( item->y() == -1 )
	    continue;
	if ( yp >= item->y() && yp < item->y() + item->height )
	    margin = QMAX( margin, w - item->x() - space );
    }
    return margin;
}


int KoTextFlow::adjustFlow( int y, int /*w*/, int h )
{
    if ( pagesize > 0 ) { // check pages
	int yinpage = y % pagesize;
	if ( yinpage <= 2 )
	    return 2 - yinpage;
	else
	    if ( yinpage + h > pagesize - 2 )
		return ( pagesize - yinpage ) + 2;
    }
    return 0;
}

void KoTextFlow::unregisterFloatingItem( KoTextCustomItem* item )
{
    leftItems.removeRef( item );
    rightItems.removeRef( item );
}

void KoTextFlow::registerFloatingItem( KoTextCustomItem* item )
{
    if ( item->placement() == KoTextCustomItem::PlaceRight ) {
	if ( !rightItems.contains( item ) )
	    rightItems.append( item );
    } else if ( item->placement() == KoTextCustomItem::PlaceLeft &&
		!leftItems.contains( item ) ) {
	leftItems.append( item );
    }
}

QRect KoTextFlow::boundingRect() const
{
    QRect br;
    QPtrListIterator<KoTextCustomItem> l( leftItems );
    while( l.current() ) {
	br = br.unite( l.current()->geometry() );
	++l;
    }
    QPtrListIterator<KoTextCustomItem> r( rightItems );
    while( r.current() ) {
	br = br.unite( r.current()->geometry() );
	++r;
    }
    return br;
}

int KoTextFlow::availableHeight() const
{
    return -1; // no limit
}

void KoTextFlow::drawFloatingItems( QPainter* p, int cx, int cy, int cw, int ch, const QColorGroup& cg, bool selected )
{
    KoTextCustomItem *item;
    for ( item = leftItems.first(); item; item = leftItems.next() ) {
	if ( item->x() == -1 || item->y() == -1 )
	    continue;
	item->draw( p, item->x(), item->y(), cx, cy, cw, ch, cg, selected );
    }

    for ( item = rightItems.first(); item; item = rightItems.next() ) {
	if ( item->x() == -1 || item->y() == -1 )
	    continue;
	item->draw( p, item->x(), item->y(), cx, cy, cw, ch, cg, selected );
    }
}

void KoTextFlow::setPageSize( int ps ) { pagesize = ps; }
bool KoTextFlow::isEmpty() { return leftItems.isEmpty() && rightItems.isEmpty(); }

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#if 0 // port this to WYSIWYG if you want to use it.....
#ifdef QTEXTTABLE_AVAILABLE
void KoTextTableCell::invalidate() { cached_width = -1; cached_sizehint = -1; }

void KoTextTable::invalidate() { cachewidth = -1; }
#endif

KoTextTable::KoTextTable( KoTextDocument *p, const QMap<QString, QString> & attr  )
    : KoTextCustomItem( p ), painter( 0 )
{
    cells.setAutoDelete( FALSE );
#if defined(PARSER_DEBUG)
    debug_indent += "\t";
    qDebug( debug_indent + "new KoTextTable (%p)", this );
    debug_indent += "\t";
#endif
    cellspacing = 2;
    if ( attr.contains("cellspacing") )
	cellspacing = attr["cellspacing"].toInt();
    cellpadding = 1;
    if ( attr.contains("cellpadding") )
	cellpadding = attr["cellpadding"].toInt();
    border = innerborder = 0;
    if ( attr.contains("border" ) ) {
	QString s( attr["border"] );
	if ( s == "TRUE" )
	    border = 1;
	else
	    border = attr["border"].toInt();
    }
    us_b = border;

    innerborder = us_ib = border ? 1 : 0;

    if ( border )
	cellspacing += 2;

    us_ib = innerborder;
    us_cs = cellspacing;
    us_cp = cellpadding;
    outerborder = cellspacing + border;
    us_ob = outerborder;
    layout = new QGridLayout( 1, 1, cellspacing );

    fixwidth = 0;
    stretch = 0;
    if ( attr.contains("width") ) {
	bool b;
	QString s( attr["width"] );
	int w = s.toInt( &b );
	if ( b ) {
	    fixwidth = w;
	} else {
	    s = s.stripWhiteSpace();
	    if ( s.length() > 1 && s[ (int)s.length()-1 ] == '%' )
		stretch = s.left( s.length()-1).toInt();
	}
    }

    place = PlaceInline;
    if ( attr["align"] == "left" )
	place = PlaceLeft;
    else if ( attr["align"] == "right" )
	place = PlaceRight;
    cachewidth = 0;
    attributes = attr;
    pageBreakFor = -1;
}

KoTextTable::~KoTextTable()
{
    delete layout;
}

QString KoTextTable::richText() const
{
    QString s;
    s = "<table ";
    QMap<QString, QString>::ConstIterator it = attributes.begin();
    for ( ; it != attributes.end(); ++it )
	s += it.key() + "=" + *it + " ";
    s += ">\n";

    int lastRow = -1;
    bool needEnd = FALSE;
    QPtrListIterator<KoTextTableCell> it2( cells );
    while ( it2.current() ) {
	KoTextTableCell *cell = it2.current();
	++it2;
	if ( lastRow != cell->row() ) {
	    if ( lastRow != -1 )
		s += "</tr>\n";
	    s += "<tr>";
	    lastRow = cell->row();
	    needEnd = TRUE;
	}
	s += "<td ";
	it = cell->attributes.begin();
	for ( ; it != cell->attributes.end(); ++it )
	    s += it.key() + "=" + *it + " ";
	s += ">";
	s += cell->richText()->richText();
	s += "</td>";
    }
    if ( needEnd )
	s += "</tr>\n";
    s += "</table>\n";
    return s;
}

void KoTextTable::setPainter( QPainter* p, bool adjust )
{
    painter = p;
    if ( adjust ) {
	cellspacing = scale( us_cs, p );
	cellpadding = scale( us_cp, p );
	border = scale( us_b , p );
	innerborder = scale( us_ib, p );
	outerborder = scale( us_ob ,p );
	width = 0;
	cachewidth = 0;
    }
    for ( KoTextTableCell* cell = cells.first(); cell; cell = cells.next() )
	cell->setPainter( p, adjust );
}

void KoTextTable::adjustCells( int y , int shift )
{
    QPtrListIterator<KoTextTableCell> it( cells );
    KoTextTableCell* cell;
    bool enlarge = FALSE;
    while ( ( cell = it.current() ) ) {
	++it;
	QRect r = cell->geometry();
	if ( y <= r.top() ) {
	    r.moveBy(0, shift );
	    cell->setGeometry( r );
	    enlarge = TRUE;
	} else if ( y <= r.bottom() ) {
	    r.rBottom() += shift;
	    cell->setGeometry( r );
	    enlarge = TRUE;
	}
    }
    if ( enlarge )
	height += shift;
}

void KoTextTable::pageBreak( int  yt, KoTextFlow* flow )
{
    if ( flow->pageSize() <= 0 )
        return;
    if ( layout && pageBreakFor > 0 && pageBreakFor != yt ) {
	layout->invalidate();
	int h = layout->heightForWidth( width-2*outerborder );
	layout->setGeometry( QRect(0, 0, width-2*outerborder, h)  );
	height = layout->geometry().height()+2*outerborder;
    }
    pageBreakFor = yt;
    QPtrListIterator<KoTextTableCell> it( cells );
    KoTextTableCell* cell;
    while ( ( cell = it.current() ) ) {
	++it;
	int y = yt + outerborder + cell->geometry().y();
	int shift = flow->adjustFlow( y - cellspacing, width, cell->richText()->height() + 2*cellspacing );
	adjustCells( y - outerborder - yt, shift );
    }
}


void KoTextTable::draw(QPainter* p, int x, int y, int cx, int cy, int cw, int ch, const QColorGroup& cg, bool selected )
{
    if ( placement() != PlaceInline ) {
	x = xpos;
	y = ypos;
    }

    painter = p;

    for (KoTextTableCell* cell = cells.first(); cell; cell = cells.next() ) {
	if ( cx < 0 && cy < 0 ||
	     QRect( cx, cy, cw, ch ).intersects( QRect( x + outerborder + cell->geometry().x(),
							y + outerborder + cell->geometry().y(),
							cell->geometry().width(), cell->geometry().height() ) ) ) {
	    cell->draw( x+outerborder, y+outerborder, cx, cy, cw, ch, cg, selected );
	    if ( border ) {
		QRect r( x+outerborder+cell->geometry().x() - innerborder,
			 y+outerborder+cell->geometry().y() - innerborder,
			 cell->geometry().width() + 2 * innerborder,
			 cell->geometry().height() + 2 * innerborder );
		if ( is_printer( p ) || ( p && p->device() && p->device()->devType() == QInternal::Printer ) ) {
		    QPen oldPen = p->pen();
		    QRect r2 = r;
		    r2.addCoords( innerborder/2, innerborder/2, -innerborder/2, -innerborder/2 );
		    p->setPen( QPen( cg.text(), innerborder ) );
		    p->drawRect( r2 );
		    p->setPen( oldPen );
		} else {
		    int s =  QMAX( cellspacing-2*innerborder, 0);
		    if ( s ) {
			p->fillRect( r.left()-s, r.top(), s+1, r.height(), cg.button() );
			p->fillRect( r.right(), r.top(), s+1, r.height(), cg.button() );
			p->fillRect( r.left()-s, r.top()-s, r.width()+2*s, s, cg.button() );
			p->fillRect( r.left()-s, r.bottom(), r.width()+2*s, s, cg.button() );
		    }
		    qDrawShadePanel( p, r, cg, TRUE, innerborder );
		}
	    }
	}
    }
    if ( border ) {
	QRect r ( x, y, width, height );
	if ( is_printer( p ) || ( p && p->device() && p->device()->devType() == QInternal::Printer ) ) {
 	    QRect r2 = r;
 	    r2.addCoords( border/2, border/2, -border/2, -border/2 );
	    QPen oldPen = p->pen();
	    p->setPen( QPen( cg.text(), border ) );
	    p->drawRect( r2 );
	    p->setPen( oldPen );
	} else {
	    int s = border+QMAX( cellspacing-2*innerborder, 0);
	    if ( s ) {
		p->fillRect( r.left(), r.top(), s, r.height(), cg.button() );
		p->fillRect( r.right()-s, r.top(), s, r.height(), cg.button() );
		p->fillRect( r.left(), r.top(), r.width(), s, cg.button() );
		p->fillRect( r.left(), r.bottom()-s, r.width(), s, cg.button() );
	    }
	    qDrawShadePanel( p, r, cg, FALSE, border );
	}
    }

#if defined(DEBUG_TABLE_RENDERING)
    p->save();
    p->setPen( Qt::red );
    p->drawRect( x, y, width, height );
    p->restore();
#endif
}

void KoTextTable::resize( QPainter* p, int nwidth )
{
    if ( fixwidth && cachewidth != 0 )
	return;
    if ( nwidth == cachewidth )
	return;
    cachewidth = nwidth;
    int w = nwidth;
    painter = p;
    format( w );
    if ( nwidth >= 32000 )
	nwidth = w;
    if ( stretch )
	nwidth = nwidth * stretch / 100;

    width = nwidth + 2*outerborder;
    layout->invalidate();
    int shw = layout->sizeHint().width() + 2*outerborder;
    int mw = layout->minimumSize().width() + 2*outerborder;
    if ( stretch )
	width = QMAX( mw, nwidth );
    else
	width = QMAX( mw, QMIN( nwidth, shw ) );

    if ( fixwidth )
	width = fixwidth;

    layout->invalidate();
    mw = layout->minimumSize().width() + 2*outerborder;
    width = QMAX( width, mw );

    int h = layout->heightForWidth( width-2*outerborder );
    layout->setGeometry( QRect(0, 0, width-2*outerborder, h)  );
    height = layout->geometry().height()+2*outerborder;
}

void KoTextTable::format( int &w )
{
    for ( int i = 0; i < (int)cells.count(); ++i ) {
	KoTextTableCell *cell = cells.at( i );
	cell->richText()->doLayout( painter, QWIDGETSIZE_MAX );
	cell->cached_sizehint = cell->richText()->widthUsed() + 2 * ( innerborder + 4 );
	if ( cell->cached_sizehint > 32000 ) // nested table in paragraph
	    cell->cached_sizehint = cell->minimumSize().width();
	cell->richText()->doLayout( painter, cell->cached_sizehint );
	cell->cached_width = -1;
    }
    w = widthHint();
    layout->invalidate();
    layout->activate();
    width = minimumWidth();
}

void KoTextTable::addCell( KoTextTableCell* cell )
{
    cells.append( cell );
    layout->addMultiCell( cell, cell->row(), cell->row() + cell->rowspan()-1,
			  cell->column(), cell->column() + cell->colspan()-1 );
}

bool KoTextTable::enter( KoTextCursor *c, KoTextDocument *&doc, KoTextParag *&parag, int &idx, int &ox, int &oy, bool atEnd )
{
    currCell.remove( c );
    if ( !atEnd )
	return next( c, doc, parag, idx, ox, oy );
    currCell.insert( c, cells.count() );
    return prev( c, doc, parag, idx, ox, oy );
}

bool KoTextTable::enterAt( KoTextCursor *c, KoTextDocument *&doc, KoTextParag *&parag, int &idx, int &ox, int &oy, const QPoint &pos )
{
    currCell.remove( c );
    int lastCell = -1;
    int lastY = -1;
    int i;
    for ( i = 0; i < (int)cells.count(); ++i ) {
	KoTextTableCell *cell = cells.at( i );
	if ( !cell )
	    continue;
	QRect r( outerborder+cell->geometry().x(),
		 outerborder+cell->geometry().y(),
		 cell->geometry().width(),
		 cell->geometry().height() );

	if ( r.left() <= pos.x() && r.right() >= pos.x() ) {
	    if ( cell->geometry().y() > lastY ) {
		lastCell = i;
		lastY = cell->geometry().y();
	    }
	    if ( r.top() <= pos.y() && r.bottom() >= pos.y() ) {
		currCell.insert( c, i );
		break;
	    }
	}
    }
    if ( i == (int) cells.count() )
 	return FALSE; // no cell found

    if ( currCell.find( c ) == currCell.end() ) {
	if ( lastY != -1 )
	    currCell.insert( c, lastCell );
	else
	    return FALSE;
    }

    KoTextTableCell *cell = cells.at( *currCell.find( c ) );
    if ( !cell )
	return FALSE;
    doc = cell->richText();
    parag = doc->firstParag();
    idx = 0;
    ox += cell->geometry().x() + cell->horizontalAlignmentOffset() + outerborder + parent->x();
    oy += cell->geometry().y() + cell->verticalAlignmentOffset() + outerborder;
    return TRUE;
}

bool KoTextTable::next( KoTextCursor *c, KoTextDocument *&doc, KoTextParag *&parag, int &idx, int &ox, int &oy )
{
    int cc = -1;
    if ( currCell.find( c ) != currCell.end() )
	cc = *currCell.find( c );
    if ( cc > (int)cells.count() - 1 || cc < 0 )
	cc = -1;
    currCell.remove( c );
    currCell.insert( c, ++cc );
    if ( cc >= (int)cells.count() ) {
	currCell.insert( c, 0 );
	KoTextCustomItem::next( c, doc, parag, idx, ox, oy );
	KoTextTableCell *cell = cells.first();
	if ( !cell )
	    return FALSE;
	doc = cell->richText();
	idx = -1;
	return TRUE;
    }

    if ( currCell.find( c ) == currCell.end() )
	return FALSE;
    KoTextTableCell *cell = cells.at( *currCell.find( c ) );
    if ( !cell )
	return FALSE;
    doc = cell->richText();
    parag = doc->firstParag();
    idx = 0;
    ox += cell->geometry().x() + cell->horizontalAlignmentOffset() + outerborder + parent->x();
    oy += cell->geometry().y() + cell->verticalAlignmentOffset() + outerborder;
    return TRUE;
}

bool KoTextTable::prev( KoTextCursor *c, KoTextDocument *&doc, KoTextParag *&parag, int &idx, int &ox, int &oy )
{
    int cc = -1;
    if ( currCell.find( c ) != currCell.end() )
	cc = *currCell.find( c );
    if ( cc > (int)cells.count() - 1 || cc < 0 )
	cc = cells.count();
    currCell.remove( c );
    currCell.insert( c, --cc );
    if ( cc < 0 ) {
	currCell.insert( c, 0 );
	KoTextCustomItem::prev( c, doc, parag, idx, ox, oy );
	KoTextTableCell *cell = cells.first();
	if ( !cell )
	    return FALSE;
	doc = cell->richText();
	idx = -1;
	return TRUE;
    }

    if ( currCell.find( c ) == currCell.end() )
	return FALSE;
    KoTextTableCell *cell = cells.at( *currCell.find( c ) );
    if ( !cell )
	return FALSE;
    doc = cell->richText();
    parag = doc->firstParag();
    idx = parag->length() - 1;
    ox += cell->geometry().x() + cell->horizontalAlignmentOffset() + outerborder + parent->x();
    oy += cell->geometry().y()  + cell->verticalAlignmentOffset() + outerborder;
    return TRUE;
}

bool KoTextTable::down( KoTextCursor *c, KoTextDocument *&doc, KoTextParag *&parag, int &idx, int &ox, int &oy )
{
    if ( currCell.find( c ) == currCell.end() )
	return FALSE;
    KoTextTableCell *cell = cells.at( *currCell.find( c ) );
    if ( cell->row_ == layout->numRows() - 1 ) {
	currCell.insert( c, 0 );
	KoTextCustomItem::down( c, doc, parag, idx, ox, oy );
	KoTextTableCell *cell = cells.first();
	if ( !cell )
	    return FALSE;
	doc = cell->richText();
	idx = -1;
	return TRUE;
    }

    int oldRow = cell->row_;
    int oldCol = cell->col_;
    if ( currCell.find( c ) == currCell.end() )
	return FALSE;
    int cc = *currCell.find( c );
    for ( int i = cc; i < (int)cells.count(); ++i ) {
	cell = cells.at( i );
	if ( cell->row_ > oldRow && cell->col_ == oldCol ) {
	    currCell.insert( c, i );
	    break;
	}
    }
    doc = cell->richText();
    if ( !cell )
	return FALSE;
    parag = doc->firstParag();
    idx = 0;
    ox += cell->geometry().x() + cell->horizontalAlignmentOffset() + outerborder + parent->x();
    oy += cell->geometry().y()  + cell->verticalAlignmentOffset() + outerborder;
    return TRUE;
}

bool KoTextTable::up( KoTextCursor *c, KoTextDocument *&doc, KoTextParag *&parag, int &idx, int &ox, int &oy )
{
    if ( currCell.find( c ) == currCell.end() )
	return FALSE;
    KoTextTableCell *cell = cells.at( *currCell.find( c ) );
    if ( cell->row_ == 0 ) {
	currCell.insert( c, 0 );
	KoTextCustomItem::up( c, doc, parag, idx, ox, oy );
	KoTextTableCell *cell = cells.first();
	if ( !cell )
	    return FALSE;
	doc = cell->richText();
	idx = -1;
	return TRUE;
    }

    int oldRow = cell->row_;
    int oldCol = cell->col_;
    if ( currCell.find( c ) == currCell.end() )
	return FALSE;
    int cc = *currCell.find( c );
    for ( int i = cc; i >= 0; --i ) {
	cell = cells.at( i );
	if ( cell->row_ < oldRow && cell->col_ == oldCol ) {
	    currCell.insert( c, i );
	    break;
	}
    }
    doc = cell->richText();
    if ( !cell )
	return FALSE;
    parag = doc->lastParag();
    idx = parag->length() - 1;
    ox += cell->geometry().x() + cell->horizontalAlignmentOffset() + outerborder + parent->x();
    oy += cell->geometry().y()  + cell->verticalAlignmentOffset() + outerborder;
    return TRUE;
}

KoTextTableCell::KoTextTableCell( KoTextTable* table,
				int row, int column,
				const QMap<QString, QString> &attr,
				const QStyleSheetItem* /*style*/, // ### use them
				const KoTextFormat& /*fmt*/, const QString& context,
				QMimeSourceFactory &factory, QStyleSheet *sheet,
				const QString& doc)
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new KoTextTableCell1 (pappi: %p)", table );
    qDebug( debug_indent + doc );
#endif
    cached_width = -1;
    cached_sizehint = -1;

    maxw = QWIDGETSIZE_MAX;
    minw = 0;

    parent = table;
    row_ = row;
    col_ = column;
    stretch_ = 0;
    richtext = new KoTextDocument( table->parent );
    richtext->setTableCell( this );
    QString a = *attr.find( "align" );
    if ( !a.isEmpty() ) {
	a = a.lower();
	if ( a == "left" )
	    richtext->setAlignment( Qt::AlignLeft );
	else if ( a == "center" )
	    richtext->setAlignment( Qt::AlignHCenter );
	else if ( a == "right" )
	    richtext->setAlignment( Qt::AlignRight );
    }
    align = 0;
    QString va = *attr.find( "valign" );
    if ( !va.isEmpty() ) {
	va = va.lower();
	if ( va == "center" )
	    align |= Qt::AlignVCenter;
	else if ( va == "bottom" )
	    align |= Qt::AlignBottom;
    }
    richtext->setFormatter( table->parent->formatter() );
    richtext->setUseFormatCollection( table->parent->useFormatCollection() );
    richtext->setMimeSourceFactory( &factory );
    richtext->setStyleSheet( sheet );
    richtext->setDefaultFont( table->parent->formatCollection()->defaultFormat()->font() );
    richtext->setRichText( doc, context );
    rowspan_ = 1;
    colspan_ = 1;
    if ( attr.contains("colspan") )
	colspan_ = attr["colspan"].toInt();
    if ( attr.contains("rowspan") )
	rowspan_ = attr["rowspan"].toInt();

    background = 0;
    if ( attr.contains("bgcolor") ) {
	background = new QBrush(QColor( attr["bgcolor"] ));
    }


    hasFixedWidth = FALSE;
    if ( attr.contains("width") ) {
	bool b;
	QString s( attr["width"] );
	int w = s.toInt( &b );
	if ( b ) {
	    maxw = w;
	    minw = maxw;
	    hasFixedWidth = TRUE;
	} else {
	    s = s.stripWhiteSpace();
	    if ( s.length() > 1 && s[ (int)s.length()-1 ] == '%' )
		stretch_ = s.left( s.length()-1).toInt();
	}
    }

    attributes = attr;

    parent->addCell( this );
}

KoTextTableCell::KoTextTableCell( KoTextTable* table, int row, int column )
{
#if defined(PARSER_DEBUG)
    qDebug( debug_indent + "new KoTextTableCell2( pappi: %p", table );
#endif
    maxw = QWIDGETSIZE_MAX;
    minw = 0;
    cached_width = -1;
    cached_sizehint = -1;

    parent = table;
    row_ = row;
    col_ = column;
    stretch_ = 0;
    richtext = new KoTextDocument( table->parent );
    richtext->setTableCell( this );
    richtext->setFormatter( table->parent->formatter() );
    richtext->setUseFormatCollection( table->parent->useFormatCollection() );
    richtext->setDefaultFont( table->parent->formatCollection()->defaultFormat()->font() );
    richtext->setRichText( "<html></html>", QString::null );
    rowspan_ = 1;
    colspan_ = 1;
    background = 0;
    hasFixedWidth = FALSE;
    parent->addCell( this );
}


KoTextTableCell::~KoTextTableCell()
{
    delete background;
    background = 0;
    delete richtext;
    richtext = 0;
}

QSize KoTextTableCell::sizeHint() const
{
    if ( cached_sizehint != -1 )
 	return QSize( cached_sizehint, 0 );
    KoTextTableCell *that = (KoTextTableCell*)this;
    if ( stretch_ == 100 ) {
	return QSize( ( that->cached_sizehint = QWIDGETSIZE_MAX ), 0 );
    } else if ( stretch_ > 0 ) {
	return QSize( QMAX( QMAX( richtext->widthUsed(), minw ),
			    parent->width * stretch_ / 100 - 2*parent->cellspacing - 2*parent->cellpadding ), 0 );
    }
    return QSize( ( that->cached_sizehint = richtext->widthUsed() + 2 * ( parent->innerborder + parent->cellpadding + 4 ) ), 0 );
}

QSize KoTextTableCell::minimumSize() const
{
    if ( stretch_ )
	return QSize( QMAX( QMAX( richtext->widthUsed(), minw ),
			    parent->width * stretch_ / 100 - 2*parent->cellspacing - 2*parent->cellpadding), 0 );
    return QSize(QMAX( richtext->minimumWidth(), minw ),0);
}

QSize KoTextTableCell::maximumSize() const
{
    return QSize( QMAX( richtext->widthUsed(), maxw), QWIDGETSIZE_MAX );
//     return QSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
}

QSizePolicy::ExpandData KoTextTableCell::expanding() const
{
    return QSizePolicy::BothDirections;
}

bool KoTextTableCell::isEmpty() const
{
    return FALSE;
}
void KoTextTableCell::setGeometry( const QRect& r )
{
    if ( r.width() != cached_width )
	richtext->doLayout( painter(), r.width() );
    cached_width = r.width();
    geom = r;
}

QRect KoTextTableCell::geometry() const
{
    return geom;
}

bool KoTextTableCell::hasHeightForWidth() const
{
    return TRUE;
}

int KoTextTableCell::heightForWidth( int w ) const
{
    w = QMAX( minw, w );

    if ( cached_width != w ) {
	KoTextTableCell* that = (KoTextTableCell*) this;
	that->richtext->doLayout( painter(), w - 2 * parent->cellpadding );
	that->cached_width = w;
    }
    return richtext->height() + 2 * parent->innerborder + 2* parent->cellpadding;
}

void KoTextTableCell::setPainter( QPainter* p, bool adjust )
{
    richtext->formatCollection()->setPainter( p );
    KoTextParag *parag = richtext->firstParag();
    while ( parag ) {
	parag->setPainter( p, adjust  );
	parag = parag->next();
    }
}

QPainter* KoTextTableCell::painter() const
{
    return parent->painter;
}

int KoTextTableCell::horizontalAlignmentOffset() const
{
    return parent->cellpadding;
}

int KoTextTableCell::verticalAlignmentOffset() const
{
    if ( (align & Qt::AlignVCenter ) == Qt::AlignVCenter )
	return ( geom.height() - richtext->height() ) / 2;
    else if ( ( align & Qt::AlignBottom ) == Qt::AlignBottom )
	return geom.height() - parent->cellpadding - richtext->height();
    return parent->cellpadding;
}

void KoTextTableCell::draw( int x, int y, int cx, int cy, int cw, int ch, const QColorGroup& cg, bool )
{
    if ( cached_width != geom.width() ) {
	richtext->doLayout( painter(), geom.width() );
	cached_width = geom.width();
    }
    QColorGroup g( cg );
    if ( background )
	g.setBrush( QColorGroup::Base, *background );
    else if ( richtext->paper() )
	g.setBrush( QColorGroup::Base, *richtext->paper() );

    painter()->save();
    painter()->translate( x + geom.x(), y + geom.y() );
    if ( background )
	painter()->fillRect( 0, 0, geom.width(), geom.height(), *background );
    else if ( richtext->paper() )
	painter()->fillRect( 0, 0, geom.width(), geom.height(), *richtext->paper() );

    painter()->translate( horizontalAlignmentOffset(), verticalAlignmentOffset() );

    QRegion r;
    KoTextCursor *c = 0;
    if ( richtext->parent()->tmpCursor )
	c = richtext->parent()->tmpCursor;
    if ( cx >= 0 && cy >= 0 )
	richtext->draw( painter(), cx - ( x + horizontalAlignmentOffset() + geom.x() ),
			cy - ( y + geom.y() + verticalAlignmentOffset() ),
			cw, ch, g, FALSE, (c != 0), c );
    else
	richtext->draw( painter(), -1, -1, -1, -1, g, FALSE, (c != 0), c );

    painter()->restore();
}

#undef KoTextDocument
#undef KoTextParag
#undef KoTextFlow
#endif //QT_NO_RICHTEXT
