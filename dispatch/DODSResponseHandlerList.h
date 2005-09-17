// DODSResponseHandlerList.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSResponseHandlerList_h
#define I_DODSResponseHandlerList_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

class DODSResponseHandler ;

typedef DODSResponseHandler * (*p_response_handler)( string name ) ;

/** @brief List of all registered response handlers for this server
 *
 * A DODSResponseHandlerList allows the developer to add or remove response
 * handlers from the list of handlers available for this server. When a
 * request comes in, for example of the for 'get das for d1;', there is a
 * response handler that knows how to build a response object for the get
 * request, or for the command 'set container values sym1,real1,type1;' there
 * is a response handler that knows how to build a response object for the set
 * request.
 *
 * The response handler knows how to build a response object, whereas a
 * request handler knows how to fill in the response object built by the
 * response handler.
 *
 * In the case of some response handlers, such as get and show, there is a sub
 * response handler that knows how to build a specific get request, such as
 * 'get das' or 'get dds' or 'show version;' or 'show help;'
 * @see DODSResponseHandler
 * @see DODSResponseObject
 * @see DODSRequestHandler
 */
class DODSResponseHandlerList {
private:
    map< string, p_response_handler > _handler_list ;
public:
				DODSResponseHandlerList(void) {}
    virtual			~DODSResponseHandlerList(void) {}

    typedef map< string, p_response_handler >::const_iterator Handler_citer ;
    typedef map< string, p_response_handler >::iterator Handler_iter ;

    virtual bool		add_handler( string handler_name,
					   p_response_handler handler_method ) ;
    virtual bool		remove_handler( string handler_name ) ;
    virtual DODSResponseHandler *find_handler( string handler_name ) ;

    virtual string		get_handler_names() ;
};

#endif // I_DODSResponseHandlerList_h

// $Log: DODSResponseHandlerList.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
