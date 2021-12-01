
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

#include <libdap/BaseType.h>
#include <libdap/DDS.h>
#include "WWWOutput.h"

namespace dap_html_form {

extern WWWOutput *wo;

BaseType *basetype_to_wwwtype( BaseType *bt );
DDS *dds_to_www_dds( DDS *dds );

#ifdef FILE_METHODS
void write_html_form_interface(FILE *dest, DDS *dds,
			       const string &url,
			       bool html_header = true,
			       const string &admin_name = "support@opendap.org",
			       const string &help_location = "http://www.opendap.org/online_help_files/opendap_form_help.html");
#endif

void write_html_form_interface(ostream &strm, DDS *dds,
			       const string &url,
			       bool html_header = true,
			       bool netcdf3_file_response = false, bool netcdf4_file_response = false,
			       const string &admin_name = "support@opendap.org",
			       const string &help_location = "http://www.opendap.org/online_help_files/opendap_form_help.html");

/** Write out the HTML for simple type variables. This is used not only for
    single instances of simple types, but also for those types when they
    appear within Structure and Sequence types.

    @param name The name of the varaible. */
void write_simple_variable(FILE *os, BaseType *var);
                           // const string &name,
                           // const string &type);
void write_simple_variable(ostream &strm, BaseType *var);
                           // const string &name,
                           // const string &type);
#if 0
void write_attributes(ostream &os, AttrTable &attr, const string &prefix);
void write_simple_var_attributes(ostream &os, int rows, int cols, BaseType *btp);
#endif

/** Return a string which describes the datatype of a DODS variable.

    @param v The variable.
    @return A string describing the variable's type. */
string fancy_typename(BaseType *v);

/** Generate variable names to be used by the JavaScript code. These names
    must not interfere with JavaScript itself, which seems to have an always
    expanding set of reserved words. Maybe that's stopped now (1/25/2001
    jhrg) but the set is large and includes words such as `Location' which is
    also a dataset variable name. By making the JS variable names
    `dods_<var>' the HTML/JS page/code is still readable without stepping on
    JS' reserved words.

    Note that this is a function and not a class member; I included it in the
    WWWOutput class since it seems to fit here.

    @param dods_name A string that contains the name of a variable.
    @return A name suitable for use in JavaScript code similar to the
    #dods_name# parameter. */

string name_for_js_code(const string &dods_name);

string get_fqn(BaseType *var);

} // namespace dap_html_form
