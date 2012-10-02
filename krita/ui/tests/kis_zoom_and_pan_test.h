/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_ZOOM_AND_PAN_TEST_H
#define __KIS_ZOOM_AND_PAN_TEST_H

#include <QtTest/QtTest>

class ZoomAndPanTester;


class KisZoomAndPanTest : public QObject
{
    Q_OBJECT
private slots:
    void testZoom100ChangingWidgetSize();

    void testSequentialActionZoomAndPan();
    void testSequentialActionZoomAndPanFullscreen();
    void testSequentialActionZoomAndPanRotate();
    void testSequentialActionZoomAndPanRotateFullscreen();

    void testSequentialWheelZoomAndPan();
    void testSequentialWheelZoomAndPanFullscreen();
    void testSequentialWheelZoomAndPanRotate();
    void testSequentialWheelZoomAndPanRotateFullscreen();


private:

    bool checkInvariants(const QPointF &baseFlakePoint,
                         const QPoint &oldOffset,
                         const QPointF &oldPreferredCenter,
                         qreal oldZoom,
                         const QPoint &newOffset,
                         const QPointF &newPreferredCenter,
                         qreal newZoom,
                         QPointF newTopLeft);

    bool checkZoomWithAction(ZoomAndPanTester &t, qreal newZoom);
    bool checkZoomWithWheel(ZoomAndPanTester &t, const QPoint &widgetPoint, qreal zoomCoeff);
    bool checkPan(ZoomAndPanTester &t, QPoint shift);

    void initializeViewport(ZoomAndPanTester &t, bool fullscreenMode, bool rotate);
    void testSequentialActionZoomAndPan(bool fullscreenMode, bool rotate);
    void testSequentialWheelZoomAndPan(bool fullscreenMode, bool rotate);
};

#endif /* __KIS_ZOOM_AND_PAN_TEST_H */
