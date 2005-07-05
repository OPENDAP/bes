// DODSParser.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSParser.h"
#include "DODSTokenizer.h"
#include "TheResponseHandlerList.h"
#include "DODSResponseHandler.h"
#include "DODSParserException.h"

DODSParser::DODSParser( )
{
}

DODSParser::~DODSParser()
{
}

/** @brief parse the request string and build the execution plan for the
 * request.
 *
 * Parse the request string and builds the execution plan for the request.
 * This plan includes the type of response object that is being requested.
 *
 * @param request string representing the request from the client
 * @param dhi information needed to build the request and to store request
 * information for the server
 * @throws DODSParserException thrown if there is an error in syntax
 * @see DODSContainer
 * @see DODSContainerPersistence
 * @see DODS
 * @see _DODSDataHandlerInterface
 */
void
DODSParser::parse( const string &request, DODSDataHandlerInterface &dhi )
{
    DODSTokenizer t ;
    t.tokenize( request.c_str() ) ;
    string my_token = t.get_first_token() ;
    dhi.response_handler = TheResponseHandlerList->find_handler( my_token ) ;
    if( dhi.response_handler )
    {
	dhi.response_handler->parse( t, dhi ) ;
    }
    else
    {
	throw DODSParserException( (string)"Improper command " + my_token ) ;
    }
}

// $Log: DODSParser.cc,v $
// Revision 1.5  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:36:01  pwest
//
// Changed the way in which the parser retrieves container information, going
// instead to ThePersistenceList, which goes through the list of container
// persistence instances it has.
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
