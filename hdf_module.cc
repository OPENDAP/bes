
#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "TheRequestHandlerList.h"
#include "HDFRequestHandler.h"
#include "TheDODSLog.h"

static bool
HDFInit(int, char**)
{
    if( TheDODSLog->is_verbose() )
       (*TheDODSLog) << "Initializing HDF:" << endl ;

    if( TheDODSLog->is_verbose() )
        (*TheDODSLog) << "    adding hdf4 request handler" << endl ;

    TheRequestHandlerList->add_handler( "hdf4",
                                       new NCRequestHandler( "hdf4" ) ) ;

    return true ;
}

static bool
HDFTerm(void)
{
    if( TheDODSLog->is_verbose() )
        (*TheDODSLog) << "Removing hdf4 Handlers" << endl;

    DODSRequestHandler *rh = TheRequestHandlerList->remove_handler( "hdf4" ) ;
    if( rh ) delete rh ;
    return true ;
}

FUNINITQUIT( HDFInit, HDFTerm, 3 ) ;
