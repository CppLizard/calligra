/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
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

#include "kis_image_manager.h"

#include <QString>
#include <QDesktopServices>

#include <kaction.h>
#include <klocale.h>
#include <kurl.h>
#include <kactioncollection.h>
#include <kcolordialog.h>

#include <KoColor.h>
#include <KoIcon.h>
#include <KisImportExportManager.h>
#include <KoFileDialog.h>

#include <kis_types.h>
#include <kis_image.h>

#include "kis_import_catcher.h"
#include "KisViewManager.h"
#include "KisDocument.h"
#include "dialogs/kis_dlg_image_properties.h"
#include "commands/kis_image_commands.h"
#include "kis_action.h"
#include "kis_action_manager.h"

KisImageManager::KisImageManager(KisViewManager * view)
        : m_view(view)
{
}

void KisImageManager::setView(QPointer<KisView>imageView)
{
    Q_UNUSED(imageView);
}

void KisImageManager::setup(KActionCollection * actionCollection, KisActionManager *actionManager)
{

    KisAction *action  = new KisAction(i18n("I&mport Layer..."), this);
    actionManager->addAction("import_layer_from_file", action, actionCollection);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImportLayerFromFile()));

    action  = new KisAction(koIcon("document-properties"), i18n("Properties..."), this);
    actionManager->addAction("image_properties", action, actionCollection);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImageProperties()));

    action  = new KisAction(koIcon("document-new"), i18n("as Paint Layer..."), this);
    actionManager->addAction("import_layer_as_paint_layer", action, actionCollection);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImportLayerFromFile()));

    action  = new KisAction(koIcon("edit-copy"), i18n("as Transparency Mask..."), this);
    actionManager->addAction("import_layer_as_transparency_mask", action, actionCollection);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImportLayerAsTransparencyMask()));

    action  = new KisAction(koIcon("bookmarks"), i18n("as Filter Mask..."), this);
    actionManager->addAction("import_layer_as_filter_mask", action, actionCollection);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImportLayerAsFilterMask()));

    action  = new KisAction(koIcon("edit-paste"), i18n("as Selection Mask..."), this);
    actionManager->addAction("import_layer_as_selection_mask", action, actionCollection);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImportLayerAsSelectionMask()));

    action = new KisAction(koIcon("format-stroke-color"), i18n("Image Background Color and Transparency..."), this);
    action->setToolTip(i18n("Change the background color of the image"));
    actionCollection->addAction("image_color", action);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImageColor()));

}

void KisImageManager::slotImportLayerFromFile()
{
    importImage(KUrl(), "KisPaintLayer");
}

void KisImageManager::slotImportLayerAsTransparencyMask()
{
    importImage(KUrl(), "KisTransparencyMask");
}

void KisImageManager::slotImportLayerAsFilterMask()
{
    importImage(KUrl(), "KisFilterMask");
}

void KisImageManager::slotImportLayerAsSelectionMask()
{
    importImage(KUrl(), "KisSelectionMask");
}


qint32 KisImageManager::importImage(const KUrl& urlArg, const QString &layerType)
{
    KisImageWSP currentImage = m_view->image();

    if (!currentImage) {
        return 0;
    }

    KUrl::List urls;
    qint32 rc = 0;

    if (urlArg.isEmpty()) {
        KoFileDialog dialog(m_view->mainWindow(), KoFileDialog::OpenFiles, "OpenDocument");
        dialog.setCaption(i18n("Import Image"));
        dialog.setDefaultDir(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
        dialog.setMimeTypeFilters(KisImportExportManager::mimeFilter("application/x-krita", KisImportExportManager::Import));
        QStringList fileNames = dialog.urls();
        foreach(const QString &fileName, fileNames) {
            urls << KUrl::fromLocalFile(fileName);
        }

    } else {
        urls.push_back(urlArg);
    }

    if (urls.empty())
        return 0;

    for (KUrl::List::iterator it = urls.begin(); it != urls.end(); ++it) {
        new KisImportCatcher(*it, m_view, layerType);
    }

    m_view->canvas()->update();

    return rc;
}

void KisImageManager::resizeCurrentImage(qint32 w, qint32 h, qint32 xOffset, qint32 yOffset)
{
    if (!m_view->image()) return;

    m_view->image()->resizeImage(QRect(-xOffset, -yOffset, w, h));
}

void KisImageManager::scaleCurrentImage(const QSize &size, qreal xres, qreal yres, KisFilterStrategy *filterStrategy)
{
    if (!m_view->image()) return;
    m_view->image()->scaleImage(size, xres, yres, filterStrategy);
}

void KisImageManager::rotateCurrentImage(double radians)
{
    if (!m_view->image()) return;
    m_view->image()->rotateImage(radians);
}

void KisImageManager::shearCurrentImage(double angleX, double angleY)
{
    if (!m_view->image()) return;
    m_view->image()->shear(angleX, angleY);
}


void KisImageManager::slotImageProperties()
{
    KisImageWSP image = m_view->image();
    if (!image) return;

    QPointer<KisDlgImageProperties> dlg = new KisDlgImageProperties(image, m_view->mainWindow());
    if (dlg->exec() == QDialog::Accepted) {
        image->convertProjectionColorSpace(dlg->colorSpace());
    }
    delete dlg;
}

void KisImageManager::slotImageColor()
{
    KisImageWSP image = m_view->image();
    if (!image) return;

    KColorDialog dlg;
    dlg.setAlphaChannelEnabled(true);

    KoColor bg = image->defaultProjectionColor();

    dlg.setColor(bg.toQColor());
    dlg.setButtons(KColorDialog::Ok | KColorDialog::Cancel);
    if (dlg.exec() == KColorDialog::Accepted) {
        QColor c = dlg.color();
        bg.fromQColor(c);
        image->setDefaultProjectionColor(bg);
        image->refreshGraphAsync();
    }
}


#include "kis_image_manager.moc"
