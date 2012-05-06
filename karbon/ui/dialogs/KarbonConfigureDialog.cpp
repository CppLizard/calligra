/* This file is part of the KDE project
Copyright (C) 2002, 2003 Laurent Montel <lmontel@mandrakesoft.com>
Copyright (C) 2006-2007 Jan Hambrecht <jaham@gmx.net>

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
* Boston, MA 02110-1301, USA.
*/

#include "KarbonConfigureDialog.h"

#include "KarbonView.h"
#include "KarbonPart.h"
#include "KarbonConfigInterfacePage.h"
#include "KarbonCanvas.h"

#include <KoConfigGridPage.h>
#include <KoConfigDocumentPage.h>
#include <KoConfigMiscPage.h>
#include <KoConfigAuthorPage.h>

#include <KLocale>
#include <KIcon>

KarbonConfigureDialog::KarbonConfigureDialog(KarbonView* parent)
    : KPageDialog(parent)
{
    setFaceType(List);
    setCaption(i18n("Configure"));
    setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Default);
    setDefaultButton(KDialog::Ok);

    m_interfacePage = new KarbonConfigInterfacePage(parent);
    KPageWidgetItem* item = addPage(m_interfacePage, i18n("Interface"));
    item->setHeader(i18n("Interface"));
    item->setIcon(KIcon(BarIcon("preferences-desktop-theme", KIconLoader::SizeMedium)));

    m_miscPage = new KoConfigMiscPage(parent->part()->componentData().config(), parent->part()->resourceManager(), parent->canvasWidget()->resourceManager());
    item = addPage(m_miscPage, i18n("Misc"));
    item->setHeader(i18n("Misc"));
    item->setIcon(KIcon(BarIcon("preferences-other", KIconLoader::SizeMedium)));

    m_gridPage = new KoConfigGridPage(parent->part());
    item = addPage(m_gridPage, i18n("Grid"));
    item->setHeader(i18n("Grid"));
    item->setIcon(KIcon(BarIcon("grid", KIconLoader::SizeMedium)));

    connect(m_miscPage, SIGNAL(unitChanged(int)), m_gridPage, SLOT(slotUnitChanged(int)));

    m_defaultDocPage = new KoConfigDocumentPage(parent->part());
    item = addPage(m_defaultDocPage, i18nc("@title:tab Document settings page", "Document"));
    item->setHeader(i18n("Document Settings"));
    item->setIcon(KIcon(BarIcon("document-properties", KIconLoader::SizeMedium)));

    m_authorPage = new KoConfigAuthorPage();
    item = addPage(m_authorPage, i18nc("@title:tab Author page", "Author" ));
    item->setHeader(i18n("Author"));
    item->setIcon(KIcon("user-identity"));


    connect(this, SIGNAL(okClicked()), this, SLOT(slotApply()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));
    connect(this, SIGNAL(defaultClicked()), this, SLOT(slotDefault()));
}

void KarbonConfigureDialog::slotApply()
{
    m_interfacePage->apply();
    m_miscPage->apply();
    m_defaultDocPage->apply();
    m_gridPage->apply();
    m_authorPage->apply();
}

void KarbonConfigureDialog::slotDefault()
{
    QWidget* curr = currentPage()->widget();

    if (curr == m_interfacePage)
        m_interfacePage->slotDefault();
    else if (curr == m_miscPage)
        m_miscPage->slotDefault();
    else if (curr == m_gridPage)
        m_gridPage->slotDefault();
    else if (curr == m_defaultDocPage)
        m_defaultDocPage->slotDefault();
}

#include "KarbonConfigureDialog.moc"
