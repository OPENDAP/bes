// BESStopWatch.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cerrno>
#include <string>
#include <iostream>
#include <cstring>

using std::string ;
using std::endl ;

#include "BESStopWatch.h"
#include "BESDebug.h"

bool
BESStopWatch::start()
{
    // get timer for current usage
    if( getrusage( RUSAGE_SELF, &_start_usage ) != 0 )
    {
	int myerrno = errno ;
	char *c_err = strerror( myerrno ) ;
	string err = "getrusage failed in start: " ;
	if( c_err )
	{
	    err += c_err ;
	}
	else
	{
	    err += "unknown error" ;
	}
	BESDEBUG( "timing", err ) ;
	_started = false ;
    }
    else
    {
	_started = true ;
    }

    // either we started the stop watch, or failed to start it. Either way,
    // no timings are available, so set stopped to false.
    _stopped = false ;

    return _started ;
}

bool
BESStopWatch::stop()
{
    // if we have started, the we can stop. Otherwise just fall through.
    if( _started )
    {
	// get timer for current usage
	if( getrusage( RUSAGE_SELF, &_stop_usage ) != 0 )
	{
	    int myerrno = errno ;
	    char *c_err = strerror( myerrno ) ;
	    string err = "getrusage failed in stop: " ;
	    if( c_err )
	    {
		err += c_err ;
	    }
	    else
	    {
		err += "unknown error" ;
	    }
	    BESDEBUG( "timing", err ) ;
	    _started = false ;
	    _stopped = false ;
	}
	else
	{
	    // get the difference between the _start_usage and the
	    // _stop_usage and save the difference.
	    bool success = timeval_subtract() ;
	    if( !success )
	    {
		BESDEBUG( "timing", "failed to get timing" ) ;
		_started = false ;
		_stopped = false ;
	    }
	    else
	    {
		_stopped = true ;
	    }
	}
    }
    
    return _stopped ;
}

bool
BESStopWatch::timeval_subtract()
{
    struct timeval &start = _start_usage.ru_utime ;
    struct timeval &stop = _stop_usage.ru_utime ;

    /* Perform the carry for the later subtraction by updating y. */
    if( stop.tv_usec < start.tv_usec )
    {
	int nsec = (start.tv_usec - stop.tv_usec) / 1000000 + 1 ;
	start.tv_usec -= 1000000 * nsec ;
	start.tv_sec += nsec ;
    }
    if( stop.tv_usec - start.tv_usec > 1000000 )
    {
	int nsec = (start.tv_usec - stop.tv_usec) / 1000000 ;
	start.tv_usec += 1000000 * nsec ;
	start.tv_sec -= nsec ;
    }

    /* Compute the time remaining to wait.
    tv_usec  is certainly positive. */
    _result.tv_sec = stop.tv_sec - start.tv_sec ;
    _result.tv_usec = stop.tv_usec - start.tv_usec ;

    /* Return 1 if result is negative. */
    return !(stop.tv_sec < start.tv_sec) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESStopWatch::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESStopWatch::dump - ("
			     << (void *)this << ")" << endl ;
}

