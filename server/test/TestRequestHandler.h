// TestRequestHandler.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cdf-handler, a C++ server of OPeNDAP for access to cdf
// data

// Copyright (c) 2004-2009 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
 
// cdf-handler implementation of OPeNDAP BES REquestHandler class that knows
// how to fill in OPeNDAP objects
//
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

// TestRequestHandler.h

#ifndef I_TestRequestHandler_H
#define I_TestRequestHandler_H 1

#include "BESRequestHandler.h"

class TestRequestHandler : public BESRequestHandler {
public:
			TestRequestHandler( string name ) ;
    virtual		~TestRequestHandler( void ) ;

    virtual void	dump( ostream &strm ) const ;

    static bool		cdf_build_help( BESDataHandlerInterface &dhi ) ;
};

#endif

