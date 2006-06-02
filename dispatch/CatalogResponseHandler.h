// CatalogResponseHandler.h

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

#ifndef I_CatalogResponseHandler_h
#define I_CatalogResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that returns nodes or leaves within the catalog
 * either at the root or at a specified node.
 *
 * A request 'show catalog [for &lt;node&gt;];' or
 * 'show leaves for &lt;node&gt;;
 * will be handled by this response handler. It returns nodes or leaves either
 * at the root level if no node is specified in the request, or the nodes or
 * leaves under the specified node.
 *
 * @see DODSResponseObject
 * @see DODSContainer
 * @see DODSTransmitter
 */
class CatalogResponseHandler : public DODSResponseHandler
{
private:
public:
				CatalogResponseHandler( string name ) ;
    virtual			~CatalogResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *CatalogResponseBuilder( string handler_name ) ;
};

#endif // I_CatalogResponseHandler_h

