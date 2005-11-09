// DASResponseHandler.h

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

#ifndef I_DASResponseHandler_h
#define I_DASResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that builds an OPeNDAP DAS object
 *
 * A request 'get das for &lt;def_name&gt;;' will be handled by this
 * response handler. Given a definition name it determines what containers
 * are to be used to build the OPeNDAP DAS response object. It then
 * transmits the DAS object using the method send_das.
 *
 * @see DAS
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DASResponseHandler : public DODSResponseHandler {
public:
				DASResponseHandler( string name ) ;
    virtual			~DASResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DASResponseBuilder( string handler_name ) ;
};

#endif // I_DASResponseHandler_h

// $Log: DASResponseHandler.h,v $
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
