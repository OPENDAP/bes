// BESMemoryGlobalArea.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <errno.h>
#include <iostream>

using std::cerr ;
using std::endl ;

#include "BESMemoryGlobalArea.h"
#include "BESMemoryException.h"
#include "BESLog.h"
#include "TheBESKeys.h"

int BESMemoryGlobalArea::_counter = 0 ;
unsigned long BESMemoryGlobalArea::_size = 0 ;
void* BESMemoryGlobalArea::_buffer = 0 ;

BESMemoryGlobalArea::BESMemoryGlobalArea()
{
    if( _counter++ == 0 )
    {
	try
	{
	    bool found = false ;
	    string key = "OPeNDAP.Memory.GlobalArea." ;
	    string eps = TheBESKeys::TheKeys()->get_key( key + "EmergencyPoolSize", found ) ;
	    string mhs = TheBESKeys::TheKeys()->get_key( key + "MaximunHeapSize", found ) ;
	    string verbose = TheBESKeys::TheKeys()->get_key( key + "Verbose", found ) ;
	    string control_heap = TheBESKeys::TheKeys()->get_key( key + "ControlHeap", found ) ;
	    if( (eps=="") || (mhs=="") || (verbose=="") || (control_heap=="") )
	    {
		(*BESLog::TheLog()) << "OPeNDAP: Unable to start: "
			      << "unable to determine memory keys.\n";
		string line = "can not determine memory keys.\n"  ;
		throw BESMemoryException( line, __FILE__, __LINE__ ) ;
	    }
	    else
	    {
		if( verbose=="no" )
		    BESLog::TheLog()->suspend();

		unsigned int emergency=atol(eps.c_str());

		if( control_heap == "yes" )
		{
		    unsigned int max = atol(mhs.c_str());
		    (*BESLog::TheLog()) << "Trying to initialize emergency size to "
				  << (long int)emergency
				  << " and maximun size to ";
		    (*BESLog::TheLog()) << (long int)(max+1) << " megabytes" << endl ;
		    if( emergency > max )
		    {
			string s = string ( "OPeNDAP: " )
				   + "unable to start since the emergency "
				   + "pool is larger than the maximun size of "
				   + "the heap.\n" ;
			(*BESLog::TheLog()) << s ;
			throw BESMemoryException( s, __FILE__, __LINE__ ) ;
		    }
		    log_limits() ;
		    limit.rlim_cur = megabytes( max + 1 ) ;
		    limit.rlim_max = megabytes( max + 1 ) ;
		    (*BESLog::TheLog()) << "OPeNDAP: Trying limits soft to "
			          << (long int)limit.rlim_cur ;
		    (*BESLog::TheLog()) << " and hard to "
			          << (long int)limit.rlim_max
			          << endl ;
		    if( setrlimit( RLIMIT_DATA, &limit ) < 0 )
		    {
			string s = string( "OPeNDAP: " )
				   + "Could not set limit for the heap "
			           + "because " + strerror(errno) + "\n" ;
			if( errno == EPERM )
			{
			    s = s + "Attempting to increase the soft/hard "
			          + "limit above the current hard limit, "
				  + "must be superuser\n" ;
			}
			(*BESLog::TheLog()) << s ;
			throw BESMemoryException( s, __FILE__, __LINE__ ) ;
		    }
		    log_limits() ;
		    _buffer = 0 ;
		    _buffer = malloc( megabytes( max ) ) ;
		    if( !_buffer )
		    {
			string s = string( "OPeNDAP: " ) 
				   + "can not get heap of size "
				   + mhs + " to start running" ;
			(*BESLog::TheLog()) << s ;
			throw BESMemoryException( s, __FILE__, __LINE__ ) ;
		    }
		    free( _buffer ) ;
		}
		else
		{
		    if( emergency > 10 )
		    {
			string s = "Emergency pool is larger than 10 Megabytes";
			throw BESMemoryException( s, __FILE__, __LINE__ ) ;
		    }
		}

		_size = megabytes( emergency ) ;
		_buffer = 0 ;
		_buffer = malloc( _size ) ;
		if( !_buffer )
		{
		    string s = (string)"BES: can not expand heap to "
		               + eps + " to start running" ;
		    (*BESLog::TheLog()) << s << endl ;
		    throw BESMemoryException( s, __FILE__, __LINE__ ) ;
		}
		else
		{
		    if( BESLog::TheLog()->is_verbose() )
		    {
			(*BESLog::TheLog()) << "OPeNDAP: Memory emergency area "
				      << "initialized with size " 
				      << _size << " megabytes" << endl;
		    }
		}
	    }
	}
	catch(BESException &ex)
	{
	    cerr << "OPeNDAP: unable to start properly because "
		 << ex.get_message()
		 << endl ;
	    exit(1) ;
	}
	catch(...)
	{
	    cerr << "OPeNDAP: unable to start: undefined exception happened\n" ;
	    exit(1) ;
	}
    }
    BESLog::TheLog()->resume();
}

BESMemoryGlobalArea::~BESMemoryGlobalArea()
{
    if (--_counter == 0)
    {
	if (_buffer)
	    free( _buffer ) ;
	_buffer = 0 ;
    }
}

inline void
BESMemoryGlobalArea::log_limits()
{
    if( getrlimit( RLIMIT_DATA, &limit ) < 0 )
    {
	(*BESLog::TheLog()) << "Could not get limits because "
		      << strerror( errno ) << endl ;
	_counter-- ;
	throw BESMemoryException( strerror( errno ), __FILE__, __LINE__ ) ;
    }
    if( limit.rlim_cur == RLIM_INFINITY )
	(*BESLog::TheLog()) << "I have infinity soft limit for the heap" << endl ;
    else
	(*BESLog::TheLog()) << "I have soft limit "
		      << (long int)limit.rlim_cur << endl ;
    if( limit.rlim_max == RLIM_INFINITY )
	(*BESLog::TheLog()) << "I have infinity hard limit for the heap" << endl ;
    else
	(*BESLog::TheLog()) << "I have hard limit "
		      << (long int)limit.rlim_max << endl ;
}

void
BESMemoryGlobalArea::release_memory()
{
    if( _buffer )
    {
	free( _buffer ) ;
	_buffer = 0 ;
    }
}

bool
BESMemoryGlobalArea::reclaim_memory()
{
    if( !_buffer )
	_buffer = malloc( _size ) ;
    if( _buffer )
	return true ;
    else
	return false ;
}

