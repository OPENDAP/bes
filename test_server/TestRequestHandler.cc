// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
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

// TestRequestHandler.cc

#include "config.h"

#include <string>
#include <sstream>
#include <exception>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESVersionInfo.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESDebug.h>
#include <BESContextManager.h>

#include <InternalErr.h>
// #include <Ancillary.h>
#include <mime_util.h>

#include <test/TestTypeFactory.h>

#include "TestRequestHandler.h"

#define NAME "ts"

using namespace libdap;

int test_variable_sleep_interval = 0;

bool TestRequestHandler::d_use_series_values = true;
bool TestRequestHandler::d_use_series_values_set = false;

#if 0
/** Is the version number string greater than or equal to the value.
 * @note Works only for versions with zero or one dot. If the conversion of
 * the string to a float fails for any reason, this returns false.
 * @param version The string value (e.g., 3.2)
 * @param value A floating point value.
 */
static bool version_ge(const string &version, float value)
{
    try {
        float v;
        istringstream iss(version);
        iss >> v;
        return (v >= value);
    }
    catch (...) {
        return false;
    }

    return false; // quiet warnings...
}
#endif

TestRequestHandler::TestRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    BESDEBUG(NAME, "In TestRequestHandler::TestRequestHandler" << endl);

    add_handler(DAS_RESPONSE, TestRequestHandler::build_das);
    add_handler(DDS_RESPONSE, TestRequestHandler::build_dds);
    add_handler(DATA_RESPONSE, TestRequestHandler::build_data);
    add_handler(HELP_RESPONSE, TestRequestHandler::build_help);
    add_handler(VERS_RESPONSE, TestRequestHandler::build_version);

    // Look for the SHowSharedDims property, if it has not been set
    if (TestRequestHandler::d_use_series_values_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("TS.UseSeriesValues", doset, key_found);
        if (key_found) {
            // It was set in the conf file
            TestRequestHandler::d_use_series_values_set = true;

            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes") {
                TestRequestHandler::d_use_series_values = true;
            }
            else
                TestRequestHandler::d_use_series_values = false;
        }
    }

    BESDEBUG(NAME, "Exiting TestRequestHandler::TestRequestHandler" << endl);
}

TestRequestHandler::~TestRequestHandler()
{
}

bool TestRequestHandler::build_das(BESDataHandlerInterface & dhi)
{
    BESDEBUG(NAME, "In TestRequestHandler::build_das" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();
        string accessed = dhi.container->access();

		das->parse(accessed + ".das");

		bdas->set_constraint(dhi);
		bdas->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (std::exception &e) {
        throw BESInternalFatalError(string("C++ Exception: ") + e.what(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalFatalError("unknown exception caught building DAS", __FILE__, __LINE__);
    }

    BESDEBUG(NAME, "Exiting TestRequestHandler::build_das" << endl);
    return true;
}

bool TestRequestHandler::build_dds(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("BESDDSResponse cast error", __FILE__, __LINE__);

    try {
#if 0
    	// If there's no value for this set in the conf file, look at the context
        // and set the default behavior based on the protocol version clients say
        // they will accept.
        if (TestRequestHandler::_show_shared_dims_set == false) {
            bool context_found = false;
            string context_value = BESContextManager::TheManager()->get_context("xdap_accept", context_found);
            if (context_found) {
                BESDEBUG(NAME, "xdap_accept: " << context_value << endl);
                if (version_ge(context_value, 3.2))
                    TestRequestHandler::_show_shared_dims = false;
                else
                    TestRequestHandler::_show_shared_dims = true;
            }
        }

        BESDEBUG(NAME, "Fiddled with xdap_accept" << endl);

        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();
        string accessed = dhi.container->access();
        dds->filename(accessed);

        BESDEBUG(NAME, "Prior to nc_read_dataset_variables" << endl);

        nc_read_dataset_variables(*dds, accessed);

        BESDEBUG(NAME, "Prior to Ancillary::read_ancillary_dds, accessed: " << accessed << endl);

        Ancillary::read_ancillary_dds(*dds, accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        nc_read_dataset_attributes(*das, accessed);
        Ancillary::read_ancillary_das(*das, accessed);

        BESDEBUG(NAME, "Prior to dds->transfer_attributes" << endl);

        dds->transfer_attributes(das);

        bdds->set_constraint(dhi);
#endif
		// Take the dataset pathname (accessed) and append a .dds to find the
		// file that holds the DDS; do the equivalent for the DAS. This is nothing
        // more than a simple convention to get the DDS built from a file on
        // disk.

		bdds->set_container(dhi.container->get_symbolic_name());
		string accessed = dhi.container->access();

		DDS *dds = bdds->get_dds();
	    // TestTypeFactory ttf;
	    BaseTypeFactory ttf;
	    DDS server(&ttf);
		dds->filename(accessed);
		dds->set_dataset_name(name_path(accessed));
		BESDEBUG(NAME, "accessed: " << accessed << endl);
		dds->parse(accessed);
		dds->set_factory(0);
#if 0
		DAS *das = new DAS;
		BESDASResponse bdas(das);
		bdas.set_container(dhi.container->get_symbolic_name());

		das->parse(accessed + ".das");
		dds->transfer_attributes(das);
#endif
		bdds->set_constraint(dhi);
		bdds->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (std::exception &e) {
        throw BESInternalFatalError(string("C++ Exception: ") + e.what(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalFatalError("unknown exception caught building DDS", __FILE__, __LINE__);
    }

    return true;
}

bool TestRequestHandler::build_data(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
#if 0
        if (TestRequestHandler::_show_shared_dims_set == false) {
            bool context_found = false;
            string context_value = BESContextManager::TheManager()->get_context("xdap_accept", context_found);
            if (context_found) {
                BESDEBUG(NAME, "xdap_accept: " << context_value << endl);
                if (version_ge(context_value, 3.2))
                    TestRequestHandler::_show_shared_dims = false;
                else
                    TestRequestHandler::_show_shared_dims = true;
            }
        }

        bdds->set_container(dhi.container->get_symbolic_name());
        DataDDS *dds = bdds->get_dds();
        string accessed = dhi.container->access();
        dds->filename(accessed);

        nc_read_dataset_variables(*dds, accessed);

        Ancillary::read_ancillary_dds(*dds, accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        nc_read_dataset_attributes(*das, accessed);
        Ancillary::read_ancillary_das(*das, accessed);

        dds->transfer_attributes(das);

        bdds->set_constraint(dhi);
#endif
		bdds->set_container(dhi.container->get_symbolic_name());
		string accessed = dhi.container->access();

		DDS *dds = bdds->get_dds();

		dds->filename(accessed);
		dds->parse(accessed + ".dds");

		DAS *das = new DAS;
		BESDASResponse bdas(das);
		bdas.set_container(dhi.container->get_symbolic_name());

		das->parse(accessed + ".das");
		dds->transfer_attributes(das);

		bdds->set_constraint(dhi);
		bdds->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (std::exception &e) {
        throw BESInternalFatalError(string("C++ Exception: ") + e.what(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalFatalError("unknown exception caught building a Data response", __FILE__, __LINE__);
    }

    return true;
}

bool TestRequestHandler::build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    map < string, string > attrs;
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
    list < string > services;
    BESServiceRegistry::TheRegistry()->services_handled(NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

bool TestRequestHandler::build_version(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);

    return true;
}
