// BESCatalogEntry.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_BESCatalogEntry_h
#define I_BESCatalogEntry_h 1

#include <list>
#include <map>
#include <string>
#include <sys/types.h>

#include "BESObj.h"

/**
 * @deprecated Use CatalogNode and CatlogItem instead. jhrg 7/25/18
 */
class BESCatalogEntry : public BESObj {
private:
    std::string _name;
    std::string _catalog;
    std::string _size;
    std::string _mod_date;
    std::string _mod_time;
    std::list<std::string> _services;

    std::map<std::string, BESCatalogEntry *> _entry_list;
    std::map<std::string, std::string> _metadata;

    BESCatalogEntry() {}

public:
    BESCatalogEntry(const std::string &name, const std::string &catalog);

    ~BESCatalogEntry() override;

    virtual void add_entry(BESCatalogEntry *entry) {
        if (entry) {
            _entry_list[entry->get_name()] = entry;
        }
    }

    virtual std::string get_name() { return _name; }

    virtual std::string get_catalog() { return _catalog; }

    virtual bool is_collection() { return (get_count() > 0); }

    virtual std::string get_size() { return _size; }

    virtual void set_size(off_t size);

    virtual std::string get_mod_date() { return _mod_date; }

    virtual void set_mod_date(const std::string &mod_date) { _mod_date = mod_date; }

    virtual std::string get_mod_time() { return _mod_time; }

    virtual void set_mod_time(const std::string &mod_time) { _mod_time = mod_time; }

    virtual std::list<std::string> get_service_list() { return _services; }

    virtual void set_service_list(std::list<std::string> &slist) { _services = slist; }

    virtual unsigned int get_count() { return _entry_list.size(); }

    virtual std::map<std::string, std::string> get_info() { return _metadata; }

    virtual void add_info(const std::string &name, const std::string &value) { _metadata[name] = value; }

    using catalog_citer = std::map<std::string, BESCatalogEntry *>::const_iterator;

    virtual catalog_citer get_beginning_entry() { return _entry_list.begin(); }

    virtual catalog_citer get_ending_entry() { return _entry_list.end(); }

    void dump(std::ostream &strm) const override;
};

#endif // I_BESCatalogEntry_h
