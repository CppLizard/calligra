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

#include "kis_layer_contents.h"
#include "kis_animation_frame.h"
#include "kis_debug.h"

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>

KisLayerContents::KisLayerContents(KisFrameBox *parent)
{
    this->setFixedHeight(20);
    this->setFixedWidth(100000);

    this->m_parent = parent;
    this->setParent(parent);

    this->initialize();
}


void KisLayerContents::initialize()
{

    int length;

    if(this->m_parent->getFirstLayer()) {
        length = this->m_parent->getFirstLayer()->getContentLength();
    }
    else {
        length = 1;
    }

    KisAnimationFrame* firstFrame = new KisAnimationFrame(this, KisAnimationFrame::FRAME, length * 10);
    firstFrame->setGeometry(0, 0, length * 10, 20);
    this->m_frames[0] = firstFrame;
}

void KisLayerContents::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    for(int i = 0; i < 10000; i++) {
        if(i % 10 == 0) {
            painter.setPen(Qt::red);
            painter.drawRect(QRectF(10 * i, 0, 9, height() - 1));
        }
        else {
            painter.setPen(Qt::lightGray);
            painter.drawRect(QRectF(10 * i, 0, 10, height() - 1));
        }

    }
}

void KisLayerContents::mouseReleaseEvent(QMouseEvent *event)
{
    int x = event->x();
    x = x - (x % 10);
    this->m_parent->setSelectedFrame(x, this);
}

void KisLayerContents::mapFrame(int frameNumber, KisAnimationFrame *frame)
{
    if(this->m_frames.contains(frameNumber)) {
        this->m_frames.remove(frameNumber);
    }

    this->m_frames[frameNumber] = frame;
}

void KisLayerContents::unmapFrame(int frameNumber)
{
    if(this->m_frames.contains(frameNumber)) {
        this->m_frames.remove(frameNumber);
    }
}

int KisLayerContents::getLastFrameIndex()
{
    QList<int> keys = m_frames.keys();
    int maximum = keys.at(0);
    for(int i = 1; i < keys.length(); i++) {
        if(maximum < keys.at(i)) {
            maximum = keys.at(i);
        }
    }
    return maximum;
}

int KisLayerContents::getPreviousFrameIndexFrom(int index)
{
    QList<int> keys = m_frames.keys();
    int previous = 0;
    for(int i = 0; i < keys.length(); i++) {
        if(keys.at(i) < index) {
            if(keys.at(i) > previous) {
                previous = keys.at(i);
            }
        }
    }
    return previous;
}

int KisLayerContents::getNextFrameIndexFrom(int index)
{
    QList<int> keys = m_frames.keys();
    int next = this->getLastFrameIndex();
    for(int i = 0; i < keys.length(); i++) {
        if(keys.at(i) > index) {
            if(keys.at(i) < next){
                next = keys.at(i);
            }
        }
    }
    return next;
}

int KisLayerContents::getContentLength()
{
    int lastFrameLength = this->m_frames.value(this->getLastFrameIndex())->getWidth() / 10;
    return this->getLastFrameIndex() + lastFrameLength;
}

int KisLayerContents::getIndex(KisAnimationFrame *frame)
{
    return this->m_frames.key(frame);
}

KisAnimationFrame* KisLayerContents::getNextFrameFrom(KisAnimationFrame *frame)
{
    return this->m_frames.value(this->getNextFrameIndexFrom(this->getIndex(frame)));
}

KisAnimationFrame* KisLayerContents::getNextFrameFrom(int index)
{
    return this->m_frames.value(this->getNextFrameIndexFrom(index));
}

KisAnimationFrame* KisLayerContents::getPreviousFrameFrom(KisAnimationFrame *frame)
{
    return this->m_frames.value(this->getPreviousFrameIndexFrom(this->getIndex(frame)));
}

KisAnimationFrame* KisLayerContents::getPreviousFrameFrom(int index)
{
    return this->m_frames.value(this->getNextFrameIndexFrom(index));
}

KisAnimationFrame* KisLayerContents::getFrameAt(int index)
{
    QList<int> keys = m_frames.keys();
    int previous = 0;
    for(int i = 0; i < keys.length(); i++) {
        if(keys.at(i) <= index) {
            if(keys.at(i) > previous) {
                previous = keys.at(i);
            }
        }
    }
    return m_frames.value(previous);
}

KisFrameBox* KisLayerContents::getParent()
{
    return m_parent;
}

int KisLayerContents::getLayerIndex()
{
    return this->getParent()->getLayerContents().indexOf(this);
}
