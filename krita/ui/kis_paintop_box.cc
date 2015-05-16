/*
 *  kis_paintop_box.cc - part of KImageShop/Krayon/Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
 *  Copyright (c) 2009-2011 Sven Langkamp (sven.langkamp@gmail.com)
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (C) 2011 Silvio Heinrich <plassy@web.de>
 *  Copyright (C) 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
 *  Copyright (c) 2014 Mohit Goyal <mohit.bits2011@gmail.com>
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

#include "kis_paintop_box.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QAction>
#include <QPixmap>

#include <kis_debug.h>

#include <kactioncollection.h>
#include <kacceleratormanager.h>

#include <KoIcon.h>
#include <KoColorSpace.h>
#include <KoCompositeOpRegistry.h>
#include <KoResourceSelector.h>
#include <KoResourceServerAdapter.h>
#include <KoToolManager.h>
#include <QTemporaryFile>
#include <KoColorSpaceRegistry.h>

#include <kis_paint_device.h>
#include <kis_paintop_registry.h>
#include <kis_paintop_preset.h>
#include <kis_paintop_settings.h>
#include <kis_config_widget.h>
#include <kis_image.h>
#include <kis_node.h>
#include <kis_paintop_settings_widget.h>
#include <kis_action.h>

#include "kis_canvas2.h"
#include "kis_node_manager.h"
#include "KisViewManager.h"
#include "kis_factory2.h"
#include "kis_canvas_resource_provider.h"
#include "kis_resource_server_provider.h"
#include "kis_favorite_resource_manager.h"
#include "kis_config.h"

#include "widgets/kis_popup_button.h"
#include "widgets/kis_paintop_presets_popup.h"
#include "widgets/kis_paintop_presets_chooser_popup.h"
#include "widgets/kis_workspace_chooser.h"
#include "widgets/kis_paintop_list_widget.h"
#include "widgets/kis_slider_spin_box.h"
#include "widgets/kis_cmb_composite.h"
#include "widgets/kis_widget_chooser.h"
#include "tool/kis_tool.h"
#include "kis_signals_blocker.h"



typedef KoResourceServerSimpleConstruction<KisPaintOpPreset, SharedPointerStoragePolicy<KisPaintOpPresetSP> > KisPaintOpPresetResourceServer;
typedef KoResourceServerAdapter<KisPaintOpPreset, SharedPointerStoragePolicy<KisPaintOpPresetSP> > KisPaintOpPresetResourceServerAdapter;

KisPaintopBox::KisPaintopBox(KisViewManager *view, QWidget *parent, const char *name)
    : QWidget(parent)
    , m_resourceProvider(view->resourceProvider())
    , m_optionWidget(0)
    , m_settingsWidget(0)
    , m_presetWidget(0)
    , m_brushChooser(0)
    , m_viewManager(view)
    , m_previousNode(0)
    , m_currTabletToolID(KoInputDevice::invalid())
    , m_presetsEnabled(true)
    , m_blockUpdate(false)
    , m_dirtyPresetsEnabled(false)
    , m_eraserBrushSizeEnabled(false)
{
    Q_ASSERT(view != 0);

    KGlobal::mainComponent().dirs()->addResourceType("kis_defaultpresets", "data", "krita/defaultpresets/");

    setObjectName(name);
    KisConfig cfg;
    m_dirtyPresetsEnabled = cfg.useDirtyPresets();
    m_eraserBrushSizeEnabled = cfg.useEraserBrushSize();

    KAcceleratorManager::setNoAccel(this);

    setWindowTitle(i18n("Painter's Toolchest"));

    m_settingsWidget = new KisPopupButton(this);
    m_settingsWidget->setIcon(koIcon("paintop_settings_02"));
    m_settingsWidget->setToolTip(i18n("Edit brush settings"));
    m_settingsWidget->setFixedSize(32, 32);

    m_presetWidget = new KisPopupButton(this);
    m_presetWidget->setIcon(koIcon("paintop_settings_01"));
    m_presetWidget->setToolTip(i18n("Choose brush preset"));
    m_presetWidget->setFixedSize(32, 32);

    m_eraseModeButton = new QToolButton(this);
    m_eraseModeButton->setFixedSize(32, 32);
    m_eraseModeButton->setCheckable(true);

    KisAction* eraseAction = new KisAction(i18n("Set eraser mode"), m_eraseModeButton);
    eraseAction->setActivationFlags(KisAction::ACTIVE_DEVICE);
    eraseAction->setIcon(koIcon("eraser-toggle"));
    eraseAction->setShortcut(Qt::Key_E);
    eraseAction->setCheckable(true);
    m_eraseModeButton->setDefaultAction(eraseAction);
    m_viewManager->actionCollection()->addAction("erase_action", eraseAction);

    eraserBrushSize = 0; // brush size changed when using erase mode

    m_reloadButton = new QToolButton(this);
    m_reloadButton->setFixedSize(32, 32);
    m_reloadButton->setCheckable(true);

    KisAction* reloadAction = new KisAction(i18n("Reload Original Preset"), m_reloadButton);
    reloadAction->setActivationFlags(KisAction::ACTIVE_DEVICE);
    reloadAction->setIcon(koIcon("reload_preset"));
    m_reloadButton->setDefaultAction(reloadAction);
    m_viewManager->actionCollection()->addAction("reload_preset_action", reloadAction);


    m_alphaLockButton = new QToolButton(this);
    m_alphaLockButton->setFixedSize(32, 32);
    m_alphaLockButton->setCheckable(true);
    KisAction* alphaLockAction = new KisAction(i18n("Preserve Alpha"), m_alphaLockButton);
    alphaLockAction->setActivationFlags(KisAction::ACTIVE_DEVICE);
    alphaLockAction->setIcon(koIcon("transparency-unlocked"));
    alphaLockAction->setCheckable(true);
    m_alphaLockButton->setDefaultAction(alphaLockAction);
    m_viewManager->actionCollection()->addAction("preserve_alpha", alphaLockAction);

    hMirrorButton = new QToolButton(this);
    hMirrorButton->setFixedSize(32, 32);
    hMirrorButton->setCheckable(true);

    m_hMirrorAction = new KisAction(i18n("Set horizontal mirror mode"), hMirrorButton);
    m_hMirrorAction->setIcon(themedIcon("symmetry-horizontal"));
    m_hMirrorAction->setActivationFlags(KisAction::ACTIVE_DEVICE);
    m_hMirrorAction->setCheckable(true);
    hMirrorButton->setDefaultAction(m_hMirrorAction);
    m_viewManager->actionCollection()->addAction("hmirror_action", m_hMirrorAction);

    vMirrorButton = new QToolButton(this);
    vMirrorButton->setFixedSize(32, 32);
    vMirrorButton->setCheckable(true);

    m_vMirrorAction = new KisAction(i18n("Set vertical mirror mode"), vMirrorButton);
    m_vMirrorAction->setActivationFlags(KisAction::ACTIVE_DEVICE);
    m_vMirrorAction->setIcon(themedIcon("symmetry-vertical"));
    m_vMirrorAction->setCheckable(true);
    vMirrorButton->setDefaultAction(m_vMirrorAction);
    m_viewManager->actionCollection()->addAction("vmirror_action", m_vMirrorAction);

    const bool sliderLabels = cfg.sliderLabels();
    int sliderWidth;

    if (sliderLabels) {
        sliderWidth = 150 * logicalDpiX() / 96;
    }
    else {
        sliderWidth = 120 * logicalDpiX() / 96;
    }

    for (int i = 0; i < 3; ++i) {
        m_sliderChooser[i] = new KisWidgetChooser(i + 1);
        KisDoubleSliderSpinBox* slOpacity;
        KisDoubleSliderSpinBox* slFlow;
        KisDoubleSliderSpinBox* slSize;
        if (sliderLabels) {
            slOpacity = m_sliderChooser[i]->addWidget<KisDoubleSliderSpinBox>("opacity");
            slFlow    = m_sliderChooser[i]->addWidget<KisDoubleSliderSpinBox>("flow");
            slSize    = m_sliderChooser[i]->addWidget<KisDoubleSliderSpinBox>("size");
            slOpacity->setPrefix(QString("%1  ").arg(i18n("Opacity:")));
            slFlow->setPrefix(QString("%1  ").arg(i18n("Flow:")));
            slSize->setPrefix(QString("%1  ").arg(i18n("Size:")));
        }
        else {
            slOpacity = m_sliderChooser[i]->addWidget<KisDoubleSliderSpinBox>("opacity", i18n("Opacity:"));
            slFlow    = m_sliderChooser[i]->addWidget<KisDoubleSliderSpinBox>("flow", i18n("Flow:"));
            slSize    = m_sliderChooser[i]->addWidget<KisDoubleSliderSpinBox>("size", i18n("Size:"));
        }

        slOpacity->setRange(0.0, 1.0, 2);
        slOpacity->setValue(1.0);
        slOpacity->setSingleStep(0.05);
        slOpacity->setMinimumWidth(qMax(sliderWidth, slOpacity->sizeHint().width()));
        slOpacity->setFixedHeight(32);

        slFlow->setRange(0.0, 1.0, 2);
        slFlow->setValue(1.0);
        slFlow->setSingleStep(0.05);
        slFlow->setMinimumWidth(qMax(sliderWidth, slFlow->sizeHint().width()));
        slFlow->setFixedHeight(32);

        slSize->setRange(0, 1000, 2);
        slSize->setValue(100);

        slSize->setSingleStep(1);
        slSize->setExponentRatio(3.0);
        slSize->setSuffix(" px");
        slSize->setMinimumWidth(qMax(sliderWidth, slSize->sizeHint().width()));
        slSize->setFixedHeight(32);

        m_sliderChooser[i]->chooseWidget(cfg.toolbarSlider(i + 1));
    }

    m_cmbCompositeOp = new KisCompositeOpComboBox();
    m_cmbCompositeOp->setFixedHeight(32);
    foreach(KAction * a, m_cmbCompositeOp->blendmodeActions()) {
        m_viewManager->actionCollection()->addAction(a->text(), a);
    }

    m_workspaceWidget = new KisPopupButton(this);
    m_workspaceWidget->setIcon(koIcon("workspace-chooser"));
    m_workspaceWidget->setToolTip(i18n("Choose workspace"));
    m_workspaceWidget->setFixedSize(32, 32);
    m_workspaceWidget->setPopupWidget(new KisWorkspaceChooser(view));

    QHBoxLayout* baseLayout = new QHBoxLayout(this);
    m_paintopWidget = new QWidget(this);
    baseLayout->addWidget(m_paintopWidget);
    baseLayout->setSpacing(4);
    baseLayout->setContentsMargins(0, 0, 0, 0);

    m_layout = new QHBoxLayout(m_paintopWidget);
    m_layout->addWidget(m_settingsWidget);
    m_layout->addWidget(m_presetWidget);
    m_layout->setSpacing(4);
    m_layout->setContentsMargins(0, 0, 0, 0);

    QWidget* compositeActions = new QWidget(this);
    QHBoxLayout* compositeLayout = new QHBoxLayout(compositeActions);
    compositeLayout->addWidget(m_cmbCompositeOp);
    compositeLayout->addWidget(m_eraseModeButton);
    compositeLayout->addWidget(m_alphaLockButton);

    compositeLayout->setSpacing(4);
    compositeLayout->setContentsMargins(0, 0, 0, 0);

    compositeLayout->addWidget(m_reloadButton);

    KAction* action;

    action = new KAction(i18n("Brush composite"), this);
    view->actionCollection()->addAction("composite_actions", action);
    action->setDefaultWidget(compositeActions);

    action = new KAction(i18n("Brush option slider 1"), this);
    view->actionCollection()->addAction("brushslider1", action);
    action->setDefaultWidget(m_sliderChooser[0]);
    connect(action, SIGNAL(triggered()), m_sliderChooser[0], SLOT(showPopupWidget()));

    action = new KAction(i18n("Brush option slider 2"), this);
    view->actionCollection()->addAction("brushslider2", action);
    action->setDefaultWidget(m_sliderChooser[1]);
    connect(action, SIGNAL(triggered()), m_sliderChooser[1], SLOT(showPopupWidget()));

    action = new KAction(i18n("Brush option slider 3"), this);
    view->actionCollection()->addAction("brushslider3", action);
    action->setDefaultWidget(m_sliderChooser[2]);
    connect(action, SIGNAL(triggered()), m_sliderChooser[2], SLOT(showPopupWidget()));

    action = new KAction(i18n("Next Favourite Preset"), this);
    view->actionCollection()->addAction("next_favorite_preset", action);
    action->setShortcut(KShortcut(Qt::Key_Comma));
    connect(action, SIGNAL(triggered()), this, SLOT(slotNextFavoritePreset()));

    action = new KAction(i18n("Previous Favourite Preset"), this);
    view->actionCollection()->addAction("previous_favorite_preset", action);
    action->setShortcut(KShortcut(Qt::Key_Period));
    connect(action, SIGNAL(triggered()), this, SLOT(slotPreviousFavoritePreset()));

    action = new KAction(i18n("Switch to Previous Preset"), this);
    view->actionCollection()->addAction("previous_preset", action);
    action->setShortcut(KShortcut(Qt::Key_Slash));
    connect(action, SIGNAL(triggered()), this, SLOT(slotSwitchToPreviousPreset()));

    QWidget* mirrorActions = new QWidget(this);
    QHBoxLayout* mirrorLayout = new QHBoxLayout(mirrorActions);
    mirrorLayout->addWidget(hMirrorButton);
    mirrorLayout->addWidget(vMirrorButton);
    mirrorLayout->setSpacing(4);
    mirrorLayout->setContentsMargins(0, 0, 0, 0);
    action = new KAction(i18n("Mirror"), this);
    view->actionCollection()->addAction("mirror_actions", action);
    action->setDefaultWidget(mirrorActions);

    action = new KAction(i18n("Workspaces"), this);
    view->actionCollection()->addAction("workspaces", action);
    action->setDefaultWidget(m_workspaceWidget);

    m_presetsPopup = new KisPaintOpPresetsPopup(m_resourceProvider);
    m_settingsWidget->setPopupWidget(m_presetsPopup);
    m_presetsPopup->switchDetached(false);

    m_presetsChooserPopup = new KisPaintOpPresetsChooserPopup();
    m_presetsChooserPopup->setFixedSize(500, 600);
    m_presetWidget->setPopupWidget(m_presetsChooserPopup);

    m_prevCompositeOpID = KoCompositeOpRegistry::instance().getDefaultCompositeOp().id();
    m_currCompositeOpID = KoCompositeOpRegistry::instance().getDefaultCompositeOp().id();

    slotNodeChanged(view->activeNode());
    // Get all the paintops
    QList<QString> keys = KisPaintOpRegistry::instance()->keys();
    QList<KisPaintOpFactory*> factoryList;

    foreach(const QString & paintopId, keys) {
        factoryList.append(KisPaintOpRegistry::instance()->get(paintopId));
    }
    m_presetsPopup->setPaintOpList(factoryList);


    connect(m_presetsPopup       , SIGNAL(paintopActivated(QString))          , SLOT(slotSetPaintop(QString)));
    connect(m_presetsPopup       , SIGNAL(savePresetClicked())                , SLOT(slotSaveActivePreset()));
    connect(m_presetsPopup       , SIGNAL(defaultPresetClicked())             , SLOT(slotSetupDefaultPreset()));
    connect(m_presetsPopup       , SIGNAL(signalResourceSelected(KoResource*)), SLOT(resourceSelected(KoResource*)));
    connect(m_presetsPopup       , SIGNAL(reloadPresetClicked())              , SLOT(slotReloadPreset()));
    connect(m_presetsPopup       , SIGNAL(dirtyPresetToggled(bool))           , SLOT(slotDirtyPresetToggled(bool)));
    connect(m_presetsPopup       , SIGNAL(eraserBrushSizeToggled(bool))       , SLOT(slotEraserBrushSizeToggled(bool)));

    connect(m_presetsChooserPopup, SIGNAL(resourceSelected(KoResource*))      , SLOT(resourceSelected(KoResource*)));
    connect(m_resourceProvider   , SIGNAL(sigNodeChanged(const KisNodeSP))    , SLOT(slotNodeChanged(const KisNodeSP)));
    connect(m_cmbCompositeOp     , SIGNAL(currentIndexChanged(int))           , SLOT(slotSetCompositeMode(int)));
    connect(eraseAction          , SIGNAL(triggered(bool))                    , SLOT(slotToggleEraseMode(bool)));
    connect(alphaLockAction      , SIGNAL(triggered(bool))                    , SLOT(slotToggleAlphaLockMode(bool)));
    connect(m_hMirrorAction        , SIGNAL(triggered(bool))                    , SLOT(slotHorizontalMirrorChanged(bool)));
    connect(m_vMirrorAction        , SIGNAL(triggered(bool))                    , SLOT(slotVerticalMirrorChanged(bool)));
    connect(reloadAction         , SIGNAL(triggered())                        , SLOT(slotReloadPreset()));

    connect(m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("opacity"), SIGNAL(valueChanged(qreal)), SLOT(slotSlider1Changed()));
    connect(m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("flow")   , SIGNAL(valueChanged(qreal)), SLOT(slotSlider1Changed()));
    connect(m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("size")   , SIGNAL(valueChanged(qreal)), SLOT(slotSlider1Changed()));
    connect(m_sliderChooser[1]->getWidget<KisDoubleSliderSpinBox>("opacity"), SIGNAL(valueChanged(qreal)), SLOT(slotSlider2Changed()));
    connect(m_sliderChooser[1]->getWidget<KisDoubleSliderSpinBox>("flow")   , SIGNAL(valueChanged(qreal)), SLOT(slotSlider2Changed()));
    connect(m_sliderChooser[1]->getWidget<KisDoubleSliderSpinBox>("size")   , SIGNAL(valueChanged(qreal)), SLOT(slotSlider2Changed()));
    connect(m_sliderChooser[2]->getWidget<KisDoubleSliderSpinBox>("opacity"), SIGNAL(valueChanged(qreal)), SLOT(slotSlider3Changed()));
    connect(m_sliderChooser[2]->getWidget<KisDoubleSliderSpinBox>("flow")   , SIGNAL(valueChanged(qreal)), SLOT(slotSlider3Changed()));
    connect(m_sliderChooser[2]->getWidget<KisDoubleSliderSpinBox>("size")   , SIGNAL(valueChanged(qreal)), SLOT(slotSlider3Changed()));

    //Needed to connect canvas to favorite resource manager
    connect(m_viewManager->resourceProvider(), SIGNAL(sigOpacityChanged(qreal)), SLOT(slotOpacityChanged(qreal)));
    connect(m_viewManager->resourceProvider(), SIGNAL(sigFGColorChanged(KoColor)), SLOT(slotUnsetEraseMode()));

    m_favoriteResourceManager = new KisFavoriteResourceManager(this);
    connect(m_resourceProvider, SIGNAL(sigFGColorUsed(KoColor)), m_favoriteResourceManager, SLOT(slotAddRecentColor(KoColor)));

    connect(m_resourceProvider, SIGNAL(sigFGColorChanged(KoColor)), m_favoriteResourceManager, SLOT(slotChangeFGColorSelector(KoColor)));
    connect(m_resourceProvider, SIGNAL(sigBGColorChanged(KoColor)), m_favoriteResourceManager, SLOT(slotSetBGColor(KoColor)));
    // cold initialization
    m_favoriteResourceManager->slotChangeFGColorSelector(m_resourceProvider->fgColor());
    m_favoriteResourceManager->slotSetBGColor(m_resourceProvider->bgColor());

    connect(m_favoriteResourceManager, SIGNAL(sigSetFGColor(KoColor)), m_resourceProvider, SLOT(slotSetFGColor(KoColor)));
    connect(m_favoriteResourceManager, SIGNAL(sigSetBGColor(KoColor)), m_resourceProvider, SLOT(slotSetBGColor(KoColor)));
    connect(m_favoriteResourceManager, SIGNAL(sigEnableChangeColor(bool)), m_resourceProvider, SLOT(slotResetEnableFGChange(bool)));

    connect(view->mainWindow(), SIGNAL(themeChanged()), this, SLOT(slotUpdateSelectionIcon()));

    slotInputDeviceChanged(KoToolManager::instance()->currentInputDevice());
}

KisPaintopBox::~KisPaintopBox()
{
    KisConfig cfg;
    QMapIterator<TabletToolID, TabletToolData> iter(m_tabletToolMap);
    while (iter.hasNext()) {
        iter.next();
        if ((iter.key().pointer) == QTabletEvent::Eraser) {
            cfg.writeEntry(QString("LastEraser_%1").arg(iter.key().uniqueID) , iter.value().preset->name());
        }
        else {
            cfg.writeEntry(QString("LastPreset_%1").arg(iter.key().uniqueID) , iter.value().preset->name());
        }
    }
    // Do not delete the widget, since it it is global to the application, not owned by the view
    m_presetsPopup->setPaintOpSettingsWidget(0);
    qDeleteAll(m_paintopOptionWidgets);
    delete m_favoriteResourceManager;
}

void KisPaintopBox::restoreResource(KoResource* resource)
{
    KisPaintOpPreset* preset = dynamic_cast<KisPaintOpPreset*>(resource);
    if (preset) {
        setCurrentPaintop(preset->paintOp(), preset);

        m_presetsPopup->setPresetImage(preset->image());
        m_presetsPopup->resourceSelected(resource);
    }
}

void KisPaintopBox::resourceSelected(KoResource* resource)
{
    KisPaintOpPreset* preset = dynamic_cast<KisPaintOpPreset*>(resource);
    if (preset) {
        if (!preset->settings()->isLoadable())
            return;

        setCurrentPaintopAndReload(preset->paintOp(), preset);
        m_presetsPopup->setPresetImage(preset->image());
        m_presetsPopup->resourceSelected(resource);
    }
}

QPixmap KisPaintopBox::paintopPixmap(const KoID& paintop)
{
    QString pixmapName = KisPaintOpRegistry::instance()->pixmap(paintop);

    if (pixmapName.isEmpty())
        return QPixmap();

    return QPixmap(KisFactory::componentData().dirs()->findResource("kis_images", pixmapName));
}

KoID KisPaintopBox::currentPaintop()
{
    return m_resourceProvider->currentPreset()->paintOp();
}

void KisPaintopBox::setCurrentPaintopAndReload(const KoID& paintop, KisPaintOpPresetSP preset)
{
    if (!m_dirtyPresetsEnabled) {
        KisSignalsBlocker blocker(m_optionWidget);
        if (!preset->load()) {
            qWarning() << "failed to load the preset.";
        }
    }

    setCurrentPaintop(paintop, preset);
}

void KisPaintopBox::setCurrentPaintop(const KoID& paintop, KisPaintOpPresetSP preset)
{
    if (m_resourceProvider->currentPreset()) {

        m_resourceProvider->setPreviousPaintOpPreset(m_resourceProvider->currentPreset());

        if (m_optionWidget) {
            m_optionWidget->disconnect(this);
            m_optionWidget->hide();
        }

        m_paintOpPresetMap[m_resourceProvider->currentPreset()->paintOp()] = m_resourceProvider->currentPreset();
        m_tabletToolMap[m_currTabletToolID].preset = m_resourceProvider->currentPreset();
        m_tabletToolMap[m_currTabletToolID].paintOpID = m_resourceProvider->currentPreset()->paintOp();
    }

    preset = (!preset) ? activePreset(paintop) : preset;

    Q_ASSERT(preset && preset->settings());


    if (!m_paintopOptionWidgets.contains(paintop))
        m_paintopOptionWidgets[paintop] = KisPaintOpRegistry::instance()->get(paintop.id())->createSettingsWidget(this);

    m_optionWidget = m_paintopOptionWidgets[paintop];

    preset->settings()->setOptionsWidget(m_optionWidget);

    m_optionWidget->setImage(m_viewManager->image());
    m_optionWidget->setNode(m_viewManager->activeNode());
    m_optionWidget->setConfiguration(preset->settings());

    m_presetsPopup->setPaintOpSettingsWidget(m_optionWidget);

    Q_ASSERT(m_optionWidget && m_presetWidget);
    connect(m_optionWidget, SIGNAL(sigConfigurationUpdated()), this, SLOT(slotUpdatePreset()));
    connect(m_optionWidget, SIGNAL(sigSaveLockedConfig(KisPropertiesConfiguration*)), this, SLOT(slotSaveLockedOptionToPreset(KisPropertiesConfiguration*)));
    connect(m_optionWidget, SIGNAL(sigDropLockedConfig(KisPropertiesConfiguration*)), this, SLOT(slotDropLockedOption(KisPropertiesConfiguration*)));
    connect(m_optionWidget, SIGNAL(sigConfigurationItemChanged()), this, SLOT(slotConfigurationItemChanged()));


    KisPaintOpFactory* paintOp = KisPaintOpRegistry::instance()->get(paintop.id());
    QString pixFilename = KisFactory::componentData().dirs()->findResource("kis_images", paintOp->pixmap());

    m_settingsWidget->setIcon(QIcon(pixFilename));
    m_resourceProvider->setPaintOpPreset(preset);
    m_presetsPopup->setCurrentPaintOp(paintop.id());

    if (m_presetsPopup->currentPaintOp() != paintop.id()) {
        // Must change the paintop as the current one is not supported
        // by the new colorspace.
        kWarning() << "current paintop " << paintop.name() << " was not set, not supported by colorspace";
    }

    /**
     * We will get more update signals from the configuration widgets
     * but they might be delayed by some internal deferring timers,
     * so just call the slot directly
     */
    slotUpdatePreset();
}

KoID KisPaintopBox::defaultPaintOp()
{
    return KoID("paintbrush");
}

KisPaintOpPresetSP KisPaintopBox::defaultPreset(const KoID& paintOp)
{
    QString defaultName = paintOp.id() + ".kpp";
    QString path = KGlobal::mainComponent().dirs()->findResource("kis_defaultpresets", defaultName);

    KisPaintOpPresetSP preset = new KisPaintOpPreset(path);

    if (!preset->load()) {
        preset = KisPaintOpRegistry::instance()->defaultPreset(paintOp);
    }

    Q_ASSERT(preset);
    Q_ASSERT(preset->valid());

    return preset;
}

KisPaintOpPresetSP KisPaintopBox::activePreset(const KoID& paintOp)
{
    if (m_paintOpPresetMap[paintOp] == 0) {
        m_paintOpPresetMap[paintOp] = defaultPreset(paintOp);
    }

    return m_paintOpPresetMap[paintOp];
}

void KisPaintopBox::updateCompositeOp(QString compositeOpID, bool localUpdate)
{
    if (!m_optionWidget) return;
    KisSignalsBlocker blocker(m_optionWidget);

    KisNodeSP node = m_resourceProvider->currentNode();

    if (node && node->paintDevice()) {
        if (!node->paintDevice()->colorSpace()->hasCompositeOp(compositeOpID))
            compositeOpID = KoCompositeOpRegistry::instance().getDefaultCompositeOp().id();

        m_cmbCompositeOp->blockSignals(true);
        m_cmbCompositeOp->selectCompositeOp(KoID(compositeOpID));
        m_cmbCompositeOp->blockSignals(false);

        m_eraseModeButton->defaultAction()->blockSignals(true);
        m_eraseModeButton->blockSignals(true);
        m_eraseModeButton->setChecked(compositeOpID == COMPOSITE_ERASE);
        m_eraseModeButton->defaultAction()->setChecked(compositeOpID == COMPOSITE_ERASE);
        m_eraseModeButton->blockSignals(false);
        m_eraseModeButton->defaultAction()->blockSignals(false);

        if (compositeOpID != m_currCompositeOpID) {
            m_resourceProvider->currentPreset()->settings()->setPaintOpCompositeOp(compositeOpID);
            m_optionWidget->setConfiguration(m_resourceProvider->currentPreset()->settings().data());
            if (!localUpdate)
                m_resourceProvider->setCurrentCompositeOp(compositeOpID);
            m_prevCompositeOpID = m_currCompositeOpID;
            m_currCompositeOpID = compositeOpID;
        }
    }
}

void KisPaintopBox::setWidgetState(int flags)
{
    if (flags & (ENABLE_COMPOSITEOP | DISABLE_COMPOSITEOP)) {
        m_cmbCompositeOp->setEnabled(flags & ENABLE_COMPOSITEOP);
        m_eraseModeButton->setEnabled(flags & ENABLE_COMPOSITEOP);
    }

    if (flags & (ENABLE_PRESETS | DISABLE_PRESETS)) {
        m_presetWidget->setEnabled(flags & ENABLE_PRESETS);
        m_settingsWidget->setEnabled(flags & ENABLE_PRESETS);
    }

    for (int i = 0; i < 3; ++i) {
        if (flags & (ENABLE_OPACITY | DISABLE_OPACITY))
            m_sliderChooser[i]->getWidget("opacity")->setEnabled(flags & ENABLE_OPACITY);

        if (flags & (ENABLE_FLOW | DISABLE_FLOW))
            m_sliderChooser[i]->getWidget("flow")->setEnabled(flags & ENABLE_FLOW);

        if (flags & (ENABLE_SIZE | DISABLE_SIZE))
            m_sliderChooser[i]->getWidget("size")->setEnabled(flags & ENABLE_SIZE);
    }
}

void KisPaintopBox::setSliderValue(const QString& sliderID, qreal value)
{
    for (int i = 0; i < 3; ++i) {
        KisDoubleSliderSpinBox* slider = m_sliderChooser[i]->getWidget<KisDoubleSliderSpinBox>(sliderID);
        slider->blockSignals(true);
        slider->setValue(value);
        slider->blockSignals(false);
    }
}

void KisPaintopBox::slotSetPaintop(const QString& paintOpId)
{
    if (KisPaintOpRegistry::instance()->get(paintOpId) != 0) {
        KoID id(paintOpId, KisPaintOpRegistry::instance()->get(paintOpId)->name());
        setCurrentPaintop(id);
    }
}

void KisPaintopBox::slotInputDeviceChanged(const KoInputDevice& inputDevice)
{
    TabletToolMap::iterator toolData = m_tabletToolMap.find(inputDevice);

    if (toolData == m_tabletToolMap.end()) {
        KisConfig cfg;
        KisPaintOpPresetResourceServer *rserver = KisResourceServerProvider::instance()->paintOpPresetServer(false);
        KisPaintOpPresetSP preset;
        if (inputDevice.pointer() == QTabletEvent::Eraser) {
            preset = rserver->resourceByName(cfg.readEntry<QString>(QString("LastEraser_%1").arg(inputDevice.uniqueTabletId()), "Eraser_circle"));
        }
        else {
            preset = rserver->resourceByName(cfg.readEntry<QString>(QString("LastPreset_%1").arg(inputDevice.uniqueTabletId()), "Basic_tip_default"));
        }
        if (!preset) {
            preset = rserver->resourceByName("Basic_tip_default");
        }
        if (preset) {
            setCurrentPaintop(preset->paintOp(), preset);
        }
    }
    else {
        setCurrentPaintop(toolData->paintOpID, toolData->preset);
    }

    m_currTabletToolID = TabletToolID(inputDevice);
}

void KisPaintopBox::slotCanvasResourceChanged(int /*key*/, const QVariant& /*v*/)
{
    if (m_viewManager) {
        sender()->blockSignals(true);
        KisPaintOpPresetSP preset = m_viewManager->resourceProvider()->resourceManager()->resource(KisCanvasResourceProvider::CurrentPaintOpPreset).value<KisPaintOpPresetSP>();
        if (preset && m_resourceProvider->currentPreset()->name() != preset->name()) {
            QString compositeOp = preset->settings()->getString("CompositeOp");
            updateCompositeOp(compositeOp);
            resourceSelected(preset.data());
        }
        m_presetsChooserPopup->canvasResourceChanged(preset.data(), preset);

        if (m_resourceProvider->currentCompositeOp() != m_currCompositeOpID) {
            QString compositeOp = m_resourceProvider->currentCompositeOp();

            m_cmbCompositeOp->blockSignals(true);
            m_cmbCompositeOp->selectCompositeOp(KoID(compositeOp));
            m_cmbCompositeOp->blockSignals(false);

            m_eraseModeButton->defaultAction()->blockSignals(true);
            m_eraseModeButton->blockSignals(true);
            m_eraseModeButton->setChecked(compositeOp == COMPOSITE_ERASE);
            m_eraseModeButton->defaultAction()->setChecked(compositeOp == COMPOSITE_ERASE);
            m_eraseModeButton->blockSignals(false);
            m_eraseModeButton->defaultAction()->blockSignals(false);
        }
        sender()->blockSignals(false);
    }
}

void KisPaintopBox::slotSaveActivePreset()
{
    KisPaintOpPresetSP curPreset = m_resourceProvider->currentPreset();

    if (!curPreset)
        return;

    m_favoriteResourceManager->setBlockUpdates(true);

    KisPaintOpPresetSP newPreset = curPreset->clone();
    KisPaintOpPresetResourceServer * rServer = KisResourceServerProvider::instance()->paintOpPresetServer();
    QString saveLocation = rServer->saveLocation();
    QString presetName = m_presetsPopup->getPresetName();
    QString presetFilename = saveLocation + presetName + newPreset->defaultFileExtension();

    QStringList tags;
    KisPaintOpPresetSP resource = rServer->resourceByName(presetName);
    if (resource) {

        tags = rServer->assignedTagsList(resource.data());
        rServer->removeResourceAndBlacklist(resource);

    }

    newPreset->setImage(m_presetsPopup->cutOutOverlay());
    newPreset->setFilename(presetFilename);
    newPreset->setName(presetName);
    newPreset->setPresetDirty(false);

    rServer->addResource(newPreset);
    foreach(const QString & tag, tags) {
        rServer->addTag(newPreset.data(), tag);
    }

    // HACK ALERT! the server does not notify the observers
    // automatically, so we need to call theupdate manually!
    rServer->tagCategoryMembersChanged();

    restoreResource(newPreset.data());

    m_favoriteResourceManager->setBlockUpdates(false);
}

void KisPaintopBox::slotUpdatePreset()
{
    if (!m_resourceProvider->currentPreset()) return;

    // block updates of avoid some over updating of the option widget
    m_blockUpdate = true;

    setSliderValue("size", m_resourceProvider->currentPreset()->settings()->paintOpSize().width());

    {
        qreal opacity = m_resourceProvider->currentPreset()->settings()->paintOpOpacity();
        m_resourceProvider->setOpacity(opacity);
        setSliderValue("opacity", opacity);
        setWidgetState(ENABLE_OPACITY);
    }

    {
        setSliderValue("flow", m_resourceProvider->currentPreset()->settings()->paintOpFlow());
        setWidgetState(ENABLE_FLOW);
    }

    {
        updateCompositeOp(m_resourceProvider->currentPreset()->settings()->paintOpCompositeOp());
        setWidgetState(ENABLE_COMPOSITEOP);
    }

    m_blockUpdate = false;
}

void KisPaintopBox::slotSetupDefaultPreset()
{
    KisPaintOpPresetSP preset = defaultPreset(m_resourceProvider->currentPreset()->paintOp());
    preset->settings()->setOptionsWidget(m_optionWidget);
    m_optionWidget->setConfiguration(preset->settings());
    m_optionWidget->writeConfiguration(const_cast<KisPaintOpSettings*>(preset->settings().data()));
}

void KisPaintopBox::slotNodeChanged(const KisNodeSP node)
{
    if (m_previousNode.isValid() && m_previousNode->paintDevice())
        disconnect(m_previousNode->paintDevice().data(), SIGNAL(colorSpaceChanged(const KoColorSpace*)), this, SLOT(slotColorSpaceChanged(const KoColorSpace*)));

    // Reconnect colorspace change of node
    if (node && node->paintDevice()) {
        connect(node->paintDevice().data(), SIGNAL(colorSpaceChanged(const KoColorSpace*)), this, SLOT(slotColorSpaceChanged(const KoColorSpace*)));
        m_resourceProvider->setCurrentCompositeOp(m_currCompositeOpID);
        m_previousNode = node;
        slotColorSpaceChanged(node->colorSpace());
    }

    if (m_optionWidget) {
        m_optionWidget->setNode(node);
    }
}

void KisPaintopBox::slotColorSpaceChanged(const KoColorSpace* colorSpace)
{
    m_cmbCompositeOp->validate(colorSpace);
}

void KisPaintopBox::slotToggleEraseMode(bool checked)
{
    if (checked)
    {
        updateCompositeOp(COMPOSITE_ERASE);
        //add an option to enable eraser brush size
        if (m_eraserBrushSizeEnabled==true)
        {
            // remember brush size. set the eraser size to the normal brush size if not set
            normalBrushSize = m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("size")->value();
            if (!eraserBrushSize)
                eraserBrushSize = normalBrushSize;
        }
        else
        {
            normalBrushSize = eraserBrushSize;
            eraserBrushSize = m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("size")->value();
        }
    }

    else
    {
        updateCompositeOp(m_prevCompositeOpID);

        if (m_eraserBrushSizeEnabled==true)
        {
            // save eraser brush size as eraserBrushSize (they are all the same, so just grab the first one)
            eraserBrushSize = m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("size")->value();
        }
        else
        {
            normalBrushSize = m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("size")->value();
        }
    }


    //update value in UI (this is the main place the value is 'stored' in memory)
    int updateSize = checked ? eraserBrushSize : normalBrushSize;
    m_sliderChooser[0]->getWidget<KisDoubleSliderSpinBox>("size")->setValue(updateSize);
    m_sliderChooser[1]->getWidget<KisDoubleSliderSpinBox>("size")->setValue(updateSize);
    m_sliderChooser[2]->getWidget<KisDoubleSliderSpinBox>("size")->setValue(updateSize);


    toggleHighlightedButton(m_eraseModeButton);
}

void KisPaintopBox::slotSetCompositeMode(int index)
{
    Q_UNUSED(index);

    if (m_resourceProvider->currentPreset()->settings()->hasProperty("CompositeOp")) {
        QString compositeOp = m_cmbCompositeOp->selectedCompositeOp().id();
        updateCompositeOp(compositeOp);
    }
}

void KisPaintopBox::slotHorizontalMirrorChanged(bool value)
{
    m_resourceProvider->setMirrorHorizontal(value);
    toggleHighlightedButton(hMirrorButton);
}

void KisPaintopBox::slotVerticalMirrorChanged(bool value)
{
    m_resourceProvider->setMirrorVertical(value);
    toggleHighlightedButton(vMirrorButton);
}

void KisPaintopBox::sliderChanged(int n)
{
    if (!m_optionWidget) // widget will not exist if the are no documents open
        return;

    KisSignalsBlocker blocker(m_optionWidget);

    m_optionWidget->writeConfiguration(const_cast<KisPaintOpSettings*>(m_resourceProvider->currentPreset()->settings().data()));
    qreal opacity = m_sliderChooser[n]->getWidget<KisDoubleSliderSpinBox>("opacity")->value();
    qreal flow    = m_sliderChooser[n]->getWidget<KisDoubleSliderSpinBox>("flow")->value();
    qreal size    = m_sliderChooser[n]->getWidget<KisDoubleSliderSpinBox>("size")->value();


    setSliderValue("opacity", opacity);
    setSliderValue("flow"   , flow);
    setSliderValue("size"   , size);

    if (m_presetsEnabled) {
        // IMPORTANT: set the PaintOp size before setting the other properties
        //            it wont work the other way
        qreal sizeDiff = size - m_resourceProvider->currentPreset()->settings()->paintOpSize().width();
        m_resourceProvider->currentPreset()->settings()->changePaintOpSize(sizeDiff, 0);

        m_resourceProvider->currentPreset()->settings()->setPaintOpOpacity(opacity);
        m_resourceProvider->currentPreset()->settings()->setPaintOpFlow(flow);

        KisLockedPropertiesProxy *propertiesProxy = KisLockedPropertiesServer::instance()->createLockedPropertiesProxy(m_resourceProvider->currentPreset()->settings());
        propertiesProxy->setProperty("OpacityValue", opacity);
        propertiesProxy->setProperty("FlowValue", flow);
        delete propertiesProxy;

        m_optionWidget->setConfiguration(m_resourceProvider->currentPreset()->settings().data());
    }

    m_resourceProvider->setOpacity(opacity);

    m_presetsPopup->resourceSelected(m_resourceProvider->currentPreset().data());
}

void KisPaintopBox::slotSlider1Changed()
{
    sliderChanged(0);
}

void KisPaintopBox::slotSlider2Changed()
{
    sliderChanged(1);
}

void KisPaintopBox::slotSlider3Changed()
{
    sliderChanged(2);
}

void KisPaintopBox::slotToolChanged(KoCanvasController* canvas, int toolId)
{
    Q_UNUSED(canvas);
    Q_UNUSED(toolId);

    if (!m_viewManager->canvasBase()) return;

    QString  id   = KoToolManager::instance()->activeToolId();
    KisTool* tool = dynamic_cast<KisTool*>(KoToolManager::instance()->toolById(m_viewManager->canvasBase(), id));

    if (tool) {
        int flags = tool->flags();

        if (flags & KisTool::FLAG_USES_CUSTOM_COMPOSITEOP) {
            setWidgetState(ENABLE_COMPOSITEOP | ENABLE_OPACITY);
        } else                                              {
            setWidgetState(DISABLE_COMPOSITEOP | DISABLE_OPACITY);
        }

        if (flags & KisTool::FLAG_USES_CUSTOM_PRESET) {
            setWidgetState(ENABLE_PRESETS | ENABLE_SIZE | ENABLE_FLOW);
            slotUpdatePreset();
            m_presetsEnabled = true;
        } else {
            setWidgetState(DISABLE_PRESETS | DISABLE_SIZE | DISABLE_FLOW);
            m_presetsEnabled = false;
        }
    } else setWidgetState(DISABLE_ALL);
}

void KisPaintopBox::slotOpacityChanged(qreal opacity)
{
    if (m_blockUpdate) {
        return;
    }
    m_blockUpdate = true;

    for (int i = 0; i < 3; ++i) {
        KisDoubleSliderSpinBox *opacitySlider = m_sliderChooser[i]->getWidget<KisDoubleSliderSpinBox>("opacity");
        opacitySlider->blockSignals(true);
        opacitySlider->setValue(opacity);
        opacitySlider->blockSignals(false);
    }
    if (m_presetsEnabled) {
        m_resourceProvider->currentPreset()->settings()->setPaintOpOpacity(opacity);
        m_optionWidget->setConfiguration(m_resourceProvider->currentPreset()->settings().data());
    }
    m_blockUpdate = false;
}

void KisPaintopBox::slotPreviousFavoritePreset()
{
    if (!m_favoriteResourceManager) return;

    int i = 0;
    foreach (KisPaintOpPresetSP preset, m_favoriteResourceManager->favoritePresetList()) {
        if (m_resourceProvider->currentPreset() && m_resourceProvider->currentPreset()->name() == preset->name()) {
            if (i > 0) {
                m_favoriteResourceManager->slotChangeActivePaintop(i - 1);
            } else {
                m_favoriteResourceManager->slotChangeActivePaintop(m_favoriteResourceManager->numFavoritePresets() - 1);
            }
            return;
        }
        i++;
    }

}

void KisPaintopBox::slotNextFavoritePreset()
{
    if (!m_favoriteResourceManager) return;

    int i = 0;
    foreach (KisPaintOpPresetSP preset, m_favoriteResourceManager->favoritePresetList()) {
        if (m_resourceProvider->currentPreset()->name() == preset->name()) {
            if (i < m_favoriteResourceManager->numFavoritePresets() - 1) {
                m_favoriteResourceManager->slotChangeActivePaintop(i + 1);
            } else {
                m_favoriteResourceManager->slotChangeActivePaintop(0);
            }
            return;
        }
        i++;
    }
}

void KisPaintopBox::slotSwitchToPreviousPreset()
{
    if (m_resourceProvider->previousPreset()) {
        setCurrentPaintop(m_resourceProvider->previousPreset()->paintOp(), m_resourceProvider->previousPreset());
    }
}

void KisPaintopBox::slotUnsetEraseMode()
{
    if (m_currCompositeOpID == COMPOSITE_ERASE) {
        updateCompositeOp(m_prevCompositeOpID);
    }
}

void KisPaintopBox::slotToggleAlphaLockMode(bool checked)
{
    if (checked) {
        m_alphaLockButton->actions()[0]->setIcon(koIcon("transparency-locked"));
    } else {
        m_alphaLockButton->actions()[0]->setIcon(koIcon("transparency-unlocked"));
    }
    toggleHighlightedButton(m_alphaLockButton);
    m_resourceProvider->setGlobalAlphaLock(checked);
}


void KisPaintopBox::toggleHighlightedButton(QToolButton* m_tool)
{
    QPalette p = palette();
    if (m_tool->isChecked()) {
        QPalette palette_highlight(p);
        palette_highlight.setColor(QPalette::Button, p.color(QPalette::Highlight));
        m_tool->setPalette(palette_highlight);
    }
    else {
        m_tool->setPalette(p);
    }
}
void KisPaintopBox::slotReloadPreset()
{
    KisSignalsBlocker blocker(m_optionWidget);

    //Here using the name and fetching the preset from the server was the only way the load was working. Otherwise it was not loading.
    KisPaintOpPresetResourceServer * rserver = KisResourceServerProvider::instance()->paintOpPresetServer();
    KisPaintOpPresetSP preset = rserver->resourceByName(m_resourceProvider->currentPreset()->name());
    if (preset) {
        preset->load();
        preset->settings()->setOptionsWidget(m_optionWidget);
        m_optionWidget->setConfiguration(preset->settings());
        m_presetsPopup->resourceSelected(preset.data());
    }
    slotUpdatePreset();
}
void KisPaintopBox::slotConfigurationItemChanged() // Called only when UI is changed and not when preset is changed
{
    m_optionWidget->writeConfiguration(const_cast<KisPaintOpSettings*>(m_resourceProvider->currentPreset()->settings().data()));
    m_presetsPopup->resourceSelected(m_resourceProvider->currentPreset().data());
    m_presetsPopup->updateViewSettings();
}
void KisPaintopBox::slotSaveLockedOptionToPreset(KisPropertiesConfiguration* p)
{

    QMapIterator<QString, QVariant> i(p->getProperties());
    while (i.hasNext()) {
        i.next();
        m_resourceProvider->currentPreset()->settings()->setProperty(i.key(), QVariant(i.value()));
        if (m_resourceProvider->currentPreset()->settings()->hasProperty(i.key() + "_previous")) {
            m_resourceProvider->currentPreset()->settings()->removeProperty(i.key() + "_previous");
        }

    }
    slotConfigurationItemChanged();

}

void KisPaintopBox::slotDropLockedOption(KisPropertiesConfiguration* p)
{
    KisSignalsBlocker blocker(m_optionWidget);
    KisPaintOpPresetSP preset = m_resourceProvider->currentPreset();

    {
        KisPaintOpPreset::DirtyStateSaver dirtySaver(preset.data());

        QMapIterator<QString, QVariant> i(p->getProperties());
        while (i.hasNext()) {
            i.next();
            if (preset->settings()->hasProperty(i.key() + "_previous")) {
                preset->settings()->setProperty(i.key(), preset->settings()->getProperty(i.key() + "_previous"));
                preset->settings()->removeProperty(i.key() + "_previous");
            }

        }
        m_optionWidget->setConfiguration(preset->settings());
    }

    slotUpdatePreset();
}
void KisPaintopBox::slotDirtyPresetToggled(bool value)
{
    if (!value) {
        slotReloadPreset();
        m_presetsPopup->resourceSelected(m_resourceProvider->currentPreset().data());
        m_presetsPopup->updateViewSettings();
    }
    m_dirtyPresetsEnabled = value;
    KisConfig cfg;
    cfg.setUseDirtyPresets(m_dirtyPresetsEnabled);

}
void KisPaintopBox::slotEraserBrushSizeToggled(bool value)
{
    m_eraserBrushSizeEnabled = value;
    KisConfig cfg;
    cfg.setUseEraserBrushSize(m_eraserBrushSizeEnabled);
}

void KisPaintopBox::slotUpdateSelectionIcon()
{
    m_hMirrorAction->setIcon(themedIcon("symmetry-horizontal"));
    m_vMirrorAction->setIcon(themedIcon("symmetry-vertical"));
}
