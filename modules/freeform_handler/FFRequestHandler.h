
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf5_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// CDFRequestHandler.h

#ifndef I_FFRequestHandler_H
#define I_FFRequestHandler_H 1

#include "BESRequestHandler.h"

class FFRequestHandler : public BESRequestHandler {
private:
    static bool d_RSS_format_support;
    static string d_RSS_format_files;

    static bool d_Regex_format_support;
    static map<string,string> d_fmt_regex_map;
    //static string d_Regex_expr;
public:
	FFRequestHandler( const string &name ) ;
    virtual	~FFRequestHandler( void ) ;

    static bool ff_build_das(BESDataHandlerInterface &dhi);
    static bool ff_build_dds(BESDataHandlerInterface &dhi);
    static bool ff_build_data(BESDataHandlerInterface &dhi);

    static bool ff_build_dmr(BESDataHandlerInterface &dhi);

    static bool ff_build_help(BESDataHandlerInterface &dhi);
    static bool ff_build_version(BESDataHandlerInterface &dhi);

    static bool get_RSS_format_support() { return d_RSS_format_support; }
    static string get_RSS_format_files() { return d_RSS_format_files; }

    void add_attributes(BESDataHandlerInterface &dhi);

    static bool get_Regex_format_support() { return d_Regex_format_support; }
    static map<string,string> get_fmt_regex_map() { return d_fmt_regex_map; }

};

#endif

