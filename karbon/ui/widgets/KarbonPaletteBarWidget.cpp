/* This file is part of the KDE project
 * Copyright (C) 2011 Jan Hambrecht <jaham@gmx.net>
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

#include "KarbonPaletteBarWidget.h"
#include "KarbonPaletteWidget.h"
#include <KoResourceServerProvider.h>
#include <KLocale>
#include <KGlobal>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>

const int FixedWidgetSize = 20;
const int ScrollUpdateIntervall = 25;

KarbonPaletteBarWidget::KarbonPaletteBarWidget(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent)
    , m_prevButton(0), m_nextButton(0), m_choosePalette(0), m_colorBar(0)
    , m_palettes(KoResourceServerProvider::instance()->paletteServer())
{
    m_prevButton = new QToolButton(this);
    m_prevButton->setAutoRepeat(true);
    m_prevButton->setAutoRepeatInterval(ScrollUpdateIntervall);
    m_nextButton = new QToolButton(this);
    m_nextButton->setAutoRepeat(true);
    m_nextButton->setAutoRepeatInterval(ScrollUpdateIntervall);

    m_choosePalette = new QToolButton(this);
    m_choosePalette->setToolTip(i18n("Select palette"));
    m_choosePalette->setArrowType(Qt::DownArrow);

    m_colorBar = new KarbonPaletteWidget(this);
    m_colorBar->setOrientation(orientation);
    connect(m_prevButton, SIGNAL(clicked()), m_colorBar, SLOT(scrollBackward()));
    connect(m_nextButton, SIGNAL(clicked()), m_colorBar, SLOT(scrollForward()));
    connect(m_colorBar, SIGNAL(colorSelected(const KoColor&)), this, SIGNAL(colorSelected(const KoColor&)));
    connect(m_colorBar, SIGNAL(scrollOffsetChanged()), this, SLOT(updateButtons()));
    connect(m_choosePalette, SIGNAL(clicked()), this, SLOT(selectPalette()));

    setMinimumSize(FixedWidgetSize, FixedWidgetSize);
    m_colorBar->setMinimumSize(FixedWidgetSize, FixedWidgetSize);

    createLayout();

    QList<KoResource*> resources = m_palettes.resources();
    if (resources.count()) {
        KConfigGroup paletteGroup = KGlobal::mainComponent().config()->group("PaletteBar");
        QString lastPalette = paletteGroup.readEntry("LastPalette", "SVG Colors");
        KoResource * r = resources.first();
        foreach(KoResource *res, resources) {
            if (res->name() == lastPalette) {
                r = res;
                break;
            }
        }
        m_colorBar->setPalette(dynamic_cast<KoColorSet*>(r));
    }
}

KarbonPaletteBarWidget::~KarbonPaletteBarWidget()
{
}

void KarbonPaletteBarWidget::setOrientation(Qt::Orientation orientation)
{
    if (m_colorBar->orientation() == orientation)
        return;

    m_colorBar->setOrientation(orientation);
    createLayout();
}

void KarbonPaletteBarWidget::createLayout()
{
    if (m_colorBar->orientation() == Qt::Horizontal) {
        m_prevButton->setArrowType(Qt::LeftArrow);
        m_nextButton->setArrowType(Qt::RightArrow);
        QHBoxLayout *h = new QHBoxLayout();
        h->addWidget(m_choosePalette);
        h->addWidget(m_colorBar, 1, Qt::AlignVCenter);
        h->addWidget(m_prevButton);
        h->addWidget(m_nextButton);
        setLayout(h);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_colorBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    } else {
        m_prevButton->setArrowType(Qt::UpArrow);
        m_nextButton->setArrowType(Qt::DownArrow);
        QVBoxLayout *v = new QVBoxLayout();
        v->addWidget(m_choosePalette);
        v->addWidget(m_colorBar, 1, Qt::AlignHCenter);
        v->addWidget(m_prevButton);
        v->addWidget(m_nextButton);
        setLayout(v);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
        m_colorBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    }
    layout()->setMargin(0);
    layout()->setSpacing(2);
}

void KarbonPaletteBarWidget::updateButtons()
{
    m_prevButton->setEnabled(m_colorBar->currentScrollOffset() > 0);
    m_nextButton->setEnabled(m_colorBar->currentScrollOffset() < m_colorBar->maximalScrollOffset());
}

void KarbonPaletteBarWidget::selectPalette()
{
    QList<KoResource*> resources = m_palettes.resources();
    if (!resources.count())
        return;

    QMenu palletteMenu;
    int index = 0;
    foreach(KoResource* r, resources) {
        QAction *a = palletteMenu.addAction(r->name());
        if (r == m_colorBar->palette()) {
            a->setCheckable(true);
            a->setChecked(true);
        }
        a->setData(QVariant(index++));
    }

    QAction *selectedAction = palletteMenu.exec(m_choosePalette->mapToGlobal(QPoint(0,0)));
    if (selectedAction) {
        int selectedIndex = selectedAction->data().toInt();
        KoColorSet *colorSet = dynamic_cast<KoColorSet*>(resources.at(selectedIndex));
        if (colorSet) {
            m_colorBar->setPalette(colorSet);
            KConfigGroup paletteGroup = KGlobal::mainComponent().config()->group("PaletteBar");
            paletteGroup.writeEntry("LastPalette", colorSet->name());
        }
    }
}

#include "KarbonPaletteBarWidget.moc"
