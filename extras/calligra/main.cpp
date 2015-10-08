/* This file is part of the KDE project
   Copyright (C) 2010 Jarosław Staniek <staniek@kde.org>

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

#include <QTextStream>

#include <k4aboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <KoServiceLocator.h>
#include <kmimetypetrader.h>
#include <kapplication.h>
#include <kdebug.h>
#include <krun.h>
#include <ktoolinvocation.h>
#include <kmessagebox.h>
#include <kguiitem.h>
#include <kurl.h>

#include <CalligraVersionWrapper.h>

static void listApplicationNames()
{
    const KService::List services = KoServiceLocator::instance()->entries("Calligra/Application");
    QTextStream out(stdout, QIODevice::WriteOnly);
    QStringList names;
    foreach (KService::Ptr service, services) {
        names.append(service->desktopEntryName());
    }
    names.sort();
    foreach (const QString& name, names) {
        out << name << '\n';
    }
}

static void showWelcomeWindow()
{
}

static bool startService(KService::Ptr service, const KUrl& url)
{
    kDebug() << "service->entryPath():" << service->entryPath();
    QString error;
    QString serviceName;
    int pid = 0;
    int res = KToolInvocation::startServiceByDesktopPath(
        service->entryPath(), url.url(), &error, &serviceName, &pid
    );
    // could not check result - does not work for custom KDEDIRS: ok = res == 0;
    //ok = true;
    kDebug() << "KToolInvocation::startServiceByDesktopPath:" << res << pid << error << serviceName;
    return res == 0;
}

static int handleUrls(const KCmdLineArgs *args)
{
    KMimeTypeTrader* mimeTrader = KMimeTypeTrader::self();
    KUrl::List notHandledUrls;
    for (int i = 0; i < args->count(); i++) {
        KUrl url = args->url(i);
        KMimeType::Ptr mimetype = KMimeType::findByUrl(url);
        if (mimetype->name() == KMimeType::defaultMimeType()) {
            KMessageBox::error(0, i18nc("@info", "Mimetype for <filename>%1</filename> not found!",
                                        url.prettyUrl()));
            return 1;
        }
        kDebug() << url << mimetype->name();
        /* 
            Find apps marked with Calligra/Applications service type
            and having given mimetype on the X-Calligra-Default-MimeTypes list.
            This is needed because, single mimetype can be supported by more than
            one Calligra application but only one application should be selected
            for opening given document using the calligra command.
            
            Example: application/vnd.oasis.opendocument.graphic, supported
            by Flow and Karbon already; out of these two, Karbon is selected
            for opening the document.
        */
        const QString constraint = QString("'%1' in [X-Calligra-DefaultMimeTypes]").arg(mimetype->name());
        kDebug() << constraint;
        KService::List services;
        KService::List offers = KoServiceLocator::instance()->entries("Calligra/Application");
        foreach(KService::Ptr offer, offers) {
            QStringList defaultMimeTypes = offer->property("X-Calligra-DefaultMimeTypes").toStringList();
            if (defaultMimeTypes.contains(mimetype->name())) {
                services << offer;
            }
        }

        KService::Ptr service;
        //bool ok = false;
        if (!services.isEmpty()) {
            service = services.first();
            kDebug() << "-" << service->name() << service->property("X-Calligra-DefaultMimeTypes");
#if 0
            ok = KRun::run(*service.data(), KUrl::List() << url, 0);
            kDebug() << "KRun::run:" << ok;
#else
            startService(service, url);
            return 0;
#endif
        }
        //if (!ok) {
        // if above not found or app cannot be executed:
        KService::List mimeServices = mimeTrader->query(mimetype->name(), "Calligra/Application");
        kDebug() << "Found" << mimeServices.count() << "services by MimeType field:";
        foreach (KService::Ptr service, mimeServices) {
            //kDebug() << "-" << service->name() << service->property("X-DBUS-ServiceName", QVariant::String);
            kDebug() << "-" << service->name() << service->hasServiceType("Calligra/Application")
                << service->hasMimeType(mimetype->name());
            //QVariant isCalligraApp = service->property("X-Calligra-App", QVariant::Bool);
            /*if (isCalligraApp.isValid() && isCalligraApp.toBool()) {
                kDebug() << "FOUND:" << service->name();
            }*/
        }
        if (mimeServices.isEmpty()) {
            service = mimeTrader->preferredService(mimetype->name());
            kDebug() << "mimeTrader->preferredService():" << service->name();
            if (service) {
                KGuiItem openItem(KStandardGuiItem::open());
                openItem.setText(i18nc("@action:button", "Open with <application>%1</application>",
                                      service->property("Name").toString()));
                if (KMessageBox::Yes != KMessageBox::questionYesNo(
                    0, 
                    i18nc("@info",
                         "Could not find Calligra application suitable for opening <filename>%1</filename>.<nl/>"
                         "Do you want to open the file using default application <application>%2</application>?",
                         url.url(), service->property("Name").toString()),
                    i18nc("@title:window", "Calligra Application Not Found"), openItem, KStandardGuiItem::cancel()))
                {
                    return 2;
                }
            }
        }
        else {
            service = mimeServices.first();
        }
        if (service) {
            startService(service, url);
            return 0;
        }
        else {
            notHandledUrls.append(url);
        }
        //}
    }
    if (!notHandledUrls.isEmpty()) {
        KRun::displayOpenWithDialog(notHandledUrls, 0);
        return 0;
    }
    return 0;
}

int main( int argc, char **argv )
{
    KLocalizedString::setApplicationDomain( "calligra-opener" );

    K4AboutData aboutData("calligra", 0, ki18n("Calligra Opener"), CalligraVersionWrapper::versionString().toLatin1(),
                         ki18n("Calligra Document Opener"),
                         K4AboutData::License_GPL,
                         ki18n("(c) 2010-2011 Calligra developers"));
    aboutData.addAuthor(ki18n("Jarosław Staniek"),KLocalizedString(), "staniek@kde.org");
    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("apps", ki18n("Lists names of all available Calligra applications"));
    options.add("+FILES", ki18n("Files to open"));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if (args->isSet("apps")) {
        listApplicationNames();
        return 0;
    }
    if (args->count() == 0) {
        //! @todo show cool Welcome window for app selection
        showWelcomeWindow();
    }
    else {
        return handleUrls(args);
    }
    return 0;
}
