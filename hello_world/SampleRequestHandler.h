// SampleRequestHandler.h

#ifndef I_SampleRequestHandler_H
#define I_SampleRequestHandler_H

#include "BESRequestHandler.h"

class SampleRequestHandler : public BESRequestHandler {
public:
			SampleRequestHandler( const string &name ) ;
    virtual		~SampleRequestHandler( void ) ;

    virtual void	dump( ostream &strm ) const ;

    static bool		sample_build_vers( BESDataHandlerInterface &dhi ) ;
    static bool		sample_build_help( BESDataHandlerInterface &dhi ) ;
};

#endif // SampleRequestHandler.h

