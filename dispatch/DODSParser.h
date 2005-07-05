// DODSParser.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSParser_h_
#define DODSParser_h_ 1

#include <string>

#include "DODSDataHandlerInterface.h"

using std::string ;

/** @brief parses an incoming request and creates the information necessary to
 * carry out the request.
 *
 * Parses the incoming request, retrieving the type of response object to be
 * built, the type of data being requested, the physical information about
 * where the data is, and constraints on the data to be retrieved. Multiple
 * sets of data can be requested, each one can have a constraint string.
 *
 * Format for requests:
 * <pre>
 * get <action> for <symbolic_name1, symbolic_name2, ..., symbolic_namen>
 *     where <symbolic_name1>.constraint="<constraint>",
 *           <symbolic_name2>.constraint="<constraint>",
 *           ...,
 *           <symbolic_namen>.constraint="<constraint>";
 * </pre>
 *
 * requests also available include:
 *
 * show version;
 * show help;
 *
 * All requests must end with a semicolon.
 *
 * <action> can be any string that represents a response object requested by the
 * user, such as das, dds, dods, etc...
 *
 * <symbolic_name> are names used to represent the type of data being
 * requested (netcdf, cdf, cedar, etc...) and the actual file being requested.
 * See DODSContainer and DODSContainerPersistence for more information and
 * examples.
 *
 * <constraint> represents a constraint string to be used for accessing the
 * data for the specific symbolic name.
 *
 * @see DODSParserException
 * @see DODSContainer
 * @see DODSContainerPersistence
 * @see DODS
 */
class DODSParser
{
public:
    				DODSParser() ;
    				~DODSParser();

    void			parse( const string &,
                                       DODSDataHandlerInterface & ) ;
} ;

#endif // DODSParser_h_

// $Log: DODSParser.h,v $
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
