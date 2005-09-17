// DODSRequestHandlerList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSRequestHandlerList_h
#define I_DODSRequestHandlerList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"

class DODSRequestHandler ;

/** @brief maintains the list of registered request handlers for this server
 *
 * For a type of data to be handled by an OPeNDAP-g server the data type must
 * registered a request handler with the server. This request handler knows
 * how to fill in specific response objects, such as DAS, DDS, help, version,
 * etc... The request handlers are registered with this request handler list.
 */
class DODSRequestHandlerList {
private:
    map< string, DODSRequestHandler * > _handler_list ;
public:
				DODSRequestHandlerList(void) {}
    virtual			~DODSRequestHandlerList(void) {}

    typedef map< string, DODSRequestHandler * >::const_iterator Handler_citer ;
    typedef map< string, DODSRequestHandler * >::iterator Handler_iter ;

    virtual bool		add_handler( string handler_name,
					 DODSRequestHandler * handler ) ;
    virtual DODSRequestHandler *remove_handler( string handler_name ) ;
    virtual DODSRequestHandler *find_handler( string handler_name ) ;

    virtual Handler_citer	get_first_handler() ;
    virtual Handler_citer	get_last_handler() ;

    virtual string		get_handler_names() ;

    virtual void		execute_each( DODSDataHandlerInterface &dhi ) ;
    virtual void		execute_all( DODSDataHandlerInterface &dhi ) ;
    virtual void		execute_once( DODSDataHandlerInterface &dhi ) ;
};

#endif // I_DODSRequestHandlerList_h

// $Log: DODSRequestHandlerList.h,v $
// Revision 1.4  2005/03/15 20:00:14  pwest
// added execute_once so that a single function can execute the request using all the containers instead of executing a function for each container. This is for requests that are handled by the same request type, for example, all containers are of type nc
//
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
