/**
 * $Id$
 *
 * Kalle Dalheimer <kalle@kde.org>
 */

#include "kchart_factory.h"
#include "kchart_part.h"
#include <klocale.h>
#include <kinstance.h>
#include <kaboutdata.h>

static const char* description=I18N_NOOP("KOffice Chart Generator");
static const char* version="0.1";

extern "C"
{
    void* init_libkchart()
    {
	return new KChartFactory;
    }
};

KInstance* KChartFactory::s_global = 0;

KChartFactory::KChartFactory( QObject* parent, const char* name )
    : KLibFactory( parent, name )
{
}

KChartFactory::~KChartFactory()
{
    if ( s_global ) 
      delete s_global;
}

QObject* KChartFactory::create( QObject* parent, const char* name, const char *classname, const QStringList & )
{
/*
    if ( parent && !parent->inherits("KoDocument") ) {
		qDebug("KChartFactory: parent does not inherit KoDocument");
		return 0;
    }
*/

    bool bWantKoDocument = ( strcmp( classname, "KofficeDocument" ) == 0 );

    KChartPart *part = new KChartPart( (KoDocument*)parent, name, !bWantKoDocument );

    if ( !bWantKoDocument )
      part->setReadWrite( false );

    emit objectCreated(part);
    return part;
}

KInstance* KChartFactory::global()
{
    if ( !s_global )
    {
         KAboutData *aboutData = new KAboutData("kchart", I18N_NOOP("KChart"),
             version, description, KAboutData::License_GPL,
             "(c) 1998-2000, Kalle Dalheimer");
         aboutData->addAuthor("Kalle Dalheimer",0, "kalle@kde.org");
     
         s_global = new KInstance(aboutData);
    }
    return s_global;
}

#include "kchart_factory.moc"
