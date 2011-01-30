/* This file is part of the KDE project
 * Copyright (C) 2010-2011 Carlos Licea <carlos@kdab.com>
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

#ifndef MSOOXMLDOCUMENTTABLESTYLE_H
#define MSOOXMLDOCUMENTTABLESTYLE_H

#include "MsooXmlTableStyle.h"

#include <msooxml_export.h>

namespace MSOOXML {

class MSOOXML_EXPORT DocumentTableStyle : public TableStyle
{
public:
    DocumentTableStyle();
    virtual ~DocumentTableStyle();

    QString baseStyleName() const;
    void setBaseStyleName(QString base);

    TableStyleProperties* properties() const;
    /// the style takes ownership of the properties
    void setProperties(TableStyleProperties* properties);

private:
    QString m_baseStyleName;
    TableStyleProperties* m_properties;
};

class MSOOXML_EXPORT DocumentTableStyleConverterProperties : public TableStyleConverterProperties
{
public:
    DocumentTableStyleConverterProperties();
    virtual ~DocumentTableStyleConverterProperties();

    QMap<QString, DocumentTableStyle*> styleList() const;
    void setStyleList(QMap<QString, DocumentTableStyle*> styleList);

private:
    QMap<QString, DocumentTableStyle*> m_styleList;
};

class MSOOXML_EXPORT DocumentTableStyleConverter : public TableStyleConverter
{
public:
    DocumentTableStyleConverter(DocumentTableStyleConverterProperties properties, DocumentTableStyle* style =0);
    virtual ~DocumentTableStyleConverter();

    virtual KoCellStyle::Ptr style(int row, int column);
    void applyBasedStylesProperties(MSOOXML::DocumentTableStyle* style, KoCellStyle::Ptr& odfStyle, int row, int column);

private:
    DocumentTableStyleConverterProperties m_properties;
    DocumentTableStyle* m_style;
};

}

#endif
