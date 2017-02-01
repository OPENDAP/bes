// FONcModule.h

// This file is part of BES Fileout NetCDF Module.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef A_FONcModule_H
#define A_FONcModule_H 1

#include <BESAbstractModule.h>

/** @brief Module that allows for OPeNDAP Data objects to be returned as
 * netcdf files
 *
 * The FONcModule (File Out Netcdf Module) is provided to allow for OPenDAP
 * Data objects to be returned as netcdf files. The get request to the BES
 * would be for the dods data product with the added element attribute of
 * returnAs="netcdf".
 *
 * This is accomplished by adding a BESTransmitter called "netcdf". When the
 * BES sees the returnAs property of the get command it looks for a
 * transmitter with that name. The FONcTransmitter is used to transmit
 * the DAP DataDDS object by first writing it out as a netcdf data file
 * and then transmitting it back to the caller.
 *
 * @see FONcTransmitter
 */
class FONcModule: public BESAbstractModule {
public:
    FONcModule()
    {
    }
    virtual ~FONcModule()
    {
    }
    virtual void initialize(const string &modname);
    virtual void terminate(const string &modname);

    virtual void dump(ostream &strm) const;
};

#endif // A_FONcModule_H

