/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include "main.h"

#include <qmessagebox.h>

#include <klocale.h>
#include <kspell.h>

/***************************************************
 *
 * Factory
 *
 ***************************************************/

extern "C"
{
    void* init_libkspelltool()
    {
	return new SpellCheckerFactory;
    }
};

SpellCheckerFactory::SpellCheckerFactory( QObject* parent, const char* name )
    : KLibFactory( parent, name )
{
}

SpellCheckerFactory::~SpellCheckerFactory()
{
}

QObject* SpellCheckerFactory::create( QObject* parent, const char* name, const char* classname )
{
    return new SpellChecker( parent, name );
}

/***************************************************
 *
 * Spellchecker
 *
 ***************************************************/

SpellChecker::SpellChecker( QObject* parent, const char* name )
    : KoDataTool( parent, name )
{
}

bool SpellChecker::run( const QString& command, void* data, const QString& datatype, const QString& mimetype )
{
    if ( command != "spellcheck" )
    {
	qDebug("SpellChecker does only accept the command 'spellcheck'");
	qDebug("   The commands %s is not accepted", command.latin1() );
	return FALSE;
    }

    // Check wether we can accept the data
    if ( datatype != "QString" )
    {
	qDebug("SpellChecker does only accept datatype QString");
	return FALSE;
    }
    if ( mimetype != "text/plain" )
    {
	qDebug("SpellChecker does only accept mimetype text/plain");
	return FALSE;
    }

    // Get data
    QString buffer = *((QString*)data);
    buffer = buffer.stripWhiteSpace();

    // #### handle errors
    // Call the spell checker
    KSpell::modalCheck( buffer );

    // Set data
    *((QString*)data) = buffer;

    return TRUE;
}

#include "main.moc"
