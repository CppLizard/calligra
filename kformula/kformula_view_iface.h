/* This file is part of the KDE project
   Copyright (C) 2002 Laurent Montel <lmontel@mandrakesoft.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KFORMULA_VIEW_IFACE_H
#define KFORMULA_VIEW_IFACE_H

#include <KoViewIface.h>

#include <qstring.h>

class KFormulaPartView;

class KformulaViewIface : public KoViewIface
{
    K_DCOP
public:
    KformulaViewIface( KFormulaPartView *view_ );
k_dcop:
    void addThinSpace();
    void addMediumSpace();
    void addThickSpace();
    void addQuadSpace();
    void addDefaultBracket();
    void addSquareBracket();
    void addCurlyBracket();
    void addLineBracket();
    void addFraction();
    void addRoot();
    void addIntegral();
    void addProduct();
    void addSum();
    void addMatrix();


    void addLowerLeftIndex();
    void addUpperLeftIndex();
    void addLowerRightIndex();
    void addUpperRightIndex();
    void addGenericLowerIndex();
    void addGenericUpperIndex();
    void removeEnclosing();
    void makeGreek();
    void insertSymbol();

    void appendColumn();
    void insertColumn();
    void removeColumn();
    void appendRow();
    void insertRow();
    void removeRow();

private:
    KFormulaPartView *m_view;
};

#endif
