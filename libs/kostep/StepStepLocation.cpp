/*
    kostep -- handles changetracking using operational transformation for calligra
    Copyright (C) 2013  Luke Wolf <Lukewolf101010@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "StepStepLocation.h"
#include "StepStepLocation_p.h"
#include <QtGui/QTextCursor>
#include <QDebug>
StepStepLocation::StepStepLocation(QObject *parent) :
    QObject(parent), d(new StepStepLocationPrivate())
{
}
StepStepLocation::StepStepLocation(QTextCursor cursor, QObject *parent):
    QObject(parent), d(new StepStepLocationPrivate())
{
    d->constructor(cursor);
}
StepStepLocation::~StepStepLocation()
{
    delete d;
}

QTextCursor StepStepLocation::toQTextCursor(QTextDocument *document)
{
    return d->convertToQTextCursor(document);
}

StepStepLocation* StepStepLocation::operator=(const StepStepLocation &other)
{
    d->location = other.d->location;
    return this;

}
StepStepLocation::StepStepLocation(const StepStepLocation& locate): QObject(),d(new StepStepLocationPrivate())
{
    d->location = locate.d->location;

}
void StepStepLocation::fromString(QString string)
{
    string.remove("s=\"");
    string.remove(string.length()-1,1);
    QStringList locationList = string.split("/");
    while(!locationList.isEmpty())
    {
        d->location.push(locationList.last().toInt());
        locationList.removeLast();
    }
}

QString StepStepLocation::toString ()
{
    return d->toString();
}
