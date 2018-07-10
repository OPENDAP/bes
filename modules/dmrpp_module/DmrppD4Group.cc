
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <string>

#include <D4Enum.h>
#include <D4EnumDefs.h>
#include <D4Attributes.h>
#include <D4Maps.h>
#include <D4Group.h>
#include <XMLWriter.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppD4Group.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppD4Group::_duplicate(const DmrppD4Group &)
{
}

DmrppD4Group::DmrppD4Group(const string &n) : D4Group(n), DmrppCommon()
{
}

DmrppD4Group::DmrppD4Group(const string &n, const string &d) : D4Group(n, d), DmrppCommon()
{
}

BaseType *
DmrppD4Group::ptr_duplicate()
{
    return new DmrppD4Group(*this);
}

DmrppD4Group::DmrppD4Group(const DmrppD4Group &rhs) : D4Group(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppD4Group &
DmrppD4Group::operator=(const DmrppD4Group &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<D4Group &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::m_duplicate_common(rhs);

    return *this;
}

#if 0
void
DmrppD4Group::print_dap4(XMLWriter &xml, bool constrained)
{
    if (!name().empty() && name() != "/") {
        // For named groups, if constrained is true only print if this group
        // has variables that are marked for transmission. For the root group
        // this test is not made.
        if (constrained && !send_p())
        return;

        if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*) type_name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + type_name() + " element");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*) name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
    }

    // dims
    if (!dims()->empty())
    dims()->print_dap4(xml, constrained);

    // enums
    if (!enum_defs()->empty())
    enum_defs()->print_dap4(xml, constrained);

    // variables
    Constructor::Vars_iter v = var_begin();
    while (v != var_end())
    (*v++)->print_dap4(xml, constrained);

    // attributes
    attributes()->print_dap4(xml);

    // groups
    groupsIter g = d_groups.begin();
    while (g != d_groups.end())
    (*g++)->print_dap4(xml, constrained);

    if (!name().empty() && name() != "/") {
        if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + type_name() + " element");
    }
}
#endif


void DmrppD4Group::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppD4Group::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    D4Group::dump(strm);
    strm << BESIndent::LMarg << "value:    " << "----" << /*d_buf <<*/ endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
