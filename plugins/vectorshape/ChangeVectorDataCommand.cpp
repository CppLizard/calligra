/* This file is part of the KDE project
   Copyright 2009 Thorsten Zachmann <zachmann@kde.org>
   Copyright 2011 Boudewijn Rempt <boud@valdyas.org>

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
#include "ChangeVectorDataCommand.h"

#include <math.h>
#include <klocale.h>
#include <KoImageData.h>
#include <KDebug>

#include "VectorShape.h"

ChangeVectorDataCommand::ChangeVectorDataCommand(VectorShape *shape, QByteArray &newImageData, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_shape(shape)
{
    Q_ASSERT( shape );
    m_oldImageData = m_shape->compressedContents();
    m_newImageData = newImageData;
    setText(i18n("Change Vector Data"));
}

ChangeVectorDataCommand::~ChangeVectorDataCommand()
{
}

void ChangeVectorDataCommand::redo()
{
    m_shape->update();
    m_shape->setCompressedContents(m_newImageData);
    m_shape->update();
}

void ChangeVectorDataCommand::undo()
{
    m_shape->update();
    m_shape->setCompressedContents(m_oldImageData);
    m_shape->update();
}
