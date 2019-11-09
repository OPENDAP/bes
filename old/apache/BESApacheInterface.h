// BESApacheInterface.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESApacheInterface_h_
#define BESApacheInterface_h_ 1

#include <new>

#include "BESCmdInterface.h"
#include "BESDataHandlerInterface.h"
#include "BESDataRequestInterface.h"

class BESMemoryManager ;
class BESError ;

/** @brief Entry point into OPeNDAP using apache modules

    The BESApacheInterface class is the entry point for accessing information using
    OPeNDAP through apache modules.

    The format of the request looks somethink like:

    get das for sym1,sym2
	with sym1.constraint="constraint",sym2.constraint="constraint";

    In this example a DAS object response is being requested. The DAS object
    is to be built from the two symbolic names where each symbolic name has
    a constraint associated with it. The symbolic names are resoleved to be
    real files for a given server type. For example, sym1 could resolve to a
    file accessed through cedar and sym2 could resolve to a file accessed
    through netcdf.

    BESApacheInterface uses BESParser in order to build the list of containers
    (symbolic containers resolving to real file and server type). Because
    the request is being made from a browser, a BESBasicHttpTransmitter
    object is used to transmit the response object back to the requesting
    browser.

    @see BESInterface
    @see BESParser
    @see BESBasicHttpTransmitter
 */
class BESApacheInterface : public BESCmdInterface
{
private:
    void			welcome_browser() ;
    const			BESDataRequestInterface * _dri ;
protected:
    virtual int			exception_manager(BESError &e) ;
    virtual void		initialize() ;
    virtual void		validate_data_request() ;
public:
    				BESApacheInterface( const BESDataRequestInterface &dri ) ;
    virtual			~BESApacheInterface() ;

    virtual int			execute_request() ;
} ;

#endif // BESApacheInterface_h_

