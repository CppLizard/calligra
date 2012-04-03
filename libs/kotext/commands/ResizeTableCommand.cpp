/*
 This file is part of the KDE project
 * Copyright (C) 2012 C. Boemann <cbo@boemann.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "ResizeTableCommand.h"

#include "KoTableColumnAndRowStyleManager.h"
#include "KoTableColumnStyle.h"
#include "KoTableRowStyle.h"

#include <QTextTableCell>
#include <QTextTable>
#include <QTextCursor>
#include <QTextDocument>

#include <klocale.h>
#include <kdebug.h>

ResizeTableCommand::ResizeTableCommand(QTextTable *t, bool horizontal, int band, qreal size, KUndo2Command *parent) :
    KUndo2Command (parent)
    , m_first(true)
    , m_tablePosition(t->firstPosition())
    , m_document(t->document())
    , m_horizontal(horizontal)
    , m_band(band)
    , m_size(size)
    , m_oldColumnStyle(0)
    , m_oldRowStyle(0)
{
    if (horizontal) {
        setText(i18nc("(qtundo-format)", "Adjust Column Width"));
    } else {
        setText(i18nc("(qtundo-format)", "Adjust Row Height"));
    }
}

ResizeTableCommand::~ResizeTableCommand()
{
    delete m_oldColumnStyle;
    delete m_oldRowStyle;
}

void ResizeTableCommand::undo()
{
    QTextCursor c(m_document);
    c.setPosition(m_tablePosition);
    QTextTable *table = c.currentTable();

    KoTableColumnAndRowStyleManager carsManager = KoTableColumnAndRowStyleManager::getManager(table);

    if (m_oldColumnStyle) {
        carsManager.columnStyle(m_band).copyProperties(m_oldColumnStyle);
    }
    if (m_oldRowStyle) {
        carsManager.rowStyle(m_band).copyProperties(m_oldRowStyle);
    }
    KUndo2Command::undo();
    m_document->markContentsDirty(m_tablePosition, 0);
}

void ResizeTableCommand::redo()
{
    QTextCursor c(m_document);
    c.setPosition(m_tablePosition);
    QTextTable *table = c.currentTable();

    KoTableColumnAndRowStyleManager carsManager = KoTableColumnAndRowStyleManager::getManager(table);

    if (!m_first) {
        if (m_horizontal) {
            carsManager.columnStyle(m_band).copyProperties(m_newColumnStyle);
        } else {
            carsManager.rowStyle(m_band).copyProperties(m_newRowStyle);
        }
        KUndo2Command::redo();
    } else {
        m_first = false;
        if (m_horizontal) {
            m_oldColumnStyle = carsManager.columnStyle(m_band).clone();

            // make sure the style is set (could have been a default style)
            carsManager.setColumnStyle(m_band, carsManager.columnStyle(m_band));

            carsManager.columnStyle(m_band).setColumnWidth(m_size);

            m_newColumnStyle = carsManager.columnStyle(m_band).clone();
        } else {
            m_oldRowStyle = carsManager.rowStyle(m_band).clone();

            // make sure the style is set (could have been a default style)
            carsManager.setRowStyle(m_band, carsManager.rowStyle(m_band));

            carsManager.rowStyle(m_band).setMinimumRowHeight(m_size);

            m_newRowStyle = carsManager.rowStyle(m_band).clone();
        }
    }
    m_document->markContentsDirty(m_tablePosition, 0);
}
