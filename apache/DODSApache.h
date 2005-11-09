// DODSApache.h

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

#ifndef DODSApache_h_
#define DODSApache_h_ 1

#include <new>

using std::new_handler ;
using std::bad_alloc ;

#include "OPeNDAPCmdInterface.h"
#include "DODSDataHandlerInterface.h"
#include "DODSDataRequestInterface.h"

class DODSMemoryManager ;
class DODSException ;

/** @brief Entry point into OPeNDAP using apache modules

    The DODSApache class is the entry point for accessing information using
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

    DODSApache uses DODSParser in order to build the list of containers
    (symbolic containers resolving to real file and server type). Because
    the request is being made from a browser, a DODSBasicHttpTransmitter
    object is used to transmit the response object back to the requesting
    browser.

    @see DODS
    @see DODSParser
    @see DODSBasicHttpTransmitter
 */
class DODSApache : public OPeNDAPCmdInterface
{
private:
    void			welcome_browser() ;
    const			DODSDataRequestInterface * _dri ;
protected:
    virtual int			exception_manager(DODSException &e) ;
    virtual void		initialize() ;
    virtual void		validate_data_request() ;
public:
    				DODSApache( const DODSDataRequestInterface &dri ) ;
    virtual			~DODSApache() ;

    virtual int			execute_request() ;
} ;

#endif // DODSApache_h_

// $Log: DODSApache.h,v $
// Revision 1.5  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
