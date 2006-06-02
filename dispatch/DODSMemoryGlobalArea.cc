// DODSMemoryGlobalArea.cc

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

#include "DODSMemoryGlobalArea.h"
#include "DODSMemoryException.h"
#include "DODSLog.h"
#include "TheDODSKeys.h"

int DODSMemoryGlobalArea::_counter = 0 ;
unsigned long DODSMemoryGlobalArea::_size = 0 ;
void* DODSMemoryGlobalArea::_buffer = 0 ;

DODSMemoryGlobalArea::DODSMemoryGlobalArea()
{
    if( _counter++ == 0 )
    {
	try
	{
	    bool found = false ;
	    string key = "OPeNDAP.Memory.GlobalArea." ;
	    string s = TheDODSKeys::TheKeys()->get_key( key + "EmergencyPoolSize", found ) ;
	    string s1 = TheDODSKeys::TheKeys()->get_key( key + "MaximunHeapSize", found ) ;
	    string verbose = TheDODSKeys::TheKeys()->get_key( key + "Verbose", found ) ;
	    string control_heap = TheDODSKeys::TheKeys()->get_key( key + "ControlHeap", found ) ;
	    if( (s=="") || (s1=="") || (verbose=="") || (control_heap=="") )
	    {
		(*DODSLog::TheLog()) << "OPeNDAP: Unable to start: "
			      << "unable to determine memory keys.\n";
		DODSMemoryException me;
		me.set_error_description( "can not determine memory keys.\n" ) ;
		throw me;
	    }
	    else
	    {
		if( verbose=="no" )
		    DODSLog::TheLog()->suspend();

		unsigned int emergency=atol(s.c_str());

		if( control_heap == "yes" )
		{
		    unsigned int max = atol(s1.c_str());
		    (*DODSLog::TheLog()) << "Trying to initialize emergency size to "
				  << (long int)emergency
				  << " and maximun size to ";
		    (*DODSLog::TheLog()) << (long int)(max+1) << " megabytes" << endl ;
		    if( emergency > max )
		    {
			string s = string ( "OPeNDAP: " )
				   + "unable to start since the emergency "
				   + "pool is larger than the maximun size of "
				   + "the heap.\n" ;
			(*DODSLog::TheLog()) << s ;
			DODSMemoryException me;
			me.set_error_description( s );
			throw me;
		    }
		    log_limits() ;
		    limit.rlim_cur = megabytes( max + 1 ) ;
		    limit.rlim_max = megabytes( max + 1 ) ;
		    (*DODSLog::TheLog()) << "OPeNDAP: Trying limits soft to "
			          << (long int)limit.rlim_cur ;
		    (*DODSLog::TheLog()) << " and hard to "
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
			(*DODSLog::TheLog()) << s ;
			DODSMemoryException me;
			me.set_error_description( s ) ;
			throw me;
		    }
		    log_limits() ;
		    _buffer = 0 ;
		    _buffer = malloc( megabytes( max ) ) ;
		    if( !_buffer )
		    {
			string s = string( "OPeNDAP: " ) 
				   + "can not get heap large enough to "
				   + "start running\n" ;
			(*DODSLog::TheLog()) << s ;
			DODSMemoryException me;
			me.set_error_description( s );
			me.set_amount_of_memory_required ( atoi( s.c_str() ) ) ;
			throw me ;
		    }
		    free( _buffer ) ;
		}
		else
		{
		    if( emergency > 10 )
		    {
			DODSMemoryException me ;
			me.set_error_description( "Emergency pool is larger than 10 Megabytes\n" ) ;
			throw me ;
		    }
		}

		_size = megabytes( emergency ) ;
		_buffer = 0 ;
		_buffer = malloc( _size ) ;
		if( !_buffer )
		{
		    (*DODSLog::TheLog()) << "OPeNDAP: can not expand heap enough to start running.\n";
		    DODSMemoryException me ;
		    me.set_error_description( "Can not allocate memory\n" ) ;
		    me.set_amount_of_memory_required( atoi( s.c_str() ) ) ;
		    throw me ;
		}
		else
		{
		    if( DODSLog::TheLog()->is_verbose() )
		    {
			(*DODSLog::TheLog()) << "OPeNDAP: Memory emergency area "
				      << "initialized with size " 
				      << _size << " megabytes" << endl;
		    }
		}
	    }
	}
	catch(DODSException &ex)
	{
	    cerr << "OPeNDAP: unable to start properly because "
		 << ex.get_error_description()
		 << endl ;
	    exit(1) ;
	}
	catch(...)
	{
	    cerr << "OPeNDAP: unable to start: undefined exception happened\n" ;
	    exit(1) ;
	}
    }
    DODSLog::TheLog()->resume();
}

DODSMemoryGlobalArea::~DODSMemoryGlobalArea()
{
    if (--_counter == 0)
    {
	if (_buffer)
	    free( _buffer ) ;
	_buffer = 0 ;
    }
}

inline void
DODSMemoryGlobalArea::log_limits()
{
    if( getrlimit( RLIMIT_DATA, &limit ) < 0 )
    {
	(*DODSLog::TheLog()) << "Could not get limits because "
		      << strerror( errno ) << endl ;
	DODSMemoryException me ;
	me.set_error_description( strerror( errno ) ) ;
	_counter-- ;
	throw me ;
    }
    if( limit.rlim_cur == RLIM_INFINITY )
	(*DODSLog::TheLog()) << "I have infinity soft limit for the heap" << endl ;
    else
	(*DODSLog::TheLog()) << "I have soft limit "
		      << (long int)limit.rlim_cur << endl ;
    if( limit.rlim_max == RLIM_INFINITY )
	(*DODSLog::TheLog()) << "I have infinity hard limit for the heap" << endl ;
    else
	(*DODSLog::TheLog()) << "I have hard limit "
		      << (long int)limit.rlim_max << endl ;
}

void
DODSMemoryGlobalArea::release_memory()
{
    if( _buffer )
    {
	free( _buffer ) ;
	_buffer = 0 ;
    }
}

bool
DODSMemoryGlobalArea::reclaim_memory()
{
    if( !_buffer )
	_buffer = malloc( _size ) ;
    if( _buffer )
	return true ;
    else
	return false ;
}

//static DODSMemoryGlobalArea memoryglobalarea;

// $Log: DODSMemoryGlobalArea.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
