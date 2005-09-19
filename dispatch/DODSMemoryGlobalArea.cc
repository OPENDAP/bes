// DODSMemoryGlobalArea.cc

// 2004 Copyright University Corporation for Atmospheric Research

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
	    string s = TheDODSKeys::TheKeys()->get_key( "DODS.Memory.GlobalArea.EmergencyPoolSize", found ) ;
	    string s1 = TheDODSKeys::TheKeys()->get_key( "DODS.Memory.GlobalArea.MaximunHeapSize", found ) ;
	    string verbose = TheDODSKeys::TheKeys()->get_key( "DODS.Memory.GlobalArea.Verbose", found ) ;
	    string control_heap = TheDODSKeys::TheKeys()->get_key( "DODS.Memory.GlobalArea.ControlHeap", found ) ;
	    if( (s=="") || (s1=="") || (verbose=="") || (control_heap=="") )
	    {
		(*DODSLog::TheLog()) << "DODS: Unable to start: "
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
			string s = string ( "DODS: " )
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
		    (*DODSLog::TheLog()) << "DODS: Trying limits soft to "
			          << (long int)limit.rlim_cur ;
		    (*DODSLog::TheLog()) << " and hard to "
			          << (long int)limit.rlim_max
			          << endl ;
		    if( setrlimit( RLIMIT_DATA, &limit ) < 0 )
		    {
			string s = string( "DODS: " )
				   + "Could not set limit for the heap "
			           + "because " + strerror(errno) + "\n" ;
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
			string s = string( "DODS: " ) 
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
		    (*DODSLog::TheLog()) << "DODS: can not expand heap enough to start running.\n";
		    DODSMemoryException me ;
		    me.set_error_description( "Can no allocated memory\n" ) ;
		    me.set_amount_of_memory_required( atoi( s.c_str() ) ) ;
		    throw me ;
		}
		else
		{
		    if( DODSLog::TheLog()->is_verbose() )
		    {
			(*DODSLog::TheLog()) << "DODS: Memory emergency area "
				      << "initialized with size " 
				      << _size << " megabytes" << endl;
		    }
		}
	    }
	}
	catch(DODSException &ex)
	{
	    cerr << "DODS: unable to start properly because "
		 << ex.get_error_description()
		 << endl ;
	    exit(1) ;
	}
	catch(...)
	{
	    cerr << "DODS: unable to start: undefined exception happened\n" ;
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
