/* This file is part of the KDE project
   Copyright 2010 Marijn Kruisselbrink <mkruisselbrink@kde.org>
   Copyright 1998, 1999 Torben Weis <weis@kde.org>
   Copyright 1999- 2006 The KSpread Team <calligra-devel@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef CALLIGRA_SHEETS_CONDITION_H
#define CALLIGRA_SHEETS_CONDITION_H

#include "Style.h"
#include "Value.h"

#include <QDomElement>
#include <QLinkedList>
#include <QSharedData>
#include <QVariant>

#include <KoXmlReader.h>

class QDomDocument;
class QString;
class KoGenStyle;

namespace Calligra
{
namespace Sheets
{
class Cell;
class ValueConverter;
class ValueParser;

/**
 * \class Conditional
 * \ingroup Style
 * Conditional formatting.
 * Holds the actual condition and the applicable style for conditional
 * Cell formattings.
 */
class CALLIGRA_SHEETS_ODF_EXPORT Conditional
{
public:
    enum Type { None, Equal, Superior, Inferior, SuperiorEqual,
                InferiorEqual, Between, Different, DifferentTo,
                IsTrueFormula
              };

    Value          value1;
    Value          value2;
    QString        styleName;
    Type           cond;
    QString        baseCellAddress;

    Conditional();

    bool operator==(const Conditional &other) const;
};


class Conditions;
uint qHash(const Conditions& conditions);
uint qHash(const Conditional& condition);

/**
 * \class Conditions
 * \ingroup Style
 * Manages a set of conditions for a cell.
 */
class CALLIGRA_SHEETS_ODF_EXPORT Conditions
{
public:
    /**
     * Constructor.
     */
    Conditions();

    /**
     * Copy Constructor.
     */
    Conditions(const Conditions& other);

    /**
     * Destructor.
     */
    ~Conditions();

    /**
     * \return \c true if there are no conditions defined
     */
    bool isEmpty() const;

    /**
     * \return the style that matches first (or 0 if no condition matches)
     */
    Style testConditions(const Cell &cell) const;

    /**
     * Retrieve the current list of conditions we're checking
     */
    QLinkedList<Conditional> conditionList() const;

    /**
     * Replace the current list of conditions with this new one
     */
    void setConditionList(const QLinkedList<Conditional> & list);

    /**
      * Add a new condition.
      */
    void addCondition(Conditional cond);

    /**
     * Returns an optional default style, which is returned by testConditons if none of
     * the conditions matches.
     */
    Style defaultStyle() const;

    /**
     * Set an optional default style. This style is returned by testConditions if none of
     * the conditions matches.
     */
    void setDefaultStyle(const Style& style);

    /**
     * \ingroup NativeFormat
     * Takes a parsed DOM element and recreates the conditions structure out of
     * it
     */
    void loadConditions(const KoXmlElement &element, const ValueParser *parser);

    /**
     * \ingroup NativeFormat
     * Saves the conditions to a DOM tree structure.
     * \return the DOM element for the conditions.
     */
    QDomElement saveConditions(QDomDocument &doc, ValueConverter *converter) const;

    /// \note implementation to make QMap happy (which is needed by RectStorage)
    bool operator<(const Conditions& conditions) const {
        return qHash(*this) < qHash(conditions);
    }
    void operator=(const Conditions& other);
    bool operator==(const Conditions& other) const;
    inline bool operator!=(const Conditions& other) const {
        return !operator==(other);
    }

private:
    /**
     * Use this function to see what conditions actually apply currently
     *
     * \param condition a reference to a condition that will be set to the
     *                  matching condition.  If none of the conditions are true
     *                  then this parameter is undefined on exit (check the
     *                  return value).
     *
     * \return true if one of the conditions is true, false if not.
     */
    bool currentCondition(const Cell& cell, Conditional & condition) const;

    bool isTrueFormula(const Cell& cell, const QString& formula, const QString& baseCellAddress) const;

    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace Sheets
} // namespace Calligra

Q_DECLARE_METATYPE(Calligra::Sheets::Conditions)
Q_DECLARE_TYPEINFO(Calligra::Sheets::Conditions, Q_MOVABLE_TYPE);

#endif // CALLIGRA_SHEETS_CONDITION_H
