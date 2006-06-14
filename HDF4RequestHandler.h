
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf4_handler, a data handler for the OPeNDAP data
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// CDFRequestHandler.h

#ifndef I_HDF4RequestHandler_H
#define I_HDF4RequestHandler_H 1

#include <string>

using std::string ;

#include "BESRequestHandler.h"

class HDF4RequestHandler : public BESRequestHandler {
private:
    static string	_cachedir ;
public:
			HDF4RequestHandler( string name ) ;
    virtual		~HDF4RequestHandler( void ) ;

    static bool		hdf4_build_das( BESDataHandlerInterface &dhi ) ;
    static bool		hdf4_build_dds( BESDataHandlerInterface &dhi ) ;
    static bool		hdf4_build_data( BESDataHandlerInterface &dhi ) ;
    static bool		hdf4_build_help( BESDataHandlerInterface &dhi ) ;
    static bool		hdf4_build_version( BESDataHandlerInterface &dhi ) ;
};

#endif

