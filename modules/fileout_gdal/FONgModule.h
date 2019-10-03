// FONgModule.h

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef A_FONgModule_H
#define A_FONgModule_H 1

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
class FONgModule : public BESAbstractModule
{
public:
    				FONgModule() {}
    virtual		    	~FONgModule() {}
    virtual void		initialize( const std::string &modname ) ;
    virtual void		terminate( const std::string &modname ) ;

    virtual void		dump( std::ostream &strm ) const ;
} ;

#endif // A_FONgModule_H

