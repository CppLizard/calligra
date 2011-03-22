/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
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
#include "kis_brush_selection_widget.h"
#include <QLayout>
#include <QTabWidget>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QToolButton>

#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include <widgets/kis_preset_chooser.h>
#include <kis_image.h>

#include "kis_brush.h"
#include "kis_auto_brush.h"
#include "kis_imagepipe_brush.h"
#include "kis_brush_chooser.h"
#include "kis_auto_brush_widget.h"
#include "kis_custom_brush_widget.h"
#include "kis_text_brush_chooser.h"

KisBrushSelectionWidget::KisBrushSelectionWidget(QWidget * parent)
        : QWidget(parent), m_currentBrushWidget(0)
{
    uiWdgBrushChooser.setupUi(this);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);

    m_layout = new QGridLayout(uiWdgBrushChooser.settingsFrame);
    m_layout->setSpacing(0);
    m_layout->setMargin(0);

    m_autoBrushWidget = new KisAutoBrushWidget(this, "autobrush");
    connect(m_autoBrushWidget, SIGNAL(sigBrushChanged()), SIGNAL(sigBrushChanged()));
    addChooser(i18n("Autobrush"), m_autoBrushWidget, AUTOBRUSH);

    m_brushChooser = new KisBrushChooser(this);
    connect(m_brushChooser, SIGNAL(sigBrushChanged()), SIGNAL(sigBrushChanged()));
    addChooser(i18n("Predefined Brushes"), m_brushChooser, PREDEFINEDBRUSH);

    m_customBrushWidget = new KisCustomBrushWidget(0, i18n("Custom Brush"), 0);
    connect(m_customBrushWidget, SIGNAL(sigBrushChanged()), SIGNAL(sigBrushChanged()));
    addChooser(i18n("Custom Brush"), m_customBrushWidget, CUSTOMBRUSH);

    m_textBrushWidget = new KisTextBrushChooser(this, "textbrush", i18n("Text Brush"));
    connect(m_textBrushWidget, SIGNAL(sigBrushChanged()), SIGNAL(sigBrushChanged()));
    addChooser(i18n("Text Brush"), m_textBrushWidget, TEXTBRUSH);

    connect(m_buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(buttonClicked(int)));

    foreach(QWidget * widget, m_chooserMap.values()) {
         m_mininmumSize = m_mininmumSize.expandedTo(widget->sizeHint());
    }

    setCurrentWidget(m_autoBrushWidget);

    m_presetIsValid = true;
}


KisBrushSelectionWidget::~KisBrushSelectionWidget()
{
}

KisBrushSP KisBrushSelectionWidget::brush()
{
    KisBrushSP theBrush;
    switch (m_buttonGroup->checkedId()) {
    case AUTOBRUSH:
        theBrush = m_autoBrushWidget->brush();
        break;
    case PREDEFINEDBRUSH:
        theBrush = m_brushChooser->brush();
        break;
    case CUSTOMBRUSH:
       theBrush = m_customBrushWidget->brush();
       break;
    case TEXTBRUSH:
        theBrush = m_textBrushWidget->brush();
        break;
    default:
        ;
    }
    // Fallback to auto brush if no brush selected
    // Can happen if there is no predefined brush found
    if (!theBrush)
        theBrush = m_autoBrushWidget->brush();

    return theBrush;

}


void KisBrushSelectionWidget::setAutoBrush(bool on)
{
    m_buttonGroup->button(AUTOBRUSH)->setVisible(on);
}

void KisBrushSelectionWidget::setPredefinedBrushes(bool on)
{
    m_buttonGroup->button(PREDEFINEDBRUSH)->setVisible(on);
}

void KisBrushSelectionWidget::setCustomBrush(bool on)
{
    m_buttonGroup->button(CUSTOMBRUSH)->setVisible(on);
}

void KisBrushSelectionWidget::setTextBrush(bool on)
{
    m_buttonGroup->button(TEXTBRUSH)->setVisible(on);
}

void KisBrushSelectionWidget::setImage(KisImageWSP image)
{
    m_customBrushWidget->setImage(image);
}

void KisBrushSelectionWidget::setCurrentBrush(KisBrushSP brush)
{
    if (!brush) {
        return;
    }
    // XXX: clever code have brush plugins know their configuration
    //      pane, so we don't have to have this if statement and
    //      have an extensible set of brush types
    if (dynamic_cast<KisAutoBrush*>(brush.data())) {
        setCurrentWidget(m_autoBrushWidget);
        m_autoBrushWidget->setBrush(brush);
    } else if (dynamic_cast<KisTextBrush*>(brush.data())) {
        setCurrentWidget(m_textBrushWidget);
        m_textBrushWidget->setBrush(brush);
    } else {
        setCurrentWidget(m_brushChooser);
        m_brushChooser->setBrush(brush);
    }

}

void KisBrushSelectionWidget::setBrushSize(qreal dxPixels, qreal dyPixels)
{
    if (m_buttonGroup->checkedId() == AUTOBRUSH){
        m_autoBrushWidget->setBrushSize(dxPixels, dyPixels);
    }else if (m_buttonGroup->checkedId() == PREDEFINEDBRUSH){
        m_brushChooser->setBrushSize(dxPixels, dyPixels);
    }
}


QSizeF KisBrushSelectionWidget::brushSize() const
{
    switch (m_buttonGroup->checkedId()) {
        case AUTOBRUSH: {
            return m_autoBrushWidget->brushSize();
        }
        case PREDEFINEDBRUSH: {
            return m_brushChooser->brushSize();
        }
        default: {
            break;
        }
    }
    // return neutral value
    return QSizeF(1.0, 1.0);
}



void KisBrushSelectionWidget::buttonClicked(int id)
{
    setCurrentWidget(m_chooserMap[id]);
    emit sigBrushChanged();
}

void KisBrushSelectionWidget::setCurrentWidget(QWidget* widget)
{
    if (m_currentBrushWidget) {
        m_layout->removeWidget(m_currentBrushWidget);
        m_currentBrushWidget->setParent(this);
        m_currentBrushWidget->hide();
    }
    widget->setMinimumSize(m_mininmumSize);

    m_currentBrushWidget = widget;
    m_layout->addWidget(widget);

    m_currentBrushWidget->show();
    m_buttonGroup->button(m_chooserMap.key(widget))->setChecked(true);

    m_presetIsValid = (m_buttonGroup->checkedId() != CUSTOMBRUSH);
}

void KisBrushSelectionWidget::addChooser(const QString& text, QWidget* widget, int id)
{
    QToolButton * button = new QToolButton(this);
    button->setText(text);
    button->setAutoRaise(true);
    button->setCheckable(true);
    uiWdgBrushChooser.brushChooserButtonLayout->addWidget(button);

    m_buttonGroup->addButton(button, id);
    m_chooserMap[m_buttonGroup->id(button)] = widget;
    widget->hide();
}

#include "kis_brush_selection_widget.moc"
