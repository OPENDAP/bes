// DODSRequestHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSRequestHandler_h
#define I_DODSRequestHandler_h 1

#include <map>
#include <string>

using std::map ;
using std::less ;
using std::allocator ;
using std::string ;

#include "DODSDataHandlerInterface.h"

typedef bool (*p_request_handler)(DODSDataHandlerInterface &);

class DODSRequestHandler {
private:
    map< string, p_request_handler, less< string >, allocator< string > > _handler_list ;
    string			_name ;
public:
				DODSRequestHandler( string name )
				    : _name( name ) {}
    virtual			~DODSRequestHandler(void) {}

    typedef map< string, p_request_handler, less< string >, allocator< string > >::const_iterator Handler_citer ;
    typedef map< string, p_request_handler, less< string >, allocator< string > >::iterator Handler_iter ;

    virtual p_request_handler	find_handler( string handler_name ) ;
    virtual string		get_name( ) { return _name ; }

    virtual bool		add_handler( string handler_name,
					    p_request_handler handler_method ) ;
    virtual bool		remove_handler( string handler_name ) ;

    virtual string		get_handler_names() ;
};

#endif // I_DODSRequestHandler_h

// $Log: DODSRequestHandler.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
