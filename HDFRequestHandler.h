
#ifndef I_HDFRequestHandler_H
#define I_HDFRequestHandler_H 1

#include "DODSRequestHandler.h"

class HDFRequestHandler : public DODSRequestHandler {
public:
    HDFRequestHandler( string name ) ;
    virtual ~HDFRequestHandler( void ) ;

    static bool hdf_build_das( DODSDataHandlerInterface &dhi ) ;
    static bool hdf_build_dds( DODSDataHandlerInterface &dhi ) ;
    static bool hdf_build_data( DODSDataHandlerInterface &dhi ) ;
    static bool hdf_build_help( DODSDataHandlerInterface &dhi ) ;
    static bool hdf_build_version( DODSDataHandlerInterface &dhi ) ;
};

#endif
