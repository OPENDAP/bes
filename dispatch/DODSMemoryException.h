// DODSMemoryException.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSMemoryException_h_
#define DODSMemoryException_h_ 1

#include <new>

using std::bad_alloc ;

#include "DODSBasicException.h"

class DODSMemoryException: public DODSBasicException
{
private:
    unsigned long	_mem_required;
public:
    			DODSMemoryException() { _mem_required = 0 ; }
    virtual		~DODSMemoryException() {}
    void		set_amount_of_memory_required( const unsigned long &a );
    unsigned int	get_amount_of_memory_required();
};

#endif // DODSMemoryException_h_

// $Log: DODSMemoryException.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
