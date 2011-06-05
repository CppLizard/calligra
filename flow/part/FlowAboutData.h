/* This file is part of the KDE project
   Copyright (C)  2006 Peter Simonsson <peter.simonsson@gmail.com>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef FLOWABOUTDATA_H
#define FLOWABOUTDATA_H

#include <kaboutdata.h>
#include <klocale.h>
#include <kofficeversion.h>

static const char* FLOW_DESCRIPTION = I18N_NOOP("Calligra Flowchart And Diagram Tool");
static const char* FLOW_VERSION = KOFFICE_VERSION_STRING;

KAboutData* newFlowAboutData()
{
  KAboutData* aboutData = new KAboutData("flow", 0, ki18nc("application name", "Flow"),
                                FLOW_VERSION, ki18n(FLOW_DESCRIPTION), KAboutData::License_LGPL,
                                ki18n("(C) 2000-2010, The Flow Team"), KLocalizedString(),
                                "http://www.calligra-suite.org/flow/");
  aboutData->setProductName("calligra-flow"); // for bugs.kde.org
  aboutData->addAuthor(ki18n("Yue Liu"), ki18n("Maintainer"), "opuspace@gmail.com");

  return aboutData;
}

#endif
