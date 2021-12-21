
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

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

#ifndef _www_output_h
#define _www_output_h

#include <string>
#include <iostream>

#include <libdap/BaseType.h>
#include <libdap/DDS.h>
#include <libdap/DAS.h>

using namespace libdap ;

/** Write various parts of the HTML form for a dataset.

    @author jhrg. */

class WWWOutput {
private:
    // This pointer doesn't appear to be used anywhere, including in main()
    // so commenting it out for now... pcw 08/15/07

    // This pointer is allocated in main() and passed in here so that other
    // functions/methods which use the global instance of WWWOutput name 'wo'
    // can access the DAS object. There are better ways to do this, but this
    // little tool is so simple that I don't think it needs to be
    // rewritten... jhrg

    //DAS *d_das;

 protected:
    ostream *d_strm;
    int d_attr_rows;
    int d_attr_cols;

    void write_attributes(AttrTable *attr, const string prefix = "");

 public:
    /** Create a WWWOutput.

	@param os The output stream to which HTML should be sent.
	@param rows The number of rows to show in the attribute textbox
	(default 5).
	@param cols The number of columns to show in the attribute textbox
	(default 70). */
    WWWOutput(ostream &strm, int rows = 5, int cols = 70);

    /** Write out the header for the HTML document. */
#if 1
    // TODO: Can this be removed?
    void write_html_header();
#endif
    /** Write the disposition section of the HTML page. This section shows
	the URL and provides buttons with which the user can choose the type
	of output.

	@param url The initial URL to display.
	@param FONc True if the Fileout netCDF button should be shown. */
    void write_disposition(string url, bool netcdf3_file_response, bool netcdf4_file_response);

    void write_global_attributes(AttrTable &attr);

    /** Write the dataset variable list. This is a scrolling select box.

	@deprecated
	@param dds The dataset's DDS. */
    void write_variable_list(DDS &dds);

    void write_variable_entries(DDS &dds);

    void write_variable_attributes(BaseType * btp);
};

#endif // __www_output_h
