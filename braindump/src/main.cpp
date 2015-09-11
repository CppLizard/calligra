/*
 *  Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */



#include <QApplication>
#include <QCommandLineParser>

#include <kiconloader.h>
#include <memory>

#include "AboutData.h"
#include "MainWindow.h"
#include "KoGlobal.h"
#include "RootSection.h"
#include "SectionsIO.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KAboutData about = newBrainDumpAboutData();
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(app);

    about.setupCommandLine(&parser);
    about.processCommandLine(&parser);

    KIconLoader::global()->addAppDir("calligra");
    KoGlobal::initialize();

    RootSection* doc = new RootSection;

    MainWindow* window = new MainWindow(doc);
    window->setVisible(true);

    app.exec();

    // Ensure the root section is saved
    doc->sectionsIO()->save();

    delete doc;
    app.exit(0);
}
