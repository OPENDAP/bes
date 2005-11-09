// DeleteResponseHandler.h

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

#ifndef I_DeleteResponseHandler_h
#define I_DeleteResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that deletes a containers, a definition or all
 * definitions.
 *
 * Possible requests handled by this response handler are:
 *
 * delete container &lt;container_name&gt;;
 * <BR />
 * delete definition &lt;def_name&gt;;
 * <BR />
 * delete definitions;
 *
 * There is no command to delete all containers.
 *
 * An informational response object is created and returned to the requester
 * to inform them whether the request was successful.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DeleteResponseHandler : public DODSResponseHandler {
public:
				DeleteResponseHandler( string name ) ;
    virtual			~DeleteResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DeleteResponseBuilder( string handler_name ) ;
};

#endif // I_DeleteResponseHandler_h

// $Log: DeleteResponseHandler.h,v $
// Revision 1.1  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
