/***************************************************************************
 * kexidbschema.h
 * copyright (C)2004-2005 by Sebastian Sauer (mail@dipe.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef KROSS_KEXIDB_KEXIDBSCHEMA_H
#define KROSS_KEXIDB_KEXIDBSCHEMA_H

#include <qstring.h>

#include "../api/object.h"
//#include "../api/list.h"
//#include "../api/module.h"
#include "../api/class.h"

#include <kexidb/drivermanager.h>
#include <kexidb/schemadata.h>
#include <kexidb/tableschema.h>
#include <kexidb/queryschema.h>

namespace Kross { namespace KexiDB {

    /**
     * The KexiDBSchema template class spends a base to wrap from
     * \a ::KexiDB::SchemaData and \a ::KexiDB::FieldList inherited
     * classes.
     */
    template<class T>
    class KexiDBSchema : public Kross::Api::Class<T>
    {
        public:
            KexiDBSchema(const QString& name, ::KexiDB::SchemaData* schema, ::KexiDB::FieldList* fieldlist);
            virtual ~KexiDBSchema();

        protected:
            ::KexiDB::SchemaData* m_schema;
            ::KexiDB::FieldList* m_fieldlist;

        private:
            Kross::Api::Object* name(Kross::Api::List*);
            Kross::Api::Object* setName(Kross::Api::List*);

            Kross::Api::Object* caption(Kross::Api::List*);
            Kross::Api::Object* setCaption(Kross::Api::List*);

            Kross::Api::Object* description(Kross::Api::List*);
            Kross::Api::Object* setDescription(Kross::Api::List*);

            Kross::Api::Object* fieldlist(Kross::Api::List*);
    };

    /**
     * The KexiDBTableSchema class wraps \a ::KexiDB::TableSchema
     * objects.
     */
    class KexiDBTableSchema : public KexiDBSchema<KexiDBTableSchema>
    {
        public:
            KexiDBTableSchema(::KexiDB::TableSchema* tableschema);
            virtual ~KexiDBTableSchema();
            virtual const QString getClassName() const;
            virtual const QString getDescription() const;
            ::KexiDB::TableSchema* tableschema();
    };

    /**
     * The KexiDBQuerySchema class wraps \a ::KexiDB::QuerySchema
     * objects.
     */
    class KexiDBQuerySchema : public KexiDBSchema<KexiDBQuerySchema>
    {
        public:
            KexiDBQuerySchema(::KexiDB::QuerySchema* queryschema);
            virtual ~KexiDBQuerySchema();
            virtual const QString getClassName() const;
            virtual const QString getDescription() const;
            ::KexiDB::QuerySchema* queryschema();

        private:
            Kross::Api::Object* statement(Kross::Api::List*);
            Kross::Api::Object* setStatement(Kross::Api::List*);
    };

}}

#endif

