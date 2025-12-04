// BESContainer.cc

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

#include "BESContainer.h"

using std::endl;
using std::ostream;

/** @brief duplicate this instance into the passed container
 *
 * @param copy_to The container to copy this instance into
 */
void BESContainer::_duplicate(BESContainer &copy_to) {
    copy_to.d_symbolic_name = d_symbolic_name;
    copy_to.d_real_name = d_real_name;
    copy_to.d_relative_name = d_relative_name;
    copy_to.d_constraint = d_constraint;
    copy_to.d_dap4_constraint = d_dap4_constraint;
    copy_to.d_dap4_function = d_dap4_function;
    copy_to.d_container_type = d_container_type;
    copy_to.d_attributes = d_attributes;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this container.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESContainer::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESContainer::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "symbolic name: " << d_symbolic_name << endl;
    strm << BESIndent::LMarg << "real name: " << d_real_name << endl;
    strm << BESIndent::LMarg << "relative name: " << d_relative_name << endl;
    strm << BESIndent::LMarg << "data type: " << d_container_type << endl;
    strm << BESIndent::LMarg << "constraint: " << d_constraint << endl;
    strm << BESIndent::LMarg << "dap4_constraint: " << d_dap4_constraint << endl;
    strm << BESIndent::LMarg << "dap4_function: " << d_dap4_function << endl;
    strm << BESIndent::LMarg << "attributes: " << d_attributes << endl;
    BESIndent::UnIndent();
}
