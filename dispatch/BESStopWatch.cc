#include <cerrno>
#include <string>
#include <iostream>

using std::string ;
using std::cerr ;
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
	BESDEBUG( "timing", err )
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
	    BESDEBUG( "timing", err )
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
		BESDEBUG( "timing", "failed to get timing" )
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

