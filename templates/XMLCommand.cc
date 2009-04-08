// OPENDAP_CLASSOPENDAP_COMMANDXMLCommand.cc

#include "OPENDAP_CLASSOPENDAP_COMMANDXMLCommand.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::OPENDAP_CLASSOPENDAP_COMMANDXMLCommand( const BESDataHandlerInterface &base_dhi )
    : BESXMLCommand( base_dhi )
{
}

/** @brief parse a OPENDAP_COMMAND command.
 *
 *
 * @param node xml2 element node pointer
 */
void
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::parse_request( xmlNode *node )
{
    // your code here - parsing the node for your purposes

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response() ;
}

/** @brief the request has been parsed, take the information and prepare
 * to execute the request
 */
void
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::prep_request()
{
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESXMLCommand::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESXMLCommand *
OPENDAP_CLASSOPENDAP_COMMANDXMLCommand::CommandBuilder( const BESDataHandlerInterface &base_dhi )
{
    return new OPENDAP_CLASSOPENDAP_COMMANDXMLCommand( base_dhi ) ;
}

