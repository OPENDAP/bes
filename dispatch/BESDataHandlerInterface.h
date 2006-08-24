// BESDataHandlerInterface.h

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

#ifndef BESDataHandlerInterface_h_
#define BESDataHandlerInterface_h_ 1

#include <string>
#include <list>
#include <map>

using std::string ;
using std::list ;
using std::map ;

class BESResponseHandler ;
class BESInfo ;

#include "BESContainer.h"
#include "ConstraintEvaluator.h"

/** @brief Structure storing information used by OpenDAP to handle the request

    This information is used throughout the OpenDAP server to handle the
    request and to also store information for logging and reporting.
 */

typedef struct _BESDataHandlerInterface
{
    _BESDataHandlerInterface()
	: response_handler( 0 ),
	  container( 0 ),
	  error_info( 0 ) {}

    BESResponseHandler *response_handler ;

    list<BESContainer> containers ;
    list<BESContainer>::iterator containers_iterator ;

    /** @brief pointer to current container in this interface
     */
    BESContainer *container ;

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
    string action_name ;

    /** @brief request protocol, such as HTTP
     */
    string transmit_protocol ;

    /** @brief the map of string data that will be required for the current
     * request.
     */
    map<string, string> data ;
    const map<string, string> &data_c() const { return data ; }
    typedef map<string, string>::const_iterator data_citer ;

    /** @brief constraint evaulator
     */
    ConstraintEvaluator ce ;

    /** @brief error information object
     */
    BESInfo *error_info ;

} BESDataHandlerInterface ;

#endif //  BESDataHandlerInterface_h_

