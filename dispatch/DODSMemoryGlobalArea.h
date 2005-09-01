// DODSMemoryGlobalArea.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSMemoryGlobalArea_h_
#define DODSMemoryGlobalArea_h_ 1

#include <sys/resource.h>

#define MEGABYTE 1024*1024

class DODSMemoryGlobalArea
{
    struct rlimit limit;
    static unsigned long _size;
    static int _counter;
    static void *_buffer;

    unsigned long megabytes(unsigned int s) 
    {
	return s*MEGABYTE;
    }
    void log_limits();

public:
    DODSMemoryGlobalArea();
    ~DODSMemoryGlobalArea();

    void release_memory();
    bool reclaim_memory();
    unsigned long SizeOfEmergencyPool(){return _size;}
};

#endif // DODSMemoryGlobalArea_h_

// $Log: DODSMemoryGlobalArea.h,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
