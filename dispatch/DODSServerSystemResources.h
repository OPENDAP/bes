// DODSServerSystemResources.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSServerSystemResources_h_
#define DODSServerSystemResources_h_ 1

#include "DODSMemoryStorage.h"

class DODSServerSystemResources
{
    static int _counter;
    static DODSMemoryStorage *_storage;
public:
    DODSServerSystemResources();
    ~DODSServerSystemResources();

    void release_the_memory(){_storage->release_the_memory();}
    bool try_to_get_the_memory(){return _storage->try_to_get_the_memory();}
};

#endif // DODSServerSystemResources_h_

// $Log: DODSServerSystemResources.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
