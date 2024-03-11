// -*- mode: c++; c-basic-offset:4 -*-

// Copyright (c) 2006 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// This file holds the interface for the 'get data as ascii' function of the
// OPeNDAP/HAO data server. This function is called by the BES when it loads
// this as a module. The functions in the file ascii_val.cc also use this, so
// the same basic processing software can be used both by Hyrax and tie older
// Server3.

#include <stdio.h>

#include <iostream>

#include <libxml/xmlerror.h>

using std::cerr;
using std::endl;

#include <libdap/DDS.h>

#include <BESDebug.h>
#include <RequestServiceTimer.h>

#include "get_xml_data.h"
#include "XDOutput.h"

#include "XDByte.h"
#include "XDInt16.h"
#include "XDUInt16.h"
#include "XDInt32.h"
#include "XDUInt32.h"
#include "XDFloat32.h"
#include "XDFloat64.h"
#include "XDStr.h"
#include "XDUrl.h"
#include "XDArray.h"
#include "XDStructure.h"
#include "XDSequence.h"
#include "XDGrid.h"

const char *DAP_SCHEMA = "http://xml.opendap.org/ns/dap/3.3#";

using namespace libdap;

#define MODULE "xml_data"
#define prolog string("get_xml_data::").append(__func__).append("() - ")

namespace xml_data {

/** Using the XDOutput::print_ascii(), write the data values to an
 output file/stream as ASCII.

 @param dds A DDS loaded with data. The variables must use the XDByte,
 et c., type classes. Use the function datadds_to_ascii_datadds() to
 build such a DataDDS from one whose types are, say NCByte, et cetera.
 @param strm Write ASCII to stream. */
void get_data_values_as_xml(DDS *dds, XMLWriter *writer)
{
    try {
        /* Start an element named "Dataset". Since this is the first element,
         * this will be the root element of the document */
        if (xmlTextWriterStartElementNS(writer->get_writer(), NULL, (const xmlChar*)"Dataset", (const xmlChar*)DAP_SCHEMA) < 0)
            throw InternalErr(__FILE__, __LINE__,  "Error starting the Dataset element for response ");

        DDS::Vars_iter i = dds->var_begin();
        while (i != dds->var_end()) {
            if ((*i)->send_p()) {
                RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmit " + (*i)->name() , __FILE__, __LINE__);

                BESDEBUG("xd", "Printing the values for " << (*i)->name() << " (" << (*i)->type_name() << ")" << endl);
                dynamic_cast<XDOutput &>(**i).print_xml_data(writer, true);
            }
            ++i;
        }

        if (xmlTextWriterEndElement(writer->get_writer()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Error ending Dataset element.");

    }
    catch (InternalErr &e) {
        const xmlError *error = xmlGetLastError();
        if (error && error->message)
            throw InternalErr(e.get_error_message() + "; libxml: " + error->message);
        else
            throw InternalErr(e.get_error_message() + "; libxml: no message");
    }
}

DDS *dds_to_xd_dds(DDS * dds)
{
    BESDEBUG("xd", "In datadds_to_xd_datadds" << endl);
    // Should the following use XDOutputFactory instead of the source DDS'
    // factory class? It doesn't matter for the following since the function
    // basetype_to_asciitype() doesn't use the factory. So long as no other
    // code uses the DDS' factory, this is fine. jhrg 9/5/06
    DDS *xd_dds = new DDS(dds->get_factory(), dds->get_dataset_name());

    DDS::Vars_iter i = dds->var_begin();
    while (i != dds->var_end()) {
        BaseType *abt = basetype_to_xd(*i);
        xd_dds->add_var(abt);
        // add_var makes a copy of the base type passed to it, so delete
        // it here
        delete abt;
        ++i;
    }

    // Calling tag_nested_sequences() makes it easier to figure out if a
    // sequence has parent or child sequences or if it is a 'flat' sequence.
    xd_dds->tag_nested_sequences();

    return xd_dds;
}

BaseType *
basetype_to_xd(BaseType *bt)
{
    if (!bt)
        throw InternalErr(__FILE__, __LINE__, "Null BaseType to XD factory");

    switch (bt->type()) {
        case dods_byte_c:
            return new XDByte(dynamic_cast<Byte *>(bt));

        case dods_int16_c:
            return new XDInt16(dynamic_cast<Int16 *>(bt));

        case dods_uint16_c:
            return new XDUInt16(dynamic_cast<UInt16 *>(bt));

        case dods_int32_c:
            return new XDInt32(dynamic_cast<Int32 *>(bt));

        case dods_uint32_c:
            return new XDUInt32(dynamic_cast<UInt32 *>(bt));

        case dods_float32_c:
            return new XDFloat32(dynamic_cast<Float32 *>(bt));

        case dods_float64_c:
            return new XDFloat64(dynamic_cast<Float64 *>(bt));

        case dods_str_c:
            return new XDStr(dynamic_cast<Str *>(bt));

        case dods_url_c:
            return new XDUrl(dynamic_cast<Url *>(bt));

        case dods_array_c:
            return new XDArray(dynamic_cast<Array *>(bt));

        case dods_structure_c:
            return new XDStructure(dynamic_cast<Structure *>(bt));

        case dods_sequence_c:
            return new XDSequence(dynamic_cast<Sequence *>(bt));

        case dods_grid_c:
            return new XDGrid(dynamic_cast<Grid *>(bt));

        default:
            throw InternalErr(__FILE__, __LINE__, "Unknown type");
    }
}

} // namespace xml_data
