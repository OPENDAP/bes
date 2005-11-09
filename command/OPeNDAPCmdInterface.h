// OPeNDAPCmdInterface.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef OPeNDAPCmdInterface_h_
#define OPeNDAPCmdInterface_h_ 1

#include <new>

using std::new_handler ;
using std::bad_alloc ;

#include "DODS.h"
#include "DODSDataHandlerInterface.h"
#include "DODSDataRequestInterface.h"

class DODSMemoryGlobalArea ;
class DODSException ;

/** @brief Entry point into OPeNDAP using apache modules

    The OPeNDAPCmdInterface class is the entry point for accessing information using
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

    OPeNDAPCmdInterface uses DODSParser in order to build the list of containers
    (symbolic containers resolving to real file and server type). Because
    the request is being made from a browser, a DODSBasicHttpTransmitter
    object is used to transmit the response object back to the requesting
    browser.

    @see DODS
    @see DODSParser
    @see DODSBasicHttpTransmitter
 */
class OPeNDAPCmdInterface : public DODS
{
protected:
    virtual void		initialize() ;
    virtual void		validate_data_request() ;
    virtual void		build_data_request_plan() ;
    virtual void		execute_data_request_plan() ;
    virtual void		invoke_aggregation();
    virtual void		transmit_data() ;
    virtual void		log_status() ;
    virtual void		clean() ;
public:
    				OPeNDAPCmdInterface() ;
    				OPeNDAPCmdInterface( const string &cmd ) ;
    virtual			~OPeNDAPCmdInterface() ;

    virtual int			execute_request() ;
} ;

#endif // OPeNDAPCmdInterface_h_

// $Log: OPeNDAPCmdInterface.h,v $
