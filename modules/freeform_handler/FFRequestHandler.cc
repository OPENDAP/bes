// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// FFRequestHandler.cc

#include "config_ff.h"

#include <iostream>
#include <string>
#include <sstream>
#include <exception>

#include <DDS.h>
#include <DataDDS.h>
#include <DMR.h>
#include <D4BaseTypeFactory.h>
#include <Ancillary.h>
#include <Error.h>
#include <InternalErr.h>
#include <mime_util.h>
#include <escaping.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESDMRResponse.h>
#include <BESVersionInfo.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESDebug.h>
#include <BESContextManager.h>

#include "FFRequestHandler.h"
//#include "D4FFTypeFactory.h"
#include "ff_ce_functions.h"
#include "util_ff.h"

using namespace libdap;
using namespace std;

#define FF_NAME "ff"

long BufPtr = 0; // cache pointer
long BufSiz = 0; // Cache size
char *BufVal = NULL; // cache buffer

extern void ff_read_descriptors(DDS & dds, const string & filename);
extern void ff_get_attributes(DAS & das, string filename);

bool FFRequestHandler::d_RSS_format_support = false;
string FFRequestHandler::d_RSS_format_files = "";

FFRequestHandler::FFRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_handler(DAS_RESPONSE, FFRequestHandler::ff_build_das);
    add_handler(DDS_RESPONSE, FFRequestHandler::ff_build_dds);
    add_handler(DATA_RESPONSE, FFRequestHandler::ff_build_data);

    add_handler(DMR_RESPONSE, FFRequestHandler::ff_build_dmr);
    add_handler(DAP4DATA_RESPONSE, FFRequestHandler::ff_build_dmr);

    add_handler(HELP_RESPONSE, FFRequestHandler::ff_build_help);
    add_handler(VERS_RESPONSE, FFRequestHandler::ff_build_version);

    ff_register_functions();

    bool key_found = false;
    string doset;
    TheBESKeys::TheKeys()->get_value("FF.RSSFormatSupport", doset, key_found);
    if (key_found) {
        doset = BESUtil::lowercase(doset);
        if (doset == "true" || doset == "yes")
            FFRequestHandler::d_RSS_format_support = true;
        else
            FFRequestHandler::d_RSS_format_support = false;
    }
    else
        FFRequestHandler::d_RSS_format_support = false;

    key_found = false;
    string path;
    TheBESKeys::TheKeys()->get_value("FF.RSSFormatFiles", path, key_found);
    if (key_found)
        FFRequestHandler::d_RSS_format_files = path;
    else
        FFRequestHandler::d_RSS_format_files = "";

    BESDEBUG("ff", "d_RSS_format_support: " << d_RSS_format_support << endl);
    BESDEBUG("ff", "d_RSS_format_files: " << d_RSS_format_files << endl);
}

FFRequestHandler::~FFRequestHandler()
{
}

bool FFRequestHandler::ff_build_das(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();

        string accessed = dhi.container->access();
        ff_get_attributes(*das, accessed);

        string name;
        if (FFRequestHandler::get_RSS_format_support()) {
            name = find_ancillary_rss_das(accessed);
        }
        else {
            name = Ancillary::find_ancillary_file(accessed, "das", "", "");
        }

        struct stat st;
        if (!name.empty() && (stat(name.c_str(), &st) == 0)) {
            das->parse(name);
        }

        bdas->clear_container();
    } catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    } catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    } catch (...) {
        BESInternalFatalError ex("unknown exception caught building Freeform DAS", __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool FFRequestHandler::ff_build_dds(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();
        string accessed = dhi.container->access();
        dds->filename(accessed);

        BESDEBUG("ff", "FFRequestHandler::ff_build_dds, accessed: " << accessed << endl);

        ff_read_descriptors(*dds, accessed);

        BESDEBUG("ff", "FFRequestHandler::ff_build_dds, reading attributes" << endl);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        ff_get_attributes(*das, accessed);
        Ancillary::read_ancillary_das(*das, accessed);

        BESDEBUG("ff", "FFRequestHandler::ff_build_dds, transferring attributes" << endl);

        dds->transfer_attributes(das);

        bdds->set_constraint(dhi);

        bdds->clear_container();

    } catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    } catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    } catch (...) {
        BESInternalFatalError ex("unknown exception caught building Freeform DDS", __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool FFRequestHandler::ff_build_data(BESDataHandlerInterface & dhi)
{
    BufPtr = 0; // cache pointer
    BufSiz = 0; // Cache size
    BufVal = NULL; // cache buffer

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();
        string accessed = dhi.container->access();
        dds->filename(accessed);
        ff_read_descriptors(*dds, accessed);
        Ancillary::read_ancillary_dds(*dds, accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        ff_get_attributes(*das, accessed);
        Ancillary::read_ancillary_das(*das, accessed);

        dds->transfer_attributes(das);

        bdds->set_constraint(dhi);

        bdds->clear_container();
    } catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    } catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    } catch (...) {
        BESInternalFatalError ex("unknown exception caught building Freeform DataDDS", __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

/**
 * Build the DMR object.
 *
 * This is used for both the DAP4 DMR and Data responses. This version of the
 * method uses the DMR's 'DDS constructor'.
 *
 * @param dhi
 * @return True on success or throw an exception on error.
 */
bool FFRequestHandler::ff_build_dmr(BESDataHandlerInterface &dhi)
{
    BufPtr = 0; // cache pointer
    BufSiz = 0; // Cache size
    BufVal = NULL; // cache buffer

	// Because this code does not yet know how to build a DMR directly, use
	// the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
	// First step, build the 'full DDS'
	string data_path = dhi.container->access();

	BaseTypeFactory factory;
	DDS dds(&factory, name_path(data_path), "3.2");
	dds.filename(data_path);

	try {
		ff_read_descriptors(dds, data_path);
		// ancillary DDS objects never took off - this does nothing. jhrg 8/12/14
		// Ancillary::read_ancillary_dds(*dds, data_path);

		DAS das;
		ff_get_attributes(das, data_path);
		Ancillary::read_ancillary_das(das, data_path);
		dds.transfer_attributes(&das);
	}
	catch (InternalErr &e) {
		throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (Error &e) {
		throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (...) {
		throw BESDapError("Caught unknown error build FF DMR response", true, unknown_error, __FILE__, __LINE__);
	}

	// Extract the DMR Response object - this holds the DMR used by the
	// other parts of the framework.
	BESResponseObject *response = dhi.response_handler->get_response_object();
	BESDMRResponse &bdmr = dynamic_cast<BESDMRResponse &>(*response);

	// Extract the DMR Response object - this holds the DMR used by the
	// other parts of the framework.
	DMR *dmr = bdmr.get_dmr();
	dmr->set_factory(new D4BaseTypeFactory);
	dmr->build_using_dds(dds);

	// Instead of fiddling with the internal storage of the DHI object,
	// (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
	// methods to set the constraints. But, why? Maybe setting data[]
	// directly is better? jhrg 8/14/14
	bdmr.set_dap4_constraint(dhi);
	bdmr.set_dap4_function(dhi);

	// What about async and store_result? See BESDapTransmit::send_dap4_data()

	return true;
}

bool FFRequestHandler::ff_build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    map < string, string > attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    list < string > services;
    BESServiceRegistry::TheRegistry()->services_handled(FF_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

bool FFRequestHandler::ff_build_version(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}
