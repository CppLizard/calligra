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

#include "ArtisticTextTool.h"
#include "ArtisticTextToolSelection.h"
#include "AttachTextToPathCommand.h"
#include "DetachTextFromPathCommand.h"
#include "AddTextRangeCommand.h"
#include "RemoveTextRangeCommand.h"
#include "ArtisticTextShapeConfigWidget.h"
#include "MoveStartOffsetStrategy.h"
#include "SelectTextStrategy.h"

#include <KoCanvasBase.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <KoPointerEvent.h>
#include <KoPathShape.h>
#include <KoShapeController.h>
#include <KoShapeContainer.h>
#include <KoLineBorder.h>
#include <KoInteractionStrategy.h>

#include <KLocale>
#include <KIcon>
#include <KDebug>

#include <QtGui/QAction>
#include <QtGui/QGridLayout>
#include <QtGui/QToolButton>
#include <QtGui/QCheckBox>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QUndoCommand>

#include <float.h>

const int BlinkInterval = 500;

ArtisticTextTool::ArtisticTextTool(KoCanvasBase *canvas)
    : KoToolBase(canvas), m_selection(canvas, this), m_currentShape(0), m_hoverText(0), m_hoverPath(0), m_hoverHandle(false)
    , m_textCursor( -1 ), m_showCursor( true ), m_currentStrategy(0)
{
    m_detachPath  = new QAction(KIcon("artistictext-detach-path"), i18n("Detach Path"), this);
    m_detachPath->setEnabled( false );
    connect( m_detachPath, SIGNAL(triggered()), this, SLOT(detachPath()) );

    m_convertText  = new QAction(KIcon("pathshape"), i18n("Convert to Path"), this);
    m_convertText->setEnabled( false );
    connect( m_convertText, SIGNAL(triggered()), this, SLOT(convertText()) );

    KoShapeManager *manager = canvas->shapeManager();
    connect(manager, SIGNAL(selectionContentChanged()), this, SLOT(textChanged()));
    connect(manager, SIGNAL(selectionChanged()), this, SLOT(shapeSelectionChanged()));

    setTextMode(true);
}

ArtisticTextTool::~ArtisticTextTool()
{
    delete m_currentStrategy;
}

QTransform ArtisticTextTool::cursorTransform() const
{
    if (!m_currentShape)
        return QTransform();

    QTransform transform;

    const int textLength = m_currentShape->plainText().length();
    if (m_textCursor <= textLength) {
        const QPointF pos = m_currentShape->charPositionAt(m_textCursor);
        const qreal angle = m_currentShape->charAngleAt(m_textCursor);
        QFontMetrics metrics(m_currentShape->fontAt(m_textCursor));

        transform.translate( pos.x() - 1, pos.y() );
        transform.rotate( 360. - angle );
        transform.translate( 0, metrics.descent() );
    } else if (m_textCursor <= textLength + m_linefeedPositions.size()) {
        const QPointF pos = m_linefeedPositions.value(m_textCursor-textLength-1);
        QFontMetrics metrics(m_currentShape->fontAt(textLength-1));
        transform.translate(pos.x(), pos.y());
        transform.translate(0, metrics.descent());
    }

    return transform * m_currentShape->absoluteTransformation(0);
}

void ArtisticTextTool::paint( QPainter &painter, const KoViewConverter &converter)
{
    if (! m_currentShape)
        return;

    if (m_showCursor && m_blinkingCursor.isActive() && !m_currentStrategy) {
        painter.save();
        m_currentShape->applyConversion( painter, converter );
        painter.setBrush( Qt::black );
        painter.setWorldTransform( cursorTransform(), true );
        painter.setClipping( false );
        painter.drawPath( m_textCursorShape );
        painter.restore();
    }
    m_showCursor = !m_showCursor;

    if (m_currentShape->isOnPath()) {
        painter.save();
        m_currentShape->applyConversion( painter, converter );
        if (!m_currentShape->baselineShape()) {
            painter.setPen(Qt::DotLine);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(m_currentShape->baseline());
        }
        painter.setPen(Qt::blue);
        painter.setBrush(m_hoverHandle ? Qt::red : Qt::white);
        painter.drawPath(offsetHandleShape());
        painter.restore();
    }
    if (m_selection.hasSelection()) {
        painter.save();
        m_selection.paint(painter, converter);
        painter.restore();
    }
}

void ArtisticTextTool::repaintDecorations()
{
    canvas()->updateCanvas(offsetHandleShape().boundingRect());
    if (m_currentShape && m_currentShape->isOnPath()) {
        if (!m_currentShape->baselineShape())
            canvas()->updateCanvas(m_currentShape->baseline().boundingRect());
    }
    m_selection.repaintDecoration();
}

int ArtisticTextTool::cursorFromMousePosition(const QPointF &mousePosition)
{
    if (!m_currentShape)
        return -1;

    const QPointF pos = m_currentShape->documentToShape(mousePosition);
    const int len = m_currentShape->plainText().length();
    int hit = -1;
    qreal mindist = DBL_MAX;
    for ( int i = 0; i <= len;++i ) {
        QPointF center = pos - m_currentShape->charPositionAt(i);
        if ( (fabs(center.x()) + fabs(center.y())) < mindist ) {
            hit = i;
            mindist = fabs(center.x()) + fabs(center.y());
        }
    }
    return hit;
}

void ArtisticTextTool::mousePressEvent( KoPointerEvent *event )
{
    if (m_hoverHandle) {
        m_currentStrategy = new MoveStartOffsetStrategy(this, m_currentShape);
    }
    if (m_hoverText) {
        KoSelection *selection = canvas()->shapeManager()->selection();
        if(m_hoverText != m_currentShape) {
            // if we hit another text shape, select that shape
            selection->deselectAll();
            setCurrentShape(m_hoverText);
            selection->select( m_currentShape );
        }
        // change the text cursor position
        int hitCursorPos = cursorFromMousePosition(event->point);
        if (hitCursorPos >= 0) {
            setTextCursorInternal(hitCursorPos);
            m_selection.clear();
        }
        m_currentStrategy = new SelectTextStrategy(this, m_textCursor);
    }
    event->ignore();
}

void ArtisticTextTool::mouseMoveEvent( KoPointerEvent *event )
{
    m_hoverPath = 0;
    m_hoverText = 0;

    if (m_currentStrategy) {
        m_currentStrategy->handleMouseMove(event->point, event->modifiers());
        return;
    }

    const bool textOnPath = m_currentShape && m_currentShape->isOnPath();
    if (textOnPath) {
        QPainterPath handle = offsetHandleShape();
        QPointF handleCenter = handle.boundingRect().center();
        if (handleGrabRect(event->point).contains(handleCenter)) {
            // mouse is on offset handle
            if (!m_hoverHandle)
                canvas()->updateCanvas(handle.boundingRect());
            m_hoverHandle = true;
        } else {
            if (m_hoverHandle)
                canvas()->updateCanvas(handle.boundingRect());
            m_hoverHandle = false;
        }
    }
    if (!m_hoverHandle) {
        // find text or path shape at cursor position
        QList<KoShape*> shapes = canvas()->shapeManager()->shapesAt( handleGrabRect(event->point) );
        if (shapes.contains(m_currentShape)) {
            m_hoverText = m_currentShape;
        } else {
            foreach( KoShape * shape, shapes ) {
                ArtisticTextShape * text = dynamic_cast<ArtisticTextShape*>( shape );
                if (text && !m_hoverText) {
                    m_hoverText = text;
                }
                KoPathShape * path = dynamic_cast<KoPathShape*>( shape );
                if (path && !m_hoverPath) {
                    m_hoverPath = path;
                }
                if(m_hoverPath && m_hoverText)
                    break;
            }
        }
    }

    const bool hoverOnBaseline = textOnPath && m_currentShape->baselineShape() == m_hoverPath;
    // update cursor and status text
    if ( m_hoverText ) {
        useCursor( QCursor( Qt::IBeamCursor ) );
        if (m_hoverText == m_currentShape)
            emit statusTextChanged(i18n("Click to change cursor position."));
        else
            emit statusTextChanged(i18n("Click to select text shape."));
    } else if( m_hoverPath && m_currentShape && !hoverOnBaseline) {
        useCursor( QCursor( Qt::PointingHandCursor ) );
        emit statusTextChanged(i18n("Double click to put text on path."));
    } else  if (m_hoverHandle) {
        useCursor( QCursor( Qt::ArrowCursor ) );
        emit statusTextChanged( i18n("Drag handle to change start offset.") );
    } else {
        useCursor( QCursor( Qt::ArrowCursor ) );
        if (m_currentShape)
            emit statusTextChanged( i18n("Press escape to finish editing.") );
        else
            emit statusTextChanged("");
    }
}

void ArtisticTextTool::mouseReleaseEvent( KoPointerEvent *event )
{
    if (m_currentStrategy) {
        m_currentStrategy->finishInteraction(event->modifiers());
        QUndoCommand *cmd = m_currentStrategy->createCommand();
        if (cmd)
            canvas()->addCommand(cmd);
        delete m_currentStrategy;
        m_currentStrategy = 0;
    }
    updateActions();
}

void ArtisticTextTool::mouseDoubleClickEvent(KoPointerEvent */*event*/)
{
    if (m_hoverPath && m_currentShape) {
        if (!m_currentShape->isOnPath() || m_currentShape->baselineShape() != m_hoverPath) {
            m_blinkingCursor.stop();
            m_showCursor = false;
            updateTextCursorArea();
            canvas()->addCommand( new AttachTextToPathCommand( m_currentShape, m_hoverPath ) );
            m_blinkingCursor.start( BlinkInterval );
            updateActions();
            m_hoverPath = 0;
            m_linefeedPositions.clear();
            return;
        }
    }
}

void ArtisticTextTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
        return;
    }

    event->accept();
    if ( m_currentShape && textCursor() > -1 ) {
        switch(event->key())
        {
        case Qt::Key_Delete:
            if (m_selection.hasSelection()) {
                removeFromTextCursor(m_selection.selectionStart(), m_selection.selectionCount());
                m_selection.clear();
            } else if( textCursor() >= 0 && textCursor() < m_currentShape->plainText().length()) {
                removeFromTextCursor( textCursor(), 1 );
            }
            break;
        case Qt::Key_Backspace:
            if (m_selection.hasSelection()) {
                removeFromTextCursor(m_selection.selectionStart(), m_selection.selectionCount());
                m_selection.clear();
            } else {
                removeFromTextCursor( textCursor()-1, 1 );
            }
            break;
        case Qt::Key_Right:
            if (event->modifiers() & Qt::ShiftModifier) {
                int selectionStart, selectionEnd;
                if (m_selection.hasSelection()) {
                    selectionStart = m_selection.selectionStart();
                    selectionEnd = selectionStart + m_selection.selectionCount();
                    if (textCursor() == selectionStart)
                        selectionStart = textCursor()+1;
                    else if(textCursor() == selectionEnd)
                        selectionEnd = textCursor()+1;
                } else {
                    selectionStart = textCursor();
                    selectionEnd = textCursor()+1;
                }
                m_selection.selectText(selectionStart, selectionEnd);
            } else {
                m_selection.clear();
            }
            setTextCursor(m_currentShape, textCursor() + 1);
            break;
        case Qt::Key_Left:
            if (event->modifiers() & Qt::ShiftModifier) {
                int selectionStart, selectionEnd;
                if (m_selection.hasSelection()) {
                    selectionStart = m_selection.selectionStart();
                    selectionEnd = selectionStart + m_selection.selectionCount();
                    if (textCursor() == selectionStart)
                        selectionStart = textCursor()-1;
                    else if(textCursor() == selectionEnd)
                        selectionEnd = textCursor()-1;
                } else {
                    selectionEnd = textCursor();
                    selectionStart = textCursor()-1;
                }
                m_selection.selectText(selectionStart, selectionEnd);
            } else {
                m_selection.clear();
            }
            setTextCursor(m_currentShape, textCursor() - 1);
            break;
        case Qt::Key_Home:
            if (event->modifiers() & Qt::ShiftModifier) {
                const int selectionStart = 0;
                const int selectionEnd = m_selection.hasSelection() ? m_selection.selectionStart()+m_selection.selectionCount() : m_textCursor;
                m_selection.selectText(selectionStart, selectionEnd);
            } else {
                m_selection.clear();
            }
            setTextCursor(m_currentShape, 0);
            break;
        case Qt::Key_End:
            if (event->modifiers() & Qt::ShiftModifier) {
                const int selectionStart = m_selection.hasSelection() ? m_selection.selectionStart() : m_textCursor;
                const int selectionEnd = m_currentShape->plainText().length();
                m_selection.selectText(selectionStart, selectionEnd);
            } else {
                m_selection.clear();
            }
            setTextCursor(m_currentShape, m_currentShape->plainText().length());
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            const int textLength = m_currentShape->plainText().length();
            if (m_textCursor >= textLength) {
                // get font metrics for last character
                QFontMetrics metrics(m_currentShape->fontAt(textLength-1));
                const qreal offset = m_currentShape->size().height() + (m_linefeedPositions.size() + 1) * metrics.height();
                m_linefeedPositions.append(QPointF(0, offset));
                setTextCursor(m_currentShape, textCursor()+1);
            }
            break;
        }
        default:
            if (event->text().isEmpty()) {
                event->ignore();
                return;
            }
            if (m_selection.hasSelection()) {
                removeFromTextCursor(m_selection.selectionStart(), m_selection.selectionCount());
                m_selection.clear();
            }
            addToTextCursor( event->text() );
        }
    } else {
        event->ignore();
    }
}

void ArtisticTextTool::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    Q_UNUSED(toolActivation);
    foreach (KoShape *shape, shapes) {
        ArtisticTextShape *text = dynamic_cast<ArtisticTextShape*>( shape );
        if(text) {
            setCurrentShape(text);
            break;
        }
    }
    if(!m_currentShape) {
        // none found
        emit done();
        return;
    }

    m_hoverText = 0;
    m_hoverPath = 0;

    updateActions();
    emit statusTextChanged( i18n("Press return to finish editing.") );
    repaintDecorations();
}

void ArtisticTextTool::blinkCursor()
{
    updateTextCursorArea();
}

void ArtisticTextTool::deactivate()
{
    if ( m_currentShape ) {
        if( m_currentShape->plainText().isEmpty() ) {
            canvas()->addCommand( canvas()->shapeController()->removeShape( m_currentShape ) );
        }
        setCurrentShape(0);
    }
    m_hoverPath = 0;
    m_hoverText = 0;
}

void ArtisticTextTool::updateActions()
{
    if( m_currentShape ) {
        m_detachPath->setEnabled( m_currentShape->isOnPath() );
        m_convertText->setEnabled( true );
    } else {
        m_detachPath->setEnabled( false );
        m_convertText->setEnabled( false );
    }
}

void ArtisticTextTool::detachPath()
{
    if( m_currentShape && m_currentShape->isOnPath() )
    {
        canvas()->addCommand( new DetachTextFromPathCommand( m_currentShape ) );
        updateActions();
    }
}

void ArtisticTextTool::convertText()
{
    if( ! m_currentShape )
        return;

    KoPathShape * path = KoPathShape::createShapeFromPainterPath( m_currentShape->outline() );
    path->setParent( m_currentShape->parent() );
    path->setZIndex( m_currentShape->zIndex() );
    path->setBorder( m_currentShape->border() );
    path->setBackground( m_currentShape->background() );
    path->setTransformation( m_currentShape->transformation() );
    path->setShapeId( KoPathShapeId );

    QUndoCommand * cmd = canvas()->shapeController()->addShapeDirect( path );
    cmd->setText( i18n("Convert to Path") );
    canvas()->shapeController()->removeShape( m_currentShape, cmd );
    canvas()->addCommand( cmd );

    emit done();
}

QMap<QString, QWidget *> ArtisticTextTool::createOptionWidgets()
{
    QMap<QString, QWidget *> widgets;
    
    QWidget * pathWidget = new QWidget();
    pathWidget->setObjectName("ArtisticTextPathWidget");
    
    QGridLayout * layout = new QGridLayout(pathWidget);
    
    QToolButton * detachButton = new QToolButton(pathWidget);
    detachButton->setDefaultAction( m_detachPath );
    layout->addWidget( detachButton, 0, 0 );
    
    QToolButton * convertButton = new QToolButton(pathWidget);
    convertButton->setDefaultAction( m_convertText );
    layout->addWidget( convertButton, 0, 2 );
    
    layout->setSpacing(0);
    layout->setMargin(6);
    layout->setRowStretch(3, 1);
    layout->setColumnStretch(1, 1);
    
    widgets.insert(i18n("Text On Path"), pathWidget);
    
    ArtisticTextShapeConfigWidget * configWidget = new ArtisticTextShapeConfigWidget(this);
    configWidget->setObjectName("ArtisticTextConfigWidget");
    if (m_currentShape) {
        configWidget->updateWidget();
    }
    connect(this, SIGNAL(shapeSelected()), configWidget, SLOT(updateWidget()));
    connect(canvas()->shapeManager(), SIGNAL(selectionContentChanged()),
            configWidget, SLOT(updateWidget()));
            
    widgets.insert(i18n("Text Properties"), configWidget);
    
    return widgets;
}

KoToolSelection *ArtisticTextTool::selection()
{
    return &m_selection;
}

void ArtisticTextTool::enableTextCursor( bool enable )
{
    if ( enable ) {
        if( m_currentShape )
            setTextCursorInternal( m_currentShape->plainText().length() );
        connect( &m_blinkingCursor, SIGNAL(timeout()), this, SLOT(blinkCursor()) );
        m_blinkingCursor.start( BlinkInterval );
    } else {
        m_blinkingCursor.stop();
        disconnect( &m_blinkingCursor, SIGNAL(timeout()), this, SLOT(blinkCursor()) );
        setTextCursorInternal( -1 );
        m_showCursor = false;
    }
}

void ArtisticTextTool::setTextCursor(ArtisticTextShape *textShape, int textCursor)
{
    if (!m_currentShape || textShape != m_currentShape)
        return;
    if (m_textCursor == textCursor || textCursor < 0)
        return;
    const int textLength = m_currentShape->plainText().length() + m_linefeedPositions.size();
    if (textCursor > textLength)
        return;
    setTextCursorInternal(textCursor);
}

int ArtisticTextTool::textCursor() const
{
    return m_textCursor;
}

void ArtisticTextTool::updateTextCursorArea() const
{
    if( ! m_currentShape || m_textCursor < 0 )
        return;

    QRectF bbox = cursorTransform().mapRect( m_textCursorShape.boundingRect() );
    canvas()->updateCanvas( bbox );
}

void ArtisticTextTool::setCurrentShape(ArtisticTextShape *currentShape)
{
    if (m_currentShape == currentShape)
        return;
    enableTextCursor( false );
    m_currentShape = currentShape;
    m_selection.setSelectedShape(m_currentShape);
    if (m_currentShape)
        enableTextCursor( true );
    emit shapeSelected();
}

void ArtisticTextTool::setTextCursorInternal( int textCursor )
{
    updateTextCursorArea();
    m_textCursor = textCursor;
    createTextCursorShape();
    updateTextCursorArea();
    emit shapeSelected();
}

void ArtisticTextTool::createTextCursorShape()
{
    if ( m_textCursor < 0 || ! m_currentShape ) 
        return;
    const QRectF extents = m_currentShape->charExtentsAt(m_textCursor);
    m_textCursorShape = QPainterPath();
    m_textCursorShape.addRect( 0, 0, 1, -extents.height() );
    m_textCursorShape.closeSubpath();
}

void ArtisticTextTool::removeFromTextCursor( int from, unsigned int count )
{
    if ( from >= 0 ) {
        QUndoCommand *cmd = new RemoveTextRangeCommand(this, m_currentShape, from, count);
        canvas()->addCommand( cmd );
    }
}

void ArtisticTextTool::addToTextCursor( const QString &str )
{
    if ( !str.isEmpty() && m_textCursor > -1 ) {
        QString printable;
        for ( int i = 0;i < str.length();i++ ) {
            if ( str[i].isPrint() )
                printable.append( str[i] );
        }
        unsigned int len = printable.length();
        if ( len ) {
            const int textLength = m_currentShape->plainText().length();
            if (m_textCursor <= textLength) {
                QUndoCommand *cmd = new AddTextRangeCommand(this, m_currentShape, printable, m_textCursor);
                canvas()->addCommand( cmd );
            } else if (m_textCursor > textLength && m_textCursor <= textLength + m_linefeedPositions.size()) {
                const QPointF pos = m_linefeedPositions.value(m_textCursor-textLength-1);
                ArtisticTextRange newLine(printable, m_currentShape->fontAt(textLength-1));
                newLine.setXOffsets(QList<qreal>() << pos.x(), ArtisticTextRange::AbsoluteOffset);
                newLine.setYOffsets(QList<qreal>() << pos.y()-m_currentShape->baselineOffset(), ArtisticTextRange::AbsoluteOffset);
                QUndoCommand *cmd = new AddTextRangeCommand(this, m_currentShape, newLine, m_textCursor);
                canvas()->addCommand( cmd );
                m_linefeedPositions.clear();
            }
        }
    }
}

void ArtisticTextTool::textChanged()
{
    if ( !m_currentShape)
        return;

    const QString currentText = m_currentShape->plainText();
    if (m_textCursor > currentText.length())
        setTextCursorInternal(currentText.length());
}

void ArtisticTextTool::shapeSelectionChanged()
{
    KoSelection *selection = canvas()->shapeManager()->selection();
    if (selection->isSelected(m_currentShape))
        return;

    foreach (KoShape *shape, selection->selectedShapes()) {
        ArtisticTextShape *text = dynamic_cast<ArtisticTextShape*>(shape);
        if(text) {
            setCurrentShape(text);
            break;
        }
    }
}

QPainterPath ArtisticTextTool::offsetHandleShape()
{
    QPainterPath handle;
    if (!m_currentShape || !m_currentShape->isOnPath())
        return handle;

    const QPainterPath baseline = m_currentShape->baseline();
    const qreal offset = m_currentShape->startOffset();
    QPointF offsetPoint = baseline.pointAtPercent(offset);
    QSizeF paintSize = handlePaintRect(QPointF()).size();

    handle.moveTo(0, 0);
    handle.lineTo(0.5*paintSize.width(), paintSize.height());
    handle.lineTo(-0.5*paintSize.width(), paintSize.height());
    handle.closeSubpath();

    QTransform transform;
    transform.translate( offsetPoint.x(), offsetPoint.y() );
    transform.rotate(360. - baseline.angleAtPercent(offset));

    return transform.map(handle);
}

#include <ArtisticTextTool.moc>
