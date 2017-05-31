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
#include <sstream>

#include <DMR.h>
#include <mime_util.h>
#include <D4BaseTypeFactory.h>
#include <InternalErr.h>
#include <Ancillary.h>
#include <debug.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESDMRResponse.h>
#include <BESVersionInfo.h>
#include <InternalErr.h>
#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include <BESUtil.h>
#include <Ancillary.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESContextManager.h>

#include <BESDebug.h>
#include <TheBESKeys.h>

#include "GDAL_DDS.h"
#include "GDAL_DMR.h"
#include "GDALRequestHandler.h"

#define GDAL_NAME "gdal"

using namespace libdap;

extern void gdal_read_dataset_attributes(DAS & das, GDALDatasetH &hDS/*const string & filename*/);
extern void /*GDALDatasetH*/ gdal_read_dataset_variables(DDS *dds, GDALDatasetH &hDS, const string &filename);

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
    DBG(cerr << "In GDALRequestHandler::gdal_build_dds" << endl);

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
        dds->set_dataset_name(filename.substr(filename.find_last_of('/') + 1));

        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

        if (hDS == NULL)
            throw Error(string(CPLGetLastErrorMsg()));

        gdal_read_dataset_variables(dds, hDS, filename);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        gdal_read_dataset_attributes(*das, hDS);

        GDALClose(hDS);
        hDS = 0;

        Ancillary::read_ancillary_das(*das, filename);

        dds->transfer_attributes(das);
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
    DBG(cerr << "In GDALRequestHandler::gdal_build_data" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    GDALDatasetH hDS = 0;
    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        DDS *gdds = bdds->get_dds();
#if 0
        // This copies the vanilla DDS into a new GDALDDS object, where the
        // handler can store extra information about the dataset.
        GDALDDS *gdds = new GDALDDS(bdds->get_dds());

        // Now delete the old DDS object
        delete bdds->get_dds();

        // Now make the BESDataDDSResponse object use our new object. When it
        // deletes the GDALDDS, the GDAL library will be used to close the
        // dataset.
        bdds->set_dds(gdds);
#endif
        string filename = dhi.container->access();
        gdds->filename(filename);
        gdds->set_dataset_name(filename.substr(filename.find_last_of('/') + 1));

        hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

        if (hDS == NULL)
            throw Error(string(CPLGetLastErrorMsg()));

        // Save the dataset handle so that it can be closed later
        // when the BES is done with the DDS (which is really a GDALDDS,
        // spawn of DataDDS...)
        gdal_read_dataset_variables(gdds, hDS, filename);
#if 0
        gdds->setGDALDataset(hDS);
#endif

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        gdal_read_dataset_attributes(*das, hDS);

        GDALClose(hDS);
        hDS = 0;

        Ancillary::read_ancillary_das(*das, filename);

        gdds->transfer_attributes(das);
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

bool GDALRequestHandler::gdal_build_dmr(BESDataHandlerInterface &dhi)
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

		DAS das;
		gdal_read_dataset_attributes(das, hDS);

		Ancillary::read_ancillary_das(das, filename);
		dds.transfer_attributes(&das);
	}
	catch (InternalErr &e) {
		throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (Error &e) {
		throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (...) {
		throw BESDapError("Caught unknown error building GDAL DMR response", true, unknown_error, __FILE__, __LINE__);
	}

	// Extract the DMR Response object - this holds the DMR used by the
	// other parts of the framework.
	BESResponseObject *response = dhi.response_handler->get_response_object();
	BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

	// In this handler we use a different pattern since the handler specializes the DDS/DMR.
	// First, build the DMR adding the open handle to the GDAL dataset, then free the DMR
	// the BES built and add this one. The GDALDMR object will close the open dataset when
	// the BES runs the DMR's destructor.

	DMR *dmr = bes_dmr.get_dmr();
	D4BaseTypeFactory d4_factory;
	dmr->set_factory(&d4_factory);
	dmr->build_using_dds(dds);

	GDALDMR *gdal_dmr = new GDALDMR(dmr);
	gdal_dmr->setGDALDataset(hDS);
	gdal_dmr->set_factory(0);

	delete dmr;	// The call below will make 'dmr' unreachable; delete it now to avoid a leak.
	bes_dmr.set_dmr(gdal_dmr); // BESDMRResponse will delete gdal_dmr

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
