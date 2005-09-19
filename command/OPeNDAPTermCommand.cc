// OPeNDAPTermCommand.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "OPeNDAPTermCommand.h"
#include "DODSTokenizer.h"

DODSResponseHandler *
OPeNDAPTermCommand::parse_request( DODSTokenizer &tokenizer,
                                   DODSDataHandlerInterface & )
{
    tokenizer.parse_error( "Invalid command" ) ;
    return NULL ;
}

// $Log: OPeNDAPTermCommand.cc,v $
