// KeysResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "KeysResponseHandler.h"
#include "TheDODSKeys.h"
#include "DODSTextInfo.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"

KeysResponseHandler::KeysResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

KeysResponseHandler::~KeysResponseHandler( )
{
}

void
KeysResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

void
KeysResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;

    info->add_data( (string)"List of currently defined keys "
                    + " from the initialization file "
                    + TheDODSKeys->keys_file_name() + "\n" ) ;

    DODSKeys::Keys_citer ki = TheDODSKeys->keys_begin() ;
    DODSKeys::Keys_citer ke = TheDODSKeys->keys_end() ;
    for( ; ki != ke; ki++ )
    {
	string line = (string)"key: \"" + (*ki).first
	              + "\", value:\"" + (*ki).second
		      + "\"\n" ;
	info->add_data( line ) ;
    }
}

void
KeysResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi );
    }
}

DODSResponseHandler *
KeysResponseHandler::KeysResponseBuilder( string handler_name )
{
    return new KeysResponseHandler( handler_name ) ;
}

// $Log: KeysResponseHandler.cc,v $
// Revision 1.1  2005/04/19 17:53:08  pwest
// show keys handler
//
