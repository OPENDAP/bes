// BESExceptionManager.cc

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

#include <time.h>       /* time_t, struct tm, difftime, time, mktime */


#include "BESExceptionManager.h"

#include "BESError.h"
#include "TheBESKeys.h"
#include "BESInfoList.h"
#include "BESLog.h"

#define DEFAULT_ADMINISTRATOR "support@opendap.org"

BESExceptionManager *BESExceptionManager::_instance = 0;

BESExceptionManager::BESExceptionManager()
{
}

BESExceptionManager::~BESExceptionManager()
{
}

/** @brief Register an exception handler with the manager

 Signature of the function is as follows:

 int function_name( BESError &e, BESDataHandlerInterface &dhi ) ;

 If the handler does not handle the exception then it should return
 0. Otherwise, return a status code. Pre-defined status
 codes can be found in BESError.h

 @param ehm exception handler function
 @see BESError
 */
void BESExceptionManager::add_ehm_callback(p_bes_ehm ehm)
{
    _ehm_list.push_back(ehm);
}

/** @brief Manage any exceptions thrown during the handling of a request

 An informational object should be created and assigned to
 BESDataHandlerInterface.error_info variable.

 The manager first determines if a registered exception handler
 can handle the exception. First one to handle the exception wins.
 0 is returned from the registered handler if it cannot
 handle the exception.

 If no registered handlers can handle the exception then the default
 is to create an informational object (BESInfo instance) and the exception
 information stored there.

 @param e excption to be managed
 @param dhi information related to request and response
 @return status after exception is handled
 @see BESError
 @see BESInfo
 */
int BESExceptionManager::handle_exception(BESError &e, BESDataHandlerInterface &dhi)
{
    // Let's see if any of these exception callbacks can handle the
    // exception. The first callback that can handle the exception wins
    ehm_iter i = _ehm_list.begin();
    for (; i != _ehm_list.end(); i++) {
        p_bes_ehm p = *i;
        int handled = p(e, dhi);
        if (handled) {
            return handled;
        }
    }

    dhi.error_info = BESInfoList::TheList()->build_info();
    string action_name = dhi.action_name;
    if (action_name == "") action_name = "BES";
    dhi.error_info->begin_response(action_name, dhi);


    string administrator = "";
    try {
        bool found = false;
        vector<string> vals;
        string key = "BES.ServerAdministrator";
        TheBESKeys::TheKeys()->get_value(key, administrator, found);
    }
    catch (...) {
        administrator = DEFAULT_ADMINISTRATOR;
    }
    if (administrator.empty()) {
        administrator = DEFAULT_ADMINISTRATOR;
    }
    dhi.error_info->add_exception(e, administrator);
    dhi.error_info->end_response();

    /**--------------------------------------------------
     * ERROR LOG
     */
    struct tm *ptm;
    time_t timer = time(NULL);
    ptm = gmtime ( &timer );


    (*BESLog::TheLog()) <<
        "time: " << asctime(ptm) << " " <<
        "error: " << e.get_error_type() << " " <<
        "file: " << e.get_file() << " " <<
        "line: " << e.get_line() << " " <<
        "message: " << e.get_message() << endl ;

    /* ------------------------------------------------*/

    return e.get_error_type();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the number of
 * registered exception handler callbacks. Currently there is no way of
 * telling what callbacks are registered, as no names are passed to the add
 * method.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESExceptionManager::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESExceptionManager::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "# registered callbacks: " << _ehm_list.size() << endl;
    BESIndent::UnIndent();
}

BESExceptionManager *
BESExceptionManager::TheEHM()
{
    if (_instance == 0) {
        _instance = new BESExceptionManager();
    }
    return _instance;
}

