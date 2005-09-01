// DODSMemoryManager.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSMemoryManager_h_
#define DODSMemoryManager_h_ 1

#include <new>

using std::new_handler ;
using std::bad_alloc ;

class DODSMemoryGlobalArea ;

class DODSMemoryManager
{
private:
    static DODSMemoryGlobalArea *_memory ;
    static bool			_storage_used ;
    static new_handler		_global_handler ;
public:
    static DODSMemoryGlobalArea* initialize_memory_pool() ;
    static void			swap_memory() ;
    static void			release_global_pool()throw (bad_alloc) ;
    static void			register_global_pool() ;
    static bool			unregister_global_pool() ;
    static bool			check_memory_pool() ;
} ;

#endif // DODSMemoryManager_h_

// $Log: DODSMemoryManager.h,v $
