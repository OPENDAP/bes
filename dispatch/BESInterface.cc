// BESInterface.cc

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

#include <cstdlib>

#include <signal.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <setjmp.h> // Used for the timeout processing

#include <string>
#include <sstream>
#include <iostream>

#include "BESInterface.h"

#include "TheBESKeys.h"
#include "BESResponseHandler.h"
#include "BESAggFactory.h"
#include "BESAggregationServer.h"
#include "BESReporterList.h"
#include "BESContextManager.h"

#include "BESExceptionManager.h"

#include "BESDapError.h"

#include "BESTransmitterNames.h"
#include "BESDataNames.h"
#include "BESTransmitterNames.h"
#include "BESReturnManager.h"
#include "BESSyntaxUserError.h"

#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESTimeoutError.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"

#include "BESLog.h"

using namespace std;

static jmp_buf timeout_jump;
static bool timeout_jump_valid = false;

// Define this to use sigwait() in a child thread to detect that SIGALRM
// has been raised (i.e., that the timeout interval has elapsed). This
// does not currently work, but could be a way to get information about
// a timeout back to the BES's client if the BES itslef were structured
// differently. See my comment further down. jhrg 12/28/15
#undef USE_SIGWAIT

// timeout period in seconds; 0 --> no timeout. This is a static value so
// that it can be accessed by the signal handler. jhrg 1/4/16
// I've made this globally visible so that other code that might want to
// alter the time out value can do so and this variable can be kept consistent.
// See BESStreamResponseHandler::execute() for an example. jhrg 1/24/17
volatile int bes_timeout = 0;

#define BES_TIMEOUT_KEY "BES.TimeOutInSeconds"

// This function uses the static variables timeout_jump_valid and timeout_jump
// The code looks at the value of BES.TimeOutInSeconds and/or the timeout
// context sent in the current request and, if that is greater than zero,
// uses that as the maximum amount of time for the request. The system alarm
// is set and this function is registered as the handler. If timeout_jump_valid
// is true, then it will use longjmp() (yes, really...) to end the request. Look
// below in execute_request() for the call to setjump() to see how this works.
// See the SIGWAIT code that's commented out below for an alternative impl.
// jhrg 5/31/16
static void catch_sig_alarm(int sig)
{
    if (sig == SIGALRM) {
        LOG("BES timeout after " << bes_timeout << " seconds." << endl);

        // Causes setjmp() below to return 1; see the call to
        // execute_data_request_plan() in execute_request() below.
        // jhrg 12/29/15
        if (timeout_jump_valid)
            longjmp(timeout_jump, 1);
        else {
            // This is the old version of this code; it forces the BES child
            // listener to exit without returning an error message to the
            // OLFS/client. jhrg 12/29/15
            signal(SIGTERM, SIG_DFL);
            raise(SIGTERM);
        }
    }
}

static void register_signal_handler()
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);
    act.sa_flags = 0;

    // Note that we do not set SA_RESTART so an interrupted system call
    // will return with an error and errno set to EINTR.

    act.sa_handler = catch_sig_alarm;
    if (sigaction(SIGALRM, &act, 0))
        throw BESInternalFatalError("Could not register a handler to catch alarm/timeout.", __FILE__, __LINE__);
}

#if USE_SIGWAIT

// If the BES is changed so that the plan built here is run in a child thread,
// then we can have a much more flexible signal catching scheme, including catching
// the alarm signal used for the timeout. It's not possible to throw from a child
// thread to a parent thread, but if the parent thread sees that SIGALRM is
// raised, then it can stop the child thread (which is running the 'plan') and
// return a suitable message to the front end. Similarly, the BES could also
// handle a number of other signals using this scheme. These signals (SIGPIPE, ...)
// are currently processed using while/for loop(s) in the bes/server code. It may
// be that these signals are caught only in the master listener, but I can't
// quite figure that out now... jhrg 12/28/15
//
// NB: It might be possible to edit this so that it writes info to the OLFS and
// then uses the 'raise SIGTERM' technique to exit. That way the OLFS will at least
// get a message about the timeout. I'm not sure how to close up the PPT part
// of the conversation, however. The idea would be that the current command's DHI
// would be passed in as an arg and then the stream accessed that way. The BESError
// would be written to the stream and the child process killed. jhrg 12/2/9/15

#include <pthread.h>

// An alternative to a function that catches the signal; use sigwait()
// in a child thread after marking the signal as blocked. When/if sigwait()
// returns, look at the signal number and if it is the alarm, sort out
// what to do (throw an exception, ...). NB: A signal handler cannot
// portably throw an exception, but this code can.

static pthread_t alarm_thread;

static void* alarm_wait(void * /* arg */)
{
    BESDEBUG("bes", "Starting: " << __PRETTY_FUNCTION__ << endl);

    // block SIGALRM
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // Might replace this with a while loop. Not sure about interactions
    // with other signal processing code in the BES. jhrg 12/28/15
    int sig;
    int result = sigwait(&sigset, &sig);
    if (result != 0) {
        BESDEBUG("bes", "Fatal error establishing timeout: " << strerror(result) << endl);
        throw BESInternalFatalError(string("Fatal error establishing timeout: ") + strerror(result), __FILE__, __LINE__);
    }
    else if (result == 0 && sig == SIGALRM) {
        BESDEBUG("bes", "Timeout found in " << __PRETTY_FUNCTION__ << endl);
        throw BESTimeoutError("Timeout", __FILE__, __LINE__);
    }
    else {
        stringstream oss;
        oss << "While waiting for a timeout, found signal '" << result << "' in " << __PRETTY_FUNCTION__ << ends;
        BESDEBUG("bes", oss.str() << endl);
        throw BESInternalFatalError(oss.str(), __FILE__, __LINE__);
    }
}

static void wait_for_timeout()
{
    BESDEBUG("bes", "Entering: " << __PRETTY_FUNCTION__ << endl);

    pthread_attr_t thread_attr;

    if (pthread_attr_init(&thread_attr) != 0)
    throw BESInternalFatalError("Failed to initialize pthread attributes.", __FILE__, __LINE__);
    if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED /*PTHREAD_CREATE_JOINABLE*/) != 0)
    throw BESInternalFatalError("Failed to complete pthread attribute initialization.", __FILE__, __LINE__);

    int status = pthread_create(&alarm_thread, &thread_attr, alarm_wait, NULL);
    if (status != 0)
    throw BESInternalFatalError("Failed to start the timeout wait thread.", __FILE__, __LINE__);
}
#endif

BESInterface::BESInterface(ostream *output_stream) :
    d_strm(output_stream), d_timeout_from_keys(0), d_dhi_ptr(0), d_transmitter(0)
{
    if (!d_strm) {
        throw BESInternalError("Output stream must be set in order to output responses", __FILE__, __LINE__);
    }

    // Grab the BES Key for the timeout. Note that the Hyrax server generally
    // overrides this value using a 'context' that is set/sent by the OLFS.
    // Also note that a value of zero means no timeout, but that the context
    // can override that too. jhrg 1/4/16
    bool found;
    string timeout_key_value;
    TheBESKeys::TheKeys()->get_value(BES_TIMEOUT_KEY, timeout_key_value, found);
    if (found) {
        istringstream iss(timeout_key_value);
        iss >> d_timeout_from_keys;
    }

    // Install signal handler for alarm() here
    register_signal_handler();

#if USE_SIGWAIT
    wait_for_timeout();
#endif
}

#if 0
extern BESStopWatch *bes_timing::elapsedTimeToReadStart;
extern BESStopWatch *bes_timing::elapsedTimeToTransmitStart;
#endif

/** @brief The entry point for command execution; called by BESServerHandler::execute()

 Execute the request by:
 1. initializing BES

 3. build the request plan (i.e., filling in the BESDataHandlerInterface)
 4. execute the request plan using the BESDataHandlerInterface
 5. transmit the resulting response object
 6. log the status of the execution

 8. end the request, which allows developers to add callbacks to notify
 them of the end of the request

 If an exception is thrown in any of these steps the exception is handed
 over to the exception manager in order to generate the proper response.
 Control is returned back to the calling method if an exception is thrown
 and it is the responsibility of the calling method to call finish_with_error
 in order to transmit the error message back to the client.

 @param from A string that tells where this request came from. Literally,
 the IP and port number or the string 'standalone'.
 See void BESServerHandler::execute(Connection *c) or
 void StandAloneClient::executeCommand(const string & cmd, int repeat)

 @return status of the execution of the request, 0 if okay, !0 otherwise

 @see initialize()

 @see build_data_request_plan()
 @see execute_data_request_plan()
 @see finish()
 @see finish_with_error()
 @see transmit_data()
 @see log_status()

 @see end_request()
 @see exception_manager()
 */
int BESInterface::execute_request(const string &from)
{
    BESDEBUG("bes", "Entering: " << __PRETTY_FUNCTION__ << endl);

    if (!d_dhi_ptr) {
        throw BESInternalError("DataHandlerInterface can not be null", __FILE__, __LINE__);
    }

    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) {
        // It would be great to have more info to put here, but that is buried in
        // BESXMLInterface::build_data_request_plan() where the XML document is
        // parsed. jhrg 11/9/17
        sw.start("BESInterface::execute_request", d_dhi_ptr->data[REQUEST_ID]);
#if 0
        bes_timing::elapsedTimeToReadStart = new BESStopWatch();
        bes_timing::elapsedTimeToReadStart->start("TIME_TO_READ_START", d_dhi_ptr->data[REQUEST_ID]);

        bes_timing::elapsedTimeToTransmitStart = new BESStopWatch();
        bes_timing::elapsedTimeToTransmitStart->start("TIME_TO_TRANSMIT_START", d_dhi_ptr->data[REQUEST_ID]);
#endif
    }

    // TODO These never change for the life of a BES, so maybe they can move out of
    // code that runs for every request? jhrg 11/8/17
    d_dhi_ptr->set_output_stream(d_strm);
    d_dhi_ptr->data[REQUEST_FROM] = from;

    // TODO If this is only used for logging, it is not needed since the log has a copy
    // of the BES PID. jhrg 11/13/17
    ostringstream ss;
    ss << getpid();
    d_dhi_ptr->data[SERVER_PID] = ss.str();

    // We split up the calls for the reason that if we catch an
    // exception during the initialization, building, execution, or response
    // transmit of the request then we can transmit the exception/error
    // information.
    //
    // TODO status is not used. jhrg 11/9/17
    int status = 0; // save the return status from exception_manager() and return that.
    try {
        VERBOSE(d_dhi_ptr->data[REQUEST_FROM] << " request received" << endl);

        // Initialize the transmitter for this interface instance to the BASIC
        // TRANSMITTER. This ensures that a simple response, such as an error,
        // can be sent back to the OLFS should that be needed.
        d_transmitter = BESReturnManager::TheManager()->find_transmitter(BASIC_TRANSMITTER);
        if (!d_transmitter)
            throw BESInternalError(string("Unable to find transmitter '") + BASIC_TRANSMITTER + "'", __FILE__, __LINE__);

        build_data_request_plan();

        // This method does two key things: Calls the request handler to make a
        // 'response object' (the C++ object that will hold the response) and
        // then calls the transmitter to actually send it or build and send it.
        //
        // The timeout is also set in execute_data_request_plan(). The alarm signal
        // handler (above), run when the timeout expires, will call longjmp with a
        // return value of 1.
        if (setjmp(timeout_jump) == 0) {
            timeout_jump_valid = true;

            // Set timeout? Use either the value from the keys or a context
            bool found = false;
            string context = BESContextManager::TheManager()->get_context("bes_timeout", found);
            if (found) {
                bes_timeout = strtol(context.c_str(), NULL, 10);
                VERBOSE(d_dhi_ptr->data[REQUEST_FROM] << "Set request timeout to " << bes_timeout << " seconds (from context)." << endl);
                alarm(bes_timeout);
            }
            else if (d_timeout_from_keys != 0) {
                bes_timeout = d_timeout_from_keys;
                VERBOSE(d_dhi_ptr->data[REQUEST_FROM] << "Set request timeout to " << bes_timeout << " seconds (from keys)." << endl);
                alarm(bes_timeout);
            }

            execute_data_request_plan();

            // Only clear the timeout if it has been set.
            if (bes_timeout != 0) {
                bes_timeout = 0;
                alarm(0);
            }

            // Once we exit the block where setjmp() was called, the jump_buf is not valid
            timeout_jump_valid = false;
        }
        else {
            ostringstream oss;
            oss << "BES listener timeout after " << bes_timeout << " seconds." << ends;
            throw BESTimeoutError(oss.str(), __FILE__, __LINE__);
        }

        d_dhi_ptr->executed = true;
    }
    catch (BESError & ex) {
        timeout_jump_valid = false;
        status = BESDapError::TheDapHandler()->handleBESError(ex, *d_dhi_ptr);
    }
    catch (bad_alloc &e) {
        timeout_jump_valid = false;
        BESInternalFatalError ex(string("BES out of memory: ") + e.what(), __FILE__, __LINE__);
        status = BESDapError::handleException(ex, *d_dhi_ptr);
    }
    catch (exception &e) {
        timeout_jump_valid = false;
        BESInternalFatalError ex(string("C++ Exception: ") + e.what(), __FILE__, __LINE__);
        status = BESDapError::handleException(ex, *d_dhi_ptr);
    }
    catch (...) {
        timeout_jump_valid = false;
        BESInternalError ex("An undefined exception has been thrown", __FILE__, __LINE__);
        status = BESDapError::handleException(ex, *d_dhi_ptr);
    }

#if 0
    delete bes_timing::elapsedTimeToReadStart;
    bes_timing::elapsedTimeToReadStart = 0;

    delete bes_timing::elapsedTimeToTransmitStart;
    bes_timing::elapsedTimeToTransmitStart = 0;

#endif

    return status;
}

/**
 * Call this once execute_request() has completed.

 * @param status
 * @return The status value.
 */
int BESInterface::finish(int status)
{
#if 0
    if (status != 0 && d_dhi_ptr->error_info == 0) {
        // there wasn't an error ... so now what?
        BESInternalError ex("Finish_with_error called with no error object", __FILE__, __LINE__);
        status = exception_manager(ex);
    }
#endif

    if (d_dhi_ptr->error_info) {
        d_dhi_ptr->error_info->print(*d_strm /*cout*/);
        delete d_dhi_ptr->error_info;
        d_dhi_ptr->error_info = 0;
    }

    // if there is a problem with the rest of these steps then all we will
    // do is log it to the BES log file and not handle the exception with
    // the exception manager.
    try {
        log_status();
        end_request();
    }
    catch (BESError &ex) {
        LOG("Problem logging status or running end of request cleanup: " << ex.get_message() << endl);
    }
    catch (...) {
        LOG("Unknown problem logging status or running end of request cleanup" << endl);
    }

    return status;
}

/** @brief End the BES request
 *
 *  This method allows developers to add callbacks at the end of a request,
 *  to do any cleanup or do any extra work at the end of a request
 */
void BESInterface::end_request()
{
    // now clean up any containers that were used in the request, release
    // the resource
    d_dhi_ptr->first_container();
    while (d_dhi_ptr->container) {
        d_dhi_ptr->container->release();
        d_dhi_ptr->next_container();
    }
}

/** @brief Manage any exceptions thrown during the whole process

 Specific responses are generated given a specific Exception caught. If
 additional exceptions are thrown within derived systems then implement
 those in the derived exception_manager methods. This is a catch-all
 manager and should be called once derived methods have caught their
 exceptions.

 @param e BESError to be managed
 @return status after exception is handled
 @see BESError
 */
#if 0
int BESInterface::exception_manager(BESError &e)
{
    return BESExceptionManager::TheEHM()->handle_exception(e, *d_dhi_ptr);
}
#endif

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * BESDataHandlerInterface, the BESTransmitter being used, and the number of
 * initialization and termination callbacks.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESInterface::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "BESInterface::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "data handler interface:" << endl;
    BESIndent::Indent();
    d_dhi_ptr->dump(strm);
    BESIndent::UnIndent();

    if (d_transmitter) {
        strm << BESIndent::LMarg << "transmitter:" << endl;
        BESIndent::Indent();
        d_transmitter->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "transmitter: not set" << endl;
    }

    BESIndent::UnIndent();
}
