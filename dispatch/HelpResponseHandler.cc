// HelpResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "HelpResponseHandler.h"
#include "DODSHTMLInfo.h"
#include "cgi_util.h"
#include "TheRequestHandlerList.h"
#include "DODSRequestHandler.h"
#include "TheResponseHandlerList.h"
#include "DODSParserException.h"
#include "TheRequestHandlerList.h"
#include "DODSTokenizer.h"

HelpResponseHandler::HelpResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

HelpResponseHandler::~HelpResponseHandler( )
{
}

void
HelpResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

void
HelpResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = new DODSHTMLInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    // if http then prepare header information for html output
    if( dhi.transmit_protocol == "HTTP" )
    {
	info->add_data( "<HTML>\n" ) ;
	info->add_data( "<HEAD>\n" ) ;
	info->add_data( "<TITLE>DODS Dispatch Help</TITLE>\n" ) ;
	info->add_data( "</HEAD>\n" ) ;
	info->add_data( "<BODY>\n" ) ;
    }

    // get the list of registered servers and the responses that they handle
    info->add_data( "Registered servers:\n" ) ;
    DODSRequestHandlerList::Handler_citer i =
	TheRequestHandlerList->get_first_handler() ;
    DODSRequestHandlerList::Handler_citer ie =
	TheRequestHandlerList->get_last_handler() ;
    for( ; i != ie; i++ ) 
    {
	if( dhi.transmit_protocol == "HTTP" )
	{
	    info->add_data( "<BR />&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n" ) ;
	}
	else
	{
	    info->add_data( "    " ) ;
	}
	DODSRequestHandler *rh = (*i).second ;
	string name = rh->get_name() ;
	string resp = rh->get_handler_names() ;
	info->add_data( name + ": handles " + resp + "\n" ) ;
    }

    string key ;
    if( dhi.transmit_protocol == "HTTP" )
	key = (string)"OPeNDAP.Help." + dhi.transmit_protocol ;
    else
	key = "OPeNDAP.Help.TXT" ;
    info->add_data_from_file( key, "general" ) ;

    // execute help for each registered request server
    if( dhi.transmit_protocol == "HTTP" )
    {
	info->add_data( "<BR /><BR />\n" ) ;
    }
    else
    {
	info->add_data( "\n\n" ) ;
    }

    TheRequestHandlerList->execute_all( dhi ) ;

    // if http then close out the html format
    if( dhi.transmit_protocol == "HTTP" )
    {
	info->add_data( "</BODY>\n" ) ;
	info->add_data( "</HTML>\n" ) ;
    }
    else
    {
	info->add_data( "\n" ) ;
    }
}

void
HelpResponseHandler::transmit( DODSTransmitter *transmitter,
                               DODSDataHandlerInterface &dhi )
{
    // FIX: what if html help display???
    if( _response )
    {
	DODSInfo *info = dynamic_cast<DODSInfo *>(_response) ;
	if( dhi.transmit_protocol == "HTTP" )
	    transmitter->send_html( *info, dhi ) ;
	else
	    transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
HelpResponseHandler::HelpResponseBuilder( string handler_name )
{
    return new HelpResponseHandler( handler_name ) ;
}

// $Log: HelpResponseHandler.cc,v $
// Revision 1.6  2005/04/19 18:00:15  pwest
// added a newline at the end of the help output
//
// Revision 1.5  2005/04/07 19:55:17  pwest
// added add_data_from_file method to allow for information to be added from a file, for example when adding help information from a file
//
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
