// UgridFunctions.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <sstream>      // std::stringstream
#include <iostream>
#include <stdexcept>

#include <csignal>
#include <cstdlib>     /* abort, NULL */
#include <ctime>
#include <unistd.h>

#include <sys/time.h>

#include <curl/curl.h>
#include <curl/easy.h>


#include "DebugFunctions.h"

#include <libdap/ServerFunctionsList.h>
#include <libdap/Int32.h>
#include <libdap/Structure.h>
#include <libdap/Str.h>

#include <BESDebug.h>
#include <BESError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>
#include <BESSyntaxUserError.h>
#include <BESForbiddenError.h>
#include <BESNotFoundError.h>
#include <BESTimeoutError.h>
#include <HttpError.h>

namespace debug_function {

static string getFunctionNames()
{
    vector<string> names;
    libdap::ServerFunctionsList::TheList()->getFunctionNames(&names);

    string msg;
    for (std::vector<string>::iterator it = names.begin(); it != names.end(); ++it) {
        if (!msg.empty()) msg += ", ";

        msg += *it;
    }

    return msg;
}

void DebugFunctions::initialize(const string &/*modname*/)
{
    BESDEBUG("DebugFunctions", "initialize() - BEGIN" << std::endl);
    BESDEBUG("DebugFunctions", "initialize() - function names: " << getFunctionNames() << std::endl);

    debug_function::AbortFunc *abortFunc = new debug_function::AbortFunc();
    libdap::ServerFunctionsList::TheList()->add_function(abortFunc);

    debug_function::SleepFunc *sleepFunc = new debug_function::SleepFunc();
    libdap::ServerFunctionsList::TheList()->add_function(sleepFunc);

    debug_function::SumUntilFunc *sumUntilFunc = new debug_function::SumUntilFunc();
    libdap::ServerFunctionsList::TheList()->add_function(sumUntilFunc);

    debug_function::ErrorFunc *errorFunc = new debug_function::ErrorFunc();
    libdap::ServerFunctionsList::TheList()->add_function(errorFunc);

    BESDEBUG("DebugFunctions", "initialize() - function names: " << getFunctionNames() << std::endl);

    BESDEBUG("DebugFunctions", "initialize() - END" << std::endl);
}

void DebugFunctions::terminate(const string &/*modname*/)
{
    BESDEBUG("DebugFunctions", "Removing DebugFunctions Modules (this does nothing)." << std::endl);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void DebugFunctions::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DebugFunctions::dump - (" << (void *) this << ")" << std::endl;
}

/*****************************************************************************************
 * 
 * Abort Function (Debug Functions)
 * 
 * This server side function calls abort(). (boom)
 *
 */
string abort_usage = "abort(##) Where ## is the number of milliseconds to sleep before calling abort.";
AbortFunc::AbortFunc()
{
    setName("abort");
    setDescriptionString((string) "This function calls abort() killing the beslistner process.");
    setUsageString(abort_usage);
    setRole("http://services.opendap.org/dap4/server-side-function/debug/abort"); // 11/18/20 SBL - https not supported
    setDocUrl("https://docs.opendap.org/index.php/Debug_Functions");
    setFunction(debug_function::abort_ssf);
    setVersion("1.0");
}

void abort_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp)
{

    std::stringstream msg;
    libdap::Str *response = new libdap::Str("info");
    *btpp = response;

    if (argc != 1) {
        msg << "Missing time parameter!  USAGE: " << abort_usage;
    }
    else {
        libdap::Int32 *param1 = dynamic_cast<libdap::Int32*>(argv[0]);
        if (param1) {
            libdap::dods_int32 milliseconds = param1->value();

            msg << "abort in " << milliseconds << "ms" << endl;
            response->set_value(msg.str());

            usleep(milliseconds * 1000);
            msg << "abort now. " << endl;
            abort();
            return;
        }
        else {
            msg << "This function only accepts integer values " << "for the time (in milliseconds) parameter.  USAGE: "
                << abort_usage;
        }

    }

    response->set_value(msg.str());
    return;
}

/*****************************************************************************************
 * 
 * Sleep Function (Debug Functions)
 * 
 * This server side function calls sleep() for the number 
 * of millisecs passed in at argv[0]. (Zzzzzzzzzzzzzzz)
 *
 */

string sleep_usage = "sleep(##) where ## is the number of milliseconds to sleep.";
SleepFunc::SleepFunc()
{
    setName("sleep");
    setDescriptionString((string) "This function calls sleep() for the specified number of millisecs.");
    setUsageString(sleep_usage);
    setRole("http://services.opendap.org/dap4/server-side-function/debug/sleep"); // 11/18/20 SBL - https not supported
    setDocUrl("https://docs.opendap.org/index.php/Debug_Functions");
    setFunction(debug_function::sleep_ssf);
    setVersion("1.0");
}

void sleep_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp)
{

    std::stringstream msg;
    libdap::Str *sleep_info = new libdap::Str("info");

    //libdap::Structure *sleepResult = new libdap::Structure("sleep_result_unwrap");
    //sleepResult->add_var_nocopy(sleep_info);
    //*btpp = sleepResult;

    *btpp = sleep_info;

    if (argc != 1) {
        msg << "Missing time parameter!  USAGE: " << sleep_usage;
    }
    else {
        libdap::Int32 *param1 = dynamic_cast<libdap::Int32*>(argv[0]);
        if (param1) {
            libdap::dods_int32 milliseconds = param1->value();
            usleep(milliseconds * 1000);
            msg << "Slept for " << milliseconds << " ms.";
        }
        else {
            msg << "This function only accepts integer values " << "for the time (in milliseconds) parameter.  USAGE: "
                << sleep_usage;
        }

    }

    sleep_info->set_value(msg.str());

    /*
     for (libdap::DDS::Vars_iter it = dds.var_begin(); it != dds.var_end(); ++it) {
     libdap::BaseType *pBT = *it;
     sleepResult->add_var(pBT);
     }
     */

    return;
}

/*****************************************************************************************
 * 
 * SumUntil (Debug Functions)
 * 
 * This server side function computes a sum until the number of 
 * millisecs (passed in at argv[0]) has transpired. (+,+...+).
 *
 * @note Modified so that the actual sum value can be printed or not.
 * When this is used in tests, the value reached can vary, so to make
 * checking for success/failure, the value can be omitted.
 *
 */

string sum_until_usage = "sum_until(<val> [,0|<true>]) Compute a sum until <val> of milliseconds has elapsed; 0|<true> print the sum value.";
SumUntilFunc::SumUntilFunc()
{
    setName("sum_until");
    setDescriptionString((string) "This function calls sleep() for the specified number of millisecs.");
    setUsageString(sum_until_usage);
    setRole("http://services.opendap.org/dap4/server-side-function/debug/sum_until"); // 11/18/20 SBL - https not supported
    setDocUrl("https://docs.opendap.org/index.php/Debug_Functions");
    setFunction(debug_function::sum_until_ssf);
    setVersion("1.0");
}

void sum_until_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp)
{

    std::stringstream msg;
    libdap::Str *response = new libdap::Str("info");
    *btpp = response;

    if (!(argc == 1 || argc == 2)) {
        msg << "Missing time parameter!  USAGE: " << sum_until_usage;

        response->set_value(msg.str());
        return;
    }

    libdap::Int32 *param1 = dynamic_cast<libdap::Int32*>(argv[0]);
    // param1 is required
    if (!param1) {
        msg << "This function only accepts integer values " << "for the time (in milliseconds) parameter.  USAGE: "
            << sum_until_usage;

        response->set_value(msg.str());
        return;
    }

    bool print_sum_value = true;
    // argument #2 is optional
    if (argc == 2) {
        libdap::Int32 *temp = dynamic_cast<libdap::Int32*>(argv[1]);
        if (temp && temp->value() == 0)
            print_sum_value = false;
    }
    
    libdap::dods_int32 milliseconds = param1->value();

    struct timeval tv;
    gettimeofday(&tv, NULL);
    double start_time = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000; // convert tv_sec & tv_usec to millisecond
    double end_time = start_time;

    long fib;
    long one_past = 1;
    long two_past = 0;
    long n = 1;

    bool done = false;
    while (!done) {
        n++;
        fib = one_past + two_past;
        two_past = one_past;
        one_past = fib;
        gettimeofday(&tv, NULL);
        end_time = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000; // convert tv_sec & tv_usec to millisecond
        if (end_time - start_time >= milliseconds) {
            done = true;
        }
    }

    if (!print_sum_value)
        msg << "Summed for " << end_time - start_time << " ms.";
    else
        msg << "Summed for " << end_time - start_time << " ms. n: " << n;
    
    response->set_value(msg.str());
    return;
}

/*****************************************************************************************
 * 
 * Error Function (Debug Functions)
 * 
 * This server side function calls calls sleep() for the number 
 * of ms passed in at argv[0]. (Zzzzzzzzzzzzzzz)
 *
 */
string error_usage = "error(##) where ## is the BESError type to generate.";
ErrorFunc::ErrorFunc()
{
    setName("error");
    setDescriptionString((string) "This function triggers a BES Error of the type specified");
    setUsageString(error_usage);
    setRole("http://services.opendap.org/dap4/server-side-function/debug/error"); // 11/18/20 SBL - https not supported
    setDocUrl("https://docs.opendap.org/index.php/Debug_Functions");
    setFunction(debug_function::error_ssf);
    setVersion("1.0");
}

void error_ssf(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp)
{
    std::stringstream msg;
    auto *response = new libdap::Str("info");
    *btpp = response;

    string location = "error_ssf";

    if (argc < 1 || argc>2) {
        msg << "Missing error type parameter!  USAGE: " << error_usage;
    }
    else {
        auto param1 = dynamic_cast<const libdap::Int32*>(argv[0]);
        auto param2 = dynamic_cast<const libdap::Int32*>(argv[1]);
        if (param1) {
            libdap::dods_int32 error_type = param1->value();

            switch (error_type) {

            case BES_INTERNAL_ERROR: {   // 1
                msg << "A BESInternalError was requested.";
                throw BESInternalError(msg.str(), location, 0);
            }

            case BES_INTERNAL_FATAL_ERROR: {   // 2
                msg << "A BESInternalFatalError was requested.";
                throw BESInternalFatalError(msg.str(), location, 0);
            }

            case BES_SYNTAX_USER_ERROR: { // ...
                msg << "A BESSyntaxUserError was requested.";
                throw BESSyntaxUserError(msg.str(), location, 0);
            }

            case BES_FORBIDDEN_ERROR: {
                msg << "A BESForbiddenError was requested.";
                throw BESForbiddenError(msg.str(), location, 0);
            }

            case BES_NOT_FOUND_ERROR: {
                msg << "A BESNotFoundError was requested.";
                throw BESNotFoundError(msg.str(), location, 0);
            }

                case BES_TIMEOUT_ERROR: {   // 6
                    msg << "A BESTimeOutError was requested.";
                    throw BESTimeoutError(msg.str(), location, 0);
                }

                case BES_HTTP_ERROR: {   // 7
                    /*
                      HttpError(const std::string msg,
                      const std::string origin_url,
                      const std::string redirect_url,
                      const CURLcode code,
                      const unsigned int http_status,
                      const std::string file,
                      const int line):
                     */

                    /*
                         HttpError(const std::string msg,
                          const std::string origin_url,
                          const std::string redirect_url,
                          const CURLcode code,
                          const unsigned int http_status,
                          const std::vector<std::string> response_headers,
                          const std::string response_body,
                          const std::string file,
                          const int line):
                     */
                    string http_err_msg("An HttpError was requested.");
                    CURLcode curl_code = CURLE_OK;
                    unsigned int http_status = param2->value();
                    string origin_url("https://www.opendap.org");
                    string redirect_url("https://www.opendap.org/");
                    vector<string> rsp_hdrs;
                    rsp_hdrs.emplace_back("server: bes.error.functions");
                    rsp_hdrs.emplace_back("location: orbit-0");
                    string rsp_body("Every HTTP request includes an HTTP response. An HTTP response "
                                    "is a set of metadata and a response body, where the body can "
                                    "occasionally be zero bytes and thus nonexistent. An HTTP response "
                                    "however always has response headers.");

                    throw http::HttpError(http_err_msg,
                                          origin_url,
                                          redirect_url,
                                          curl_code,
                                          http_status,
                                          rsp_hdrs,
                                          rsp_body,
                                          __FILE__, __LINE__);
                }

                case 666: {
                msg << "A Segmentation Fault has been requested.";
                cerr << msg.str() << endl;
                raise(SIGSEGV);
                break;
            }

            case 314: {
                msg << "A PIPE signal has been requested.";
                cerr << msg.str() << endl;
                raise(SIGPIPE);
                break;
            }

            case 11: {
                throw std::bad_alloc(); // does not take a 'what()' value. jhrg 3/23/22
            }

            default:
            msg << "An unrecognized error_type parameter was received. Requested error_type: " << error_type;
            break;
        }

        }
        else {
            msg << "This function only accepts integer values " << "for the error type parameter.  USAGE: "
                << error_usage;
        }

    }

    response->set_value(msg.str());
}

} // namespace debug_function

extern "C" {
BESAbstractModule *maker()
{
    return new debug_function::DebugFunctions;
}
}
