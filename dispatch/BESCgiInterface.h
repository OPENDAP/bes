// BESCgiInterface.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef BESCgiInterface_h_
#define BESCgiInterface_h_ 1

#include "BESInterface.h"

class DODSFilter ;

/** @brief Represents the classic CGI interface into OPeNDAP.

    OPeNDAP data handlers have been mainly accessed through a CGI interface.
    A person goes to a website with an OPeNDAP server and makes a certain
    request of that server for files available to that site.

    This class provides an interface into the BES framework for the CGI
    interface.  It greatly simplifies server coding for developers using the
    CGI interface.

    Information from the DODSFilter class is placed in the
    BESDataHandlerInterface and a BESContainer is built from this
    information so that BES can handle the request, building the proper
    response using the appropriate data handler. BESCgiInterface also
    creates a Transmitter that interacts with the DODSFilter object for
    sending the response back to the user.

    For example, a server to handle requests for the cedar data type would
    look something like this:

    <pre>
    CedarFilter df(argc, argv);
    BESCgiInterface d( "cedar", df ) ;
    d.execute_request() ;
    </pre>

    And that's it!

    @see BESInterface
    @see BESContainer
    @see _BESDataHandlerInterface
    @see DODSFilter
    @see BESFilterTransmitter
 */
class BESCgiInterface : public BESInterface
{
private:
    string			_type ;
    DODSFilter *		_df ;
    				BESCgiInterface() : BESInterface() {}
protected:
    virtual void		build_data_request_plan() ;
public:
    				BESCgiInterface( const string &type, DODSFilter &df ) ;
    virtual			~BESCgiInterface() ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // BESCgiInterface_h_

