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

#include <libdap/ServerFunctionsList.h>

#include <BESRequestHandlerList.h>
#include <TheBESKeys.h>
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
#include "TestFunction.h"
#include "IdentityFunction.h"
#include "DapFunctionsRequestHandler.h"
#include "DapFunctions.h"

#if HAVE_STARE
#include "stare/StareFunctions.h"
#endif

// Until we sort out the GDAL linking issue, do not include the gdal functions
#if HAVE_GDAL
#include "ScaleGrid.h"
#endif

using std::endl;
using std::ostream;
using std::string;

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

    libdap::ServerFunctionsList::TheList()->add_function(new TestFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new IdentityFunction());

#if HAVE_STARE
    libdap::ServerFunctionsList::TheList()->add_function(new StareIntersectionFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new StareCountFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new StareSubsetFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new StareSubsetArrayFunction());
    libdap::ServerFunctionsList::TheList()->add_function(new StareBoxFunction());

    // The key names and module variables used here are defined in StareFunctions.cc
    // jhrg 5/21/20
    stare_storage_path = TheBESKeys::TheKeys()->read_string_key(STARE_STORAGE_PATH_KEY, stare_storage_path);
    stare_sidecar_suffix = TheBESKeys::TheKeys()->read_string_key(STARE_SIDECAR_SUFFIX_KEY, stare_sidecar_suffix);
#endif

#if HAVE_GDAL
    libdap::ServerFunctionsList::TheList()->add_function(new ScaleArray());
    libdap::ServerFunctionsList::TheList()->add_function(new ScaleGrid());
    libdap::ServerFunctionsList::TheList()->add_function(new Scale3DArray());
    GDALAllRegister();
    OGRRegisterAll();

    // What to do with the orig error handler? Pitch it. jhrg 10/17/16
    (void) CPLSetErrorHandler(CPLQuietErrorHandler);
#endif

    BESDEBUG( "dap_functions", "Done initializing DAP Functions" << endl );
}

void DapFunctions::terminate(const string &modname)
{
    BESDEBUG( "dap_functions", "Removing DAP Functions." << endl );

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    delete rh;
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

}  // namespace functions
