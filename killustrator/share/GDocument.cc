/* -*- C++ -*-

  $Id$

  This file is part of KIllustrator.
  Copyright (C) 1998-99 Kai-Uwe Sattler (kus@iti.cs.uni-magdeburg.de)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <qfile.h>

#include "GDocument.h"
#include "GDocument.moc"
#include "GPolygon.h"
#include "GText.h"
#include "GPolyline.h"
#include "GOval.h"
#include "GBezier.h"
#include "GClipart.h"
#include "GGroup.h"
#include "GPixmap.h"
#include "GCurve.h"

#include <string>
#include <map>
#include <iostream.h>
#include <cassert>
#include <fstream.h>
#include <strstream.h>

#ifdef __FreeBSD__
#include <math.h>
#else
#include <values.h>
#endif

#include <stack>
#include <vector>
#include <algorithm>

//#include "xmlutils/XmlWriter.h"
//#include "xmlutils/XmlReader.h"

#include "units.h"
#include <klocale.h>
#include <kapp.h>

using namespace std;

// default papersize in mm
#define PAPER_WIDTH 210.0
#define PAPER_HEIGHT 298.0

#define LAYER_VISIBLE   1
#define LAYER_EDITABLE  2
#define LAYER_PRINTABLE 4

template<class T>
struct del_obj {
  void operator () (T* obj) {
    delete obj;
  }
};

struct unselect_obj {
  void operator () (GObject* obj) {
    obj->select (false);
  }
};

GDocument::GDocument () {
  initialize ();
}

GDocument::~GDocument () {
  for_each (layers.begin (), layers.end (), del_obj<GLayer> ());
  layers.clear ();
  selection.clear ();
}

void GDocument::setAutoUpdate (bool flag) {
  autoUpdate = flag;
  if (autoUpdate) {
    selBoxIsValid = false;
    updateHandle ();
    emit changed ();
  }
}

void GDocument::initialize () {
  gridx = gridy = 20.0;
  snapToGrid = snapToHelplines = false;

  pLayout.format = PG_DIN_A4;
  pLayout.orientation = PG_PORTRAIT;
  pLayout.mmWidth = PG_A4_WIDTH;
  pLayout.mmHeight = PG_A4_HEIGHT;
  pLayout.mmLeft = 0; pLayout.mmRight = 0;
  pLayout.mmTop = 0; pLayout.mmBottom = 0;
  pLayout.unit = PG_MM;

  // in pt !!
  paperWidth = (int) cvtMmToPt (pLayout.mmWidth);
  paperHeight = (int) cvtMmToPt (pLayout.mmHeight);
  last = NULL;
  modifyFlag = false;
  filename = UNNAMED_FILE;

  selection.clear ();
  if (! layers.empty ()) {
    vector<GLayer*>::iterator i = layers.begin ();
    for (; i != layers.end (); i++)
      delete *i;
    layers.clear ();
  }
  // add layer for Helplines
  GLayer *l = addLayer ();
  l->setInternal ();
  l->setName (I18N("Helplines"));
  connect (l, SIGNAL(propertyChanged ()),
	   this, SLOT(helplineStatusChanged ()));

  active_layer = addLayer ();

  selBoxIsValid = false;
  autoUpdate = true;
  emit changed ();
}

void GDocument::setModified (bool flag) {
  modifyFlag = flag;
  emit wasModified (flag);
}

void GDocument::setPaperSize (int width, int height) {
  paperWidth = width;
  paperHeight = height;
}

int GDocument::getPaperWidth () const {
  return paperWidth;
}

int GDocument::getPaperHeight () const {
  return paperHeight;
}

void GDocument::drawContents (QPainter& p, bool withBasePoints, bool outline) {
  vector<GLayer*>::iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    GLayer* layer = *i;
    if (! layer->isInternal () && layer->isVisible ()) {
      const list<GObject*>& contents = layer->objects ();
      for (list<GObject*>::const_iterator oi = contents.begin ();
	   oi != contents.end (); oi++)
	(*oi)->draw (p, withBasePoints && (*oi)->isSelected (), outline);
    }
  }
}

void GDocument::drawContentsInRegion (QPainter& p, const Rect& r,
				      bool withBasePoints, bool outline) {
  vector<GLayer*>::iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    GLayer* layer = *i;
    if (! layer->isInternal () && layer->isVisible ()) {
      const list<GObject*>& contents = layer->objects ();
      for (list<GObject*>::const_iterator oi = contents.begin ();
	   oi != contents.end (); oi++) {
	// draw the object only if its bounding box
	// intersects the active region
	//	const Rect& bbox = (*oi)->boundingBox ();
	//	if (r.intersects (bbox))
        if ((*oi)->intersects (r))
	  (*oi)->draw (p, withBasePoints && (*oi)->isSelected (), outline);
      }
    }
  }
}

unsigned int GDocument::objectCount () const {
  unsigned int num = 0;
  vector<GLayer*>::const_iterator i = layers.begin ();
  for (; i != layers.end (); i++)
    num += (*i)->objectCount ();
  return num;
}

void GDocument::insertObject (GObject* obj) {
  obj->ref ();
  active_layer->insertObject (obj);
  connect (obj, SIGNAL(changed()), this, SLOT(objectChanged ()));
  connect (obj, SIGNAL(changed(const Rect&)),
	   this, SLOT(objectChanged (const Rect&)));
  setModified ();
  if (autoUpdate)
    emit changed ();
}

void GDocument::selectObject (GObject* obj) {
  list<GObject*>::iterator i = find (selection.begin (), selection.end (),
				     obj);
  if (i == selection.end ()) {
    // object isn't yet in selection list
    obj->select (true);
    selection.push_back (obj);
    selBoxIsValid = false;
    updateHandle ();
    if (autoUpdate) {
      emit changed ();
      emit selectionChanged ();
    }
  }
}

void GDocument::unselectObject (GObject* obj) {
  list<GObject*>::iterator i = find (selection.begin (), selection.end (),
				     obj);
  if (i != selection.end ()) {
    // remove object from the selection list
    obj->select (false);
    selection.erase (i);
    selBoxIsValid = false;
    updateHandle ();
    if (autoUpdate) {
      emit changed ();
      emit selectionChanged ();
    }
  }
}

void GDocument::unselectAllObjects () {
  if (selection.empty ())
    return;

  for_each (selection.begin (), selection.end (), unselect_obj ());
  selection.clear ();
  selBoxIsValid = false;
  if (autoUpdate) {
    emit changed ();
    emit selectionChanged ();
  }
}

void GDocument::selectAllObjects () {
  selection.clear ();
  vector<GLayer*>::const_iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    GLayer* layer = *i;
    if (layer->isEditable ()) {
      list<GObject*>& contents = layer->objects ();
      for (list<GObject*>::iterator oi = contents.begin ();
	   oi != contents.end (); oi++) {
	GObject* obj = *oi;
	obj->select (true);
	selection.push_back (obj);
      }
    }
  }
  selBoxIsValid = false;
  updateHandle ();
  if (autoUpdate) {
    emit changed ();
    emit selectionChanged ();
  }
}

void GDocument::setLastObject (GObject* obj) {
  if (obj == 0L || obj->getLayer () != 0L)
    last = obj;
}

void GDocument::updateHandle () {
  Rect r = boundingBoxForSelection ();
  if (selectionIsEmpty ())
    selHandle.show (false);
  else
    selHandle.setBox (r);
}

Rect GDocument::boundingBoxForSelection () {
  if (! selBoxIsValid) {
    if (! selectionIsEmpty ()) {
      list<GObject*>::iterator i = selection.begin ();
      selBox = (*i++)->boundingBox ();
      for (; i != selection.end (); i++)
        selBox = selBox.unite ((*i)->boundingBox ());
    }
    else {
      selBox = Rect ();
    }
    selBoxIsValid = true;
  }
  return selBox;
}

Rect GDocument::boundingBoxForAllObjects () {
  Rect box;

  bool init = false;

  for (vector<GLayer*>::iterator li = layers.begin ();
       li != layers.end (); li++) {
    GLayer* layer = *li;
    if (! layer->isInternal () && layer->isEditable ()) {
      list<GObject*>& contents = layer->objects ();
      list<GObject*>::iterator oi = contents.begin ();
      if (! init) {
	box = (*oi++)->boundingBox ();
	init = true;
      }
      for (; oi != contents.end (); oi++)
	box = box.unite ((*oi)->boundingBox ());
    }
  }
  return box;
}

void GDocument::deleteSelectedObjects () {
  if (! selectionIsEmpty ()) {
    for (list<GObject*>::iterator i = selection.begin ();
	 i != selection.end (); i++) {
      GObject* obj = *i;
      disconnect (obj, SIGNAL(changed()), this, SLOT(objectChanged ()));
      disconnect (obj, SIGNAL(changed(const Rect&)),
		  this, SLOT(objectChanged (const Rect&)));
      obj->getLayer ()->deleteObject (obj);
    }
    selection.clear ();
    last = 0L;
    setModified ();
    selBoxIsValid = false;
    if (autoUpdate) {
      emit changed ();
      emit selectionChanged ();
    }
  }
}

void GDocument::deleteObject (GObject* obj) {
  bool selected = false;

  GLayer* layer = obj->getLayer ();
  assert (layer);
  if (layer->isEditable ()) {
    selected = obj->isSelected ();
    if (selected)
      selection.remove (obj);
    last = 0L;
    setModified ();
    disconnect (obj, SIGNAL(changed()), this, SLOT(objectChanged ()));
    disconnect (obj, SIGNAL(changed(const Rect&)),
		this, SLOT(objectChanged (const Rect&)));
    layer->deleteObject (obj);
    if (selected) {
      selBoxIsValid = false;
      updateHandle ();
      if (autoUpdate)
	emit selectionChanged ();
    }
    if (autoUpdate)
      emit changed ();
  }
}

/**
 * Looks for an object of type <tt>otype</tt> which endpoints are distant
 * not more than <tt>max_dist</tt> from the point <tt>x, y</tt>.
 * The method returns <tt>true</tt> if an object was found as well as
 * the object in <tt>obj</tt> and the index of the nearest point in
 * <tt>pidx</tt>.
 */
bool GDocument::findNearestObject (const char* otype, int x, int y,
				   float max_dist, GObject*& obj,
				   int& pidx, bool all) {
  float d, distance = MAXFLOAT;
  obj = 0L;
  Coord p (x, y);

  for (vector<GLayer*>::reverse_iterator li = layers.rbegin ();
       li != layers.rend (); li++) {
    GLayer* layer = *li;
    if (layer->isEditable ()) {
      list<GObject*>& contents = layer->objects ();
      for (list<GObject*>::iterator oi = contents.begin ();
	   oi != contents.end (); oi++) {
	if (otype == 0L || (*oi)->isA (otype)) {
	  if ((*oi)->findNearestPoint (p, max_dist, d, pidx, all) &&
	      d < distance) {
	    obj = *oi;
	    distance = d;
	  }
	}
      }
    }
  }
  if (obj == 0L)
    pidx = -1;
  return obj != 0L;
}

GObject* GDocument::findContainingObject (int x, int y) {
  GObject* result = 0L;
  // We are looking for the most relevant object, that means the object
  // in front of all others. So, we have to start at the upper layer
  vector<GLayer*>::reverse_iterator i = layers.rbegin ();
  for (; i != layers.rend (); i++) {
    GLayer* layer = *i;
    if (layer->isEditable ()) {
      result = layer->findContainingObject (x, y);
      if (result)
	break;
    }
  }
  return result;
}

bool GDocument::findContainingObjects (int x, int y, QList<GObject>& olist) {
  Coord coord (x, y);
  for (vector<GLayer*>::iterator li = layers.begin ();
       li != layers.end (); li++) {
    if ((*li)->isEditable ()) {
      list<GObject*>& contents = (*li)->objects ();
      for (list<GObject*>::iterator oi = contents.begin ();
	   oi != contents.end (); oi++)
	if ((*oi)->contains (coord))
	  olist.append (*oi);
    }
  }
  return olist.count () > 0;
}

bool GDocument::findObjectsContainedIn (const Rect& r, QList<GObject>& olist) {
  for (vector<GLayer*>::iterator li = layers.begin ();
       li != layers.end (); li++) {
    if ((*li)->isEditable ()) {
      list<GObject*>& contents = (*li)->objects ();
      for (list<GObject*>::iterator oi = contents.begin ();
	   oi != contents.end (); oi++)
	if (r.contains ((*oi)->boundingBox ()))
	  olist.append (*oi);
    }
  }
  return olist.count () > 0;
}

void GDocument::layerChanged () {
  if (!autoUpdate)
    return;

  emit changed ();
}

void GDocument::objectChanged () {
  if (!autoUpdate)
    return;

  if (! selectionIsEmpty ()) {
    selBoxIsValid = false;
    updateHandle ();
    GObject* obj = (GObject *) sender ();
    if (obj->isSelected () && autoUpdate) {
      emit selectionChanged ();
    }
  }
  setModified ();
  if (autoUpdate)
      emit changed ();
}

void GDocument::objectChanged (const Rect& r) {
  if (!autoUpdate)
    return;

  if (! selectionIsEmpty ()) {
    selBoxIsValid = false;
    updateHandle ();
    /*
    GObject* obj = (GObject *) sender ();
    if (obj->isSelected () && autoUpdate) {
      emit selectionChanged ();
    }
    */
  }
  setModified ();
  if (autoUpdate)
      emit changed (r);
}

QDomDocument GDocument::saveToXml () {

    static const char* formats[] = {
	"a3", "a4", "a5", "us_letter", "us_legal", "screen", "custom"
	    };
    static const char* orientations[] = {
	"portrait", "landscape"
	    };

    QDomDocument document("killustator");
    document.appendChild( document.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
    QDomElement killustrator=document.createElement("killustaror");
    killustrator.setAttribute("editor", "KIllustrator");
    killustrator.setAttribute ("mime", KILLUSTRATOR_MIMETYPE);
    killustrator.setAttribute ("comment",(const char *)comment);
    killustrator.setAttribute ("keywords",(const char *)keywords);
    document.appendChild(killustrator);

    QDomElement head=document.createElement("head");
    killustrator.appendChild(head);

    QDomElement layout=document.createElement("layout");
    layout.setAttribute ("format", formats[pLayout.format]);
    layout.setAttribute ("orientation", orientations[pLayout.orientation]);
    layout.setAttribute ("width", pLayout.mmWidth);
    layout.setAttribute ("height", pLayout.mmHeight);
    layout.setAttribute ("lmargin", pLayout.mmLeft);
    layout.setAttribute ("tmargin", pLayout.mmTop);
    layout.setAttribute ("rmargin", pLayout.mmRight);
    layout.setAttribute ("bmargin", pLayout.mmBottom);
    head.appendChild(layout);

    QDomElement grid=document.createElement("grid");
    grid.setAttribute ("dx", gridx);
    grid.setAttribute ("dy", gridy);
    grid.setAttribute ("align", snapToGrid ? 1 : 0);
    head.appendChild(grid);

    QDomElement helplines=document.createElement("helplines");
    helplines.setAttribute ("align", snapToHelplines ? 1 : 0);
    vector<float>::iterator hi;
    for (hi = hHelplines.begin (); hi != hHelplines.end (); hi++) {
	QDomElement hl=document.createElement("hl");
	hl.setAttribute ("pos", *hi);
	helplines.appendChild(hl);
    }
    for (hi = vHelplines.begin (); hi != vHelplines.end (); hi++) {
	QDomElement vl=document.createElement("vl");
	vl.setAttribute ("pos", *hi);
	helplines.appendChild(vl);
    }
    grid.appendChild(helplines);

    bool save_layer_info = (layers.size () > 2);
    for (vector<GLayer*>::iterator li = layers.begin ();
	 li != layers.end (); li++) {
	if ((*li)->isInternal ())
	    continue;

	QDomElement layer;
	if (save_layer_info) {
	    int flags = ((*li)->isVisible () ? LAYER_VISIBLE : 0) +
			((*li)->isPrintable () ? LAYER_PRINTABLE : 0) +
			((*li)->isEditable () ? LAYER_EDITABLE : 0);
	    layer=document.createElement("layer");
	    layer.setAttribute ("id", (*li)->name ());
	    layer.setAttribute ("flags", flags);
	    killustrator.appendChild(layer);
	}
	list<GObject*>& contents = (*li)->objects ();
	for (list<GObject*>::iterator oi = contents.begin ();
	     oi != contents.end (); oi++)
	    layer.appendChild((*oi)->writeToXml (document));
    }

    setModified (false);
    return document;
}

bool GDocument::insertFromXml (const QDomDocument &document, list<GObject*>& newObjs) {

    if ( document.doctype().name() != "doc" )
	return false;
    QDomElement doc = document.documentElement();

    if ( doc.attribute( "mime" ) != KILLUSTRATOR_MIMETYPE )
	return false;
    return parseBody (doc, newObjs, true);
}

bool GDocument::parseBody (const QDomElement &element, std::list<GObject*>& newObjs, bool markNew) {
    /*
    GObject* obj = 0L;
    stack<GGroup*, vector<GGroup*> > groups;
    bool finished = false;
    bool endOfBody = false;
    map<string, GObject*> idtable;

    QDomElement child = element.firstChild().toElement();
    for( ; !child.isNull(); child = child.nextSibling().toElement() ) {
    */
	/*	else if (elem.isEndTag ()) {
		finished = true;
		if (elem.tag () == "group") {
		// group object is finished -> recalculate bbox
		groups.top ()->calcBoundingBox ();
		groups.pop ();
		}
		}*/
    
	//finished = elem.isClosed () && elem.tag () != "point";
    /*
	if (child.tagName() == "layer") {
	    if (layers.size () == 1 && active_layer->objectCount () == 0)
		// add objects to the current layer
		;
	    else
		active_layer = addLayer ();
	    active_layer->setName (child.attribute("id"));
	    int flags = child.attribute("flags").toInt();
	    active_layer->setVisible (flags & LAYER_VISIBLE);
	    active_layer->setPrintable (flags & LAYER_EDITABLE);
	    active_layer->setEditable (flags & LAYER_PRINTABLE);
	}
	else if (child.tagName () == "polyline")
	    obj = new GPolyline (child);
	else if (child.tagName () == "ellipse")
	    obj = new GOval (child);
	else if (child.tagName () == "bezier")
	    obj = new GBezier (child);
	else if (child.tagName () == "rectangle")
	    obj = new GPolygon (child, GPolygon::PK_Rectangle);
	else if (child.tagName () == "polygon")
	    obj = new GPolygon (child);
	else if (child.tagName () == "clipart")
	    obj = new GClipart (child);
	else if (child.tagName () == "pixmap")
	    obj = new GPixmap (child);
	else if (child.tagName () == "curve") {
	    obj = new GCurve (child);
	    QDomElement segment = child.firstChild().toElement();
	    for( ; !segment.isNull(); segment = segment.nextSibling().toElement() ) {
		if (segment.tagName() != "seg")
		    return false;
		GSegment::Kind kind = (GSegment::Kind)segment.attribute("kind").toInt();
		GSegment seg (kind);
		int lim=(kind==GSegment::sk_Bezier) ? 4 : 2;
		for (int i = 0; i < lim; i++) {
		    Coord p;
		    QDomElement point = segment.firstChild().toElement();
		    for( ; !point.isNull(); point = segment.nextSibling().toElement() ) {
			p.x(point.attribute("x").toFloat());
			p.y(point.attribute("y").toFloat());
			seg.setPoint (i, p);
		    }
		}
		((GCurve *) obj)->addSegment (seg);
	    }
	}
	else if (child.tagName() == "text") {
	    obj = new GText(child);
	    // read font attributes
	    QDomElement f = child.namedItem("font").toElement();
	    QFont font = QFont::defaultFont ();
	    font.setFamily(f.attribute("face"));
	    font.setPointSize(f.attribute("point-size").toInt());
	    font.setWeight(f.attribute("weight").toInt());
	    font.setItalic(f.attribute("italic").toInt());
	    ((GText *)obj)->setFont (font);
	    ((GText *) obj)->setText (child.text()); // Did I already say that I love QDom? :)
	}
	else if (child.tagName() == "group") {
	    GGroup* group = new GGroup (child);
	    group->setLayer (active_layer);
	    //	group->ref ();
	    if (!groups.empty ()) {
		groups.top ()->addObject (group);
	    }
	    else {
		if (markNew)
		    newObjs.push_back (group);
		insertObject (group);
	    }
	    groups.push (group);
	}
	else if (child.tagName() == "point") {
	    // add a point to the object
	    Coord point;
	    point.x(child.attribute("x").toFloat());
	    point.y(child.attribute("y").toFloat());
	    assert (obj != 0L);
	    if(obj->inherits ("GPolyline")) {
		GPolyline* poly = (GPolyline *) obj;
		poly->_addPoint (poly->numOfPoints (), point);
	    }
	}
	else {
	    GObject *proto = GObject::lookupPrototype (child.tagName());
	    if (proto != 0L) {
		obj = proto->clone (child);
	    }
	    else
		kdDebug() << "invalid object type: " << child.tagName() << endl;
	}
    }
    if (finished) {
	if (obj) {
	    if (!groups.empty ()) {
		obj->setLayer (active_layer);
		groups.top ()->addObject (obj);
	    }
	    else {
		if (markNew)
		    newObjs.push_back (obj);
		if (obj->hasId ())
		    idtable[obj->getId ()] =  obj;
	
		insertObject (obj);
	    }
	    obj = 0L;
	}
	finished = false;
    }

   //////////////////////////////////////
    do {
	if (! xml.readElement (elem))
	    break;
	if (elem.tag () == "kiml" || elem.tag () == "doc") {
	    if (! elem.isEndTag ())
		break;
	    else
		endOfBody = true;
	}
	else if (elem.isEndTag ()) {
	    finished = true;
	    if (elem.tag () == "group") {
		// group object is finished -> recalculate bbox
		groups.top ()->calcBoundingBox ();
		groups.pop ();
	    }
	}
	else {
	    finished = elem.isClosed () && elem.tag () != "point";

	    if (elem.tag () == "layer") {
		if (layers.size () == 1 && active_layer->objectCount () == 0)
		    // add objects to the current layer
		    ;
		else
		    active_layer = addLayer ();
		list<XmlAttribute>::const_iterator first =
		    elem.attributes ().begin ();
		while (first != elem.attributes ().end ()) {
		    const string& attr = (*first).name ();
		    if (attr == "id")
			active_layer->setName ((*first).stringValue ().c_str ());
		    else if (attr == "flags") {
			int flags = (*first).intValue ();
			active_layer->setVisible (flags & LAYER_VISIBLE);
			active_layer->setPrintable (flags & LAYER_EDITABLE);
			active_layer->setEditable (flags & LAYER_PRINTABLE);
		    }
		    first++;
		}
	    }
	    else if (elem.tag () == "polyline")
		obj = new GPolyline (elem.attributes ());
	    else if (elem.tag () == "ellipse")
		obj = new GOval (elem.attributes ());
	    else if (elem.tag () == "bezier")
		obj = new GBezier (elem.attributes ());
	    else if (elem.tag () == "rectangle")
		obj = new GPolygon (elem.attributes (), GPolygon::PK_Rectangle);
	    else if (elem.tag () == "polygon")
		obj = new GPolygon (elem.attributes ());
	    else if (elem.tag () == "clipart")
		obj = new GClipart (elem.attributes ());
	    else if (elem.tag () == "pixmap")
		obj = new GPixmap (elem.attributes ());
	    else if (elem.tag () == "curve") {
		obj = new GCurve (elem.attributes ());
		finished = false;
		if (! xml.readElement (elem))
		    // something goes wrong
		    return false;

		do {
		    GSegment::Kind kind = GSegment::sk_Line;
		    if (elem.tag () != "seg")
			return false;

		    list<XmlAttribute>::const_iterator first =
			elem.attributes ().begin ();
		    while (first != elem.attributes ().end ()) {
			const string& attr = (*first).name ();
			if (attr == "kind")
			    kind = (GSegment::Kind) (*first).intValue ();
			first++;
		    }
		    GSegment seg (kind);
		    if (kind == GSegment::sk_Line) {
			for (int i = 0; i < 2; i++) {
			    Coord p;
			    if (! xml.readElement (elem) || elem.tag () != "point")
				return false;
			    first = elem.attributes ().begin ();
	
			    while (first != elem.attributes ().end ()) {
				if ((*first).name () == "x")
				    p.x ((*first).floatValue ());
				else if ((*first).name () == "y")
				    p.y ((*first).floatValue ());
				first++;
			    }
			    seg.setPoint (i, p);
			}
		    }
		    else {
			for (int i = 0; i < 4; i++) {
			    Coord p;
			    if (! xml.readElement (elem) || elem.tag () != "point")
				return false;
			    first = elem.attributes ().begin ();
	
			    while (first != elem.attributes ().end ()) {
				if ((*first).name () == "x")
				    p.x ((*first).floatValue ());
				else if ((*first).name () == "y")
				    p.y ((*first).floatValue ());
				first++;
			    }
			    seg.setPoint (i, p);
			}
		    }
		    if (! xml.readElement (elem) || elem.tag () != "seg" ||
			! elem.isEndTag ())
			return false;
		    ((GCurve *) obj)->addSegment (seg);

		    if (! xml.readElement (elem))
			return false;
		    // end of element
		    if (elem.tag () == "curve" && elem.isEndTag ())
			finished = true;
		} while (! finished);
	    }
	    else if (elem.tag () == "text") {
		obj = new GText (elem.attributes ());
		// read font attributes
		if (! xml.readElement (elem) || elem.tag () != "font")
		    break;

		list<XmlAttribute>::const_iterator first = elem.attributes ().begin ();
		QFont font = QFont::defaultFont ();

		while (first != elem.attributes ().end ()) {
		    const string& attr = (*first).name ();
		    if (attr == "face")
			font.setFamily ((*first).stringValue ().c_str ());
		    else if (attr == "point-size")
			font.setPointSize ((*first).intValue ());
		    else if (attr == "weight")
			font.setWeight ((*first).intValue ());
		    else if (attr == "italic")
			font.setItalic ((*first).intValue () != 0);
		    first++;
		}
		((GText *)obj)->setFont (font);

		// and the text
		finished = false;
		QString text_str;
		do {
		    if (! xml.readElement (elem))
			// something goes wrong
			break;
		    if (elem.tag () == "#PCDATA")
			text_str += xml.getText ().c_str ();
		    else if (elem.tag () == "font" && elem.isEndTag ())
			// end of font tag - ignore it
			;
		    else if (elem.tag () == "br")
			// newline
			text_str += "\n";

		    // end of element
		    if (elem.tag () == "text" && elem.isEndTag ()) {
			((GText *) obj)->setText (text_str);
			finished = true;
		    }
		} while (! finished);
	    }
	    else if (elem.tag () == "group") {
		GGroup* group = new GGroup (elem.attributes ());
		group->setLayer (active_layer);
		//	group->ref ();

		if (!groups.empty ()) {
		    groups.top ()->addObject (group);
		}
		else {
		    if (markNew)
			newObjs.push_back (group);
		    insertObject (group);
		}
		groups.push (group);
	    }
	    else if (elem.tag () == "point") {
		// add a point to the object
		list<XmlAttribute>::const_iterator first = elem.attributes ().begin ();
		Coord point;
	
		while (first != elem.attributes ().end ()) {
		    if ((*first).name () == "x")
			point.x ((*first).floatValue ());
		    else if ((*first).name () == "y")
			point.y ((*first).floatValue ());
		    first++;
		}
		assert (obj != 0L);
		if (obj->inherits ("GPolyline")) {
		    GPolyline* poly = (GPolyline *) obj;
		    poly->_addPoint (poly->numOfPoints (), point);
		}
	    }
	    else {
		GObject *proto = GObject::lookupPrototype (elem.tag ().c_str ());
		if (proto != 0L) {
		    obj = proto->clone (elem.attributes ());
		}
		else
		    cout << "invalid object type: " << elem.tag () << endl;
	    }
	}
	if (finished) {
	    if (obj) {
		if (!groups.empty ()) {
		    obj->setLayer (active_layer);
		    groups.top ()->addObject (obj);
		}
		else {
		    if (markNew)
			newObjs.push_back (obj);
		    if (obj->hasId ())
			idtable[obj->getId ()] =  obj;
	
		    insertObject (obj);
		}
		obj = 0L;
	    }
	    finished = false;
	}
    } while (! endOfBody);
	/////////////////////////////////////////////////////////

    // update object connections
    vector<GLayer*>::iterator i = layers.begin ();
    for (; i != layers.end (); i++) {
	GLayer* layer = *i;
	list<GObject*>& contents = layer->objects ();
	for (list<GObject*>::iterator oi = contents.begin ();
	     oi != contents.end (); oi++) {
	    // this should be more general !!
	    if ((*oi)->hasRefId () && (*oi)->isA ("GText")) {
		const char* id = (*oi)->getRefId ();
		map<string, GObject*>::iterator mi = idtable.find (id);
		if (mi != idtable.end ()) {
		    GText *tobj = (GText *) *oi;
		    tobj->setPathObject (mi->second);
		}
	    }
	}
    }

    setAutoUpdate (true);
    */
    return true;
}

bool GDocument::readFromXml (const  QDomDocument &document) {

    //bool endOfHeader = false;

    if ( document.doctype().name() != "killustrator" )
	return false;

    QDomElement killustrator = document.documentElement();

    if ( killustrator.attribute( "mime" ) != KILLUSTRATOR_MIMETYPE )
	return false;

    comment=killustrator.attribute("comment");
    keywords=killustrator.attribute("keywords");

    QDomElement head=killustrator.namedItem("head").toElement();
    setAutoUpdate (false);

    kapp->processEvents (500);

    QDomElement layout=head.namedItem("layout").toElement();
    QString tmp=layout.attribute("format");
    if (tmp == "a3")
	pLayout.format = PG_DIN_A3;
    else if (tmp == "a4")
	pLayout.format = PG_DIN_A4;
    else if (tmp == "a5")
	pLayout.format = PG_DIN_A5;
    else if (tmp == "usletter")
	pLayout.format = PG_US_LETTER;
    else if (tmp == "uslegal")
	pLayout.format = PG_US_LEGAL;
    else if (tmp == "custom")
	pLayout.format = PG_CUSTOM;
    else
	pLayout.format = PG_DIN_A4;

    tmp=layout.attribute("orientation");
    if (tmp == "portrait")
	pLayout.orientation = PG_PORTRAIT;
    else if (tmp == "landscape")
	pLayout.orientation = PG_LANDSCAPE;
    else
	pLayout.orientation = PG_PORTRAIT;

    pLayout.mmWidth=layout.attribute("width").toFloat();
    pLayout.mmHeight=layout.attribute("height").toFloat();
    pLayout.mmLeft=layout.attribute("lmargin").toFloat();
    pLayout.mmRight=layout.attribute("rmargin").toFloat();
    pLayout.mmBottom=layout.attribute("bmargin").toFloat();
    pLayout.mmTop=layout.attribute("tmargin").toFloat();

    QDomElement grid=head.namedItem("grid").toElement();
    gridx=grid.attribute("dx").toFloat();
    gridy=grid.attribute("dy").toFloat();
    snapToGrid=(grid.attribute("align").toInt()==1);

    QDomElement helplines=grid.namedItem("helplines").toElement();
    snapToHelplines=(helplines.attribute("align").toInt()==1);

    QDomElement l=helplines.firstChild().toElement();
    for( ; !l.isNull(); l=helplines.nextSibling().toElement()) {
	if(l.tagName()=="hl")
	    hHelplines.push_back(l.attribute("pos").toFloat());
	else if(l.tagName()=="vl")
	    vHelplines.push_back(l.attribute("pos").toFloat());
    }

    // update page layout
    setPageLayout (pLayout);

    list<GObject*> dummy;
    bool result = parseBody (killustrator, dummy, false);

    setModified (false);
    emit gridChanged ();
    return result;
}

unsigned int GDocument::findIndexOfObject (GObject *obj) {
  assert (obj->getLayer () != 0L);
  return obj->getLayer ()->findIndexOfObject (obj);
}

void GDocument::insertObjectAtIndex (GObject* obj, unsigned int idx) {
  obj->ref ();
  GLayer* layer = obj->getLayer ();
  if (layer == 0L)
    layer = active_layer;
  layer->insertObjectAtIndex (obj, idx);
  connect (obj, SIGNAL(changed()), this, SLOT(objectChanged ()));
  connect (obj, SIGNAL(changed(const Rect&)),
	   this, SLOT(objectChanged (const Rect&)));
  setModified ();
  if (autoUpdate) {
    emit changed ();
    emit selectionChanged ();
  }
}

void GDocument::moveObjectToIndex (GObject* obj, unsigned int idx) {
  GLayer* layer = obj->getLayer ();
  if (layer == 0L)
    layer = active_layer;
  layer->moveObjectToIndex (obj, idx);

  setModified ();
  if (autoUpdate) {
    emit changed ();
    emit selectionChanged ();
  }
}

KoPageLayout GDocument::pageLayout () {
  return pLayout;
}

void GDocument::setPageLayout (const KoPageLayout& layout) {
  pLayout = layout;
  switch (layout.unit) {
  case PG_MM:
    paperWidth = (int) cvtMmToPt (pLayout.mmWidth);
    paperHeight = (int) cvtMmToPt (pLayout.mmHeight);
    break;
  case PG_PT:
    paperWidth = static_cast<int>(pLayout.ptWidth);
    paperHeight = static_cast<int>(pLayout.ptHeight);
    break;
  case PG_INCH:
    paperWidth = (int) cvtInchToPt (pLayout.inchWidth);
    paperHeight = (int) cvtInchToPt (pLayout.inchHeight);
    break;
  }
  modifyFlag = true;
  emit sizeChanged ();
}

/*
 * Get an array with all layers of the document
 */
const vector<GLayer*>& GDocument::getLayers () {
  return layers;
}

/*
 * Set the active layer where further actions take place
 */
void GDocument::setActiveLayer (GLayer *layer) {
  vector<GLayer*>::iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    if (*i == layer) {
      active_layer = layer;
      break;
    }
  }
}

/*
 * Retrieve the active layer
 */
GLayer* GDocument::activeLayer () {
    return active_layer;
}

/*
 * Raise the given layer
 */
void GDocument::raiseLayer (GLayer *layer) {
  if (layer->isInternal ())
    return;

  if (layer == layers.back ())
    // layer is already on top
    return;

  vector<GLayer*>::iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    if (*i == layer) {
      vector<GLayer*>::iterator j = layers.erase (i);
      layers.insert (++j, layer);
      break;
    }
  }
  emit changed ();
}

/*
 * Lower the given layer
 */
void GDocument::lowerLayer (GLayer *layer) {
  if (layer->isInternal ())
    return;

  if (layer == layers.front ())
    // layer is already at bottom
    return;

  vector<GLayer*>::iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    if (*i == layer) {
      vector<GLayer*>::iterator j = layers.erase (i);
      layers.insert (--j, layer);
      break;
    }
  }
  emit changed ();
}

/*
 * Add a new layer on top of existing layers
 */
GLayer* GDocument::addLayer () {
  GLayer* layer = new GLayer (this);
  connect (layer, SIGNAL(propertyChanged ()), this, SLOT(layerChanged ()));
  layers.push_back (layer);
  return layer;
}

/*
 * Delete the given layer as well as all contained objects
 */
void GDocument::deleteLayer (GLayer *layer) {
  if (layer->isInternal ())
    return;

  if (layers.size () == 1)
    // we need at least one layer
    return;

  bool update = (active_layer == layer);

  vector<GLayer*>::iterator i = layers.begin ();
  for (; i != layers.end (); i++) {
    if (*i == layer) {
      // remove the layer from the array
      vector<GLayer*>::iterator n = layers.erase (i);
      // and delete the layer
      disconnect (layer, SIGNAL(propertyChanged ()),
		  this, SLOT(layerChanged ()));
      delete layer;

      if (update) {
	// the removed layer was the active layer !
	
	if (n == layers.end ()) {
	  // this was the upper layer, so the
	  // the active layer to the last one
	  active_layer = layers.back ();
	}
	else
	  active_layer = *n;
      }
      break;
    }
  }
  emit selectionChanged ();
  emit changed ();
}

GLayer *GDocument::layerForHelplines () {
  return layers[0];
}

bool GDocument::helplineLayerIsActive () {
  return (active_layer->isInternal ());
}

void GDocument::printInfo (QString& s) {
    ostrstream os;
    int n = 0;

    for (vector<GLayer*>::iterator li = layers.begin ();
	 li != layers.end (); li++) {
	GLayer* layer = *li;
	list<GObject*>& contents = layer->objects ();
	n += contents.size ();
    }
    os << i18n ("Document") << ": "<< (const char *) filename << '\n'
       << i18n ("Layers") << ": " << layers.size () << '\n'
       << i18n ("Objects") << ": " << n << ends;
    s += os.str ();
}

void GDocument::invalidateClipRegions () {
  for (vector<GLayer*>::iterator li = layers.begin ();
       li != layers.end (); li++) {
    GLayer* layer = *li;
    if (layer->isVisible ()) {
      list<GObject*>& contents = layer->objects ();
      list<GObject*>::iterator oi = contents.begin ();
      for (; oi != contents.end (); oi++)
	(*oi)->invalidateClipRegion ();
    }
  }
}

void GDocument::setGrid (float dx, float dy, bool snap) {
  gridx = dx;
  gridy = dy;
  snapToGrid = snap;
}

void GDocument::getGrid (float& dx, float& dy, bool& snap) {
  dx = gridx;
  dy = gridy;
  snap = snapToGrid;
}

void GDocument::setHelplines (const vector<float>& hlines,
			      const vector<float>& vlines,
			      bool snap) {
  hHelplines = hlines;
  vHelplines = vlines;
  snapToHelplines = snap;
}

void GDocument::getHelplines (vector<float>& hlines, vector<float>& vlines,
			      bool& snap) {
  hlines = hHelplines;
  vlines = vHelplines;
  snap = snapToHelplines;
}

// called from internal layer when visible flag was changed
void GDocument::helplineStatusChanged () {
  emit gridChanged ();
}

void GDocument::setComment(QString s){
  comment = s;
}

void GDocument::getComment(QString &s){
  s = comment;
}

void GDocument::setKeywords(QString s){
  keywords = s;
}

void GDocument::getKeywords(QString &s){
  s = keywords;
}

void GDocument::selectNextObject () {
  GObject *newSel = 0L;

  if (selectionIsEmpty ()) {
    newSel = active_layer->objects ().front ();

  }
  else {
    GObject *oldSel = selection.front ();
    unsigned int idx = findIndexOfObject (oldSel);
    if (++idx >= active_layer->objects ().size ())
      idx = 0;
    newSel = active_layer->objectAtIndex (idx);
  }
  setAutoUpdate (false);
  unselectAllObjects ();

  setAutoUpdate (true);
  if (newSel) {
    handle ().show (true);
    selectObject (newSel);
  }
}

void GDocument::selectPrevObject () {
}

