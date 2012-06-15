/* This file is part of the KDE project
 *
 * Copyright (C) 2012 Inge Wallin <inge@lysator.liu.se>
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

#ifndef OBJECTS_H
#define OBJECTS_H

// Qt
//#include <QColor>
#include <QVector3D>

// Calligra
#include <KoShape.h>


class KoXmlElement;
class KoXmlWriter;


class Object3D : public KoShape
{
public:
    Object3D();
    virtual ~Object3D();

    virtual void paint(QPainter &painter, const KoViewConverter &converter,
                       KoShapePaintingContext &context);
    virtual bool loadOdf(const KoXmlElement &objectElement, KoShapeLoadingContext &context);
    virtual void saveOdf(KoShapeSavingContext &context) const;

    // getters

protected:
    char *m_elementName;  // For saving. includes namespace
};


class Sphere : public Object3D
{
public:
    Sphere();
    virtual ~Sphere();

    virtual void paint(QPainter &painter, const KoViewConverter &converter,
                       KoShapePaintingContext &context);
    virtual bool loadOdf(const KoXmlElement &objectElement, KoShapeLoadingContext &context);
    virtual void saveOdf(KoShapeSavingContext &context) const;

    // getters
    QVector3D sphereCenter() const { return m_center; }
    QVector3D sphereSize()   const { return m_size;   }

private:
    QVector3D  m_center;
    QVector3D  m_size;
};

class Cube : public Object3D
{
public:
    Cube();
    virtual ~Cube();

    virtual void paint(QPainter &painter, const KoViewConverter &converter,
                       KoShapePaintingContext &context);
    virtual bool loadOdf(const KoXmlElement &objectElement, KoShapeLoadingContext &context);
    virtual void saveOdf(KoShapeSavingContext &context) const;

    // getters
    QVector3D minEdge() const { return m_minEdge;   }
    QVector3D maxEdge() const { return m_maxEdge; }

private:
    QVector3D  m_minEdge;
    QVector3D  m_maxEdge;
};

#endif
