/*
 *  Copyright (c) 2014 Denis Kuplyakov <dener.kup@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KOSECTIONMANAGER_H
#define KOSECTIONMANAGER_H

#include <QTextDocument>
#include <QMetaType>
#include <QStandardItemModel>
#include <QSet>

#include <kotext_export.h>

class KoSection;
class KoSectionManagerPrivate;

/**
 * Used to handle all the sections in the document
 *
 * Sections are registered in KoSection constructor and unregistered
 * in KoSection destructor.
 *
 * Info is invalidated in DeleteCommand, because we can't check it from here.
 * Registering and unregistering section also calls invalidate().
 *
 * This object is created for QTextDocument on the first query of it.
 */
class KOTEXT_EXPORT KoSectionManager
{
public:
    explicit KoSectionManager(QTextDocument* doc);
    ~KoSectionManager();

    /**
     * Returns pointer to the deepest KoSection that covers @p pos
     * or NULL if there is no such section
     */
    KoSection *sectionAtPosition(int pos);

    /**
     * Returns name for the new section
     */
    QString possibleNewName();

    /**
     * Returns if this name is possible.
     */
    bool isValidNewName(QString name);

public slots:
    /**
     * Call this to recalc all sections information
     * @param needModel place @c true to it if you need model to use in GUI and @c false otherwise
     * @return pointer to QStandardItemModel, build according to section tree
     *         with a pointers to KoSection at Qt::UserRole + 1 and section name
     *         for display role.
     *         NOTE: it is not updated further by KoSectionManager
     */
    QStandardItemModel *update(bool needModel = false);

    /**
     * Call this to notify manager that info in it has invalidated.
     */
    void invalidate();

    /**
     * Call this to register new section in manager
     */
    void registerSection(KoSection *section);

    /**
     * Call this to unregister section from manager
     */
    void unregisterSection(KoSection *section);

protected:
    KoSectionManagerPrivate * const d_ptr;

private:
    Q_DISABLE_COPY(KoSectionManager)
    Q_DECLARE_PRIVATE(KoSectionManager)
};

Q_DECLARE_METATYPE(KoSectionManager *)

#endif //KOSECTIONMANAGER_H
