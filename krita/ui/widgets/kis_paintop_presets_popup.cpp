/* This file is part of the KDE project
 * Copyright (C) 2008 Boudewijn Rempt <boud@valdyas.org>
 * Copyright (C) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 * Copyright (C) 2011 Silvio Heinrich <plassy@web.de>
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

#include "widgets/kis_paintop_presets_popup.h"

#include <QList>
#include <QComboBox>
#include <QHBoxLayout>
#include <QToolButton>
#include <QGridLayout>
#include <QFont>
#include <QMenu>
#include <QAction>

#include <kconfig.h>
#include <kglobalsettings.h>
#include <kicon.h>
#include <klocale.h>

#include <KoColorSpaceRegistry.h>

#include <kis_paintop_preset.h>
#include <kis_paintop_settings_widget.h>
#include <kis_canvas_resource_provider.h>
#include <widgets/kis_preset_chooser.h>

#include <ui_wdgpaintopsettings.h>
#include <kis_node.h>
#include "kis_config.h"


class KisPaintOpPresetsPopup::Private
{

public:

    Ui_WdgPaintOpSettings uiWdgPaintOpPresetSettings;
    QGridLayout *layout;
    KisPaintOpSettingsWidget *settingsWidget;
    QFont smallFont;
    KisCanvasResourceProvider *resourceProvider;
    bool detached;
};

KisPaintOpPresetsPopup::KisPaintOpPresetsPopup(KisCanvasResourceProvider * resourceProvider, QWidget * parent)
    : QWidget(parent)
    , m_d(new Private())
{
    setObjectName("KisPaintOpPresetsPopup");
    KConfigGroup group(KGlobal::config(), "GUI");
    m_d->smallFont  = KGlobalSettings::generalFont();
    qreal pointSize = group.readEntry("palettefontsize", m_d->smallFont.pointSize() * 0.75);
    pointSize = qMax(pointSize, KGlobalSettings::smallestReadableFont().pointSizeF());
    m_d->smallFont.setPointSizeF(pointSize);
    setFont(m_d->smallFont);

    m_d->resourceProvider = resourceProvider;

    m_d->uiWdgPaintOpPresetSettings.setupUi(this);

    m_d->layout = new QGridLayout(m_d->uiWdgPaintOpPresetSettings.frmOptionWidgetContainer);
    m_d->layout->setSizeConstraint(QLayout::SetFixedSize);

    m_d->uiWdgPaintOpPresetSettings.scratchPad->setCanvasColor(Qt::white);
    m_d->uiWdgPaintOpPresetSettings.scratchPad->setColorSpace(KoColorSpaceRegistry::instance()->rgb8());
    m_d->uiWdgPaintOpPresetSettings.scratchPad->setCutoutOverlay(QRect(25, 25, 200, 200));
    m_d->uiWdgPaintOpPresetSettings.fillLayer->setIcon(KIcon("newlayer"));
    m_d->uiWdgPaintOpPresetSettings.fillGradient->setIcon(KIcon("krita_tool_gradient"));
    m_d->uiWdgPaintOpPresetSettings.fillSolid->setIcon(KIcon("krita_tool_color_fill"));
    m_d->uiWdgPaintOpPresetSettings.eraseScratchPad->setIcon(KIcon("list-remove"));

    connect(m_d->uiWdgPaintOpPresetSettings.eraseScratchPad, SIGNAL(clicked()),
            m_d->uiWdgPaintOpPresetSettings.scratchPad, SLOT(clear()));

    connect(m_d->resourceProvider, SIGNAL(sigFGColorChanged(const KoColor &)),
            m_d->uiWdgPaintOpPresetSettings.scratchPad, SLOT(setPaintColor(const KoColor &)));

    connect(m_d->resourceProvider, SIGNAL(sigBGColorChanged(const KoColor &)),
            m_d->uiWdgPaintOpPresetSettings.scratchPad, SLOT(setBackgroundColor(const KoColor &)));

    connect(m_d->uiWdgPaintOpPresetSettings.fillLayer, SIGNAL(clicked()),
            this, SLOT(fillScratchPadLayer()));

    connect(m_d->uiWdgPaintOpPresetSettings.fillGradient, SIGNAL(clicked()),
            this, SLOT(fillScratchPadGradient()));

    connect(m_d->uiWdgPaintOpPresetSettings.fillSolid, SIGNAL(clicked()),
            this, SLOT(fillScratchPadSolid()));

    m_d->settingsWidget = 0;
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    connect(m_d->uiWdgPaintOpPresetSettings.bnSave, SIGNAL(clicked()),
            this, SIGNAL(savePresetClicked()));
            
    connect(m_d->uiWdgPaintOpPresetSettings.bnDefaultPreset, SIGNAL(clicked()),
            this, SIGNAL(defaultPresetClicked()));
    
    connect(m_d->uiWdgPaintOpPresetSettings.txtPreset, SIGNAL(textChanged(QString)),
            this, SIGNAL(presetNameLineEditChanged(QString)));
            
    connect(m_d->uiWdgPaintOpPresetSettings.paintopList, SIGNAL(activated(const QString&)),
            this, SIGNAL(paintopActivated(QString)));

            
    KisConfig cfg;
    m_d->detached = !cfg.paintopPopupDetached();

}


KisPaintOpPresetsPopup::~KisPaintOpPresetsPopup()
{
    if (m_d->settingsWidget)
    {
        m_d->layout->removeWidget(m_d->settingsWidget);
        m_d->settingsWidget->hide();
        m_d->settingsWidget->setParent(0);
        m_d->settingsWidget = 0;
    }
    delete m_d;
}

void KisPaintOpPresetsPopup::slotCheckPresetValidity()
{
    if (m_d->settingsWidget){
        m_d->uiWdgPaintOpPresetSettings.bnSave->setEnabled( m_d->settingsWidget->presetIsValid() );
        m_d->uiWdgPaintOpPresetSettings.txtPreset->setEnabled( m_d->settingsWidget->presetIsValid() );
    }
}


void KisPaintOpPresetsPopup::setPaintOpSettingsWidget(QWidget * widget)
{
    if (m_d->settingsWidget) {
        m_d->layout->removeWidget(m_d->settingsWidget);
        m_d->uiWdgPaintOpPresetSettings.frmOptionWidgetContainer->updateGeometry();
    }
    m_d->layout->update();
    updateGeometry();

    m_d->settingsWidget = static_cast<KisPaintOpSettingsWidget*>(widget);
    if (m_d->settingsWidget){
        connect(m_d->settingsWidget,SIGNAL(sigConfigurationItemChanged()),this,SLOT(slotCheckPresetValidity()));
        slotCheckPresetValidity();
        if (m_d->settingsWidget->supportScratchBox()){
            showScratchPad();
        }else{
            hideScratchPad();
        }
    }

    if (widget) {
        widget->setFont(m_d->smallFont);

        widget->setMinimumSize(QSize(750, 450));
        m_d->settingsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
        m_d->layout->addWidget(widget);

        m_d->layout->update();
        widget->show();
    }
}

void KisPaintOpPresetsPopup::changeSavePresetButtonText(bool change)
{
    QPalette palette;
    
    if (change) {
        palette.setColor(QPalette::Base, QColor(255,200,200));
        m_d->uiWdgPaintOpPresetSettings.bnSave->setText(i18n("Overwrite Preset"));
        m_d->uiWdgPaintOpPresetSettings.txtPreset->setPalette(palette);
    }
    else {
        m_d->uiWdgPaintOpPresetSettings.bnSave->setText(i18n("Save to Presets"));
        m_d->uiWdgPaintOpPresetSettings.txtPreset->setPalette(palette);
    }
}


QString KisPaintOpPresetsPopup::getPresetName() const
{
    return m_d->uiWdgPaintOpPresetSettings.txtPreset->text();
}

void KisPaintOpPresetsPopup::setPreset(KisPaintOpPresetSP preset)
{
    m_d->uiWdgPaintOpPresetSettings.scratchPad->setPreset(preset);
}

QImage KisPaintOpPresetsPopup::cutOutOverlay()
{
    return m_d->uiWdgPaintOpPresetSettings.scratchPad->cutoutOverlay();
}

void KisPaintOpPresetsPopup::fillScratchPadGradient()
{
    m_d->uiWdgPaintOpPresetSettings.scratchPad->fillGradient(m_d->resourceProvider->currentGradient());
}

void KisPaintOpPresetsPopup::fillScratchPadSolid()
{
    m_d->uiWdgPaintOpPresetSettings.scratchPad->fillSolid(m_d->resourceProvider->bgColor());
}

void KisPaintOpPresetsPopup::fillScratchPadLayer()
{
    //TODO
}

void KisPaintOpPresetsPopup::contextMenuEvent(QContextMenuEvent *e) {

    QMenu menu(this);
    QAction* action = menu.addAction(m_d->detached ? i18n("Attach to Toolbar") : i18n("Detach from Toolbar"));
    connect(action, SIGNAL(triggered()), this, SLOT(switchDetached()));
    menu.exec(e->globalPos());
}

void KisPaintOpPresetsPopup::switchDetached()
{
    if (parentWidget()) {

        m_d->detached = !m_d->detached;
        if (m_d->detached) {
            parentWidget()->setWindowFlags(Qt::Tool);
            parentWidget()->show();
        }
        else {
            parentWidget()->setWindowFlags(Qt::Popup);
        }

        KisConfig cfg;
        cfg.setPaintopPopupDetached(m_d->detached);
    }
}

void KisPaintOpPresetsPopup::hideScratchPad()
{
    m_d->uiWdgPaintOpPresetSettings.scratchPad->setVisible(false);
}


void KisPaintOpPresetsPopup::showScratchPad()
{
    m_d->uiWdgPaintOpPresetSettings.scratchPad->setVisible(true);
}

void KisPaintOpPresetsPopup::resourceSelected(KoResource* resource)
{
	m_d->uiWdgPaintOpPresetSettings.txtPreset->setText(resource->name());
}

void KisPaintOpPresetsPopup::setPaintOpList(const QList< KisPaintOpFactory* >& list)
{
   m_d->uiWdgPaintOpPresetSettings.paintopList->setPaintOpList(list);
}

void KisPaintOpPresetsPopup::setCurrentPaintOp(const QString& paintOpId)
{
    m_d->uiWdgPaintOpPresetSettings.paintopList->setCurrent(paintOpId);
}

QString KisPaintOpPresetsPopup::currentPaintOp()
{
    return m_d->uiWdgPaintOpPresetSettings.paintopList->currentItem();
}

#include "kis_paintop_presets_popup.moc"
