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

#include <DMR.h>
#include <mime_util.h>  // name_path
#include <D4BaseTypeFactory.h>
#include <InternalErr.h>
#include <Ancillary.h>

#include <BESResponseHandler.h>
#include <BESServiceRegistry.h>

#include <BESResponseNames.h>
#include <BESDapNames.h>

#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESDMRResponse.h>
#include <BESVersionInfo.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESUtil.h>

//#include <BESDebug.h>

#include "GDALRequestHandler.h"
#include "gdal_utils.h"

#define GDAL_NAME "gdal"

using namespace libdap;

GDALRequestHandler::GDALRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_handler(DAS_RESPONSE, GDALRequestHandler::gdal_build_das);
    add_handler(DDS_RESPONSE, GDALRequestHandler::gdal_build_dds);
    add_handler(DATA_RESPONSE, GDALRequestHandler::gdal_build_data);

    add_handler(DMR_RESPONSE, GDALRequestHandler::gdal_build_dmr);
    add_handler(DAP4DATA_RESPONSE, GDALRequestHandler::gdal_build_dmr);

    add_handler(HELP_RESPONSE, GDALRequestHandler::gdal_build_help);
    add_handler(VERS_RESPONSE, GDALRequestHandler::gdal_build_version);

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

        gdal_read_dataset_variables(dds, hDS, filename);

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

        gdal_read_dataset_variables(dds, hDS, filename);

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
		gdal_read_dataset_variables(&dds, hDS, filename);

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
