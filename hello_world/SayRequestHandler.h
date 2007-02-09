// SayRequestHandler.h

#ifndef I_SayRequestHandler_H
#define I_SayRequestHandler_H

#include "BESRequestHandler.h"

class SayRequestHandler : public BESRequestHandler {
public:
			SayRequestHandler( string name ) ;
    virtual		~SayRequestHandler( void ) ;

    virtual void	dump( ostream &strm ) const ;

    static bool		say_build_vers( BESDataHandlerInterface &dhi ) ;
    static bool		say_build_help( BESDataHandlerInterface &dhi ) ;
};

#endif // SayRequestHandler.h

