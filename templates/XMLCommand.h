// OPENDAP_CLASSOPENDAP_COMMANDCommand.h

#ifndef A_OPENDAP_CLASSOPENDAP_COMMANDXMLCommand_h
#define A_OPENDAP_CLASSOPENDAP_COMMANDXMLCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

class OPENDAP_CLASSOPENDAP_COMMANDXMLCommand : public BESXMLCommand
{
public:
    				OPENDAP_CLASSOPENDAP_COMMANDXMLCommand( const BESDataHandlerInterface &base_dhi ) ;
    virtual			~OPENDAP_CLASSOPENDAP_COMMANDXMLCommand() {}

    virtual void		parse_request( xmlNode *node ) ;

    // does your command have a response. Some do not. Only one command
    // per request document can have a response.
    virtual bool		has_response() { return false ; }

    virtual void		prep_request() ;

    virtual void		dump( ostream &strm ) const ;

    static BESXMLCommand *	CommandBuilder( const BESDataHandlerInterface &base_dhi ) ;
} ;

#endif // A_OPENDAP_CLASSOPENDAP_COMMANDXMLCommand_h

