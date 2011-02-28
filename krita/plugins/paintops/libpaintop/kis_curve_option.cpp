/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2008
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
#include "kis_curve_option.h"

KisCurveOption::KisCurveOption(const QString & label, const QString& name, const QString & category, bool checked)
        : m_label(label)
        , m_category(category)
        , m_sensor(0)
        , m_name(name)
        , m_checkable(true)
        , m_checked(checked)
{
    setSensor(KisDynamicSensor::id2Sensor(PressureId.id()));
    setMinimumLabel(i18n("0.0"));
    setMaximumLabel(i18n("1.0"));
}

KisCurveOption::~KisCurveOption()
{
    delete m_sensor;
}

const QString & KisCurveOption::label() const
{
    return m_label;
}

const QString& KisCurveOption::category() const
{
    return m_category;
}

void KisCurveOption::writeOptionSetting(KisPropertiesConfiguration* setting) const
{
    if (m_checkable) {
        setting->setProperty("Pressure" + m_name, isChecked());
    }
    setting->setProperty(QString(m_name + "Sensor"), sensor()->toXML());
}

void KisCurveOption::readOptionSetting(const KisPropertiesConfiguration* setting)
{
    readNamedOptionSetting(m_name, setting);
}

void KisCurveOption::readNamedOptionSetting(const QString& prefix, const KisPropertiesConfiguration* setting)
{
    if (m_checkable) {
        setChecked(setting->getBool("Pressure" + prefix, false));
    }
    bool customCurve = setting->getBool("Custom" + prefix, false);

    KisDynamicSensor* sensor = KisDynamicSensor::createFromXML(setting->getString(QString(prefix + "Sensor")));
    if(sensor) {
        setSensor(sensor);
    }
    if (customCurve) {
        m_sensor->setCurve(setting->getCubicCurve("Curve" + prefix));
    }
}

void KisCurveOption::setSensor(KisDynamicSensor* sensor)
{
    delete m_sensor;
    Q_ASSERT(sensor);
    m_sensor = sensor;
}

KisDynamicSensor* KisCurveOption::sensor() const
{
    return m_sensor;
}

bool KisCurveOption::isCheckable()
{
    return m_checkable;
}

bool KisCurveOption::isChecked() const
{
    return m_checked;
}

void KisCurveOption::setChecked(bool checked)
{
    m_checked = checked;
}


const KisCurveLabel& KisCurveOption::minimumLabel() const
{
    return m_minimumLabel;
}

const KisCurveLabel& KisCurveOption::maximumLabel() const
{
    return m_maximumLabel;
}

void KisCurveOption::setMinimumLabel(const KisCurveLabel& _label)
{
    m_minimumLabel = _label;
}

void KisCurveOption::setMaximumLabel(const KisCurveLabel& _label)
{
    m_maximumLabel = _label;
}
