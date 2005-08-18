// OPeNDAPGetCommand.h

#ifndef A_OPeNDAPGetCommand_h
#define A_OPeNDAPGetCommand_h 1

#include "OPeNDAPCommand.h"

class OPeNDAPGetCommand : public OPeNDAPCommand
{
private:
protected:
public:
    					OPeNDAPGetCommand( const string &cmd )
					    : OPeNDAPCommand( cmd ) {}
    virtual				~OPeNDAPGetCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_OPeNDAPGetCommand_h

