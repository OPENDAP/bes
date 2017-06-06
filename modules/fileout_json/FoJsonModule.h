// -*- mode: c++; c-basic-offset:4 -*-
//
// FoJsonModule.h
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//


#ifndef FOJSONMODULE_H_
#define FOJSONMODULE_H_

#include <BESAbstractModule.h>

/** @brief Module that allows for OPeNDAP Data objects to be returned as
 * geotiff files
 *
 * The FONgModule (File Out GDAL Module) is provided to allow for OPenDAP
 * Data objects to be returned as geotiff files. The get request to the BES
 * would be for the dods data product with the added element attribute of
 * returnAs="geotiff".
 *
 * This is accomplished by adding a BESTransmitter called "geotiff". When the
 * BES sees the returnAs property of the get command it looks for a
 * transmitter with that name. The FONgTransmitter is used to transmit
 * the DAP DataDDS object by first writing it out as a geotiff data file
 * and then transmitting it back to the caller.
 *
 * @see FONgTransmitter
 */
class FoJsonModule : public BESAbstractModule
{
public:
	FoJsonModule() {}
    virtual		    	~FoJsonModule() {}
    virtual void		initialize( const string &modname ) ;
    virtual void		terminate( const string &modname ) ;

    virtual void		dump( ostream &strm ) const ;
} ;


#endif /* FOJSONMODULE_H_ */
