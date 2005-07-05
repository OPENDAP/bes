// DODSHeapManager.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSHeapManager_h_
#define DODSHeapManager_h_ 1

#include <sys/resource.h>

class DODSHeapManager
{
    static int counter;
    struct rlimit limit;
public:
    DODSHeapManager();
    ~DODSHeapManager();

    unsigned long megabytes(unsigned int s) ;
    void show_limits() ;
};

#endif // DODSHeapManager_h_

// $Log: DODSHeapManager.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
