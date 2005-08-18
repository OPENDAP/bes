// OPeNDAPCmdParser.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef OPeNDAPCmdParser_h_
#define OPeNDAPCmdParser_h_ 1

#include <string>

#include "DODSDataHandlerInterface.h"

using std::string ;

/** @brief parses an incoming request and creates the information necessary to
 * carry out the request.
 *
 * Parses the incoming request, retrieving first the type of response object
 * that is being requested and passing off the parsing of the request to that
 * response handler. For example, if a "get" request is being sent, the parser
 * parses the string "get", locates the response handler that handes a "get"
 * request, and hands off the parsing of the ramaining request string to
 * that response handler.
 *
 * First, the parser builds the list of tokens using the DODSTokernizer
 * object. This list of tokens is then passed to the response handler to
 * parse the remainder of the request.
 *
 * All requests must end with a semicolon.
 *
 * @see DODSTokenizer
 * @see DODSParserException
 */
class OPeNDAPCmdParser
{
public:
    				OPeNDAPCmdParser() ;
    				~OPeNDAPCmdParser();

    void			parse( const string &,
                                       DODSDataHandlerInterface & ) ;
} ;

#endif // OPeNDAPCmdParser_h_

// $Log: OPeNDAPCmdParser.h,v $
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
