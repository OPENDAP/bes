// BESExceptionManager.h

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

#ifndef BESExceptionManager_h_
#define BESExceptionManager_h_ 1

#include <list>

#include "BESObj.h"
#include "BESDataHandlerInterface.h"
#include "BESInfo.h"

class BESError;

typedef int (*p_bes_ehm)(BESError &e, BESDataHandlerInterface &dhi);

/** @brief manages exception handling code and default exceptions
 *
 * The BESExceptionManager, a singleton, manages exceptions that are
 * thrown during the handling of a request. Exceptions are handled by
 * creating error informational objects and/or handling the exception
 * and continuing.
 *
 * If an error informational object is created then assign the new
 * informational object to BESDataHandlerInterface.error_info variable.
 *
 * No error information should be transmitted during the handling of
 * the exception as we give the server code a chance to react to an
 * exception before the exception information is sent.
 *
 * The exception information is sent during the transmit of a response.
 * An exception is handled just like any other response in terms of
 * being transmitted.
 *
 * Modules have a chance of registering exception handlers to the manager
 * to be able to handle exceptions differently, or handle specific exceptions
 * in a specific way. Exception handling functions are registered with the
 * following signature:
 *
 * int function_name( BESError &e, BESDataHandlerInterface &dhi ) ;
 *
 * If the registered functioon does not handle the exception then the function
 * should return BES_EXECUTED_OK0. If it does handle the exception, return
 * a status code representative of the exception. Currently registered
 * status returns can be found in BESError.h
 *
 * If no handler can handle the exception then the default is to create
 * a BESInfo object with the given exception.
 *
 * @see BESError
 */

class BESExceptionManager: public BESObj {
private:
    typedef std::list<p_bes_ehm>::const_iterator ehm_citer;
    typedef std::list<p_bes_ehm>::iterator ehm_iter;

    std::list<p_bes_ehm> _ehm_list;

    static BESExceptionManager *_instance;

protected:
    BESExceptionManager();
    virtual ~BESExceptionManager();

public:
    virtual void add_ehm_callback(p_bes_ehm ehm);
    virtual int handle_exception(BESError &e, BESDataHandlerInterface &dhi);

    virtual void dump(std::ostream &strm) const;

    static BESExceptionManager *TheEHM();
};

#endif // BESExceptionManager_h_

