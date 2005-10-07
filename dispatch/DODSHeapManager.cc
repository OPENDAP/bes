// DODSHeapManager.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <string.h>
#include <errno.h>
#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "DODSHeapManager.h"

#define DODS_MEGABYTE 1024*1024

int DODSHeapManager::counter=0;

DODSHeapManager::DODSHeapManager()
{
    if (counter++ == 0) 
    {
	limit.rlim_cur=megabytes(3);
	limit.rlim_max=megabytes(3);
	if(setrlimit(RLIMIT_DATA, &limit)<0)
	{
	    cerr << "DODS: Could not set limit for the heap because "
		 << strerror(errno)
		 << endl;
	    exit(1);
	}
    }
}

DODSHeapManager::~DODSHeapManager()
{
    --counter;
}
  
unsigned long DODSHeapManager::megabytes(unsigned int s) 
{
    return DODS_MEGABYTE * s ;
}

void DODSHeapManager::show_limits()
{
    if (getrlimit(RLIMIT_DATA, &limit)<0)
    {
	cerr << "Could not get limit" <<endl;
	exit(1);
    }
    if(limit.rlim_cur==RLIM_INFINITY)
	cout << "I have infinity soft" <<endl;
    else
	cout << "I have soft limit " <<limit.rlim_cur<<endl;
    if(limit.rlim_max==RLIM_INFINITY)
	cout << "I have infinity hard" <<endl;
    else
	cout << "I have hard limit " <<limit.rlim_max<<endl;
}

static DODSHeapManager heapmanager;

#ifdef DODS_TEST_MAIN
int main()
{
    cout<<"Entering in main" <<endl;
    void *p=0;
    DODSHeapManager heapmanager;
    heapmanager.show_limits();
    struct mallinfo info;
    info=mallinfo();
    cout << info.arena << endl ;

    try
    {
	p= ::operator new(3*DODS_MEGABYTE);
    }
    catch(bad_alloc::bad_alloc)
    {
	cout << "Got a bad alloc exception" <<endl;
    }
    info = mallinfo();
    cout << info.arena << endl ;
    ::operator delete (p);
    info = mallinfo();
    cout << info.arena << endl;
}
#endif

// $Log: DODSHeapManager.cc,v $
// Revision 1.3  2005/03/23 23:43:08  pwest
// including string.h for Sun machines, okay on Linux and Win32
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
