/*
 * Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 * Copyright (c) 2010 Geoffry Song <goffrie@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
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

#include "PerspectiveAssistant.h"

#include <kdebug.h>
#include <klocale.h>

#include <QPainter>
#include <QLinearGradient>
#include <QTransform>

#include "kis_coordinates_converter.h"

#include <math.h>
#include <limits>

PerspectiveAssistant::PerspectiveAssistant()
        : KisPaintingAssistant("perspective", i18n("Perspective assistant"))
{
}

// squared distance from a point to a line
inline qreal distsqr(const QPointF& pt, const QLineF& line)
{
    // distance = |(p2 - p1) x (p1 - pt)| / |p2 - p1|
    
    // magnitude of (p2 - p1) x (p1 - pt)
    const qreal cross = (line.dx() * (line.y1() - pt.y()) - line.dy() * (line.x1() - pt.x()));
    
    return cross * cross / (line.dx() * line.dx() + line.dy() * line.dy());
}

QPointF PerspectiveAssistant::project(const QPointF& pt, const QPointF& strokeBegin)
{
    const static QPointF nullPoint(std::numeric_limits<qreal>::quiet_NaN(), std::numeric_limits<qreal>::quiet_NaN());
    Q_ASSERT(handles().size() == 4);
    if (snapLine.isNull()) {
        QPolygonF poly;
        if (!quad(poly)) return nullPoint;
        // avoid problems with multiple assistants: only snap if starting in the grid
        if (!poly.containsPoint(strokeBegin, Qt::OddEvenFill)) return nullPoint;
        
        const qreal
            dx = pt.x() - strokeBegin.x(),
            dy = pt.y() - strokeBegin.y();
        if (dx * dx + dy * dy < 4.0) {
            // allow some movement before snapping
            return strokeBegin;
        }
        
        // construct transformation
        QTransform transform;
        if (!QTransform::squareToQuad(poly, transform)) return nullPoint; // shouldn't happen
        bool invertible;
        const QTransform inverse = transform.inverted(&invertible);
        if (!invertible) return nullPoint; // shouldn't happen

        // figure out which direction to go
        const QPointF start = inverse.map(strokeBegin);
        const QLineF
            verticalLine = QLineF(strokeBegin, transform.map(start + QPointF(0, 1))),
            horizontalLine = QLineF(strokeBegin, transform.map(start + QPointF(1, 0)));
        // determine whether the horizontal or vertical line is closer to the point
        snapLine = distsqr(pt, verticalLine) < distsqr(pt, horizontalLine) ? verticalLine : horizontalLine;
    }

    // snap to line
    const qreal
        dx = snapLine.dx(),
        dy = snapLine.dy(),
        dx2 = dx * dx,
        dy2 = dy * dy,
        invsqrlen = 1.0 / (dx2 + dy2);
    QPointF r(dx2 * pt.x() + dy2 * snapLine.x1() + dx * dy * (pt.y() - snapLine.y1()),
              dx2 * snapLine.y1() + dy2 * pt.y() + dx * dy * (pt.x() - snapLine.x1()));
    r *= invsqrlen;
    return r;
}

QPointF PerspectiveAssistant::adjustPosition(const QPointF& pt, const QPointF& strokeBegin)
{
    return project(pt, strokeBegin);
}

void PerspectiveAssistant::endStroke()
{
    snapLine = QLineF();
}

bool PerspectiveAssistant::contains(const QPointF& pt) const
{
    QPolygonF poly;
    if (!quad(poly)) return false;
    return poly.containsPoint(pt, Qt::OddEvenFill);
}

inline qreal lengthSquared(const QPointF& vector)
{
    return vector.x() * vector.x() + vector.y() * vector.y();
}

inline qreal localScale(const QTransform& transform, QPointF pt)
{
//    const qreal epsilon = 1e-5, epsilonSquared = epsilon * epsilon;
//    qreal xSizeSquared = lengthSquared(transform.map(pt + QPointF(epsilon, 0.0)) - orig) / epsilonSquared;
//    qreal ySizeSquared = lengthSquared(transform.map(pt + QPointF(0.0, epsilon)) - orig) / epsilonSquared;
//    xSizeSquared /= lengthSquared(transform.map(QPointF(0.0, pt.y())) - transform.map(QPointF(1.0, pt.y())));
//    ySizeSquared /= lengthSquared(transform.map(QPointF(pt.x(), 0.0)) - transform.map(QPointF(pt.x(), 1.0)));
//  when taking the limit epsilon->0:
//  xSizeSquared=((m23*y+m33)^2*(m23*y+m33+m13)^2)/(m23*y+m13*x+m33)^4
//  ySizeSquared=((m23*y+m33)^2*(m23*y+m33+m13)^2)/(m23*y+m13*x+m33)^4
//  xSize*ySize=(abs(m13*x+m33)*abs(m13*x+m33+m23)*abs(m23*y+m33)*abs(m23*y+m33+m13))/(m23*y+m13*x+m33)^4
    const qreal x = transform.m13() * pt.x(),
                y = transform.m23() * pt.y(),
                a = x + transform.m33(),
                b = y + transform.m33(),
                c = x + y + transform.m33(),
                d = c * c;
    return fabs(a*(a + transform.m23())*b*(b + transform.m13()))/(d * d);
}

// returns the reciprocal of the maximum local scale at the points (0,0),(0,1),(1,0),(1,1)
inline qreal inverseMaxLocalScale(const QTransform& transform)
{
    const qreal a = fabs((transform.m33() + transform.m13()) * (transform.m33() + transform.m23())),
                b = fabs((transform.m33()) * (transform.m13() + transform.m33() + transform.m23())),
                d00 = transform.m33() * transform.m33(),
                d11 = (transform.m33() + transform.m23() + transform.m13())*(transform.m33() + transform.m23() + transform.m13()),
                s0011 = qMin(d00, d11) / a,
                d10 = (transform.m33() + transform.m13()) * (transform.m33() + transform.m13()),
                d01 = (transform.m33() + transform.m23()) * (transform.m33() + transform.m23()),
                s1001 = qMin(d10, d01) / b;
    return qMin(s0011, s1001);
}

qreal PerspectiveAssistant::distance(const QPointF& pt) const
{
    QPolygonF poly;
    if (!quad(poly)) return 1.0;
    QTransform transform;
    if (!QTransform::squareToQuad(poly, transform)) return 1.0;
    bool invertible;
    QTransform inverse = transform.inverted(&invertible);
    if (!invertible) return 1.0;
    if (inverse.m13() * pt.x() + inverse.m23() * pt.y() + inverse.m33() == 0.0) {
        // point at infinity
        return 0.0;
    }
    return localScale(transform, inverse.map(pt)) * inverseMaxLocalScale(transform);
}

// draw a vanishing point marker
inline void drawX(QPainter& gc, const QPointF& pt)
{
    gc.drawLine(QPointF(pt.x() - 5.0, pt.y() - 5.0), QPointF(pt.x() + 5.0, pt.y() + 5.0));
    gc.drawLine(QPointF(pt.x() - 5.0, pt.y() + 5.0), QPointF(pt.x() + 5.0, pt.y() - 5.0));
}

void PerspectiveAssistant::drawAssistant(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter *converter)
{
    Q_UNUSED(updateRect);

    QTransform initialTransform = converter->documentToWidgetTransform();
    QPolygonF poly;
    gc.save();
    gc.setTransform(initialTransform);
    if (!quad(poly)) {
        // color red for an invalid transform, but not for an incomplete one
        gc.setPen((handles().size() == 4) ? QColor(255, 0, 0, 125) : QColor(0, 0, 0, 125));
        gc.drawPolygon(poly);
        gc.restore();
    } else {
        gc.setPen(QColor(0, 0, 0, 125));

        QTransform transform;
        if (!QTransform::squareToQuad(poly, transform)) {
            qWarning("Failed to create perspective mapping");
            // bail out
            gc.restore();
            return;
        }
        gc.setTransform(transform, true);
        for (int y = 0; y <= 8; ++y)
            gc.drawLine(QPointF(0.0, y * 0.125), QPointF(1.0, y * 0.125));
        for (int x = 0; x <= 8; ++x)
            gc.drawLine(QPointF(x * 0.125, 0.0), QPointF(x * 0.125, 1.0));

        gc.restore();
        
        // draw vanishing points
        QPointF intersection(0, 0);
        if (QLineF(poly[0], poly[1]).intersect(QLineF(poly[2], poly[3]), &intersection) != QLineF::NoIntersection) {
            drawX(gc, initialTransform.map(intersection));
        }
        if (QLineF(poly[1], poly[2]).intersect(QLineF(poly[3], poly[0]), &intersection) != QLineF::NoIntersection) {
            drawX(gc, initialTransform.map(intersection));
        }
    }
}

QPointF PerspectiveAssistant::buttonPosition() const
{
    QPointF centroid(0, 0);
    for (int i = 0; i < 4; ++i) centroid += *handles()[i];
    return centroid * 0.25;
}

template <typename T> int sign(T a)
{
    return (a > 0) - (a < 0);
}
// perpendicular dot product
inline qreal pdot(const QPointF& a, const QPointF& b)
{
    return a.x() * b.y() - a.y() * b.x();
}

bool PerspectiveAssistant::quad(QPolygonF& poly) const
{
    for (int i = 0; i < handles().size(); ++i)
        poly.push_back(*handles()[i]);
    if (handles().size() != 4) {
        return false;
    }
    int sum = 0;
    int signs[4];
    for (int i = 0; i < 4; ++i) {
        int j = (i == 3) ? 0 : (i + 1);
        int k = (j == 3) ? 0 : (j + 1);
        signs[i] = sign(pdot(poly[j] - poly[i], poly[k] - poly[j]));
        sum += signs[i];
    }
    if (sum == 0) {
        // complex (crossed)
        for (int i = 0; i < 4; ++i) {
            int j = (i == 3) ? 0 : (i + 1);
            if (signs[i] * signs[j] == -1) {
                // opposite signs: uncross
                qSwap(poly[i], poly[j]);
                return true;
            }
        }
        // okay, maybe it's just a line
        return false;
    } else if (sum != 4 && sum != -4) {
        // concave, or a triangle
        if (sum == 2 || sum == -2) {
            // concave, let's return a triangle instead
            for (int i = 0; i < 4; ++i) {
                int j = (i == 3) ? 0 : (i + 1);
                if (signs[i] != sign(sum)) {
                    // wrong sign: drop the inside node
                    poly.remove(j);
                    return false;
                }
            }
        }
        return false;
    }
    // convex
    return true;
}

PerspectiveAssistantFactory::PerspectiveAssistantFactory()
{
}

PerspectiveAssistantFactory::~PerspectiveAssistantFactory()
{
}

QString PerspectiveAssistantFactory::id() const
{
    return "perspective";
}

QString PerspectiveAssistantFactory::name() const
{
    return i18n("Perspective");
}

KisPaintingAssistant* PerspectiveAssistantFactory::paintingAssistant(const QRectF& /*imageArea*/) const
{
    return new PerspectiveAssistant;
}
