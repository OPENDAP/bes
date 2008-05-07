#ifndef I_BESStopWatch_h
#define I_BESStopWatch_h 1

#include "sys/time.h"

#include "BESObj.h"

class BESStopWatch : public BESObj
{
private:
    struct rusage		_start_usage ;
    struct rusage		_stop_usage ;
    struct timeval		_result ;
    bool			_started ;
    bool			_stopped ;
    bool			timeval_subtract() ;
public:
				BESStopWatch() {}
    virtual			~BESStopWatch() {}
    virtual bool		start() ;
    virtual bool		stop() ;
    virtual int			seconds()
				{
				    if( _stopped ) return _result.tv_sec ;
				    else return 0 ;
				}
    virtual int			microseconds()
				{
				    if( _stopped ) return _result.tv_usec ;
				    else return 0 ;
				}

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // I_BESStopWatch_h

