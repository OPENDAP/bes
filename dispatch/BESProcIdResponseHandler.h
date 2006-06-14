// BESProcIdResponseHandler.h

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

#ifndef I_BESProcIdResponseHandler_h
#define I_BESProcIdResponseHandler_h 1

#include "BESResponseHandler.h"

/** @brief response handler that returns the process id for the server process
 *
 * A request 'show process;' will be handled by this response handler. It
 * returns the process id of the server process connected to the requesting
 * client.
 *
 * @see DODSResponseObject
 * @see BESContainer
 * @see BESTransmitter
 */
class BESProcIdResponseHandler : public BESResponseHandler
{
private:
    char *			fastpidconverter( long val,
                                                  char *buf,
						  int base ) ;
public:
				BESProcIdResponseHandler( string name ) ;
    virtual			~BESProcIdResponseHandler( void ) ;

    virtual void		execute( BESDataHandlerInterface &dhi ) ;
    virtual void		transmit( BESTransmitter *transmitter,
                                          BESDataHandlerInterface &dhi ) ;

    static BESResponseHandler *ProcIdResponseBuilder( string handler_name ) ;
};

#endif // I_BESProcIdResponseHandler_h

