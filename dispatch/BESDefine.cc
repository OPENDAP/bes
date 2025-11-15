// BESDefine.cc

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

#include "BESDefine.h"

using std::endl;
using std::ostream;

BESDefine::~BESDefine() {
    // delete all the containers in my list, they belong to me
    for (auto container: _containers) {
        delete container;
    }
#if 0
    while (!_containers.empty()) {
        BESDefine::containers_iter ci = _containers.begin();
        BESContainer *c = (*ci);
        _containers.erase(ci);
        if (c) {
            delete c;
        }
    }
#endif
}

void
BESDefine::add_container(BESContainer *container) {
    _containers.push_back(container);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with contents of the
 * definition, including the containers in this definition and aggregation
 * information.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDefine::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESDefine::dump - ("
            << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (!_containers.empty()) {
        strm << BESIndent::LMarg << "container list:" << endl;
        BESIndent::Indent();
        for (const auto container: _containers) {
            container->dump(strm);
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "container list: empty" << endl;
    }
    strm << BESIndent::LMarg << "aggregation command: " << _agg_cmd << endl;
    strm << BESIndent::LMarg << "aggregation server: " << _agg_handler << endl;
    BESIndent::UnIndent();
}
