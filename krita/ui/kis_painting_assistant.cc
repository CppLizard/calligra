/*
 *  Copyright (c) 2008,2011 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2010 Geoffry Song <goffrie@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_painting_assistant.h"
#include "kis_debug.h"

#include <kglobal.h>
#include <QPen>
#include <QPainter>

struct KisPaintingAssistantHandle::Private {
    QList<KisPaintingAssistant*> assistants;
};

KisPaintingAssistantHandle::KisPaintingAssistantHandle(double x, double y) : QPointF(x, y), d(new Private)
{
}
KisPaintingAssistantHandle::KisPaintingAssistantHandle(QPointF p) : QPointF(p), d(new Private)
{
}

KisPaintingAssistantHandle::KisPaintingAssistantHandle(const KisPaintingAssistantHandle& rhs)
    : QPointF(rhs)
    , KisShared()
    , d(new Private)
{
}

KisPaintingAssistantHandle& KisPaintingAssistantHandle::operator=(const QPointF &  pt)
{
    setX(pt.x());
    setY(pt.y());
    return *this;
}

KisPaintingAssistantHandle::~KisPaintingAssistantHandle()
{
    Q_ASSERT(d->assistants.empty());
    delete d;
}

void KisPaintingAssistantHandle::registerAssistant(KisPaintingAssistant* assistant)
{
    Q_ASSERT(!d->assistants.contains(assistant));
    d->assistants.append(assistant);
}

void KisPaintingAssistantHandle::unregisterAssistant(KisPaintingAssistant* assistant)
{
    d->assistants.removeOne(assistant);
    Q_ASSERT(!d->assistants.contains(assistant));
}

bool KisPaintingAssistantHandle::containsAssistant(KisPaintingAssistant* assistant)
{
    return d->assistants.contains(assistant);
}

void KisPaintingAssistantHandle::mergeWith(KisPaintingAssistantHandleSP handle)
{
    foreach(KisPaintingAssistant* assistant, handle->d->assistants) {
        if (!assistant->handles().contains(this)) {
            assistant->replaceHandle(handle, this);
        }
    }
}

QList<KisPaintingAssistantHandleSP> KisPaintingAssistantHandle::split()
{
    QList<KisPaintingAssistantHandleSP> newHandles;
    foreach(KisPaintingAssistant* assistant, d->assistants) {
        KisPaintingAssistantHandleSP newHandle(new KisPaintingAssistantHandle(*this));
        newHandles.append(newHandle);
        assistant->replaceHandle(this, newHandle);
    }
    return newHandles;
}


struct KisPaintingAssistant::Private {
    QString id;
    QString name;
    QList<KisPaintingAssistantHandleSP> handles;
};

KisPaintingAssistant::KisPaintingAssistant(const QString& id, const QString& name) : d(new Private)
{
    d->id = id;
    d->name = name;
}

void KisPaintingAssistant::drawPath(QPainter& painter, const QPainterPath &path)
{
    painter.save();
    QPen pen_a(QColor(0, 0, 0, 100), 2);
    pen_a.setCosmetic(true);
    painter.setPen(pen_a);
    painter.drawPath(path);
    QPen pen_b(Qt::white, 0.9);
    pen_b.setCosmetic(true);
    painter.setPen(pen_b);
    painter.drawPath(path);
    painter.restore();
}

void KisPaintingAssistant::initHandles(QList<KisPaintingAssistantHandleSP> _handles)
{
    Q_ASSERT(d->handles.isEmpty());
    d->handles = _handles;
    foreach(KisPaintingAssistantHandleSP handle, _handles) {
        handle->registerAssistant(this);
    }
}

KisPaintingAssistant::~KisPaintingAssistant()
{
    foreach(KisPaintingAssistantHandleSP handle, d->handles) {
        handle->unregisterAssistant(this);
    }
    delete d;
}

const QString& KisPaintingAssistant::id() const
{
    return d->id;
}

const QString& KisPaintingAssistant::name() const
{
    return d->name;
}

void KisPaintingAssistant::replaceHandle(KisPaintingAssistantHandleSP _handle, KisPaintingAssistantHandleSP _with)
{
    Q_ASSERT(d->handles.contains(_handle));
    d->handles.replace(d->handles.indexOf(_handle), _with);
    Q_ASSERT(!d->handles.contains(_handle));
    _handle->unregisterAssistant(this);
    _with->registerAssistant(this);
}

void KisPaintingAssistant::addHandle(KisPaintingAssistantHandleSP handle)
{
    Q_ASSERT(!d->handles.contains(handle));
    d->handles.append(handle);
    handle->registerAssistant(this);
}

const QList<KisPaintingAssistantHandleSP>& KisPaintingAssistant::handles() const
{
    return d->handles;
}

QList<KisPaintingAssistantHandleSP> KisPaintingAssistant::handles()
{
    return d->handles;
}

KisPaintingAssistantFactory::KisPaintingAssistantFactory()
{
}

KisPaintingAssistantFactory::~KisPaintingAssistantFactory()
{
}

KisPaintingAssistantFactoryRegistry::KisPaintingAssistantFactoryRegistry()
{
}

KisPaintingAssistantFactoryRegistry::~KisPaintingAssistantFactoryRegistry()
{
    foreach(QString id, keys()) {
        delete get(id);
    }
    dbgRegistry << "deleting KisPaintingAssistantFactoryRegistry ";
}

KisPaintingAssistantFactoryRegistry* KisPaintingAssistantFactoryRegistry::instance()
{
    K_GLOBAL_STATIC(KisPaintingAssistantFactoryRegistry, s_instance);
    return s_instance;
}

