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

#include "BESDataNames.h"

#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESTimeoutError.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"

#include "BESLog.h"

using namespace std;

list<p_bes_init> BESInterface::_init_list;
list<p_bes_end> BESInterface::_end_list;

static jmp_buf timeout_jump;
static bool timeout_jump_valid = false;

// Define this to use sigwait() is a child thread to detect that SIGALRM
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
        LOG("Child listener timeout after " << bes_timeout << " seconds, exiting." << endl);

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
    _strm(output_stream), _timeout_from_keys(0), _dhi(0), _transmitter(0)
{
    if (!output_stream) {
        throw BESInternalError("output stream must be set in order to output responses", __FILE__, __LINE__);
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
        iss >> _timeout_from_keys;
    }

    // Install signal handler for alarm() here
    register_signal_handler();

#if USE_SIGWAIT
    wait_for_timeout();
#endif
}

BESInterface::~BESInterface()
{
}

/** @brief Executes the given request to generate a specified response object

 Execute the request by:
 1. initializing BES
 2. validating the request, make sure all elements are present
 3. build the request plan (ie filling in the BESDataHandlerInterface)
 4. execute the request plan using the BESDataHandlerInterface
 5. transmit the resulting response object
 6. log the status of the execution
 7. notify the reporters of the request
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
 @see validate_data_request()
 @see build_data_request_plan()
 @see execute_data_request_plan()
 @see finish_no_error()
 @see finish_with_error()
 @see transmit_data()
 @see log_status()
 @see report_request()
 @see end_request()
 @see exception_manager()
 */
extern BESStopWatch *bes_timing::elapsedTimeToReadStart;
extern BESStopWatch *bes_timing::elapsedTimeToTransmitStart;

int BESInterface::execute_request(const string &from)
{
    BESDEBUG("bes", "Entering: " << __PRETTY_FUNCTION__ << endl);

    if (!_dhi) {
        throw BESInternalError("DataHandlerInterface can not be null", __FILE__, __LINE__);
    }

    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) {
        sw.start("BESInterface::execute_request", _dhi->data[REQUEST_ID]);

        bes_timing::elapsedTimeToReadStart = new BESStopWatch();
        bes_timing::elapsedTimeToReadStart->start("TIME_TO_READ_START", _dhi->data[REQUEST_ID]);

        bes_timing::elapsedTimeToTransmitStart = new BESStopWatch();
        bes_timing::elapsedTimeToTransmitStart->start("TIME_TO_TRANSMIT_START", _dhi->data[REQUEST_ID]);
    }

    if (!_dhi) {
        throw BESInternalError("DataHandlerInterface can not be null", __FILE__, __LINE__);
    }

    _dhi->set_output_stream(_strm);
    _dhi->data[REQUEST_FROM] = from;

    pid_t thepid = getpid();
    ostringstream ss;
    ss << thepid;
    _dhi->data[SERVER_PID] = ss.str();

    // This is never used except as an arg to finish. jhrg 12/23/15
    //int status = 0;

    // We split up the calls for the reason that if we catch an
    // exception during the initialization, building, execution, or response
    // transmit of the request then we can transmit the exception/error
    // information.
    try {
        initialize();

        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " request received"
            << endl;

        // This does not do anything here or in BESBasicInterface or BESXMLInterface.
        // Remove it? jhrg 12/23/15
        validate_data_request();

        build_data_request_plan();

        if (!_transmitter)
            throw BESInternalError("Unable to transmit the response, no transmitter", __FILE__, __LINE__);

        // This method does two key things: Calls the request handler to make a
        // 'response object' (the C++ object that will hold the response) and
        // then calls the transmitter to actually send it or build and send it.
        //
        // The timeout is also set in execute_data_request_plan(). The alarm signal
        // handler (above), run when the timeout expires, will call longjmp with a
        // return value of 1.
        if (setjmp(timeout_jump) == 0) {
            timeout_jump_valid = true;
            execute_data_request_plan();
            // Once we exit the block where setjmp() was called, the jump_buf is not valid
            timeout_jump_valid = false;
        }
        else {
            ostringstream oss;
            oss << "BES listener timeout after " << bes_timeout << " seconds." << ends;
            throw BESTimeoutError(oss.str(), __FILE__, __LINE__);
        }

        _dhi->executed = true;
    }
    catch (BESError & ex) {
        timeout_jump_valid = false;
        return exception_manager(ex);
    }
    catch (bad_alloc &e) {
        timeout_jump_valid = false;
        BESInternalFatalError ex(string("BES out of memory: ") + e.what(), __FILE__, __LINE__);
        return exception_manager(ex);
    }
    catch (exception &e) {
        timeout_jump_valid = false;
        BESInternalFatalError ex(string("C++ Exception: ") + e.what(), __FILE__, __LINE__);
        return exception_manager(ex);
    }
    catch (...) {
        timeout_jump_valid = false;
        BESInternalError ex("An undefined exception has been thrown", __FILE__, __LINE__);
        return exception_manager(ex);
    }

    delete bes_timing::elapsedTimeToReadStart;
    bes_timing::elapsedTimeToReadStart = 0;

    delete bes_timing::elapsedTimeToTransmitStart;
    bes_timing::elapsedTimeToTransmitStart = 0;

    return finish(0 /* status */);;
}

// I think this code was written when execute_request() called transmit_data()
// (and invoke_aggregation()). I think that the code up to the log_status()
// call is redundant. This means that so is the param 'status'. jhrg 12/23/15
int BESInterface::finish(int /*status*/)
{
    BESDEBUG("bes", "Entering: " << __PRETTY_FUNCTION__ << " ***" << endl);

#if 0
    int status = 0;
    try {
        // if there was an error during initialization, validation,
        // execution or transmit of the response then we need to transmit
        // the error information. Once printed, delete the error
        // information since we are done with it.
        if (_dhi->error_info) {
            transmit_data();
            delete _dhi->error_info;
            _dhi->error_info = 0;
        }
    }
    catch (BESError &ex) {
        status = exception_manager(ex);
    }
    catch (bad_alloc &) {
        string serr = "BES out of memory";
        BESInternalFatalError ex(serr, __FILE__, __LINE__);
        status = exception_manager(ex);
    }
    catch (...) {
        string serr = "An undefined exception has been thrown";
        BESInternalError ex(serr, __FILE__, __LINE__);
        status = exception_manager(ex);
    }
#endif

    // If there is error information then the transmit of the error failed,
    // print it to standard out. Once printed, delete the error
    // information since we are done with it.
    if (_dhi->error_info) {
        _dhi->error_info->print(cout);
        delete _dhi->error_info;
        _dhi->error_info = 0;
    }

    // if there is a problem with the rest of these steps then all we will
    // do is log it to the BES log file and not handle the exception with
    // the exception manager.
    try {
        log_status();
    }
    catch (BESError &ex) {
        (*BESLog::TheLog()) << "Problem logging status: " << ex.get_message() << endl;
    }
    catch (...) {
        (*BESLog::TheLog()) << "Unknown problem logging status" << endl;
    }

    try {
        report_request();
    }
    catch (BESError &ex) {
        (*BESLog::TheLog()) << "Problem reporting request: " << ex.get_message() << endl;
    }
    catch (...) {
        (*BESLog::TheLog()) << "Unknown problem reporting request" << endl;
    }

    try {
        end_request();
    }
    catch (BESError &ex) {
        (*BESLog::TheLog()) << "Problem ending request: " << ex.get_message() << endl;
    }
    catch (...) {
        (*BESLog::TheLog()) << "Unknown problem ending request" << endl;
    }

    return 0/*status*/;
}

int BESInterface::finish_with_error(int status)
{
    if (!_dhi->error_info) {
        // there wasn't an error ... so now what?
        string serr = "Finish_with_error called with no error object";
        BESInternalError ex(serr, __FILE__, __LINE__);
        status = exception_manager(ex);
    }

    return finish(status);
}

void BESInterface::add_init_callback(p_bes_init init)
{
    _init_list.push_back(init);
}

/** @brief Initialize the BES object
 *
 *  This method must be called by all derived classes as it will initialize
 *  the environment
 */
void BESInterface::initialize()
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("BESInterface::initialize", _dhi->data[REQUEST_ID]);

    BESDEBUG("bes", "Initializing request: " << _dhi->data[DATA_REQUEST] << " ... " << endl);
    bool do_continue = true;
    init_iter i = _init_list.begin();

    for (; i != _init_list.end() && do_continue == true; i++) {
        p_bes_init p = *i;
        do_continue = p(*_dhi);
    }

    if (!do_continue) {
        BESDEBUG("bes", "FAILED" << endl);
        string se = "Initialization callback failed, exiting";
        throw BESInternalError(se, __FILE__, __LINE__);
    }
    else {
        BESDEBUG("bes", "OK" << endl);
    }
}

/** @brief Validate the incoming request information
 */
void BESInterface::validate_data_request()
{
}

/** @brief Execute the data request plan

 Given the information in the BESDataHandlerInterface, execute the
 request. To do this we simply find the response handler given the action
 in the BESDataHandlerInterface and tell it to execute.

 If no BESResponseHandler can be found given the action then an
 exception is thrown.

 @note I have modified this class so that ths method can assume that
 the _transmitter field has been set.

 @see BESDataHandlerInterface
 @see BESResponseHandler
 @see BESResponseObject
 */
void BESInterface::execute_data_request_plan()
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG))
        sw.start("BESInterface::execute_data_request_plan(\"" + _dhi->data[DATA_REQUEST] + "\")",
            _dhi->data[REQUEST_ID]);

    // Set timeout if the 'bes_timeout' context value was passed in with the
    // command.
    bool found = false;
    string context = BESContextManager::TheManager()->get_context("bes_timeout", found);
    if (found) {
        bes_timeout = strtol(context.c_str(), NULL, 10);
        VERBOSE("Set request timeout to " << bes_timeout << " seconds (from context)." << endl);
        alarm(bes_timeout);
    }
    else if (_timeout_from_keys != 0) {
        bes_timeout = _timeout_from_keys;
        VERBOSE("Set request timeout to " << bes_timeout << " seconds (from keys)." << endl);
        alarm(bes_timeout);
    }

    try {
        BESDEBUG("bes", "Executing request: " << _dhi->data[DATA_REQUEST] << " ... " << endl);
        if (_dhi->response_handler) {
            _dhi->response_handler->execute(*_dhi);
        }
        else {
            BESDEBUG("bes", "FAILED" << endl);
            throw BESInternalError(string("The response handler \"") + _dhi->action + "\" does not exist", __FILE__,
                __LINE__);
        }
        BESDEBUG("bes", "OK" << endl);

        // Now we need to do the post processing piece of executing the request
        invoke_aggregation();

        // Before transmission starts, clear the timeout. This is done so users
        // on slow links are not penalized. jhrg 1/24/17
        bes_timeout = 0;
        alarm(0);

        // And finally, transmit the response of this request
        transmit_data();
    }
    catch (...) {
        bes_timeout = 0;
        alarm(0);

        throw;
    }
}

/** @brief Aggregate the resulting response object
 */
void BESInterface::invoke_aggregation()
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("BESInterface::invoke_aggregation", _dhi->data[REQUEST_ID]);

    if (_dhi->data[AGG_CMD] != "") {
        BESDEBUG("bes", "aggregating with: " << _dhi->data[AGG_CMD] << " ...  "<< endl);
        BESAggregationServer *agg = BESAggFactory::TheFactory()->find_handler(_dhi->data[AGG_HANDLER]);
        if (agg) {
            agg->aggregate(*_dhi);
        }
        else {
            BESDEBUG("bes", "FAILED" << endl);
            string se = "The aggregation handler " + _dhi->data[AGG_HANDLER] + "does not exist";
            throw BESInternalError(se, __FILE__, __LINE__);
        }
        BESDEBUG("bes", "OK" << endl);
    }
}

/** @brief Transmit the resulting response object

 The derived classes are responsible for specifying a transmitter object
 for use in transmitting the response object. Again, the
 BESResponseHandler knows how to transmit itself.

 If no response handler or no response object or no transmitter is
 specified then do nothing here.

 @see BESResponseHandler
 @see BESResponseObject
 @see BESTransmitter
 */
void BESInterface::transmit_data()
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("BESInterface::transmit_data", _dhi->data[REQUEST_ID]);

    BESDEBUG("bes", "BESInterface::transmit_data() - Transmitting request: " << _dhi->data[DATA_REQUEST] << endl);

    if (_dhi->error_info) {
        ostringstream strm;
        _dhi->error_info->print(strm);
        (*BESLog::TheLog()) << strm.str() << endl;
        BESDEBUG("bes", "  transmitting error info using transmitter ... " << endl << strm.str() << endl);

        _dhi->error_info->transmit(_transmitter, *_dhi);
    }
    else if (_dhi->response_handler) {
        BESDEBUG("bes",
            "  BESInterface::transmit_data() - Response handler  " << _dhi->response_handler->get_name() << endl);

        _dhi->response_handler->transmit(_transmitter, *_dhi);
    }

    BESDEBUG("bes", "BESInterface::transmit_data() - OK" << endl);
}

/** @brief Log the status of the request
 */
void BESInterface::log_status()
{
}

/** @brief Report the request and status of the request to
 * BESReporterList::TheList()

 If interested in reporting the request and status of the request then
 one must register a BESReporter with BESReporterList::TheList().

 If no BESReporter objects are registered then nothing happens.

 @see BESReporterList
 @see BESReporter
 */
void BESInterface::report_request()
{
    BESDEBUG("bes", "Reporting on request: " << _dhi->data[DATA_REQUEST] << " ... " << endl);

    BESReporterList::TheList()->report(*_dhi);

    BESDEBUG("bes", "OK" << endl);
}

void BESInterface::add_end_callback(p_bes_end end)
{
    _end_list.push_back(end);
}

/** @brief End the BES request
 *
 *  This method allows developers to add callbacks at the end of a request,
 *  to do any cleanup or do any extra work at the end of a request
 */
void BESInterface::end_request()
{
    BESDEBUG("bes", "Ending request: " << _dhi->data[DATA_REQUEST] << " ... " << endl);
    end_iter i = _end_list.begin();
    for (; i != _end_list.end(); i++) {
        p_bes_end p = *i;
        p(*_dhi);
    }

    // now clean up any containers that were used in the request, release
    // the resource
    _dhi->first_container();
    while (_dhi->container) {
        BESDEBUG("bes", "Calling BESContainer::release()" << endl);
        _dhi->container->release();
        _dhi->next_container();
    }

    BESDEBUG("bes", "OK" << endl);
}

/** @brief Clean up after the request
 */
void BESInterface::clean()
{
    if (_dhi) _dhi->clean();
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
int BESInterface::exception_manager(BESError &e)
{
    return BESExceptionManager::TheEHM()->handle_exception(e, *_dhi);
}

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

    if (_init_list.size()) {
        strm << BESIndent::LMarg << "termination functions:" << endl;
        BESIndent::Indent();
        init_iter i = _init_list.begin();
        for (; i != _init_list.end(); i++) {
            // TODO ISO C++ forbids casting between pointer-to-function and pointer-to-object
            // ...also below
            strm << BESIndent::LMarg << (void *) (*i) << endl;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "termination functions: none" << endl;
    }

    if (_end_list.size()) {
        strm << BESIndent::LMarg << "termination functions:" << endl;
        BESIndent::Indent();
        end_iter i = _end_list.begin();
        for (; i != _end_list.end(); i++) {
            strm << BESIndent::LMarg << (void *) (*i) << endl;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "termination functions: none" << endl;
    }

    strm << BESIndent::LMarg << "data handler interface:" << endl;
    BESIndent::Indent();
    _dhi->dump(strm);
    BESIndent::UnIndent();

    if (_transmitter) {
        strm << BESIndent::LMarg << "transmitter:" << endl;
        BESIndent::Indent();
        _transmitter->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "transmitter: not set" << endl;
    }
    BESIndent::UnIndent();
}
