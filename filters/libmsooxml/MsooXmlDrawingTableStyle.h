/* This file is part of the KDE project
 * Copyright (C) 2010 Carlos Licea <carlos@kdab.com>
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

#ifndef MSOOXMLDRAWINGTABLESTYLE_H
#define MSOOXMLDRAWINGTABLESTYLE_H

#include "msooxml_export.h"

#include <KoCellStyle.h>

#include <QFlags>

/**
 * The idea behind these classes is the following:
 * > A document has a list of table styles identifiable by ID.
 * > A table style has a number of properties to be used if the
 * table that references the style toogles them on.
 * > Those are stored on a table style properties.
 *
 * > Now the way a style for a cell is composed can be quite complex
 * depending on a lot of things. Mainly:
 *  > The properties toggled and their precedence,
 *    the rule of thumb for the precedence is that it's higher
 *    the more more specific it is.
 *  > The position in which the cell is. The styles have a
 *    particularly tricky property: borders. The styles can
 *    specify (in the same style) the style for a border
 *    depending whether is in the outside of the table or
 *    if it's an inside border. That's why the size of the
 *    table is needed.
 * For these reasons we don't apply styles directly but we instantiate
 * them for a specific table with a specific togglers for styles and
 * a size.
 */

namespace MSOOXML
{

/// Reading and storage

struct MSOOXML_EXPORT TableStyleProperties
{
    enum Property {
        BottomBorder = 1,
        InsideHBorder = 2,
        InsideVBorder = 4,
        LeftBorder = 8,
        RightBorder = 16,
        Tl2brBorder = 32,
        TopBorder = 64,
        Tr2blBorder = 128,
        BackgroundColor = 256
    };
    Q_DECLARE_FLAGS(Properties, Property)
    Properties setProperties;

    KoBorder::BorderData bottom;
    KoBorder::BorderData insideH;
    KoBorder::BorderData insideV;
    KoBorder::BorderData left;
    KoBorder::BorderData right;
    KoBorder::BorderData tl2br;
    KoBorder::BorderData top;
    KoBorder::BorderData tr2bl;

    QColor backgroundColor;
};

class MSOOXML_EXPORT TableStyle
{
public:
    enum Type {
        NoType,
        FirstRow,
        FirstCol,
        LastCol,
        LastRow,
        NeCell,
        NwCell,
        SeCell,
        SwCell,
        Band1Horizontal,
        Band2Horizontal,
        Band1Vertical,
        Band2Vertical,
        WholeTbl
    };

    TableStyle();
    ~TableStyle();

    void setId(const QString& id);
    QString id() const;

    void addProperties(Type type, TableStyleProperties* properties);
    TableStyleProperties* properties(Type type) const;

private:
    QString m_id;
    QMap<Type, TableStyleProperties*> m_properties;
    //TODO handle the table background stored in the element TblBg
};

class MSOOXML_EXPORT TableStyleList
{
public:
    TableStyleList();
    ~TableStyleList();

    TableStyle tableStyle(const QString& id) const;
    void insertStyle(QString id, MSOOXML::TableStyle style);

private:
    QMap<QString, TableStyle> m_styles;
};

/// Instantiation classes

class MSOOXML_EXPORT LocalTableStyles
{
public:
    LocalTableStyles();
    ~LocalTableStyles();

    TableStyleProperties* localStyle(int row, int column);
    void setLocalStyle(MSOOXML::TableStyleProperties* properties, int row, int column);

private:
    QMap<QPair<int,int>, TableStyleProperties*> m_properties;
};

class MSOOXML_EXPORT TableStyleInstanceProperties
{
    friend class TableStyleInstance;
public:
    TableStyleInstanceProperties(int rowCount, int columnCount);
    ~TableStyleInstanceProperties();

    TableStyleInstanceProperties& rowBandSize(int size);
    TableStyleInstanceProperties& columnBandSize(int size);
    TableStyleInstanceProperties& localStyles(const LocalTableStyles& localStyles);

    enum Role {
        FirstRow = 1,
        FirstCol = 2,
        LastCol = 4,
        LastRow = 8,
        NeCell = 16,
        NwCell = 32,
        SeCell = 64,
        SwCell = 128,
        RowBanded = 256,
        ColumnBanded = 512,
        WholeTbl = 1024
    };
    Q_DECLARE_FLAGS(Roles, Role)

    TableStyleInstanceProperties& roles(Roles roles);

private:
    int m_rowCount;
    int m_columnCount;
    int m_rowBandSize;
    int m_columnBandSize;
    Roles m_role;
    LocalTableStyles m_localStyles;
};

class MSOOXML_EXPORT TableStyleInstance
{
public:
    TableStyleInstance(TableStyle* style, TableStyleInstanceProperties properties);
    ~TableStyleInstance();

    KoCellStyle::Ptr style(int row, int column);

private:
    void applyStyle(MSOOXML::TableStyleProperties* styleProperties, KoCellStyle::Ptr& style, int row, int column);
    void applyStyle(MSOOXML::TableStyle::Type type, KoCellStyle::Ptr& style, int row, int column);
    void applyBordersStyle(MSOOXML::TableStyleProperties* styleProperties, KoCellStyle::Ptr& style, int row, int column);
    void applyBackground(MSOOXML::TableStyleProperties* styleProperties, KoCellStyle::Ptr& style, int row, int column);

    TableStyle* m_style;
    TableStyleInstanceProperties m_properties;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(MSOOXML::TableStyleInstanceProperties::Roles)
Q_DECLARE_OPERATORS_FOR_FLAGS(MSOOXML::TableStyleProperties::Properties)

#endif
