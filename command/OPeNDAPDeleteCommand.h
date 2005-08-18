// OPeNDAPDeleteCommand.h

#ifndef A_OPeNDAPDeleteCommand_h
#define A_OPeNDAPDeleteCommand_h 1

#include "OPeNDAPCommand.h"

class OPeNDAPDeleteCommand : public OPeNDAPCommand
{
private:
protected:
public:
    					OPeNDAPDeleteCommand( const string &cmd)
					    : OPeNDAPCommand( cmd ) {}
    virtual				~OPeNDAPDeleteCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_OPeNDAPDeleteCommand_h

