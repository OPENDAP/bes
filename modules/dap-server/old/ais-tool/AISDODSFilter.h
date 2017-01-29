
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2003 OPeNDAP, Inc.
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
 
#ifndef ais_dods_filter_h
#define ais_dods_filter_h

#ifndef _dodsfilter_h
#include "DODSFilter.h"
#endif

#ifndef _das_h
#include "DAS.h"
#endif

#ifndef _dds_h
#include "DDS.h"
#endif

#ifndef _objecttype_h
#include "ObjectType.h"
#endif

/** Specialization of DODSFilter to add AIS processing.

    @author jhrg 8/26/97 */

class AISDODSFilter :public DODSFilter {
private:
    string d_ais_db;

public:
    AISDODSFilter(int argc, char *argv[]);

    virtual ~AISDODSFilter() {}

    virtual int process_options(int argc, char *argv[]) throw(Error);

    string get_ais_db() { return d_ais_db; }

    virtual void print_usage() throw(Error) { 
	cerr << "AISDODSFilter::print_usage: FIX ME" << endl;
    }
};

#endif // ais_dods_filter_h
