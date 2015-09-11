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

#ifndef BRAINDUMPABOUTDATA_H
#define BRAINDUMPABOUTDATA_H

#include <kaboutdata.h>
#include <KLocalizedString>

static const char BRAINDUMP_DESCRIPTION[] = "Braindump: directly from your brain to the computer.";
static const char BRAINDUMP_VERSION[] = "0.10.9";

inline KAboutData newBrainDumpAboutData()
{
    KAboutData aboutData("braindump", ki18n("Braindump").toString(), BRAINDUMP_VERSION, ki18n(BRAINDUMP_DESCRIPTION).toString(),
                         KAboutLicense::LGPL, ki18n("(c) 2009, 2010, 2011, 2012, 2013 Cyrille Berger").toString());
    return aboutData;
}

#endif
