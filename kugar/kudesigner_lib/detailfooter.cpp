/* This file is part of the KDE project
 Copyright (C) 2003-2004 Alexander Dymo <adymo@mksat.net>

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
#include "detailfooter.h"

#include <klocale.h>

#include <qpainter.h>

#include <koproperty/property.h>

namespace Kudesigner
{

DetailFooter::DetailFooter( int x, int y, int width, int height, int level, Canvas *canvas ) :
        DetailBase( x, y, width, height, level, canvas )
{
#warning TODO port this to new Property API !!!!!
#if 0
    props.addProperty( new Property( "Height", i18n( "Height" ), i18n( "Height" ), 50, KoProperty::Integer ), "Detail" );
    props.addProperty( new Property( "Level", i18n( "Level" ), i18n( "Detail Level" ), 0, KoProperty::Integer ), "Detail" );
#endif
}

void DetailFooter::draw( QPainter &painter )
{
    QString str = QString( "%1 %2" ).arg( i18n( "Detail Footer" ) ).arg( props[ "Level" ].value().toInt() );
    painter.drawText( rect(), AlignVCenter | AlignLeft, str );
    Band::draw( painter );
}

QString DetailFooter::getXml()
{
    return "\t<DetailFooter" + Band::getXml() + "\t</DetailFooter>\n\n";
}

}
