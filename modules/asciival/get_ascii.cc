
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

using std::cerr ;
using std::endl ;

#include <DataDDS.h>

#include <BESDebug.h>

#include "get_ascii.h"
#include "AsciiOutput.h"
//#include "name_map.h"

#include "AsciiByte.h"
#include "AsciiInt16.h"
#include "AsciiUInt16.h"
#include "AsciiInt32.h"
#include "AsciiUInt32.h"
#include "AsciiFloat32.h"
#include "AsciiFloat64.h"
#include "AsciiStr.h"
#include "AsciiUrl.h"
#include "AsciiArray.h"
#include "AsciiStructure.h"
#include "AsciiSequence.h"
#include "AsciiGrid.h"

namespace dap_asciival {

/** Using the AsciiOutput::print_ascii(), write the data values to an
    output file/stream as ASCII.

    @param dds A DataDDS loaded with data. The variables must use the AsciiByte,
    et c., type classes. Use the function datadds_to_ascii_datadds() to
    build such a DataDDS from one whose types are, say NCByte, et cetera.
    @param strm Write ASCII to stream. */
void
get_data_values_as_ascii(DDS *dds, ostream &strm)
{
    BESDEBUG("ascii", "In get_data_values_as_ascii; dataset name = " << dds->get_dataset_name() << endl );
    strm << "Dataset: " << dds->get_dataset_name() << "\n" ;

    DDS::Vars_iter i = dds->var_begin();
    while (i != dds->var_end()) {
        if ((*i)->send_p()) {
            dynamic_cast<AsciiOutput &>(**i).print_ascii(strm);
            strm << "\n";
        }
        ++i;
    }

    BESDEBUG("ascii", "Out get_data_values_as_ascii" << endl );
}

DDS *datadds_to_ascii_datadds(DDS *dds)
{
    BESDEBUG("ascii", "In datadds_to_ascii_datadds" << endl);
    // Should the following use AsciiOutputFactory instead of the source DDS'
    // factory class? It doesn't matter for the following since the function
    // basetype_to_asciitype() doesn't use the factory. So long as no other
    // code uses the DDS' factory, this is fine. jhrg 9/5/06
   DDS *asciidds = new DDS(dds->get_factory(), dds->get_dataset_name());

    DDS::Vars_iter i = dds->var_begin();
    while (i != dds->var_end()) {
        BaseType *abt = basetype_to_asciitype(*i);
        asciidds->add_var_nocopy(abt);
#if 0
        // add_var makes a copy of the base type passed to it, so delete
        // it here
        delete abt;
#endif
        ++i;
    }

    // Calling tag_nested_sequences() makes it easier to figure out if a
    // sequence has parent or child sequences or if it is a 'flat' sequence.
    asciidds->tag_nested_sequences();

    return asciidds;
}


BaseType *
basetype_to_asciitype( BaseType *bt )
{
    switch( bt->type() )
    {
	case dods_byte_c:
		return new AsciiByte( dynamic_cast<Byte *>(bt) ) ;

	case dods_int16_c:
		return new AsciiInt16( dynamic_cast<Int16 *>(bt) ) ;

	case dods_uint16_c:
		return new AsciiUInt16( dynamic_cast<UInt16 *>(bt) ) ;

	case dods_int32_c:
		return new AsciiInt32( dynamic_cast<Int32 *>(bt) ) ;

	case dods_uint32_c:
		return new AsciiUInt32( dynamic_cast<UInt32 *>(bt) ) ;

	case dods_float32_c:
		return new AsciiFloat32( dynamic_cast<Float32 *>(bt) ) ;

	case dods_float64_c:
		return new AsciiFloat64( dynamic_cast<Float64 *>(bt) ) ;

	case dods_str_c:
		return new AsciiStr( dynamic_cast<Str *>(bt) ) ;

	case dods_url_c:
		return new AsciiUrl( dynamic_cast<Url *>(bt) ) ;

	case dods_array_c:
		return new AsciiArray( dynamic_cast<Array *>(bt) ) ;

	case dods_structure_c:
		return new AsciiStructure( dynamic_cast<Structure *>(bt) ) ;

	case dods_sequence_c:
		return new AsciiSequence( dynamic_cast<Sequence *>(bt) ) ;

	case dods_grid_c:
	    return new AsciiGrid( dynamic_cast<Grid *>(bt) ) ;

    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown type");
    }
}

} // namespace dap_asciival
