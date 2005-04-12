/*
 *  kis_tool_transform.h - part of Krita
 *
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef KIS_TOOL_TRANSFORM_H_
#define KIS_TOOL_TRANSFORM_H_

#include <qpoint.h>
#include <kis_tool.h>
#include <kis_tool_non_paint.h>
#include <kis_tool_factory.h>

/**
 * Transform tool
 *
 */
class KisToolTransform : public KisToolNonPaint {

	typedef KisToolNonPaint super;
	Q_OBJECT

public:
	KisToolTransform();
	virtual ~KisToolTransform();

	virtual void update(KisCanvasSubject *subject);

	virtual QWidget* createOptionWidget(QWidget* parent);
	virtual QWidget* optionWidget();

	virtual void setup(KActionCollection *collection);

	virtual void clear();
	virtual void paint(QPainter& gc);
	virtual void paint(QPainter& gc, const QRect& rc);
	virtual void buttonPress(KisButtonPressEvent *e);
	virtual void move(KisMoveEvent *e);
	virtual void buttonRelease(KisButtonReleaseEvent *e);

private:
	void paintOutline();
	void paintOutline(QPainter& gc, const QRect& rc);
	void transform();
	void recalcOutline();
	double rotX(double x, double y) { return m_cosa*x - m_sina*y;};
	double rotY(double x, double y) { return m_sina*x + m_cosa*y;};
	double invrotX(double x, double y) { return m_cosa*x + m_sina*y;};
	double invrotY(double x, double y) { return -m_sina*x + m_cosa*y;};

private slots:

	void setStartX(int x) { m_startPos.setX(x); }
	void setStartY(int y) { m_startPos.setY(y); }
	void setEndX(int x) { m_endPos.setX(x); }
	void setEndY(int y) { m_endPos.setY(y); }

protected slots:
	virtual void activate();

private:
	enum function {ROTATE,MOVE,TOPLEFTSCALE,TOPSCALE,TOPRIGHTSCALE,RIGHTSCALE,
				BOTTOMRIGHTSCALE, BOTTOMSCALE,BOTTOMLEFTSCALE, LEFTSCALE};
	function m_function;
	KisCanvasSubject *m_subject;
	QPoint m_startPos;
	QPoint m_endPos;
	bool m_selecting;
	QPoint m_topleft;
	QPoint m_topright;
	QPoint m_bottomleft;
	QPoint m_bottomright;
	double m_scaleX;
	double m_scaleY;
	double m_translateX;
	double m_translateY;
	QPoint m_clickoffset;
	double m_org_cenX;
	double m_org_cenY;
	double m_cosa;
	double m_sina;
	double m_a;
	double m_clickangle;

	QWidget * m_optWidget;
};

class KisToolTransformFactory : public KisToolFactory {
	typedef KisToolFactory super;
public:
	KisToolTransformFactory(KActionCollection * ac ) : super(ac) {};
	virtual ~KisToolTransformFactory(){};

	virtual KisTool * createTool() { 
		KisTool * t = new KisToolTransform(); 
		Q_CHECK_PTR(t);
		t -> setup(m_ac); return t;
	}
	virtual KisID id() { return KisID("transform", i18n("Transform tool")); }
};



#endif // KIS_TOOL_TRANSFORM_H_

