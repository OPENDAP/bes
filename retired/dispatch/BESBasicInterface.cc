// BESBasicInterface.cc

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

#include <iostream>
#include <sstream>

using std::endl;
using std::stringstream;

#include "BESBasicInterface.h"
#include "BESInterface.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESReturnManager.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESAggFactory.h"
#include "BESAggregationServer.h"
#include "BESTransmitterNames.h"
#include "BESDataNames.h"

/** @brief Instantiate a BESBasicInterface object given an output stream for
 * the response object
 *
 * @param strm The output stream used for the output of the response object
 * @see BESInterface
 */
BESBasicInterface::BESBasicInterface(ostream *strm) :
    BESInterface(strm)
{
}

BESBasicInterface::~BESBasicInterface()
{
}

/** @brief Override execute_request in order to register memory pool

 Once the memory pool is initialized hand over control to parent class to
 execute the request. Once completed, unregister the memory pool.

 This needs to be done here instead of the initialization method
 because???
 */
int BESBasicInterface::execute_request(const string &from)
{
    return BESInterface::execute_request(from);
}

/** @brief Initialize the BES

 Determines what transmitter this BES will be using to transmit response
 objects and then calls the parent initialization method in order to
 initialize all global variables.

 @see BESTransmitter
 @see BESDataHandlerInterface
 */
void BESBasicInterface::initialize()
{
    // dhi has not been filled in at this point, so let's set a default
    // transmitter given the protocol. The transmitter might change after
    // parsing a request and given a return manager to use. This is done in
    // build_data_plan.
    //
    // The reason I moved this from the build_data_plan method is because a
    // registered initialization routine might throw an exception and we
    // will need to transmit the exception info, which needs a transmitter.
    // If an exception happens before this then the exception info is just
    // printed to cout (see BESInterface::transmit_data()). -- pcw 09/05/06
    BESDEBUG("bes", "Finding " << BASIC_TRANSMITTER << " transmitter ... " << endl);

    d_transmitter = BESReturnManager::TheManager()->find_transmitter( BASIC_TRANSMITTER);
    if (!d_transmitter) {
        string s = (string) "Unable to find transmitter " + BASIC_TRANSMITTER;
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    BESDEBUG("bes", "OK" << endl);

    BESInterface::initialize();
}

/** @brief Validate the incoming request information
 */
void BESBasicInterface::validate_data_request()
{
    BESInterface::validate_data_request();
}

/** @brief Build the data request plan using the BESCmdParser.

 @see BESCmdParser
 */
void BESBasicInterface::build_data_request_plan()
{
    BESDEBUG("bes", "Entering: " <<  __PRETTY_FUNCTION__ << endl);

    // The derived class build_data_request_plan should be run first to
    // parse the incoming request. Once parsed we can determine if there is
    // a return command

    // The default _transmitter (either basic or http depending on the
    // protocol passed) has been set in initialize. If the parsed command
    // sets a RETURN_CMD (a different transmitter) then look it up here. If
    // it's set but not found then this is an error. If it's not set then
    // just use the defaults.
    if (_dhi->data[RETURN_CMD] != "") {
        BESDEBUG("bes", "Finding transmitter: " << _dhi->data[RETURN_CMD] << " ...  " << endl);

        d_transmitter = BESReturnManager::TheManager()->find_transmitter(_dhi->data[RETURN_CMD]);
        if (!d_transmitter) {
            string s = (string) "Unable to find transmitter " + _dhi->data[RETURN_CMD];
            throw BESSyntaxUserError(s, __FILE__, __LINE__);
        }
        BESDEBUG("bes", "OK" << endl);
    }
}

/** @brief Execute the data request plan

 Simply calls the parent method. Prior to calling the parent method logs
 a message to the dods log file.

 @see BESLog
 */
void BESBasicInterface::execute_data_request_plan()
{
    if (BESLog::TheLog()->is_verbose()) {
        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
            << _dhi->data[LOG_INFO] << "] executing" << endl;
    }
    BESInterface::execute_data_request_plan();
}

/** @brief Invoke the aggregation server, if there is one

 Simply calls the parent method. Prior to calling the parent method logs
 a message to the dods log file.

 @see BESLog
 */
void BESBasicInterface::invoke_aggregation()
{
    if (_dhi->data[AGG_CMD] == "") {
        if (BESLog::TheLog()->is_verbose()) {
            *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                << _dhi->data[LOG_INFO] << "]" << " not aggregating, command empty" << endl;
        }
    }
    else {
        BESAggregationServer *agg = BESAggFactory::TheFactory()->find_handler(_dhi->data[AGG_HANDLER]);
        if (!agg) {
            if (BESLog::TheLog()->is_verbose()) {
                *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                    << _dhi->data[LOG_INFO] << "]" << " not aggregating, no handler" << endl;
            }
        }
        else {
            if (BESLog::TheLog()->is_verbose()) {
                *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                    << _dhi->data[LOG_INFO] << "] aggregating" << endl;
            }
        }
    }
    BESInterface::invoke_aggregation();
}

/** @brief Transmit the response object

 Simply calls the parent method. Prior to calling the parent method logs
 a message to the dods log file.

 @see BESLog
 */
void BESBasicInterface::transmit_data()
{
    if (BESLog::TheLog()->is_verbose()) {
        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
            << _dhi->data[LOG_INFO] << "] transmitting" << endl;
    }
    BESInterface::transmit_data();
}

/** @brief Log the status of the request to the BESLog file

 @see BESLog
 */
void BESBasicInterface::log_status()
{
    string result = "completed";
    if (_dhi->error_info) result = "failed";
    if (BESLog::TheLog()->is_verbose()) {
        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
            << _dhi->data[LOG_INFO] << "] " << result << endl;
    }
}

/** @brief Clean up after the request is completed

 Calls the parent method clean and then logs to the BESLog file saying
 that we are done and exiting the process. The exit actually takes place
 in the module code.

 @see BESLog
 */
void BESBasicInterface::clean()
{
    BESInterface::clean();
    if (BESLog::TheLog()->is_verbose()) {
        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
            << _dhi->data[LOG_INFO] << "] cleaning" << endl;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESBasicInterface::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESBasicInterface::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESInterface::dump(strm);
    BESIndent::UnIndent();

}

