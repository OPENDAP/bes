// OPeNDAPShowCommand.h

#ifndef A_OPeNDAPShowCommand_h
#define A_OPeNDAPShowCommand_h 1

#include "OPeNDAPCommand.h"

class OPeNDAPShowCommand : public OPeNDAPCommand
{
private:
protected:
public:
    					OPeNDAPShowCommand( const string &cmd )
					    : OPeNDAPCommand( cmd ) {}
    virtual				~OPeNDAPShowCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) ;
} ;

#endif // A_OPeNDAPShowCommand_h

