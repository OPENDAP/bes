// BESApp.h

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

#ifndef A_BESApp_H
#define A_BESApp_H

#include <map>
#include <string>

#include "BESObj.h"

/** @brief Application class for BES applications
 *
 * This class represents the application class for any BES
 * applications. It provides information about the application, such as any
 * parameters passed to the application, the name of the application,
 * debugging for the application, etc...
 *
 * This is a pure abstract class, not even main is implemented. It is up to
 * the derived class to implement main, initialize, run, and terminate.
 *
 * Provides a static method to retrieve the BESApp instance being used for
 * this application.
 *
 * @see BESObj
 */
class BESApp : public BESObj {
protected:
    std::string _appName;
    bool _debug{false};
    bool _isInitialized{false};
    static BESApp *_theApplication;

    BESApp() = default;

public:
    ~BESApp() override = default;

    /** @brief main routine, the main entry point for any BES applications.
     *
     * It is up to the derived classes of BESApp to implement the main
     * routine. However, the main method should call initialize, run and
     * terminate in that order and should pass to the initialize routine the
     * arguments argc and argv passed to the main function.
     *
     * @param argC number of arguments passed to the application, which is
     * argc passed to the main function.
     * @param argV arguments passed to the application, which is argv passed
     * to the main function.
     */
    virtual int main(int argC, char **argV);

    /** @brief Initialize the application using the passed argc and argv values
     *
     * It is up to the derived classes of BESApp to implement the
     * initialize method.
     *
     * @param argC number of arguments passed to the application, which is
     * argc passed to the main function.
     * @param argV arguments passed to the application, which is argv passed
     * to the main function.
     */
    virtual int initialize(int argC, char **argV);

    /** @brief The body of the application, implementing the primary
     * functionality of the BES application
     *
     * It is up to the derived classes of BESApp to implement the
     * run method.
     */
    virtual int run();

    /** @brief Clean up after the application
     *
     * It is up to the derived classes of BESApp to implement the
     * terminate method. Memory cleanup, file descriptor cleanup, etc... might
     * go in this method.
     *
     * @param sig if the application is terminating due to a signal, pass the
     * signal to terminate routine.
     */
    virtual int terminate(int sig = 0);

    /** @brief dumps information about this object
     *
     * Displays information about this object, typically for debugging
     * purposes.
     *
     * @param strm C++ i/o stream to dump the information to
     */
    void dump(std::ostream &strm) const override = 0;

    /** @brief Returns the name of the application
     *
     * The name of the application is typically argv[0] passed into the main
     * function. But could be passed into the application or derived in a
     * different way.
     *
     * @return name of the application
     */
    std::string appName() const { return _appName; }

    /** @brief Returns the BESApp application object for this application
     *
     * @return The application object
     */
    static BESApp *TheApplication() { return _theApplication; }
};

#endif
