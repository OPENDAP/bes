// DODSDataHandlerInterface.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSDataHandlerInterface_h_
#define DODSDataHandlerInterface_h_ 1

#include <string>
#include <list>

using std::string ;
using std::list ;

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

    /** @brief list of the files accessed for this request
     */
    string real_name_list ;

    /** @brief constraint not handled by data type server that should be handled by OpenDAP
     */
    string post_constraint ;

    /** @brief aggregation command
     */
    string aggregation_command ;

    /** @brief how to return the requested response object
     */
    string return_command ;

    /** @brief name of the OpenDAP user making this request
     */
    string user_name ;

    /** @brief remote ip address of client machine
     */
    string user_address ;

    /** @brief request protocol, such as HTTP
     */
    string transmit_protocol ;

    /** @brief string representation of the request made by the user
     */
    string request ;
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
