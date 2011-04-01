/* This file is part of the KDE project
 * Copyright (C) 2007,2011 Jan Hambrecht <jaham@gmx.net>
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
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

#ifndef ARTISTICTEXTTOOL_H
#define ARTISTICTEXTTOOL_H

#include "ArtisticTextShape.h"

#include <KoToolBase.h>
#include <QtCore/QTimer>

class QAction;
class KoInteractionStrategy;

/// This is the tool for the artistic text shape.
class ArtisticTextTool : public KoToolBase 
{
    Q_OBJECT
public:
    explicit ArtisticTextTool(KoCanvasBase *canvas);
    ~ArtisticTextTool();

    /// reimplemented
    virtual void paint( QPainter &painter, const KoViewConverter &converter );
    /// reimplemented
    virtual void repaintDecorations();
    /// reimplemented
    virtual void mousePressEvent( KoPointerEvent *event ) ;
    /// reimplemented
    virtual void mouseMoveEvent( KoPointerEvent *event );
    /// reimplemented
    virtual void mouseReleaseEvent( KoPointerEvent *event );
    /// reimplemented
    virtual void mouseDoubleClickEvent(KoPointerEvent *event);
    /// reimplemented
    virtual void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes);
    /// reimplemented
    virtual void deactivate();
    /// reimplemented
    virtual QMap<QString, QWidget *> createOptionWidgets();
    /// reimplemented
    virtual void keyPressEvent(QKeyEvent *event);

protected:
    void enableTextCursor( bool enable );
    int textCursor() const { return m_textCursor; }
    void setTextCursor( int textCursor );
    void removeFromTextCursor( int from, unsigned int count );
    void addToTextCursor( const QString &str );

private slots:
    void detachPath();
    void convertText();
    void blinkCursor();
    void textChanged();
    void shapeSelectionChanged();

signals:
    void shapeSelected(ArtisticTextShape *shape, KoCanvasBase *canvas);

private:
    class AddTextRangeCommand;
    class RemoveTextRangeCommand;

private:
    void updateActions();
    void setTextCursorInternal( int textCursor );
    void createTextCursorShape();
    void updateTextCursorArea() const;

    /// returns the transformation matrix for the text cursor
    QTransform cursorTransform() const;

    /// Returns the offset handle shape for the current text shape
    QPainterPath offsetHandleShape();

    ArtisticTextShape * m_currentShape; ///< the current text shape we are working on
    ArtisticTextShape * m_hoverText;    ///< the text shape the mouse cursor is hovering over
    KoPathShape * m_hoverPath;          ///< the path shape the mouse cursor is hovering over
    QPainterPath m_textCursorShape;     ///< our visual text cursor representation
    bool m_hoverHandle;

    QAction * m_detachPath;
    QAction * m_convertText;

    int m_textCursor;
    QTimer m_blinkingCursor;
    bool m_showCursor;
    QString m_currentText;

    KoInteractionStrategy *m_currentStrategy;
};

#endif // ARTISTICTEXTTOOL_H
