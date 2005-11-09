// DODSMemoryManager.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <unistd.h>
#include <iostream>

using std::endl ;
using std::set_new_handler ;

#include "DODSMemoryManager.h"

#include "DODSLog.h"
#include "DODSMemoryGlobalArea.h"

DODSMemoryGlobalArea* DODSMemoryManager::_memory;
bool DODSMemoryManager::_storage_used(false);
new_handler DODSMemoryManager::_global_handler;

DODSMemoryGlobalArea *
DODSMemoryManager::initialize_memory_pool()
{
    static DODSMemoryGlobalArea mem ;
    _memory = &mem ;
    return _memory ;
}

void
DODSMemoryManager::register_global_pool() 
{
    _global_handler = set_new_handler( DODSMemoryManager::swap_memory ) ;
}

void
DODSMemoryManager::swap_memory()
{
    *(DODSLog::TheLog()) << "DODSMemoryManager::This is just a simulation, here we tell DODS to go to persistence state" << endl;
    set_new_handler( DODSMemoryManager::release_global_pool ) ;
}

bool
DODSMemoryManager::unregister_global_pool() 
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
DODSMemoryManager::check_memory_pool()
{ 
    if( _storage_used )
    {
	*(DODSLog::TheLog()) << "DODS: global pool is used, trying to get it back...";
	//Try to regain the memory...
	if( _memory->reclaim_memory() )
	{
	    _storage_used = false ;
	    *(DODSLog::TheLog()) << "got it!" << endl ;
	    return true ;
	}
	else
	{
	    *(DODSLog::TheLog()) << "can not get it!" << endl;
	    *(DODSLog::TheLog()) << "DODS: Unable to continue: "
			         << "no emergency memory pool"
			         << endl;
	    return false ;
	}
    }
    return true ;
}
    
void
DODSMemoryManager::release_global_pool() throw (bad_alloc)
{
    // This is really the final resource for DODS since therefore 
    // this method must be second level handler.
    // It releases enough memory for an exception sequence to be carried.
    // Without this pool of memory for emergencies we will get really
    // unexpected behavior from the program.
    *(DODSLog::TheLog()) << "DODS Warning: low in memory, "
                         << "releasing global memory pool!"
		         << endl;
    _storage_used = true ;
    _memory->release_memory() ;

    // Do not let the caller of this memory consume the global pool for
    // normal stuff this is an emergency.
    set_new_handler( 0 ) ;
    throw bad_alloc() ;
} 

// $Log: DODSMemoryManager.cc,v $
