/* This file is part of the KDE project
   Copyright 2010 Gopalakrishna Bhat A <gopalakbhat@gmail.com>

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

#include "RemoveSheet.h"

using Calligra::Tables::Sheet;
using Calligra::Tables::Map;

RemoveSheet::RemoveSheet(Calligra::Tables::Sheet* s)
{
    sheet = s;
    map = sheet->map();
    setText(i18n("Remove Sheet"));
}

void RemoveSheet::redo()
{
    sheet->map()->removeSheet(sheet);
}

void RemoveSheet::undo()
{
    sheet->map()->reviveSheet(sheet);
}

