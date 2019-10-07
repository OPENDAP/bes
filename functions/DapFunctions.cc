// DapFunctions.cc

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

#include <iostream>

#include <gdal.h>   // needed for scale_{grid,array}

#include <ServerFunctionsList.h>

#include <BESRequestHandlerList.h>

#include <BESDebug.h>

#include "GeoGridFunction.h"
#include "GridFunction.h"
#include "LinearScaleFunction.h"
#include "VersionFunction.h"
#include "MakeArrayFunction.h"
#include "MakeMaskFunction.h"
#include "BindNameFunction.h"
#include "BindShapeFunction.h"
#include "TabularFunction.h"
#include "BBoxFunction.h"
#include "RoiFunction.h"
#include "BBoxUnionFunction.h"
#include "MaskArrayFunction.h"
#include "DilateArrayFunction.h"
#include "RangeFunction.h"
#include "BBoxCombFunction.h"

#include "DapFunctionsRequestHandler.h"

#include "DapFunctions.h"
#include "ScaleGrid.h"

using std::endl;

namespace functions {

void DapFunctions::initialize(const string &modname)
{
    BESDEBUG( "dap_functions", "Initializing DAP Functions:" << endl );

    // Add this module to the Request Handler List so that it can respond
    // to version and help requests. Note the matching code to remove the
    // handler from the list in the terminate() method.
    BESRequestHandlerList::TheList()->add_handler(modname, new DapFunctionsRequestHandler(modname));

    libdap::ServerFunctionsList::TheList()->add_function(new GridFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new GeoGridFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new LinearScaleFunction());

    libdap::ServerFunctionsList::TheList()->add_function(new MakeArrayFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new MakeMaskFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new BindNameFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new BindShapeFunction());

    libdap::ServerFunctionsList::TheList()->add_function(new VersionFunction());

    libdap::ServerFunctionsList::TheList()->add_function(new TabularFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new BBoxFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new RoiFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new BBoxUnionFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new BBoxCombFunction());

    libdap::ServerFunctionsList::TheList()->add_function(new MaskArrayFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new DilateArrayFunction());

    libdap::ServerFunctionsList::TheList()->add_function(new RangeFunction());

    libdap::ServerFunctionsList::TheList()->add_function(new ScaleArray());
    libdap::ServerFunctionsList::TheList()->add_function(new ScaleGrid());
    libdap::ServerFunctionsList::TheList()->add_function(new Scale3DArray());

    GDALAllRegister();
    OGRRegisterAll();

    // What to do with the orig error handler? Pitch it for now. jhrg 10/17/16
    /*CPLErrorHandler orig_err_handler =*/ (void) CPLSetErrorHandler(CPLQuietErrorHandler);

    BESDEBUG( "dap_functions", "Done initializing DAP Functions" << endl );
}

void DapFunctions::terminate(const string &modname)
{
    BESDEBUG( "dap_functions", "Removing DAP Functions." << endl );

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh) delete rh;

}

/** @brief dumps information about this object
  *
  * Displays the pointer value of this instance
  *
  * @param strm C++ i/o stream to dump the information to
  */
void DapFunctions::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DapFunctions::dump - (" << (void *) this << ")" << endl;
}

extern "C" {
BESAbstractModule *maker()
{
    return new DapFunctions;
}
}

}
