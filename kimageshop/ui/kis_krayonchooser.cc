/*
 *  kis_krayonchooser.cc - part of KImageShop
 *
 *  A chooser for KisKrayons. Makes use of the IconChooser class and maintains
 *  all available krayons for KIS.
 *
 *  Copyright (c) 1999 Carsten Pfeiffer <pfeiffer@kde.org>
 *  Copyright (c) 2000 Matthias Elter   <elter@kde.org>
 *  Copyright (c) 2000 John Califf   <jcaliff@compuzone.net>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>

#include <kinstance.h>
#include <klocale.h>
#include <kstddirs.h>

#include "kis_factory.h"
#include "kis_resourceserver.h"
#include "iconchooser.h"
#include "integerwidget.h"
#include "kis_pattern.h"
#include "kis_brush.h"
#include "kis_krayonchooser.h"

// X11 headers
#undef Below
#undef Above

KisKrayonChooser::KisKrayonChooser( QWidget *parent, const char *name ) 
 : QWidget( parent, name )
{
    lbSpacing = new QLabel( i18n("Spacing:"), this );
    slSpacing = new IntegerWidget( 1, 100, this, "int widget" );

    slSpacing->setTickmarks( QSlider::Below );
    slSpacing->setTickInterval( 10 );
    QObject::connect( slSpacing, SIGNAL( valueChanged(int) ),
	    this, SLOT( slotSetKrayonSpacing(int) ));

    // only serves as beautifier for the iconchooser
    frame = new QHBox( this );
    frame->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    chooser = new IconChooser( frame, QSize(30,30), "krayon chooser" );  

    //container = new QWidget(frame);  
    //chooser = new IconChooser( container, QSize(30,30), "icon chooser" );

    QList<KisBrush> bList = KisFactory::rServer()->brushes();
  
    for (KisBrush *brush = bList.first(); brush != 0; brush = bList.next())
    {
        if ( brush->isValid() && (KisKrayon *)brush->isValidKrayon() )
	        chooser->addItem( (IconItem *) brush );
    }
  
    QList<KisPattern> pList = KisFactory::rServer()->patterns();
  
    for (KisPattern *pattern = pList.first(); pattern != 0; pattern = pList.next())
    {
        if ( pattern->isValid() && (KisKrayon *)pattern->isValidKrayon() )
	        chooser->addItem( (IconItem *) pattern );
    }

    QObject::connect( chooser, SIGNAL( selected( IconItem * ) ),
		    this, SLOT( slotItemSelected( IconItem * )));
  
    initGUI();

    const KisKrayon * krayon = currentKrayon();
  
    if ( krayon )
        slSpacing->setValue( /* pattern->spacing()*/ 5 );
}


KisKrayonChooser::~KisKrayonChooser()
{
  delete lbSpacing;
  delete slSpacing;
  // delete container;
  delete chooser;
  delete frame;
}


// set the active pattern in the chooser - does NOT emit selected() (should it?)
void KisKrayonChooser::setCurrentKrayon( const KisKrayon *krayon )
{
    chooser->setCurrentItem( (IconItem *) krayon );
    slSpacing->setValue( /* pattern->spacing() */ 5 );
}


// return the active pattern
const KisKrayon * KisKrayonChooser::currentKrayon() const
{
    return (const KisKrayon *) chooser->currentItem();
}


void KisKrayonChooser::initGUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout( this, 2, -1, "main layout" );
    QHBoxLayout *spacingLayout = new QHBoxLayout( -1, "spacing layout" );

    mainLayout->addWidget( frame, 10 );
    mainLayout->addLayout( spacingLayout, 1 );

    spacingLayout->addWidget( lbSpacing, 0 );
    spacingLayout->addStretch();
    spacingLayout->addWidget( slSpacing, 1 );
}


// called when an item is selected in the chooser
// set the slider to the correct position
void KisKrayonChooser::slotItemSelected( IconItem *item )
{
    const KisKrayon *krayon = (KisKrayon *) item;
    slSpacing->setValue( /* pattern->spacing() */ 5 );
    emit selected( krayon );
}


// sliderposition (spacing) changed, apply that to the current pattern
void KisKrayonChooser::slotSetKrayonSpacing( int /* spacing */ )
{
    // KisKrayon *krayon = (KisKrayon *) currentKrayon();
    // if ( pattern ) pattern->setSpacing( spacing );
}

// #include "kis_krayonchooser.moc"






