
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of www_int, software which returns an HTML form which
// can be used to build a URL to access data from a DAP data server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

// (c) COPYRIGHT URI/MIT 1999,2000
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

#include "config.h"

static char rcsid[] not_used =
    { "$Id$" };

#include <string>
#include <iostream>
#include <sstream>

#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <io.h>
#endif

#include <libdap/BaseType.h>
#include <libdap/Grid.h>
#include <libdap/DDS.h>

#include <libdap/debug.h>
#include <libdap/mime_util.h>	// remove if write_html_headers() not needed
#include <libdap/util.h>

#include "WWWOutput.h"

using namespace std;

#ifdef WIN32
#define getpid _getpid
#define access _access
#define X_OK 00                 //  Simple existence
#endif

WWWOutput::WWWOutput(ostream &strm, int rows, int cols)
    : d_strm(&strm), d_attr_rows(rows), d_attr_cols(cols)
{
}

// TODO: Can this be removed?
void
WWWOutput::write_html_header()
{
#if 1
    set_mime_html(*d_strm, unknown_type, dap_version(), x_plain);
#endif
}

void WWWOutput::write_disposition(string url, bool netcdf3_file_response, bool netcdf4_file_response)
{
    // To get the size to be a function of the image window size, you need to
    // use some JavaScript code to generate the HTML. C++ --> JS --> HTML.
    // 4/8/99 jhrg

    *d_strm << "<tr>\n\
<td align=\"right\">\n\
<h3>\n\
<a href=\"opendap_form_help.html#disposition\" target=\"help\">Action:</a></h3>\n\
<td>\n\
<input type=\"button\" value=\"Get ASCII\" onclick=\"ascii_button()\">\n\
<input type=\"button\" value=\"Get as CoverageJSON\" onclick=\"binary_button('covjson')\">\n";

    // Add new netcdf_button call here jhrg 2/9/09
    if (netcdf3_file_response)
    	*d_strm << "<input type=\"button\" value=\"Get as NetCDF 3\" onclick=\"binary_button('nc')\">\n";
    // Add new new netcdf4 button. jhrg 9/23/13
    if (netcdf4_file_response)
    	*d_strm << "<input type=\"button\" value=\"Get as NetCDF 4\" onclick=\"binary_button('nc4')\">\n";

    *d_strm <<
"<input type=\"button\" value=\"Binary (DAP2) Object\" onclick=\"binary_button('dods')\">\n\
<input type=\"button\" value=\"Show Help\" onclick=\"help_button()\">\n\
\n\
<tr>\n\
<td align=\"right\"><h3><a href=\"opendap_form_help.html#data_url\" target=\"help\">Data URL:</a>\n\
</h3>\n\
<td><input name=\"url\" type=\"text\" size=\"" << d_attr_cols << "\" value=\"" << url << "\">\n" ;
}

void WWWOutput::write_attributes(AttrTable *attr, const string prefix)
{
    if (attr) {
        for (AttrTable::Attr_iter a = attr->attr_begin(); a
                != attr->attr_end(); ++a) {
            if (attr->is_container(a))
                write_attributes(attr->get_attr_table(a),
                        (prefix == "") ? attr->get_name(a) : prefix + string(
                                ".") + attr->get_name(a));
            else {
                if (prefix != "")
		    *d_strm << prefix << "." << attr->get_name(a) << ": ";
                else
                    *d_strm << attr->get_name(a) << ": ";

                int num_attr = attr->get_attr_num(a) - 1;
                for (int i = 0; i < num_attr; ++i)
		{
		    *d_strm << attr->get_attr(a, i) << ", ";
		}
		*d_strm << attr->get_attr(a, num_attr) << "\n";
            }
        }
    }
}

/** Given the global attribute table, write the HTML which contains all the
    global attributes for this dataset. A global attribute is defined in the
    source file DDS.cc by the DDS::transfer_attributes() method.

    @param das The AttrTable with the global attributes. */
void WWWOutput::write_global_attributes(AttrTable &attr)
{
    *d_strm << "<tr>\n\
<td align=\"right\" valign=\"top\"><h3>\n\
<a href=\"opendap_form_help.html#global_attr\" target=\"help\">Global Attributes:</a></h3>\n\
<td><textarea name=\"global_attr\" rows=\"" << d_attr_rows << "\" cols=\"" << d_attr_cols << "\">\n" ;

    write_attributes(&attr);

    *d_strm << "</textarea><p>\n\n";
}

void WWWOutput::write_variable_entries(DDS &dds)
{
    // This writes the text `Variables:' and then sets up the table so that
    // the first variable's section is written into column two.
    *d_strm << "<tr>\n\
<td align=\"right\" valign=\"top\">\n\
<h3><a href=\"opendap_form_help.html#dataset_variables\" target=\"help\">Variables:</a></h3>\n\
<td>";

    for (DDS::Vars_iter p = dds.var_begin(); p != dds.var_end(); ++p) {
	(*p)->print_val(*d_strm);

        write_variable_attributes(*p);
#if 0
        (*p)->print_attributes(*d_strm, d_attr_rows, d_attr_cols);
#endif
	*d_strm << "\n<p><p>\n\n";  // End the current var's section
	*d_strm << "<tr><td><td>\n\n";      // Start the next var in column two
    }
}

/** Write a variable's attribute information. This method is a jack because
    this code does not use multiple inheritance and we'd need that to easily
    add a new method to print attributes according to class. Here I just use
    a switch stmt.

    @param btp A pointer to the variable.*/
void WWWOutput::write_variable_attributes(BaseType * btp)
{
    switch (btp->type()) {
    case dods_byte_c:
    case dods_int16_c:
    case dods_uint16_c:
    case dods_int32_c:
    case dods_uint32_c:
    case dods_float32_c:
    case dods_float64_c:
    case dods_str_c:
    case dods_url_c:
    case dods_array_c: {
	AttrTable &attr = btp->get_attr_table();

	// Don't write anything if there are no attributes.
	if (attr.get_size() == 0) {
	    DBG(cerr << "No Attributes for " << btp->name() << endl);
	    return;
	}

	*d_strm << "<textarea name=\"" << btp->name() << "_attr\" rows=\"" << d_attr_rows << "\" cols=\""
		<< d_attr_cols << "\">\n";
	write_attributes(&attr);
	*d_strm << "</textarea>\n\n";
	break;
    }

    case dods_structure_c:
    case dods_sequence_c:  {
	AttrTable &attr = btp->get_attr_table();

	// Don't write anything if there are no attributes.
	if (attr.get_size() == 0) {
	    DBG(cerr << "No Attributes for " << btp->name() << endl);
	    return;
	}

	*d_strm << "<textarea name=\"" << btp->name() << "_attr\" rows=\"" << d_attr_rows << "\" cols=\""
		<< d_attr_cols << "\">\n";
	write_attributes(&attr);
	*d_strm << "</textarea>\n\n";
	break;
    }

    case dods_grid_c: {
	Grid &g = dynamic_cast<Grid&>(*btp);
#if 0
	// Don't write anything if there are no attributes.
	if (attr.get_size() == 0 && array_attr.get_size() == 0) {
	    DBG(cerr << "No Attributes for " << btp->name() << endl);
	    return;
	}
#endif
	*d_strm << "<textarea name=\"" << btp->name() << "_attr\" rows=\"" << d_attr_rows << "\" cols=\""
		<< d_attr_cols << "\">\n";
	write_attributes(&g.get_attr_table());
	write_attributes(&g.get_array()->get_attr_table(), g.name());
	for (Grid::Map_iter m = g.map_begin(); m != g.map_end(); ++m) {
	    Array &map = dynamic_cast<Array&>(**m);
	    write_attributes(&map.get_attr_table(), map.name());
	}
	*d_strm << "</textarea>\n\n";
	break;
    }

    default:
	break;
    }
}
