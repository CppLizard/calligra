/* This file is part of the KDE project
   Made by Tomislav Lukman (tomislav.lukman@ck.tel.hr)
   Copyright (C) 2002, The Karbon Developers
*/

#ifndef __VTRANSLATE_H__
#define __VTRANSLATE_H__

#include <qwidget.h>

class QCheckBox;
class QLabel;
class QPushButton;
class QString;
class KDoubleNumInput;

class VTranslate : public QWidget
{
	Q_OBJECT

public:
	VTranslate( QWidget* parent = 0L, const char* name = 0L );
	~VTranslate();

public slots:
	//sets the unit labels do display correct text (mm, cm, pixels etc):
	void setUnits( const QString& units );

private:
	QLabel* m_labelX;
	KDoubleNumInput* m_inputX; //X coordinate
	QLabel* labely;
	KDoubleNumInput* m_inputY; //Y coordinate
	QLabel* m_labelUnit1;
	QLabel* m_labelUnit2;
	QCheckBox* m_checkBoxPosition; //If checked, displays coordinates of selected object
	QPushButton* m_buttonDuplicate; //duplicate (makes a copy of selected object(s) with a new position)
	QPushButton* m_buttonApply; //apply new position
};

#endif

