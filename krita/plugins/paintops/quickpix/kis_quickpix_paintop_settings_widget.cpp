/*
 *  Copyright (c) 2012 José Luis Vergara <pentalis@gmail.com>
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

#include "kis_quickpix_paintop_settings_widget.h"

#include "kis_quickpix_paintop_options.h"
#include "kis_quickpix_paintop_settings.h"

#include <kis_paintop_options_widget.h>

KisQuickPixPaintOpSettingsWidget:: KisQuickPixPaintOpSettingsWidget(QWidget* parent)
        : KisBrushBasedPaintopOptionWidget(parent)
{
    m_quickpixOption =  new KisQuickPixOpOption();
    addPaintOpOption(m_quickpixOption);
}

KisQuickPixPaintOpSettingsWidget::~ KisQuickPixPaintOpSettingsWidget()
{
}

KisPropertiesConfiguration*  KisQuickPixPaintOpSettingsWidget::configuration() const
{
    KisQuickPixPaintOpSettings* config = new KisQuickPixPaintOpSettings();
    config->setOptionsWidget(const_cast<KisQuickPixPaintOpSettingsWidget*>(this));
    config->setProperty("paintop", "quickpixbrush"); // XXX: make this a const id string
    writeConfiguration(config);
    return config;
}
