// BESStopWatch.h

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
//      ndp         Nathan Potter <ndp@opendap.org>
//      pwest       Patrick West  <pwest@ucar.edu>
//      jgarcia     Jose Garcia  <jgarcia@ucar.edu>

#ifndef I_BESStopWatch_h
#define I_BESStopWatch_h 1

#include "sys/time.h"
#include "sys/resource.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESObj.h"

#define TIMING_LOG "timing"
#define MISSING_LOG_PARAM ""

class BESStopWatch;

namespace bes_timing {
extern BESStopWatch *elapsedTimeToReadStart;
extern BESStopWatch *elapsedTimeToTransmitStart;
}

class BESStopWatch : public BESObj
{
 private:
	std::string _timer_name;
	std::string _req_id;
	std::string _log_name;
    bool _started ;
    bool _stopped ;

    struct rusage _start_usage ;
    struct rusage _stop_usage ;
    struct timeval _result ;

    bool timeval_subtract() ;

 public:

    /**
     * Makes a new BESStopWatch with a logName of TIMING_LOG
     */
 BESStopWatch() : _timer_name(MISSING_LOG_PARAM),
	_req_id(MISSING_LOG_PARAM),
	_log_name(TIMING_LOG),
	_started(false),
	_stopped(false)
{ 
}

    /**
     * Makes a new BESStopWatch.
     *
     * @param logName The name of the log to use in the logging output.
     */
    BESStopWatch(std::string logName)  : _timer_name(MISSING_LOG_PARAM),
	_req_id(MISSING_LOG_PARAM),
	_log_name(logName),
	_started(false),
	_stopped(false)
{ 
}

    /**
     * This destructor is "special" in that it's execution signals the
     * timer to stop if it has been started. Stopping the timer will
     * initiate an attempt to write logging information to the
     * BESDebug::GetStrm() stream. If the start method has not been
     * called then the method exits silently.
     */
    virtual ~BESStopWatch();

    /**
     * Starts the timer.
     * NB: This method will attempt to write logging
     * information to the BESDebug::GetStrm() stream.
     * @param name The name of the timer.
     */
    virtual bool		start(std::string name) ;

    /**
     * Starts the timer. NB: This method will attempt to write logging
     * information to the BESDebug::GetStrm() stream. @param name The
     * name of the timer. 
     *
     * @param reqID The client's request ID associated with this
     * activity. Available from the DataHandlerInterfact object.
     */
    virtual bool		start(std::string name, std::string reqID) ;

    virtual void		dump( std::ostream &strm ) const ;
} ;

#endif // I_BESStopWatch_h

