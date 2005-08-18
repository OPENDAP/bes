// OPeNDAPSetCommand.h

#ifndef A_OPeNDAPSetCommand_h
#define A_OPeNDAPSetCommand_h 1

#include "OPeNDAPCommand.h"

class OPeNDAPSetCommand : public OPeNDAPCommand
{
private:
protected:
public:
    					OPeNDAPSetCommand( const string &cmd )
					    : OPeNDAPCommand( cmd ) {}
    virtual				~OPeNDAPSetCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_OPeNDAPSetCommand_h

