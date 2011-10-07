/* This file is part of the KDE project
 * Copyright (C) 2011 Smit Patel <smitpatel24@gmail.com>
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

#include "ui_BibliographyConfigureDialog.h"
#include "BibliographyConfigureDialog.h"

#include <KoTextDocument.h>
#include <KoStyleManager.h>

#include <QAbstractButton>

BibliographyConfigureDialog::BibliographyConfigureDialog(const QTextDocument *document, QWidget *parent) :
    QDialog(parent),
    m_document(document),
    m_bibConfiguration(KoTextDocument(m_document).styleManager()->bibliographyConfiguration())
{
    dialog.setupUi(this);
    dialog.prefix->setText(m_bibConfiguration->prefix());
    dialog.suffix->setText(m_bibConfiguration->suffix());
    dialog.numberedEntries->setChecked(m_bibConfiguration->numberedEntries());
    dialog.sortAlgorithm->setCurrentIndex(
                dialog.sortAlgorithm->findText(m_bibConfiguration->sortAlgorithm(),Qt::MatchFixedString));

    dialog.sortByPosition->setChecked(m_bibConfiguration->sortByPosition());

    connect(dialog.buttonBox,SIGNAL(clicked(QAbstractButton*)),this,SLOT(save(QAbstractButton*)));

    show();
}

void BibliographyConfigureDialog::save(QAbstractButton *button)
{
    if (dialog.buttonBox->standardButton(button) == dialog.buttonBox->Apply) {

        m_bibConfiguration->setPrefix(dialog.prefix->text());
        m_bibConfiguration->setSuffix(dialog.suffix->text());
        m_bibConfiguration->setSortAlgorithm(dialog.sortAlgorithm->currentText());
        m_bibConfiguration->setSortByPosition(dialog.sortByPosition->isChecked());

        KoTextDocument(m_document).styleManager()->setBibliographyConfiguration(m_bibConfiguration);
    }
    emit accept();
}
