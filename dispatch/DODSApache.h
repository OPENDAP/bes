// DODSApache.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSApache_h_
#define DODSApache_h_ 1

#include <new>

using std::new_handler ;
using std::bad_alloc ;

#include "DODS.h"
#include "DODSDataHandlerInterface.h"
#include "DODSDataRequestInterface.h"

class DODSMemoryGlobalArea ;
class DODSException ;

/** @brief Entry point into OPeNDAP using apache modules

    The DODSApache class is the entry point for accessing information using
    OPeNDAP through apache modules.

    The format of the request looks somethink like:

    get das for sym1,sym2
	with sym1.constraint="constraint",sym2.constraint="constraint";

    In this example a DAS object response is being requested. The DAS object
    is to be built from the two symbolic names where each symbolic name has
    a constraint associated with it. The symbolic names are resoleved to be
    real files for a given server type. For example, sym1 could resolve to a
    file accessed through cedar and sym2 could resolve to a file accessed
    through netcdf.

    DODSApache uses DODSParser in order to build the list of containers
    (symbolic containers resolving to real file and server type). Because
    the request is being made from a browser, a DODSBasicHttpTransmitter
    object is used to transmit the response object back to the requesting
    browser.

    @see DODS
    @see DODSParser
    @see DODSBasicHttpTransmitter
 */
class DODSApache : public DODS
{
private:
    void			welcome_browser() ;
    const			DODSDataRequestInterface * _dri ;

    static			DODSMemoryGlobalArea *_memory ;
    static bool			_storage_used ;
    static new_handler		_global_handler ;

    static DODSMemoryGlobalArea* initialize_memory_pool() ;
    static void			swap_memory() ;
    static void			release_global_pool()throw (bad_alloc) ;
    static void			register_global_pool() ;
    static bool			unregister_global_pool() ;
    static bool			check_memory_pool() ;

    bool			_created_transmitter ;
protected:
    virtual int			exception_manager(DODSException &e) ;
    virtual void		initialize() ;
    virtual void		validate_data_request() ;
    virtual void		build_data_request_plan() ;
    virtual void		execute_data_request_plan() ;
    virtual void		invoke_aggregation();
    virtual void		transmit_data() ;
    virtual void		log_status() ;
    virtual void		clean() ;
public:
    				DODSApache( const DODSDataRequestInterface &dri ) ;
    virtual			~DODSApache() ;

    virtual int			execute_request() ;
} ;

#endif // DODSApache_h_

// $Log: DODSApache.h,v $
// Revision 1.5  2005/03/26 00:33:33  pwest
// fixed aggregation server invoke problems. Other commands use the aggregation command but no aggregation is needed, so set aggregation to empty string when done
//
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
