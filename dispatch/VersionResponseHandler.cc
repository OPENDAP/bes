// VersionResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "VersionResponseHandler.h"
#include "DODSTextInfo.h"
#include "cgi_util.h"
#include "util.h"
#include "dispatch_version.h"
#include "DODSParserException.h"
#include "TheRequestHandlerList.h"
#include "DODSTokenizer.h"

VersionResponseHandler::VersionResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

VersionResponseHandler::~VersionResponseHandler( )
{
}

void
VersionResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

void
VersionResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    info->add_data( "Core Libraries\n" ) ;
    info->add_data( (string)"    " + dispatch_version() + "\n" ) ;
    info->add_data( (string)"    " + dap_version() + "\n" ) ;
    info->add_data( "Request Handlers\n" ) ;
    TheRequestHandlerList->execute_all( dhi ) ;
}

void
VersionResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
VersionResponseHandler::VersionResponseBuilder( string handler_name )
{
    return new VersionResponseHandler( handler_name ) ;
}

// $Log: VersionResponseHandler.cc,v $
// Revision 1.4  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
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
