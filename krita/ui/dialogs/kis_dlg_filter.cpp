/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdysa.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kis_dlg_filter.h"

#include <filter/kis_filter.h>
#include <filter/kis_filter_configuration.h>
#include <kis_filter_mask.h>
#include <kis_node.h>
#include <kis_layer.h>
#include <kis_view2.h>
#include <kis_config.h>

#include "kis_selection.h"
#include "kis_node_commands_adapter.h"
#include "kis_filter_manager.h"
#include "ui_wdgfilterdialog.h"


struct KisDlgFilter::Private {
    Private()
            : currentFilter(0)
            , resizeCount(0)
            , view(0)
    {
    }

    KisFilterSP currentFilter;
    Ui_FilterDialog uiFilterDialog;
    KisNodeSP node;
    int resizeCount;
    KisView2 *view;
    KisFilterManager *filterManager;
};

KisDlgFilter::KisDlgFilter(KisView2 *view, KisNodeSP node, KisFilterManager *filterManager) :
        QDialog(view),
        d(new Private)
{
    setModal(false);

    d->uiFilterDialog.setupUi(this);
    d->node = node;
    d->view = view;
    d->filterManager = filterManager;

    d->uiFilterDialog.filterSelection->setView(view);
    d->uiFilterDialog.filterSelection->showFilterGallery(KisConfig().showFilterGallery());

    d->uiFilterDialog.pushButtonCreateMaskEffect->show();
    connect(d->uiFilterDialog.pushButtonCreateMaskEffect, SIGNAL(pressed()), SLOT(createMask()));

    if (node->inherits("KisMask")) {
        d->uiFilterDialog.pushButtonCreateMaskEffect->setVisible(false);
    }

    d->uiFilterDialog.filterSelection->setPaintDevice(true, d->node->original());

    connect(d->uiFilterDialog.buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->uiFilterDialog.buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(d->uiFilterDialog.checkBoxPreview, SIGNAL(stateChanged(int)), SLOT(previewCheckBoxChange(int)));

    connect(d->uiFilterDialog.filterSelection, SIGNAL(configurationChanged()), SLOT(filterSelectionChanged()));

    connect(this, SIGNAL(accepted()), SLOT(slotOnAccept()));
    connect(this, SIGNAL(rejected()), SLOT(slotOnReject()));

    KConfigGroup group(KGlobal::config(), "filterdialog");
    d->uiFilterDialog.checkBoxPreview->setChecked(group.readEntry("showPreview", true));
}

KisDlgFilter::~KisDlgFilter()
{
    delete d;
}

void KisDlgFilter::setFilter(KisFilterSP f)
{
    Q_ASSERT(f);
    setDialogTitle(f);
    d->uiFilterDialog.filterSelection->setFilter(f);
    d->uiFilterDialog.pushButtonCreateMaskEffect->setEnabled(f->supportsAdjustmentLayers());
    updatePreview();
}

void KisDlgFilter::setDialogTitle(KisFilterSP filter)
{
    setWindowTitle(filter.isNull() ? i18nc("@title:window", "Filter") : i18nc("@title:window", "Filter: %1", filter->name()));
}

void KisDlgFilter::startApplyingFilter(KisSafeFilterConfigurationSP config)
{
    if (!d->uiFilterDialog.filterSelection->configuration()) return;

    if (d->node->inherits("KisLayer")) {
        config->setChannelFlags(qobject_cast<KisLayer*>(d->node.data())->channelFlags());
    }

    d->filterManager->apply(config);
}

void KisDlgFilter::updatePreview()
{
    if (!d->uiFilterDialog.filterSelection->configuration()) return;

    if (d->uiFilterDialog.checkBoxPreview->isChecked()) {
        KisSafeFilterConfigurationSP config(d->uiFilterDialog.filterSelection->configuration());
        startApplyingFilter(config);
    }

    d->uiFilterDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void KisDlgFilter::slotOnAccept()
{
    if (!d->filterManager->isStrokeRunning()) {
        KisSafeFilterConfigurationSP config(d->uiFilterDialog.filterSelection->configuration());
        startApplyingFilter(config);
    }
    d->filterManager->finish();

    d->uiFilterDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    KisConfig().setShowFilterGallery(d->uiFilterDialog.filterSelection->isFilterGalleryVisible());
}

void KisDlgFilter::slotOnReject()
{
    if (d->filterManager->isStrokeRunning()) {
        d->filterManager->cancel();
    }

    KisConfig().setShowFilterGallery(d->uiFilterDialog.filterSelection->isFilterGalleryVisible());
}

void KisDlgFilter::createMask()
{
    if (d->node->inherits("KisMask")) return;

    if (d->filterManager->isStrokeRunning()) {
        d->filterManager->cancel();
    }

    KisLayer *layer = dynamic_cast<KisLayer*>(d->node.data());
    KisFilterMaskSP mask = new KisFilterMask();
    mask->initSelection(d->view->selection(), layer);
    mask->setFilter(d->uiFilterDialog.filterSelection->configuration());

    Q_ASSERT(layer->allowAsChild(mask));

    KisNodeCommandsAdapter adapter(d->view);
    adapter.addNode(mask, layer, layer->lastChild());
    accept();
}

void KisDlgFilter::previewCheckBoxChange(int state)
{
    if (state) {
        updatePreview();
    } else if (d->filterManager->isStrokeRunning()) {
        d->filterManager->cancel();
    }

    KConfigGroup group(KGlobal::config(), "filterdialog");
    group.writeEntry("showPreview", d->uiFilterDialog.checkBoxPreview->isChecked());
    group.config()->sync();
}

void KisDlgFilter::filterSelectionChanged()
{
    KisFilterSP filter = d->uiFilterDialog.filterSelection->currentFilter();
    setDialogTitle(filter);
    d->uiFilterDialog.pushButtonCreateMaskEffect->setEnabled(filter.isNull() ? false : filter->supportsAdjustmentLayers());
    updatePreview();
}


void KisDlgFilter::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);

    // Workaround, after the initalisation don't center the dialog anymore
    if(d->resizeCount < 2) {
        QWidget* canvas = d->view->canvas();
        QRect rect(canvas->mapToGlobal(canvas->geometry().topLeft()), size());
        int deltaX = (canvas->geometry().width() - geometry().width())/2;
        int deltaY = (canvas->geometry().height() - geometry().height())/2;
        rect.translate(deltaX, deltaY);
        setGeometry(rect);

        d->resizeCount++;
    }
}
