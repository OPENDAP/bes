// DODSCgi.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSCgi_h_
#define DODSCgi_h_ 1

#include <new>

using std::new_handler ;
using std::bad_alloc ;

#include "DODS.h"

class DODSFilter ;

/** @brief Represents the classic CGI interface into OpenDAP.

    OpenDAP has been mainly accessed through a CGI interface. A person goes to
    a website with an OpenDAP server and makes a certain request of that
    server for files available to that site.

    This class provides an interface into the dispatch for the CGI interface.
    It greatly simplifies server coding for developers using the CGI
    interface.

    Information from the DODSFilter class is placed in the
    DODSDataHandlerInterface and a DODSContainer is built from this
    information so that dispatch can handle the request and build the
    proper response using the appropriate data handler. DODSCgi also creates a
    Transmitter that interacts with the DODSFilter object for sending the
    response back to the user.

    For example, a server to handle requests for the cedar data type would
    look something like this:

    <pre>
    CedarFilter df(argc, argv);
    DODSCgi d( "cedar", df ) ;
    d.execute_request() ;
    </pre>

    And that's it!

    @see DODS
    @see DODSContainer
    @see _DODSDataHandlerInterface
    @see DODSFilter
    @see DODSFilterTransmitter
 */
class DODSCgi : public DODS
{
private:
    string			_type ;
    DODSFilter *		_df ;
protected:
    virtual void		build_data_request_plan() ;
public:
    				DODSCgi( const string &type, DODSFilter &df ) ;
    virtual			~DODSCgi() ;
} ;

#endif // DODSCgi_h_

// $Log: DODSCgi.h,v $
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
