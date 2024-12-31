// BESMemoryManager.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <iostream>

#if 0
using std::endl;
using std::set_new_handler;
#endif


#include "BESMemoryManager.h"

#include "BESLog.h"
#include "BESDebug.h"
#include "BESMemoryGlobalArea.h"
#include "BESError.h"

using namespace std;

BESMemoryGlobalArea* BESMemoryManager::_memory;
bool BESMemoryManager::_storage_used(false);
new_handler BESMemoryManager::_global_handler;

BESMemoryGlobalArea *
BESMemoryManager::initialize_memory_pool()
{
    static BESMemoryGlobalArea mem ;
    _memory = &mem ;
    return _memory ;
}

void
BESMemoryManager::register_global_pool() 
{
    _global_handler = set_new_handler( BESMemoryManager::swap_memory ) ;
}

void
BESMemoryManager::swap_memory()
{
    INFO_LOG("BESMemoryManager::This is just a simulation, here we tell BES to go to persistence state.");
    set_new_handler( BESMemoryManager::release_global_pool ) ;
}

bool
BESMemoryManager::unregister_global_pool() 
{
    if( check_memory_pool() )
    {
	set_new_handler( _global_handler ) ;
	return true ;
    } else {
	return false ;
    }
}

bool
BESMemoryManager::check_memory_pool()
{ 
    if( _storage_used )
    {
	BESDEBUG( "bes", "BES: global pool is used, trying to get it back..." << endl ) ;
	//Try to regain the memory...
	if( _memory->reclaim_memory() )
	{
	    _storage_used = false ;
	    BESDEBUG( "bes", "OK" << endl ) ;
	    return true ;
	}
	else
	{
	    BESDEBUG( "bes", "FAILED" << endl ) ;
	    return false ;
	}
    }
    return true ;
}
    
void
BESMemoryManager::release_global_pool() throw (bad_alloc)
{
    try {
        // This is really the final resource for BES since therefore
        // this method must be second level handler.
        // It releases enough memory for an exception sequence to be carried.
        // Without this pool of memory for emergencies we will get really
        // unexpected behavior from the program.
        BESDEBUG("bes", "BES Warning: low in memory, " << "releasing global memory pool!" << endl);

        INFO_LOG("BES Warning: low in memory, releasing global memory pool!\n");
    }
    catch (BESError &e) {
        // At this point, exceptions are pretty moot.
        cerr << "Exception while trying to release the global memory pool. (" << e.get_verbose_message() << ").";
    }
    catch (...) {
        cerr << "Exception while trying to release the global memory pool.";
    }

    _storage_used = true ;
    _memory->release_memory() ;

    // Do not let the caller of this memory consume the global pool for
    // normal stuff this is an emergency.
    set_new_handler( 0 ) ;
    throw bad_alloc() ;
} 

