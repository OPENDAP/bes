// BESWWWTransmit.cc

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

#include <BESDapTransmit.h>
#if 0
#include <libdap/DODSFilter.h>
#endif
#include <BESWWWTransmit.h>
// #include <libdap/DODSFilter.h>
#include <BESContainer.h>
#include <BESDapNames.h>
#include <BESWWWNames.h>
#include <libdap/mime_util.h>
#include <BESWWW.h>
#include <libdap/util.h>
#include <libdap/InternalErr.h>
#include <BESError.h>
#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESServiceRegistry.h>

#include <BESDebug.h>

#include "get_html_form.h"

using namespace dap_html_form;

void BESWWWTransmit::send_basic_form(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
	dhi.first_container();
	try {
		BESDEBUG("www", "converting dds to www dds" << endl);

		DDS *dds = dynamic_cast<BESWWW &>(*obj).get_dds()->get_dds();
		if (!dds) throw BESInternalFatalError("Expected a DDS instance", __FILE__, __LINE__);

		DDS *wwwdds = dds_to_www_dds(dds);

		BESDEBUG("www", "writing form" << endl);

		string url = dhi.data[WWW_URL];

		// Look for the netcdf format in the dap service. If present
		// then have the interface make a button for it.
		BESServiceRegistry *registry = BESServiceRegistry::TheRegistry();
		bool netcdf3_file_response = registry->service_available(OPENDAP_SERVICE, DATA_SERVICE, "netcdf");
		// TODO change this so that it actually tests for the netcdf4 capability. I'm
		// not sure how to do that, so just assume the handler has it. jhrg 9/23/13
		bool netcdf4_file_response = registry->service_available(OPENDAP_SERVICE, DATA_SERVICE, "netcdf");
		write_html_form_interface(dhi.get_output_stream(), wwwdds, url, false /*send mime headers*/,
				netcdf3_file_response, netcdf4_file_response);

		BESDEBUG("www", "done transmitting form" << endl);

		delete wwwdds;
	}
	catch (InternalErr &e) {
		string err = "Failed to write html form: " + e.get_error_message();
		throw BESDapError(err, true, e.get_error_code(), __FILE__, __LINE__);
	}
    catch (Error &e) {
        string err = "Failed to write html form: " + e.get_error_message();
        throw BESDapError(err, false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
	catch (...) {
		string err = "Failed to write html form: Unknown exception caught";
		throw BESInternalFatalError(err, __FILE__, __LINE__);
	}
}

void BESWWWTransmit::send_http_form(BESResponseObject * obj, BESDataHandlerInterface & dhi)
{
	set_mime_text(dhi.get_output_stream(), unknown_type, x_plain);
	BESWWWTransmit::send_basic_form(obj, dhi);
}

