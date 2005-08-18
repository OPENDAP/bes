// DODSApacheWrapper.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSApacheWrapper_h_
#define DODSApacheWrapper_h_ 1

/** @brief Wrapper class to OpenDAP for Apache Modules.

    @see _DODSDataRequestInterface
*/

#include "DODSDataRequestInterface.h"

class DODSApacheWrapper
{
    char *		_data_request ;
    char *		_user_name ;
public:
    			DODSApacheWrapper() ;
    			~DODSApacheWrapper() ;

    int			call_DODS( const DODSDataRequestInterface &re ) ;
    const char *	process_request( const char *s ) ;
    const char *	process_user( const char *s ) ;
};

#endif // DODSApacheWrapper_h_

// $Log: DODSApacheWrapper.h,v $
