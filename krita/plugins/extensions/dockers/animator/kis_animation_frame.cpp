/*
 *  Copyright (c) 2013 Somsubhra Bairi <somsubhra.bairi@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License, or(at you option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_animation_frame.h"
#include "kis_debug.h"

#include <QPainter>

KisAnimationFrame::KisAnimationFrame(KisLayerContents *parent, int type, int width)
{
    this->m_type = type;
    this->m_width = width;
    this->setParent(parent);
    this->m_parent = parent;
    m_dragging = false;
}

void KisAnimationFrame::paintEvent(QPaintEvent *event)
{

    QPainter painter(this);

    if(this->m_type == KisAnimationFrame::FRAME) {
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        painter.drawRect(0, 0, this->m_width, 20);
        painter.drawEllipse(QPointF(5, 10), 3, 3);

        if(this->m_width > 10) {
            painter.drawRect(this->m_width - 8, 3, 6, 14);
        }
    }

    if(this->m_type == KisAnimationFrame::SELECTION) {
        painter.setPen(Qt::blue);
        painter.setBrush(Qt::blue);
        painter.setOpacity(0.5);
        painter.drawRect(0, 0, this->m_width - 1, 19);
    }
}

void KisAnimationFrame::mouseMoveEvent(QMouseEvent *event)
{
    if(m_mousePressed) {

        m_dragging = true;

        if(this->width() != 10) {
            if(m_localMousePressStartPoint > this->width() - 10 && m_localMousePressStartPoint < this->width()) {
                qDebug() << "Entending the frame";
                return;
            }
        }

        int x = event->globalX();

        int displacement = x - m_mousePressStartPoint;
        this->setGeometry(m_startPoint + displacement, this->y(), this->width(), this->height());
    }
}

void KisAnimationFrame::mousePressEvent(QMouseEvent *event)
{
    m_mousePressed = true;

    m_mousePressStartPoint = event->globalX();
    m_localMousePressStartPoint = event->x();

    m_startPoint = this->geometry().x();
}

void KisAnimationFrame::mouseReleaseEvent(QMouseEvent *event)
{
    m_mousePressed = false;

    m_mousePressEndPoint = event->globalX();
    m_localMousePressEndPoint = event->x();

    if(m_dragging) {

        m_dragging = false;

        int x = (this->geometry().x() / 10);
        x *= 10;

        m_startPoint = x;
        this->setGeometry(x, this->y(), this->width(), this->height());
    }

    m_parent->mouseReleased(this->x() + event->x());
}

int KisAnimationFrame::getWidth()
{
    return m_width;
}

void KisAnimationFrame::setWidth(int width)
{
    this->m_width = width;
    this->repaint();
}

KisLayerContents* KisAnimationFrame::getParent()
{
    return this->m_parent;
}

int KisAnimationFrame::getType()
{
    return this->m_type;
}

void KisAnimationFrame::setType(int type)
{
    this->m_type = type;
    this->repaint();
}

int KisAnimationFrame::getIndex()
{
    int index = (int)this->geometry().x() / 10;
    return index;
}

QRect KisAnimationFrame::convertSelectionToFrame()
{
    QRect globalGeometry;

    if(this->getType() == KisAnimationFrame::SELECTION) {

        this->getParent()->mapFrame(this->geometry().x() / 10, this);

        KisAnimationFrame* oldPrevFrame = this->getParent()->getPreviousFrameFrom(this);

        if(!oldPrevFrame) {
            return QRect();
        }

        oldPrevFrame->hide();

        int oldPrevFrameWidth = oldPrevFrame->getWidth();
        int previousFrameWidth = this->geometry().x()-this->getParent()->getPreviousFrameFrom(this)->geometry().x();

        KisAnimationFrame* newPreviousFrame = new KisAnimationFrame(this->getParent(), KisAnimationFrame::FRAME, previousFrameWidth);
        newPreviousFrame->setGeometry(this->getParent()->getPreviousFrameFrom(this)->geometry().x(), 0, previousFrameWidth, 20);
        newPreviousFrame->show();

        int previousFrameIndex = this->getParent()->getPreviousFrameFrom(this)->geometry().x() / 10;

        this->getParent()->mapFrame(previousFrameIndex, newPreviousFrame);

        int newFrameWidth;

        int nextFrameIndex = this->getParent()->getNextFrameIndexFrom(this->getParent()->getIndex(this));
        newFrameWidth = nextFrameIndex * 10 - this->geometry().x();

        if(newFrameWidth == 0) {
            if(this->geometry().x() < this->getParent()->getPreviousFrameFrom(this)->geometry().x()+oldPrevFrameWidth){
                newFrameWidth = this->getParent()->getPreviousFrameFrom(this)->geometry().x() + oldPrevFrameWidth - this->geometry().x();
            }
            else {
                newFrameWidth = 10;
            }
        }

        KisAnimationFrame* newFrame = new KisAnimationFrame(this->getParent(), KisAnimationFrame::FRAME, newFrameWidth);
        newFrame->setGeometry(this->geometry().x(), 0, newFrameWidth, 20);
        newFrame->show();

        globalGeometry.setRect(this->geometry().x(), this->getParent()->getLayerIndex() * 20, newFrameWidth, 20);

        this->getParent()->mapFrame(this->geometry().x() / 10, newFrame);
        this->getParent()->getParent()->getSelectedFrame()->hide();
    }

    return globalGeometry;
}

void KisAnimationFrame::expandWidth()
{
    this->getParent()->mapFrame(this->geometry().x() / 10, this);

    KisAnimationFrame* previousFrame = this->getParent()->getPreviousFrameFrom(this);

    if(previousFrame->geometry().x() + previousFrame->getWidth() - 10 < this->geometry().x()) {
        int newWidth = this->geometry().x() - previousFrame->geometry().x() + 10;
        KisAnimationFrame* newPreviousFrame = new KisAnimationFrame(previousFrame->getParent(), previousFrame->getType(), newWidth);
        newPreviousFrame->setGeometry(previousFrame->x(), 0, newWidth, 20);
        newPreviousFrame->show();
        this->getParent()->mapFrame(previousFrame->x() / 10, newPreviousFrame);
    }

    this->getParent()->getParent()->getSelectedFrame()->hide();
    this->getParent()->unmapFrame(this->geometry().x() / 10);
    this->getParent()->getParent()->setSelectedFrame();
}

QRect KisAnimationFrame::removeFrame()
{
    QRect deletedFrame;

    KisAnimationFrame* previousFrame = this->getParent()->getFrameAt(this->geometry().x() / 10);

    if(previousFrame) {
        this->getParent()->unmapFrame(previousFrame->geometry().x() / 10);
        previousFrame->hide();

        deletedFrame.setRect(previousFrame->geometry().x(), this->getParent()->getLayerIndex() * 20, previousFrame->geometry().width(), 20);

        this->getParent()->getParent()->getSelectedFrame()->hide();
    }
    return deletedFrame;
}
