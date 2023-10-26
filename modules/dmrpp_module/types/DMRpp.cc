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

#include <sstream>
#include <vector>

#include <libdap/DDS.h>
#include <libdap/XMLWriter.h>
#include <libdap/D4Group.h>
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/InternalErr.h>

#include "DMRpp.h"
#include "DmrppCommon.h"
#include "DmrppTypeFactory.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DMRpp::DMRpp(DmrppTypeFactory *factory, const std::string &name) : DMR(factory, name)
{
}

/**
 * @brief Print the DMR++ response
 *
 * This is a clone of DMR::print_dap4() modified to include the DMR++ namespace
 * and to print the DMR++ XML which is the standard DMR XML with the addition of
 * new elements that include information about the 'chunks' that hold data values.

 * It uses a static field defined in DmrppCommon to control whether the
 * chunk information should be printed. The third argument \arg print_chunks
 * will set this static class member to true or false, which controls the
 * output of the chunk information. This method resets the field to its previous
 * value on exit.
 *
 * @param xml Writer the XML to this instance of XMLWriter
 * @param href When writing the XML root element, include this as the href attribute.
 * This href will be used as the data source when other code reads the raw (e.g.,
 * chunked) data. Default is the empty string and the attribute will not be printed.
 * @param constrained Should the DMR++ be constrained, in the same sense that the
 * DMR::print_dap4() method can print a constrained DMR. False by default.
 * @param print_chunks Print the chunks? This parameter sets the DmrppCommon::d_print_chunks
 * static field. That field is used by other methods in the Dmrpp<Type> classes to
 * control if they print the chunk information. True by default.
 *
 * @see DmrppArray::print_dap4()
 * @see DmrppCommon::print_dmrpp()
 */

void DMRpp::print_dmrpp(XMLWriter &xml, const string &href, bool constrained, bool print_chunks)
{
    bool pc_initial_value = DmrppCommon::d_print_chunks;
    DmrppCommon::d_print_chunks = print_chunks;

    try {
        if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*) "Dataset") < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write Dataset element");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "xmlns",
            (const xmlChar*) get_namespace().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for xmlns");

        // The dmrpp namespace
        if (DmrppCommon::d_print_chunks)
            if (xmlTextWriterWriteAttribute(xml.get_writer(),
                (const xmlChar*)string("xmlns:").append(DmrppCommon::d_ns_prefix).c_str(),
                    (const xmlChar*)DmrppCommon::d_dmrpp_ns.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for xmlns:dmrpp");

        if (!request_xml_base().empty()) {
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "xml:base",
                (const xmlChar*) request_xml_base().c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for xml:base");
        }

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "dapVersion",
            (const xmlChar*) dap_version().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for dapVersion");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "dmrVersion",
            (const xmlChar*) dmr_version().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for dapVersion");

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*) name().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");

        // cerr << "DMRpp::" <<__func__ << "() href: "<< href << endl;
        if (!href.empty())
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*)string(DmrppCommon::d_ns_prefix).append(":href").c_str(),
                (const xmlChar*) href.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for href");

        if (!get_version().empty())
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*)string(DmrppCommon::d_ns_prefix).append(":version").c_str(),
                                            (const xmlChar*) get_version().c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for version");

        root()->print_dap4(xml, constrained);

        if (xmlTextWriterEndElement(xml.get_writer()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not end the top-level Group element");
    }
    catch (...) {
        DmrppCommon::d_print_chunks = pc_initial_value;
        throw;
    }

    DmrppCommon::d_print_chunks = pc_initial_value;
}

/**
 * @brief override DMR::print_dap4() so the chunk info will print too.
 *
 * @param xml
 * @param constrained
 */
void DMRpp::print_dap4(XMLWriter &xml, bool constrained /* false */)
{
    print_dmrpp(xml, get_href(), constrained, get_print_chunks());
}

libdap::DDS *DMRpp::getDDS()
{
    DmrppTypeFactory factory;
    unique_ptr<DDS> dds(new DDS(&factory, name()));
    dds->filename(filename());

    // Now copy the global attributes
    unique_ptr<vector<BaseType *>> top_vars(root()->transform_to_dap2(&(dds->get_attr_table())/*, true*/));
    for (auto i = top_vars->begin(), e = top_vars->end(); i != e; i++) {
        dds->add_var_nocopy(*i);
    }

    dds->set_factory(nullptr);
    return dds.release();
}

} /* namespace dmrpp */
