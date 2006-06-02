// DODSDataHandlerInterface.h

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

#ifndef DODSDataHandlerInterface_h_
#define DODSDataHandlerInterface_h_ 1

#include <string>
#include <list>
#include <map>

using std::string ;
using std::list ;
using std::map ;

class DODSResponseHandler ;

#include "DODSContainer.h"

/** @brief Structure storing information used by OpenDAP to handle the request

    This information is used throughout the OpenDAP server to handle the
    request and to also store information for logging and reporting.
 */

typedef struct _DODSDataHandlerInterface
{
    _DODSDataHandlerInterface()
	: response_handler( 0 ),
	  container( 0 ) {}
    DODSResponseHandler *response_handler ;

    list<DODSContainer> containers ;
    list<DODSContainer>::iterator containers_iterator ;

    /** @brief pointer to current container in this interface
     */
    DODSContainer *container ;

    /** @brief set the container pointer to the first container in the containers list
     */
    void first_container()
    {
	containers_iterator = containers.begin();
	if( containers_iterator != containers.end() )
	    container = &(*containers_iterator) ;
	else
	    container = NULL ;
    }

    /** @brief set the container pointer to the next * container in the list, null if at the end or no containers in list
     */
    void next_container()
    {
	containers_iterator++ ;
	if( containers_iterator != containers.end() )
	    container = &(*containers_iterator) ;
	else
	    container = NULL ;
    }

    /** @brief the response object requested, e.g. das, dds
     */
    string action ;

    /** @brief request protocol, such as HTTP
     */
    string transmit_protocol ;

    /** @brief the map of string data that will be required for the current
     * request.
     */
    map<string, string> data ;
    const map<string, string> &data_c() const { return data ; }
    typedef map<string, string>::const_iterator data_citer ;

} DODSDataHandlerInterface ;

#endif //  DODSDataHandlerInterface_h_

// $Log: DODSDataHandlerInterface.h,v $
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
