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
#ifndef KIS_BRUSH_SELECTION_WIDGET_H
#define KIS_BRUSH_SELECTION_WIDGET_H

#include <QWidget>
#include "kis_brush.h"
#include "ui_wdgbrushchooser.h"

class QTabWidget;
class KisAutoBrushWidget;
class KisBrushChooser;
class KisTextBrushChooser;
class KisCustomBrushWidget;
class KisBrush;

class KisView2;

/**
 * Compound widget that collects all the various brush selection widgets.
 */
class PAINTOP_EXPORT KisBrushSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    KisBrushSelectionWidget(QWidget * parent = 0);

    ~KisBrushSelectionWidget();

    KisBrushSP brush();

    void setAutoBrush(bool on);
    void setPredefinedBrushes(bool on);
    void setCustomBrush(bool on);
    void setTextBrush(bool on);

    void setImage(KisImageWSP image);

    void setCurrentBrush(KisBrushSP brush);

    void setBrushSize(qreal dxPixels, qreal dyPixels);
    QSizeF brushSize() const;
    bool presetIsValid() { return m_presetIsValid; }

signals:

    void sigBrushChanged();

private slots:
    void buttonClicked(int id);

private:
    void setCurrentWidget(QWidget * widget);
    void addChooser(const QString & text, QWidget * widget, int id);

private:
    enum Type {
      AUTOBRUSH,
      PREDEFINEDBRUSH,
      CUSTOMBRUSH,
      TEXTBRUSH
    };

    bool m_presetIsValid;

    Ui_WdgBrushChooser uiWdgBrushChooser;
    QGridLayout * m_layout;
    QWidget * m_currentBrushWidget;
    QHash<int, QWidget*> m_chooserMap;
    QButtonGroup * m_buttonGroup;
    QSize m_mininmumSize;

    KisAutoBrushWidget * m_autoBrushWidget;
    KisBrushChooser * m_brushChooser;
    KisTextBrushChooser * m_textBrushWidget;
    KisCustomBrushWidget * m_customBrushWidget;

};

#endif
