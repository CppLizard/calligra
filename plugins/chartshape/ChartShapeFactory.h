/* This file is part of the KDE project
   Copyright (C) 2018 Dag Andersen <danders@get2net.dk>
   Copyright 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

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

#ifndef KCHART_CHART_SHAPE_FACTORY
#define KCHART_CHART_SHAPE_FACTORY


// Qt
#include <QStringList>

// Calligra
#include <KoShapeFactoryBase.h>
#include <QVariantList>


class KoShape;
class KoShapeConfigWidgetBase;

namespace KoChart {
    class ChartShape;
}

class ChartShapePlugin : public QObject
{
    Q_OBJECT
public:

    ChartShapePlugin(QObject *parent, const QVariantList&);
    ~ChartShapePlugin() {}
};


class ChartShapeFactory : public KoShapeFactoryBase
{
public:
    ChartShapeFactory();
    ~ChartShapeFactory() {}

    bool supports(const KoXmlElement &element, KoShapeLoadingContext &context) const;

    KoShape *createShape(const KoProperties* properties, KoDocumentResourceManager *documentResources) const;

    virtual KoShape *createDefaultShape(KoDocumentResourceManager *documentResources = 0) const;
    // reimplemented to not create a default shape to just overwrite it afterwards
    virtual KoShape *createShapeFromOdf(const KoXmlElement &element, KoShapeLoadingContext &context);
    virtual void newDocumentResourceManager(KoDocumentResourceManager *manager) const;

    QList<KoShapeConfigWidgetBase*> createShapeOptionPanels();

private:
    KoChart::ChartShape *createBarChart(KoDocumentResourceManager *documentResources, int subtype) const;
    KoChart::ChartShape *createLineChart(KoDocumentResourceManager *documentResources, int subtype) const;
    KoChart::ChartShape *createAreaChart(KoDocumentResourceManager *documentResources, int subtype) const;
    KoChart::ChartShape *createStockChart(KoDocumentResourceManager *documentResources, int subtype) const;
    KoChart::ChartShape *createPieChart(KoDocumentResourceManager *documentResources) const;
    KoChart::ChartShape *createRingChart(KoDocumentResourceManager *documentResources) const;
    KoChart::ChartShape *createBubbleChart(KoDocumentResourceManager *documentResources) const;
    KoChart::ChartShape *createScatterChart(KoDocumentResourceManager *documentResources) const;
    KoChart::ChartShape *createRadarChart(KoDocumentResourceManager *documentResources) const;
    KoChart::ChartShape *createFilledRadarChart(KoDocumentResourceManager *documentResources) const;

    void radarData(KoChart::ChartShape *shape) const;

};


#endif // KCHART_CHART_SHAPE_FACTORY
