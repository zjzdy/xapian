/** @file backendmanager_chert.h
 * @brief BackendManager subclass for chert databases.
 */
/* Copyright (C) 2007,2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef XAPIAN_INCLUDED_BACKENDMANAGER_CHERT_H
#define XAPIAN_INCLUDED_BACKENDMANAGER_CHERT_H

#include "backendmanager.h"

#include <string>

#include <xapian/base.h>
#include <xapian/types.h>
#include <xapian/postingiterator.h>

#include "positionlist.h"

/// BackendManager subclass for chert databases.
class BackendManagerChert : public BackendManager {
    /// Don't allow assignment.
    void operator=(const BackendManagerChert &);

    /// Don't allow copying.
    BackendManagerChert(const BackendManagerChert &);

    /// The path of the last writable database used.
    std::string last_wdb_name;

  public:
    BackendManagerChert() { }

    /** We have virtual methods and want to be able to delete derived classes
     *  using a pointer to the base class, so we need a virtual destructor.
     */
    virtual ~BackendManagerChert();

    /// Return a string representing the current database type.
    const char * get_dbtype() const;

    /// Create a Chert Xapian::Database object indexing multiple files.
    Xapian::Database get_database(const std::vector<std::string> & files);

    /// Create a Chert Xapian::Database object indexing a single file.
    Xapian::Database get_database(const std::string & file);

    /// Create a Chert Xapian::WritableDatabase object indexing a single file.
    Xapian::WritableDatabase get_writable_database(const std::string & name,
						   const std::string & file);

    /// Get the path of Chert Xapian::WritableDatabase instance.
    std::string get_writable_database_path(const std::string & name);

    /// Create a Database object for the last opened WritableDatabase.
    Xapian::Database get_writable_database_as_database();

    /// Create a WritableDatabase object for the last opened WritableDatabase.
    Xapian::WritableDatabase get_writable_database_again();
};

#endif // XAPIAN_INCLUDED_BACKENDMANAGER_CHERT_H
