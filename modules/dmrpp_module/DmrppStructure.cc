
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

#include <libdap/XMLWriter.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppStructure.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppStructure &
DmrppStructure::operator=(const DmrppStructure &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Structure &>(*this) = rhs; // run Constructor=

    dynamic_cast<DmrppCommon &>(*this) = rhs;

    return *this;
}

bool
DmrppStructure::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);
    throw InternalErr (__FILE__, __LINE__, "The read function of DmrppStructure is not implemented yet.");

}

void
DmrppStructure::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    Structure::set_send_p(state);
}

void 
DmrppStructure::print_dap4(libdap::XMLWriter &writer, bool constrained) {

    if (constrained && !this->send_p()) 
        return; 
 
    if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*)this->type_name().c_str()) < 0) 
        throw InternalErr(__FILE__, __LINE__, "Could not write " + this->type_name() + " element"); 
 
    if (!this->name().empty()) { 
        if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "name", (const xmlChar*)this->name().c_str()) < 0) 
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name"); 
    } 

 
    if (this->is_dap4()) 
        this->attributes()->print_dap4(writer); 
    
    if (!this->is_dap4() && this->get_attr_table().get_size() > 0) 
        this->get_attr_table().print_xml_writer(writer); 

    Constructor::Vars_iter vi = this->var_begin(); 
    Constructor::Vars_iter ve = this->var_end();

    for (; vi != ve; vi++) {
        BaseType *bt = *vi;
        bt->print_dap4(writer);
    }

    if (DmrppCommon::d_print_chunks && (get_chunks_size() > 0 || get_uses_fill_value()))
        print_chunks_element(writer, DmrppCommon::d_ns_prefix);

       // print scalar value for compact storage.
       // TODO: add this later.
    if (xmlTextWriterEndElement(writer.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + this->type_name() + " element");

}
  

void DmrppStructure::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppStructure::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Structure::dump(strm);
    strm << BESIndent::LMarg << "value:    " << "----" << /*d_buf <<*/ endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp

