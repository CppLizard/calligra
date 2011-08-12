/* This file is part of the KDE project
 * Copyright (C) 2007 Thomas Zander <zander@kde.org>
 * Copyright (C) 2010 Casper Boemann <cbo@boemann.dk>
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
#ifndef SIMPLECHARACTERWIDGET_H
#define SIMPLECHARACTERWIDGET_H

#include <ui_SimpleCharacterWidget.h>
#include <KoListStyle.h>

#include <QWidget>
#include <QTextBlock>

class TextTool;
class KoStyleManager;
class KoCharacterStyle;
class StylesWidget;
class KoStyleThumbnailer;

class SimpleCharacterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SimpleCharacterWidget(TextTool *tool, QWidget *parent = 0);
    virtual ~SimpleCharacterWidget();

public slots:
    void setStyleManager(KoStyleManager *sm);
    void setCurrentFormat(const QTextCharFormat& format);

private slots:
    void fontFamilyActivated(int index);
    void fontSizeActivated(int index);
    void hidePopup();

signals:
    void doneWithFocus();
    void characterStyleSelected(KoCharacterStyle *);

private:
    Ui::SimpleCharacterWidget widget;
    KoStyleManager *m_styleManager;
    bool m_blockSignals;
    bool m_comboboxHasBidiItems;
    int m_lastFontFamilyIndex;
    int m_lastFontSizeIndex;
    TextTool *m_tool;
    QTextCharFormat m_currentCharFormat;
    KoStyleThumbnailer *m_thumbnailer;
    StylesWidget *m_stylePopup;
};

#endif
