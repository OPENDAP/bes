// HelpResponseHandler.h

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_HelpResponseHandler_h
#define I_HelpResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns help information about the server and
 * currently loaded modules.
 *
 * A request 'show help;' will be handled by this response handler. It
 * returns general help information as well as help information for all of
 * the different types of data handled by this server. The list of request
 * handlers (data handlers) registered with the server are listed along with
 * the responses those handlers can handle. Each of those request handlers are
 * given the chance to provide further help.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 */
class HelpResponseHandler : public DODSResponseHandler {
public:
				HelpResponseHandler( string name ) ;
    virtual			~HelpResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *HelpResponseBuilder( string handler_name ) ;
};

#endif // I_HelpResponseHandler_h

