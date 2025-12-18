// BESConstraintFuncs.cc

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

#include "BESConstraintFuncs.h"
#include "BESDataNames.h"

using std::string;

string BESConstraintFuncs::pre_to_post_constraint(const string &name, const string &pre_constraint) {
    string str = pre_constraint;
    string new_name = name;
    new_name.append(".");
    if (!str.empty() /* != "" jhrg 4/1/14 */) {
        str.insert(0, new_name);
        int pos = 0;
        pos = str.find(',', pos);
        while (pos != -1) {
            pos++;
            str.insert(pos, new_name);
            pos = str.find(',', pos);
        }
    }

    return str;
}

void BESConstraintFuncs::post_append(BESDataHandlerInterface &dhi) {
    if (dhi.container) {
        string to_append;
        if (dhi.container->get_constraint() != "") {
            to_append = pre_to_post_constraint(dhi.container->get_symbolic_name(), dhi.container->get_constraint());
        } else {
            to_append = dhi.container->get_symbolic_name();
        }
        string constraint = dhi.data[POST_CONSTRAINT];
        if (constraint != "")
            constraint += ",";
        constraint.append(to_append);
        dhi.data[POST_CONSTRAINT] = constraint;
    }
}
