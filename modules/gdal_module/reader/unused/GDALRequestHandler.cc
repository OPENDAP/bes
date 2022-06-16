// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gdal_handler, a data handler for the OPeNDAP data
// server.

// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2004 OPeNDAP, Inc.
// Author: Frank Warmerdam <warmerdam@pobox.com>
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


// GDALRequestHandler.cc

#include "config.h"

#include <string>

#include <gdal.h>

#include <libdap/DMR.h>
#include <libdap/mime_util.h>  // name_path
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/InternalErr.h>
#include <libdap/Ancillary.h>

#include <dispatch/BESResponseHandler.h>
#include <dispatch/BESServiceRegistry.h>

#include <dispatch/BESResponseNames.h>
#include <dap/BESDapNames.h>

#include <dap/BESDASResponse.h>
#include <dap/BESDDSResponse.h>
#include <dap/BESDataDDSResponse.h>
#include <dap/BESDMRResponse.h>
#include <dap/BESVersionInfo.h>

#include <dap/BESDapError.h>
#include <dispatch/BESInternalFatalError.h>
#include <dispatch/BESUtil.h>

#include <dispatch/BESDebug.h>

#include "GDALRequestHandler.h"
#include "reader/gdal_utils.h"

#define GDAL_NAME "gdal"

using namespace libdap;

GDALRequestHandler::GDALRequestHandler(const string &name) : BESRequestHandler(name)
{
    add_method(DAS_RESPONSE, GDALRequestHandler::gdal_build_das);
    add_method(DDS_RESPONSE, GDALRequestHandler::gdal_build_dds);
    add_method(DATA_RESPONSE, GDALRequestHandler::gdal_build_data);

    add_method(DMR_RESPONSE, GDALRequestHandler::gdal_build_dmr);
    add_method(DAP4DATA_RESPONSE, GDALRequestHandler::gdal_build_dmr);

    add_method(HELP_RESPONSE, GDALRequestHandler::gdal_build_help);
    add_method(VERS_RESPONSE, GDALRequestHandler::gdal_build_version);

    GDALAllRegister();
}

GDALRequestHandler::~GDALRequestHandler()
{
}

bool GDALRequestHandler::gdal_build_das(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    GDALDatasetH hDS = 0;
    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();
        string filename = dhi.container->access();

        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

        if (hDS == NULL)
            throw Error(string(CPLGetLastErrorMsg()));

        gdal_read_dataset_attributes(*das, hDS);

        GDALClose(hDS);
        hDS = 0;

        Ancillary::read_ancillary_das(*das, filename);

        bdas->clear_container();
    }
    catch (BESError &e) {
        if (hDS) GDALClose(hDS);
        throw;
    }
    catch (InternalErr & e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        if (hDS) GDALClose(hDS);
        throw BESInternalFatalError("unknown exception caught building DAS", __FILE__, __LINE__);
    }

    return true;
}

bool GDALRequestHandler::gdal_build_dds(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    GDALDatasetH hDS = 0;
    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();

        string filename = dhi.container->access();
        dds->filename(filename);
        dds->set_dataset_name(name_path(filename)/*filename.substr(filename.find_last_of('/') + 1)*/);

        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

        if (hDS == NULL)
            throw Error(string(CPLGetLastErrorMsg()));

        gdal_read_dataset_variables(dds, hDS, filename,true);

        GDALClose(hDS);
        hDS = 0;

        bdds->set_constraint(dhi);
        bdds->clear_container();
    }
    catch (BESError &e) {
        if (hDS) GDALClose(hDS);
        throw;
    }
    catch (InternalErr & e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        if (hDS) GDALClose(hDS);
        throw BESInternalFatalError("unknown exception caught building DDS", __FILE__, __LINE__);
    }

    return true;
}

bool GDALRequestHandler::gdal_build_data(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    // This lines is the sole difference between this static method and
    // gdal_build_dds(...). jhrg 6/1/17
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    GDALDatasetH hDS = 0;
    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();

        string filename = dhi.container->access();
        dds->filename(filename);
        dds->set_dataset_name(name_path(filename)/*filename.substr(filename.find_last_of('/') + 1)*/);

        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

        if (hDS == NULL)
            throw Error(string(CPLGetLastErrorMsg()));

        // The das will not be generated. KY 10/30/19
        gdal_read_dataset_variables(dds, hDS, filename,false);

        GDALClose(hDS);
        hDS = 0;

        bdds->set_constraint(dhi);
        BESDEBUG("gdal", "Data ACCESS build_data(): set the including attribute flag to false: "<<filename << endl);
        bdds->set_ia_flag(false);
        bdds->clear_container();
    }
    catch (BESError &e) {
        if (hDS) GDALClose(hDS);
        throw;
    }
    catch (InternalErr & e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        if (hDS) GDALClose(hDS);
        throw BESInternalFatalError("unknown exception caught building DAS", __FILE__, __LINE__);
    }

    return true;
}

/**
 * @brief Unused
 *
 * @deprecated
 */
bool GDALRequestHandler::gdal_build_dmr_using_dds(BESDataHandlerInterface &dhi)
{
	// Because this code does not yet know how to build a DMR directly, use
	// the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
	// First step, build the 'full DDS'
	string filename = dhi.container->access();

	BaseTypeFactory factory;
	DDS dds(&factory, name_path(filename), "3.2");
	dds.filename(filename);

    GDALDatasetH hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

    if (hDS == NULL)
        throw Error(string(CPLGetLastErrorMsg()));

	try {
		gdal_read_dataset_variables(&dds, hDS, filename,true);

		GDALClose(hDS);
		hDS = 0;
	}
	catch (InternalErr &e) {
	    if (hDS) GDALClose(hDS);
		throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (Error &e) {
	    if (hDS) GDALClose(hDS);
		throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (...) {
	    if (hDS) GDALClose(hDS);
		throw BESDapError("Caught unknown error building GDAL DMR response", true, unknown_error, __FILE__, __LINE__);
	}

	// Extract the DMR Response object - this holds the DMR used by the
	// other parts of the framework.
	BESResponseObject *response = dhi.response_handler->get_response_object();
	BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

	DMR *dmr = bes_dmr.get_dmr();
	D4BaseTypeFactory d4_factory;
	dmr->set_factory(&d4_factory);
	dmr->build_using_dds(dds);

	// Instead of fiddling with the internal storage of the DHI object,
	// (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
	// methods to set the constraints. But, why? Ans: from Patrick is that
	// in the 'container' mode of BES each container can have a different
	// CE.
	bes_dmr.set_dap4_constraint(dhi);
	bes_dmr.set_dap4_function(dhi);

	return true;
}

bool GDALRequestHandler::gdal_build_dmr(BESDataHandlerInterface &dhi)
{
    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    string filename = dhi.container->access();

    DMR *dmr = bes_dmr.get_dmr();
    D4BaseTypeFactory d4_factory;
    dmr->set_factory(&d4_factory);
    dmr->set_filename(filename);
    dmr->set_name(name_path(filename)/*filename.substr(filename.find_last_of('/') + 1)*/);

    GDALDatasetH hDS = 0;

    try {
        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);
        if (hDS == NULL) throw Error(string(CPLGetLastErrorMsg()));

        gdal_read_dataset_variables(dmr, hDS, filename);

        GDALClose(hDS);
        hDS = 0;
    }
    catch (InternalErr &e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error &e) {
        if (hDS) GDALClose(hDS);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        if (hDS) GDALClose(hDS);
        throw BESDapError("Caught unknown error building GDAL DMR response", true, unknown_error, __FILE__, __LINE__);
    }

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr.set_dap4_constraint(dhi);
    bes_dmr.set_dap4_function(dhi);

    return true;
}

bool GDALRequestHandler::gdal_build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    map < string, string > attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
    list < string > services;
    BESServiceRegistry::TheRegistry()->services_handled(GDAL_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

bool GDALRequestHandler::gdal_build_version(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}

void GDALRequestHandler::add_attributes(BESDataHandlerInterface &dhi) {

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);
    DDS *dds = bdds->get_dds();

    string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
    string filename = dhi.container->access();

    GDALDatasetH hDS = 0;
    DAS *das = NULL;

    try {

        das = new DAS;
        // sets the current container for the DAS.
        if (!container_name.empty()) das->container_name(container_name);

        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);
        if (hDS == NULL)
            throw Error(string(CPLGetLastErrorMsg()));

        gdal_read_dataset_attributes(*das,hDS);
        Ancillary::read_ancillary_das(*das, filename);

        dds->transfer_attributes(das);

        delete das;
        GDALClose(hDS);
        hDS = 0;
        BESDEBUG("gdal", "Data ACCESS in add_attributes(): set the including attribute flag to true: "<<filename << endl);
        bdds->set_ia_flag(true);

    }

    catch (BESError &e) {
        if (hDS) GDALClose(hDS);
        if (das) delete das;
        throw;
    }
    catch (InternalErr & e) {
        if (hDS) GDALClose(hDS);
        if (das) delete das;
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        if (hDS) GDALClose(hDS);
        if (das) delete das;
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        if (hDS) GDALClose(hDS);
        if (das) delete das;
        throw BESInternalFatalError("unknown exception caught building DDS", __FILE__, __LINE__);
    }

    return;
}
