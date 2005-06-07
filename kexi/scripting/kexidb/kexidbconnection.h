/***************************************************************************
 * kexidbconnection.h
 * This file is part of the KDE project
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

#ifndef KROSS_KEXIDB_KEXIDBCONNECTION_H
#define KROSS_KEXIDB_KEXIDBCONNECTION_H

#include <qstring.h>

#include "../api/object.h"
#include "../api/variant.h"
#include "../api/list.h"
//#include "../api/module.h"
#include "../api/class.h"

//#include <kexidb/driver.h>
#include <kexidb/connection.h>

namespace Kross { namespace KexiDB {

    // Forward declarations.
    class KexiDBDriver;
    class KexiDBConnectionData;

    /**
     * The KexiDBConnection class handles \a ::KexiDB::Connection
     * instances and wraps parts of the functionality.
     */
    class KexiDBConnection : public Kross::Api::Class<KexiDBConnection>
    {
        public:

            /**
             * Constructor.
             *
             * \param driver The parent \a KexiDBDriver object
             *        this connection belongs too.
             * \param connection The \a ::KexiDB::Connection instance
             *        this connection wraps.
             * \param connectiondata The \a KexiDBConnectionData instance
             *        used to create this connection.
             */
            KexiDBConnection(KexiDBDriver* driver, ::KexiDB::Connection* connection, KexiDBConnectionData* connectiondata = 0);

            /**
             * Destructor.
             */
            virtual ~KexiDBConnection();

            /// See \see Kross::Api::Object::getClassName
            virtual const QString getClassName() const;

            /// See \see Kross::Api::Object::getDescription
            virtual const QString getDescription() const;

        private:
            ::KexiDB::Connection* connection();
            ::KexiDB::Connection* m_connection;

            KexiDBConnectionData* m_connectiondata;

            /**
             * Return the KexiDBConnectionData object
             * used to create this connection.
             * Wrapper for \a ::KexiDB::Connection::data
             */
            Kross::Api::Object::Ptr data(Kross::Api::List::Ptr);

            /**
             * Return the KexiDBDriver object this
             * connection belongs too.
             * Wrapper for \a ::KexiDB::Connection::driver
             */
            Kross::Api::Object::Ptr driver(Kross::Api::List::Ptr);

            /**
             * Connect and return true if we are successfully
             * connected now.
             * Wrapper for \a ::KexiDB::Connection::connect
             */
            Kross::Api::Object::Ptr connect(Kross::Api::List::Ptr);

            /**
             * Return true if we are connected.
             * Wrapper for \a ::KexiDB::Connection::isConnected
             */
            Kross::Api::Object::Ptr isConnected(Kross::Api::List::Ptr);

            /**
             * Disconnect and return true if we are successfully
             * disconnected now.
             * Wrapper for \a ::KexiDB::Connection::disconnect
             */
            Kross::Api::Object::Ptr disconnect(Kross::Api::List::Ptr);

            /**
             * Return true if the as argument passed databasename exists.
             * Wrapper for \a ::KexiDB::Connection::databaseExists
             */
            Kross::Api::Object::Ptr databaseExists(Kross::Api::List::Ptr);

            /**
             * Return the name of currently used database for this
             * connection or empty string if there is no used database.
             * Wrapper for \a ::KexiDB::Connection::currentDatabase
             */
            Kross::Api::Object::Ptr currentDatabase(Kross::Api::List::Ptr);

            /**
             * Return list of database names for opened connection.
             * Wrapper for \a ::KexiDB::Connection::databaseNames
             */
            Kross::Api::Object::Ptr databaseNames(Kross::Api::List::Ptr);

            /**
             * Return true if connection is properly established.
             * Wrapper for \a ::KexiDB::Connection::isDatabaseUsed
             */
            Kross::Api::Object::Ptr isDatabaseUsed(Kross::Api::List::Ptr);

            /**
             * Opens an existing database specified by the as
             * argument passed databasename.
             * Wrapper for \a ::KexiDB::Connection::useDatabase
             */
            Kross::Api::Object::Ptr useDatabase(Kross::Api::List::Ptr);

            /**
             * Closes currently used database for this connection.
             * Wrapper for \a ::KexiDB::Connection::closeDatabase
             */
            Kross::Api::Object::Ptr closeDatabase(Kross::Api::List::Ptr);

            /**
             * Return names of all table schemas stored in currently
             * used database. If the as argument passed boolean value
             * is true, internal KexiDB system table names (kexi__*)
             * are also returned.
             * Wrapper for \a ::KexiDB::Connection::tableNames
             */
            Kross::Api::Object::Ptr tableNames(Kross::Api::List::Ptr);

            /**
             * Executes query described by the as argument passed
             * sqlstatement-string. Returns the opened cursor
             * created for results of this query.
             * Wrapper for \a ::KexiDB::Connection::executeQuery
             */
            Kross::Api::Object::Ptr executeQueryString(Kross::Api::List::Ptr);

            /**
             * Executes query described by the as argument passed
             * \a KexiDBQuerySchema object. Returns the opened cursor
             * created for results of this query.
             * Wrapper for \a ::KexiDB::Connection::executeQuery
             */
            Kross::Api::Object::Ptr executeQuerySchema(Kross::Api::List::Ptr);

            /**
             * Executes the as argument passed sqlquery-string and
             * returns the first record's field's value.
             * Wrapper for \a ::KexiDB::Connection::querySingleString
             */
            Kross::Api::Object::Ptr querySingleString(Kross::Api::List::Ptr);

            /**
             * Executes the as argument passed sqlquery-string and
             * returns a stringlist of first record's first field's
             * values.
             * Wrapper for \a ::KexiDB::Connection::queryStringList
             */
            Kross::Api::Object::Ptr queryStringList(Kross::Api::List::Ptr);

            /**
             * Executes sql query statement and returned the first
             * record's valuelist.
             * Wrapper for \a ::KexiDB::Connection::querySingleRecord
             */
            Kross::Api::Object::Ptr querySingleRecord(Kross::Api::List::Ptr);

            /**
             * Inserts the as argument passed KexiDBField object.
             * Wrapper for \a ::KexiDB::Connection::insertRecord
             */
            Kross::Api::Object::Ptr insertRecord(Kross::Api::List::Ptr);

            /**
             * Creates new database with the as argument passed
             * databasename.
             * Wrapper for \a ::KexiDB::Connection::createDatabase
             */
            Kross::Api::Object::Ptr createDatabase(Kross::Api::List::Ptr);

            /**
             * Drops the as argument passed databasename.
             * Wrapper for \a ::KexiDB::Connection::dropDatabase
             */
            Kross::Api::Object::Ptr dropDatabase(Kross::Api::List::Ptr);

            /**
             * Creates table defined by the as argument passed
             * KexiTableSchema object.
             * Wrapper for \a ::KexiDB::Connection::createTable
             */
            Kross::Api::Object::Ptr createTable(Kross::Api::List::Ptr);

            /**
             * Drops table defined by the as argument passed
             * KexiDBTableSchema object.
             * Wrapper for \a ::KexiDB::Connection::dropTable
             */
            Kross::Api::Object::Ptr dropTable(Kross::Api::List::Ptr);

            /**
             * Alters the as first argument passed KexiDBTableSchema
             * object using the as second argument passed
             * KexiDBTableSchema.
             * Wrapper for \a ::KexiDB::Connection::alterTable
             */
            Kross::Api::Object::Ptr alterTable(Kross::Api::List::Ptr);

            /**
             * Alters the tablename of the as first argument passed
             * KexiDBTableSchema into the as second argument passed
             * new tablename.
             * Wrapper for \a ::KexiDB::Connection::alterTableName
             */
            Kross::Api::Object::Ptr alterTableName(Kross::Api::List::Ptr);

            /**
             * Returns the KexiDBTableSchema object of the table
             * matching to the as argument passed tablename.
             * Wrapper for \a ::KexiDB::Connection::tableSchema
             */
            Kross::Api::Object::Ptr tableSchema(Kross::Api::List::Ptr);

            /**
             * Returns true if there is at least one valid record
             * in the as argument passed tablename.
             * Wrapper for \a ::KexiDB::Connection::isEmpty
             */
            Kross::Api::Object::Ptr isEmptyTable(Kross::Api::List::Ptr);

            /**
             * Return true if the \"auto commit\" option is on.
             * Wrapper for \a ::KexiDB::Connection::autoCommit
             */
            Kross::Api::Object::Ptr autoCommit(Kross::Api::List::Ptr);

            /**
             * Set the auto commit option. This does not affect
             * currently started transactions and can be changed
             * even when connection is not established.
             * Wrapper for \a ::KexiDB::Connection::setAutoCommit
             */
            Kross::Api::Object::Ptr setAutoCommit(Kross::Api::List::Ptr);

            /**
             * Creates new transaction handle and starts a new transaction.
             * Wrapper for \a ::KexiDB::Connection::beginTransaction
             */
            Kross::Api::Object::Ptr beginTransaction(Kross::Api::List::Ptr);

            /**
             * Commits the as rgument passed KexiDBTransaction object.
             * Wrapper for \a ::KexiDB::Connection::commitTransaction
             */
            Kross::Api::Object::Ptr commitTransaction(Kross::Api::List::Ptr);

            /**
             * Rollback the as rgument passed KexiDBTransaction object.
             * Wrapper for \a ::KexiDB::Connection::rollbackTransaction
             */
            Kross::Api::Object::Ptr rollbackTransaction(Kross::Api::List::Ptr);

            /**
             * Return the KEXIDBTransaction object for default transaction
             * for this connection.
             * Wrapper for \a ::KexiDB::Connection::defaultTransaction
             */
            Kross::Api::Object::Ptr defaultTransaction(Kross::Api::List::Ptr);

            /**
             * Sets default transaction that will be used as context for
             * operations on data in opened database for this connection.
             * Wrapper for \a ::KexiDB::Connection::setDefaultTransaction
             */
            Kross::Api::Object::Ptr setDefaultTransaction(Kross::Api::List::Ptr);

            /**
             * Return list of currently active KexiDBTransaction objects.
             * Wrapper for \a ::KexiDB::Connection::transactions
             */
            Kross::Api::Object::Ptr transactions(Kross::Api::List::Ptr);

            /**
             * Return a KexiDBParser object.
             */
            Kross::Api::Object::Ptr parser(Kross::Api::List::Ptr);
    };

}}

#endif

