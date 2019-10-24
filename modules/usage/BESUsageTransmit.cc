// BESUsageTransmit.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#include "../usage/BESUsageTransmit.h"

#include "../usage/BESUsage.h"
#include "../usage/usage.h"
#include "BESDapTransmit.h"
#include "BESContainer.h"
#include "BESDataNames.h"
#include "mime_util.h"
#include "InternalErr.h"
#include "BESDapError.h"
#include "BESInternalFatalError.h"

#include "BESDebug.h"

using std::endl;
using namespace dap_usage;
using namespace libdap;

void
 BESUsageTransmit::send_basic_usage(BESResponseObject * obj,
                                    BESDataHandlerInterface & dhi)
{
    BESUsage &usage = dynamic_cast < BESUsage & >(*obj);
    DAS *das = usage.get_das()->get_das();
    DDS *dds = usage.get_dds()->get_dds();

    dhi.first_container();

    string dataset_name = dhi.container->access();

    try {
	BESDEBUG( "usage", "writing usage/info" << endl ) ;

        write_usage_response(dhi.get_output_stream(), *dds, *das, dataset_name, "", false);

        BESDEBUG( "usage", "done transmitting usage/info" << endl ) ;
    }
    catch( InternalErr &e )
    {
        string err = "Failed to write usage: " + e.get_error_message() ;
        throw BESDapError(err, true, e.get_error_code(), __FILE__, __LINE__ );
    }
    catch( Error &e )
    {
        string err = "Failed to write usage: " + e.get_error_message() ;
        throw BESDapError(err, false, e.get_error_code(), __FILE__, __LINE__ ) ;
    }
    catch (BESError &e) {
        throw;
    }
    catch(...)
    {
        string err = "Failed to write usage: Unknown exception caught";
        throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
    }
}

void
BESUsageTransmit::send_http_usage( BESResponseObject *obj,
                                   BESDataHandlerInterface &dhi )
{
    // TODO: Is this used?
    set_mime_html( dhi.get_output_stream(), unknown_type, x_plain ) ;
    BESUsageTransmit::send_basic_usage( obj, dhi ) ;
}

