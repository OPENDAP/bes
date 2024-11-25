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

#include <sys/time.h>
#include <sys/resource.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESObj.h"

#define COMMAND_TIMING 1

static const std::string TIMING_LOG_KEY = "timing";
static const std::string MISSING_LOG_PARAM;

// This macro is used to start a timer if any of the BESDebug flags it tests are set.
// The advantage of this macro is that it drops to zero code when NDEBUG is defined.
// Our code often uses a macro that leaves the BESStopWatch object definition in the
// code when NDEBUG is defined. This is not a problem, but it does make the code a bit
// slower. jhrg 5/17/24
#ifndef NDEBUG
#define BES_STOPWATCH_START(module, x) \
do { \
BESStopWatch besTimer; \
if (BESISDEBUG((module)) || BESISDEBUG(TIMING_LOG_KEY) || BESLog::TheLog()->is_verbose()) \
    besTimer.start((x)); \
} while(false)
#else
#define BES_STOPWATCH_START(module, x)
#endif

// This macro is used specifically to time the execution of a command. It does not depend
// on the code being built in developer mode. jhrg 11/24/24
// To get the timing messages written to the log, 'timing' must be set in the bes debug keys.
// jhrg 11/24/24
#ifdef COMMAND_TIMING
#define BES_COMMAND_TIMING(message) \
do { \
BESStopWatch besTimer; \
besTimer.start((message)); \
} while(false)
#else
#define BES_COMMAND_TIMING(message)
#endif

class BESStopWatch;

namespace bes_timing {
extern BESStopWatch *elapsedTimeToReadStart;
extern BESStopWatch *elapsedTimeToTransmitStart;
}

class BESStopWatch : public BESObj {
 private:
	std::string d_timer_name;
	std::string d_req_id;
	std::string d_log_name = TIMING_LOG_KEY;
    bool d_started = false;
    bool d_stopped = false;

	struct timeval d_start_usage{};
	struct timeval d_stop_usage{};

    unsigned long int get_elapsed_us() const;
    unsigned long int get_start_us() const;
    unsigned long int get_stop_us() const;
    bool get_time_of_day(struct timeval &time_val) const;

 public:

    /// Makes a new BESStopWatch with a logName of TIMING_LOG_KEY
    BESStopWatch() = default;
    BESStopWatch(const BESStopWatch &copy_from) = default;
    BESStopWatch &operator=(const BESStopWatch &copy_from) = default;

    /**
     * Makes a new BESStopWatch.
     *
     * @param logName The name of the log to use in the logging output.
     */
    explicit BESStopWatch(const std::string &logName)  : d_log_name(logName) { }

    /**
     * This destructor is "special" in that it's execution signals the
     * timer to stop if it has been started. Stopping the timer will
     * initiate an attempt to write logging information to the
     * BESDebug::GetStrm() stream. If the start method has not been
     * called then the method exits silently.
     */
    ~BESStopWatch() override;

    /**
     * Starts the timer.
     * NB: This method will attempt to write logging
     * information to the BESDebug::GetStrm() stream.
     * @param name The name of the timer.
     */
    virtual bool start(const std::string &name);

    /**
     * Starts the timer. NB: This method will attempt to write logging
     * information to the BESDebug::GetStrm() stream. @param name The
     * name of the timer. 
     *
     * @param reqID The client's request ID associated with this
     * activity. Available from the DataHandlerInterface object.
     */
    virtual bool start(const std::string &name, const std::string &reqID);

    void dump(std::ostream &strm) const override;
};

#endif // I_BESStopWatch_h

