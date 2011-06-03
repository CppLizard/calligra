/*
 *  Main file of animator plugin: dock factories, etc
 *  Copyright (C) 2011 Torio Mlshi <mlshi@lavabit.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "animator.h"

#include <stdlib.h>

#include <QTimer>

#include <kactioncollection.h>
#include <kcomponentdata.h>
#include <kis_debug.h>
#include <kpluginfactory.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <KoDockFactoryBase.h>
#include <KoDockRegistry.h>


#include "kis_config.h"
#include "kis_cursor.h"
#include "kis_global.h"
#include "kis_types.h"
#include "kis_view2.h"

#include "animator_dock.h"
#include "animator_light_table_dock.h"

K_PLUGIN_FACTORY(AnimatorPluginFactory, registerPlugin<AnimatorPlugin>();)
K_EXPORT_PLUGIN(AnimatorPluginFactory("krita"))

class AnimatorDockFactory : public KoDockFactoryBase {
public:
    AnimatorDockFactory()
    {
    }

    virtual QString id() const
    {
        return QString( "Animator" );
    }

    virtual Qt::DockWidgetArea defaultDockWidgetArea() const
    {
        return Qt::BottomDockWidgetArea;
    }

    virtual QDockWidget* createDockWidget()
    {
        AnimatorDock * dockWidget = new AnimatorDock();
        
        dockWidget->setObjectName(id());

        return dockWidget;
    }

    DockPosition defaultDockPosition() const
    {
        return DockMinimized;
    }
private:
};

class LightTableDockFactory : public KoDockFactoryBase {
public:
    LightTableDockFactory()
    {
    }
    
    virtual QString id() const
    {
        return QString( "Light table" );
    }
    
    virtual Qt::DockWidgetArea defaultDockWidgetArea() const
    {
        return Qt::RightDockWidgetArea;
    }
    
    virtual QDockWidget* createDockWidget()
    {
        AnimatorLightTableDock* dockWidget = new AnimatorLightTableDock();
        
        dockWidget->setObjectName(id());

        return dockWidget;
    }

    DockPosition defaultDockPosition() const
    {
        return DockMinimized;
    }
};


AnimatorPlugin::AnimatorPlugin(QObject *parent, const QVariantList &)
    : KParts::Plugin(parent)
{
    KoDockRegistry::instance()->add(new AnimatorDockFactory());
    KoDockRegistry::instance()->add(new LightTableDockFactory());
}

AnimatorPlugin::~AnimatorPlugin()
{
}

#include "animator.moc"
