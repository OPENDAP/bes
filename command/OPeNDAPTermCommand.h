// OPeNDAPTermCommand.h

#ifndef A_OPeNDAPTermCommand_h
#define A_OPeNDAPTermCommand_h 1

#include "OPeNDAPCommand.h"

class OPeNDAPTermCommand : public OPeNDAPCommand
{
private:
protected:
public:
    					OPeNDAPTermCommand( const string &cmd)
					    : OPeNDAPCommand( cmd ) {}
    virtual				~OPeNDAPTermCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_OPeNDAPTermCommand_h

