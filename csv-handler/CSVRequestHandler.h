// CSVRequestHandler.h

#ifndef I_CSVRequestHandler_H
#define I_CSVRequestHandler_H

#include "BESRequestHandler.h"

class CSVRequestHandler : public BESRequestHandler {
public:
			CSVRequestHandler( string name ) ;
    virtual		~CSVRequestHandler( void ) ;

    virtual void	dump( ostream &strm ) const ;

    static bool		csv_build_das( BESDataHandlerInterface &dhi ) ;
    static bool		csv_build_dds( BESDataHandlerInterface &dhi ) ;
    static bool		csv_build_data( BESDataHandlerInterface &dhi ) ;
    static bool		csv_build_vers( BESDataHandlerInterface &dhi ) ;
    static bool		csv_build_help( BESDataHandlerInterface &dhi ) ;
};

#endif // CSVRequestHandler.h

