// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
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

#ifndef I_NCRequestHandler_H
#define I_NCRequestHandler_H 1

#include <BESRequestHandler.h>

class ObjMemCache;  // in bes/dap

namespace libdap {
class DDS;
}

class NCRequestHandler: public BESRequestHandler {
private:
	static bool _show_shared_dims;
	static bool _show_shared_dims_set;

	static bool _ignore_unknown_types;
	static bool _ignore_unknown_types_set;

	static bool _promote_byte_to_short_set;
	static bool _promote_byte_to_short;

	static unsigned int _cache_entries;
	static float _cache_purge_level;

    static ObjMemCache *das_cache;
    static ObjMemCache *dds_cache;
    static ObjMemCache *dmr_cache;

    static void get_dds_with_attributes(const std::string& dataset_name, const std::string& container_name, libdap::DDS* dds);

public:
	NCRequestHandler(const string &name);
	virtual ~NCRequestHandler(void);

	static bool nc_build_das(BESDataHandlerInterface &dhi);
	static bool nc_build_dds(BESDataHandlerInterface &dhi);
	static bool nc_build_data(BESDataHandlerInterface &dhi);
	static bool nc_build_dmr(BESDataHandlerInterface &dhi);
	static bool nc_build_help(BESDataHandlerInterface &dhi);
	static bool nc_build_version(BESDataHandlerInterface &dhi);

	static bool get_show_shared_dims()
	{
		return _show_shared_dims;
	}
	static bool get_ignore_unknown_types()
	{
		return _ignore_unknown_types;
	}
	static bool get_promote_byte_to_short()
	{
		return _promote_byte_to_short;
	}
	static unsigned int get_cache_entries()
	{
	    return _cache_entries;
	}
	static float get_cache_purge_level()
	{
	    return _cache_purge_level;
	}
};

#endif

