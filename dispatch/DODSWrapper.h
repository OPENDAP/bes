// DODSWrapper.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSWrapper_h_
#define DODSWrapper_h_ 1

/** @brief Wrapper class to OpenDAP for Apache Modules.

    @see _DODSDataRequestInterface
*/

/*

    This wrapper is necessary because trying to include DODS.h directly from
    Apache cause a conflict since DODS.h uses the header file <string> which
    then calls:
    $(HOME_OF_GCC)/lib/gcc-lib//sparc-sun-solaris2.5.1/2.95.1/../../../../include/g++-3/std/bastring.h
    which in line 38 has the statement: #include <alloc.h> which really is:
    $(HOME_OF_GCC)/lib/gcc-lib/sparc-sun-solaris2.5.1/2.95.1/../../../../include/g++-3/alloc.h
    which is in conflict which $(APACHE_HOME)/src/include/alloc.h
*/

#include "DODSDataRequestInterface.h"

class DODSProcessEncodedString ;

class DODSWrapper
{
    DODSProcessEncodedString *	_encoder ;
public:
    				DODSWrapper() ;
    				~DODSWrapper() ;

    int				call_DODS( const DODSDataRequestInterface &re );
    void			process_commands( const char *s ) ;
    const char *		get_command( const char *s ) ;
};

#endif // DODSWrapper_h_

// $Log: DODSWrapper.h,v $
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
