// OPENDAP_CLASSOPENDAP_COMMANDCommand.h

#ifndef A_OPENDAP_CLASSOPENDAP_COMMANDCommand_h
#define A_OPENDAP_CLASSOPENDAP_COMMANDCommand_h 1

#include "BESCommand.h"

class OPENDAP_CLASSOPENDAP_COMMANDCommand : public BESCommand
{
private:
protected:
public:
    					OPENDAP_CLASSOPENDAP_COMMANDCommand( const string &cmd )
					    : BESCommand( cmd ) {}
    virtual				~OPENDAP_CLASSOPENDAP_COMMANDCommand() {}

    virtual BESResponseHandler *	parse_request( BESTokenizer &tokens,
					               BESDataHandlerInterface &dhi ) ;

    virtual void			dump( ostream &strm ) const ;
} ;

#endif // A_OPENDAP_CLASSOPENDAP_COMMANDCommand_h

