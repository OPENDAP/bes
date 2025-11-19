// BESCatalogEntry.cc

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

#include "config.h"

#include <sstream>

#include "BESCatalogEntry.h"

using namespace std;

BESCatalogEntry::BESCatalogEntry(const string &name, const string &catalog) : _name(name), _catalog(catalog) {}

BESCatalogEntry::~BESCatalogEntry() {
    // iterate through the entry list and delete them all
    map<string, BESCatalogEntry *>::iterator i = _entry_list.begin();
    map<string, BESCatalogEntry *>::iterator e = _entry_list.end();
    for (; i != e; i++) {
        BESCatalogEntry *e = (*i).second;
        delete e;
        (*i).second = 0;
    }
}

void BESCatalogEntry::set_size(off_t size) {
    ostringstream strm;
    strm << size;
    _size = strm.str();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESCatalogEntry::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESCatalogEntry::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "name: " << _name << endl;
    strm << BESIndent::LMarg << "catalog: " << _catalog << endl;
    strm << BESIndent::LMarg << "size: " << _size << endl;
    strm << BESIndent::LMarg << "modification date: " << _mod_date << endl;
    strm << BESIndent::LMarg << "modification time: " << _mod_time << endl;
    strm << BESIndent::LMarg << "services: ";
    if (_services.size()) {
        strm << endl;
        BESIndent::Indent();
        list<string>::const_iterator si = _services.begin();
        list<string>::const_iterator se = _services.end();
        for (; si != se; si++) {
            strm << BESIndent::LMarg << (*si) << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << "none" << endl;
    }

    strm << BESIndent::LMarg << "metadata: ";
    if (_metadata.size()) {
        strm << endl;
        BESIndent::Indent();
        map<string, string>::const_iterator mi = _metadata.begin();
        map<string, string>::const_iterator me = _metadata.end();
        for (; mi != me; mi++) {
            strm << BESIndent::LMarg << (*mi).first << " = " << (*mi).second << endl;
        }
        BESIndent::UnIndent();
    } else {
        strm << "none" << endl;
    }

    strm << BESIndent::LMarg << "is collection? ";
    if (_entry_list.size() > 0)
        strm << "yes" << endl;
    else
        strm << "no" << endl;
    strm << BESIndent::LMarg << "count: " << _entry_list.size() << endl;

    // display this entries information, then the list
    BESIndent::Indent();
    map<string, BESCatalogEntry *>::const_iterator i = _entry_list.begin();
    map<string, BESCatalogEntry *>::const_iterator e = _entry_list.end();
    for (; i != e; i++) {
        BESCatalogEntry *e = (*i).second;
        e->dump(strm);
    }
    BESIndent::UnIndent();

    BESIndent::UnIndent();
}
