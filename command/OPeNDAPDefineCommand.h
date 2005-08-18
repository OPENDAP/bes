// OPeNDAPDefineCommand.h

#ifndef A_OPeNDAPDefineCommand_h
#define A_OPeNDAPDefineCommand_h 1

#include "OPeNDAPCommand.h"

class OPeNDAPDefineCommand : public OPeNDAPCommand
{
private:
protected:
public:
    					OPeNDAPDefineCommand( const string &cmd)
					    : OPeNDAPCommand( cmd ) {}
    virtual				~OPeNDAPDefineCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_OPeNDAPDefineCommand_h

