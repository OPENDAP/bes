// BESApp.C

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

#include "BESApp.h"
#include "BESInternalError.h"

using namespace std;

BESApp *BESApp::_theApplication = nullptr;

/** @brief main method of the BES application
 *
 * sets the appName to argv[0], then calls initialize, run, and terminate in
 * that order. Exceptions should be caught in the individual methods
 * initialize, run and terminate and handled there.
 *
 * @return 0 if successful and not 0 otherwise
 * @param argC argc value passed to the main function
 * @param argV argv value passed to the main function
 */
int BESApp::main(int argC, char **argV) {
    _appName = argV[0];
    int retVal = initialize(argC, argV);
    if (retVal == 0) {
        retVal = run();
        retVal = terminate(retVal);
    }
    return retVal;
}

/** @brief initialize the BES application
 *
 * @return 0 if successful and not 0 otherwise
 * @param argC argc value passed to the main function
 * @param argV argv value passed to the main function
 * @throws BESError if any exceptions or errors are encountered
 */
int BESApp::initialize(int /*argC*/, char ** /*argV*/) {
    _isInitialized = true;
    return 0;
}

/** @brief the applications functionality is implemented in the run method
 *
 * It is up to the derived class to implement this method.
 *
 * @return 0 if successful and not 0 otherwise
 * @throws BESError if the derived class does not implement this
 * method
 */
int BESApp::run() {
    throw BESInternalError("BESApp::run - run method not implemented", __FILE__, __LINE__);
    return 0;
}

/** @brief clean up after the application
 *
 * Cleans up any global variables registered with BESGlobalIQ
 *
 * @return 0 if successful and not 0 otherwise
 * @param sig if the application is terminating due to a signal, otherwise 0
 * is passed.
 * @see BESGlobalIQ
 */
int BESApp::terminate(int sig) {
    _isInitialized = false;
    return sig;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the name of the
 * application, whether the application is initialized or not
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESApp::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESApp::dump - (" << (void *)this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "appName: " << appName() << endl;
    strm << BESIndent::LMarg << "is initialized: " << _isInitialized << endl;
    BESIndent::UnIndent();
}
