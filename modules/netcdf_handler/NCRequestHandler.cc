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

// NCRequestHandler.cc

#include "config_nc.h"

#include <string>
#include <sstream>
#include <exception>

#include <libdap/DMR.h>
#include <libdap/DataDDS.h>
#include <libdap/mime_util.h>
#include <libdap/D4BaseTypeFactory.h>

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
#include <BESStopWatch.h>
#include <BESContextManager.h>
#include <BESDMRResponse.h>

#include <ObjMemCache.h>

#include <libdap/InternalErr.h>
#include <libdap/Ancillary.h>

#include "NCRequestHandler.h"
#include "GlobalMetadataStore.h"

using namespace libdap;

#define NC_NAME "nc"
#define prolog std::string("NCRequestHandler::").append(__func__).append("() - ")


bool NCRequestHandler::_show_shared_dims = true;
bool NCRequestHandler::_show_shared_dims_set = false;

bool NCRequestHandler::_ignore_unknown_types = false;
bool NCRequestHandler::_ignore_unknown_types_set = false;

bool NCRequestHandler::_promote_byte_to_short = false;
bool NCRequestHandler::_promote_byte_to_short_set = false;
bool NCRequestHandler::_use_mds = false;

unsigned int NCRequestHandler::_cache_entries = 100;
float NCRequestHandler::_cache_purge_level = 0.2;

ObjMemCache *NCRequestHandler::das_cache = 0;
ObjMemCache *NCRequestHandler::dds_cache = 0;
ObjMemCache *NCRequestHandler::datadds_cache = 0;
ObjMemCache *NCRequestHandler::dmr_cache = 0;

extern void nc_read_dataset_attributes(DAS & das, const string & filename);
extern void nc_read_dataset_variables(DDS & dds, const string & filename);

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
        //cerr << "version: " << v << ", value: " << value << endl;
        return (v >= value);
    }
    catch (...) {
        return false;
    }

    return false; // quiet warnings...
}

/**
 * Stolen from the HDF5 handler code
 */
static bool get_bool_key(const string &key, bool def_val)
{
    bool found = false;
    string doset = "";
    const string dosettrue = "true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        doset = BESUtil::lowercase(doset);
        return (dosettrue == doset || dosetyes == doset);
    }
    return def_val;
}

static unsigned int get_uint_key(const string &key, unsigned int def_val)
{
    bool found = false;
    string doset = "";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        return atoi(doset.c_str()); // use better code TODO
    }
    else {
        return def_val;
    }
}

static float get_float_key(const string &key, float def_val)
{
    bool found = false;
    string doset = "";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        return atof(doset.c_str()); // use better code TODO
    }
    else {
        return def_val;
    }
}

NCRequestHandler::NCRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    BESDEBUG(NC_NAME, prolog << "BEGIN" << endl);

    add_method(DAS_RESPONSE, NCRequestHandler::nc_build_das);
    add_method(DDS_RESPONSE, NCRequestHandler::nc_build_dds);
    add_method(DATA_RESPONSE, NCRequestHandler::nc_build_data);

    add_method(DMR_RESPONSE, NCRequestHandler::nc_build_dmr);
    add_method(DAP4DATA_RESPONSE, NCRequestHandler::nc_build_dmr);

    add_method(HELP_RESPONSE, NCRequestHandler::nc_build_help);
    add_method(VERS_RESPONSE, NCRequestHandler::nc_build_version);

    // TODO replace with get_bool_key above 5/21/16 jhrg

    if (NCRequestHandler::_show_shared_dims_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.ShowSharedDimensions", doset, key_found);
        if (key_found) {
            // It was set in the conf file
            NCRequestHandler::_show_shared_dims_set = true;

            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes") {
                NCRequestHandler::_show_shared_dims = true;
            }
            else
                NCRequestHandler::_show_shared_dims = false;
        }
    }

    if (NCRequestHandler::_ignore_unknown_types_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.IgnoreUnknownTypes", doset, key_found);
        if (key_found) {
            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes")
                NCRequestHandler::_ignore_unknown_types = true;
            else
                NCRequestHandler::_ignore_unknown_types = false;

            NCRequestHandler::_ignore_unknown_types_set = true;
        }
    }

    if (NCRequestHandler::_promote_byte_to_short_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value("NC.PromoteByteToShort", doset, key_found);
        if (key_found) {
            doset = BESUtil::lowercase(doset);
            if (doset == "true" || doset == "yes")
                NCRequestHandler::_promote_byte_to_short = true;
            else
                NCRequestHandler::_promote_byte_to_short = false;

            NCRequestHandler::_promote_byte_to_short_set = true;
        }
    }

    NCRequestHandler::_use_mds = get_bool_key("NC.UseMDS",false);
    NCRequestHandler::_cache_entries = get_uint_key("NC.CacheEntries", 0);
    NCRequestHandler::_cache_purge_level = get_float_key("NC.CachePurgeLevel", 0.2);

    if (get_cache_entries()) {  // else it stays at its default of null
        das_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
        dds_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
        datadds_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
        dmr_cache = new ObjMemCache(get_cache_entries(), get_cache_purge_level());
    }

    BESDEBUG(NC_NAME, prolog << "END" << endl);
}

NCRequestHandler::~NCRequestHandler()
{
    delete das_cache;
    delete dds_cache;
    delete datadds_cache;
    delete dmr_cache;
}

bool NCRequestHandler::nc_build_das(BESDataHandlerInterface & dhi)
{
    BES_STOPWATCH_START_DHI(NC_NAME, prolog + "Timer", &dhi);

    BESDEBUG(NC_NAME, prolog << "BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        string container_name = bdas->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";

        DAS *das = bdas->get_das();
        if (!container_name.empty()) das->container_name(container_name);
        string accessed = dhi.container->access();

        // Look in memory cache if it's initialized
        DAS *cached_das_ptr = 0;
        if (das_cache && (cached_das_ptr = static_cast<DAS*>(das_cache->get(accessed)))) {
            // copy the cached DAS into the BES response object
            BESDEBUG(NC_NAME, prolog << "DAS Cached hit for : " << accessed << endl);
            *das = *cached_das_ptr;
        }
        else {
            nc_read_dataset_attributes(*das, accessed);
            Ancillary::read_ancillary_das(*das, accessed);
            if (das_cache) {
                // add a copy
                BESDEBUG(NC_NAME, prolog << "DAS added to the cache for : " << accessed << endl);
                das_cache->add(new DAS(*das), accessed);
            }
        }

        bdas->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "Unknown exception caught building DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    BESDEBUG(NC_NAME, prolog << "END" << endl);
    return true;
}

/**
 * @brief Using the empty DDS object, build a DDS
 * @param dataset_name
 * @param container_name
 * @param dds
 */
void NCRequestHandler::get_dds_with_attributes(const string& dataset_name, const string& container_name, DDS* dds)
{
    // Look in memory cache if it's initialized
    DDS* cached_dds_ptr = 0;
    if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(dataset_name)))) {
        // copy the cached DDS into the BES response object. Assume that any cached DDS
        // includes the DAS information.
        BESDEBUG(NC_NAME, prolog << "DDS Cached hit for : " << dataset_name << endl);
        *dds = *cached_dds_ptr; // Copy the referenced object
    }
    else {
        if (!container_name.empty()) dds->container_name(container_name);
        dds->filename(dataset_name);

        nc_read_dataset_variables(*dds, dataset_name);

        DAS* das = 0;
        if (das_cache && (das = static_cast<DAS*>(das_cache->get(dataset_name)))) {
            BESDEBUG(NC_NAME, prolog << "DAS Cached hit for : " << dataset_name << endl);
            dds->transfer_attributes(das); // no need to cop the cached DAS
        }
        else {
            das = new DAS;
            // This looks at the 'use explicit containers' prop, and if true
            // sets the current container for the DAS.
            if (!container_name.empty()) das->container_name(container_name);

            nc_read_dataset_attributes(*das, dataset_name);
            Ancillary::read_ancillary_das(*das, dataset_name);

            dds->transfer_attributes(das);

            // Only free the DAS if it's not added to the cache
            if (das_cache) {
                // add a copy
                BESDEBUG(NC_NAME, prolog << "DAS added to the cache for : " << dataset_name << endl);
                das_cache->add(das, dataset_name);
            }
            else {
                delete das;
            }
        }

        if (dds_cache) {
            // add a copy
            BESDEBUG(NC_NAME, prolog << "DDS added to the cache for : " << dataset_name << endl);
            dds_cache->add(new DDS(*dds), dataset_name);
        }
    }
}

void NCRequestHandler::get_dds_without_attributes(const string& dataset_name, const string& container_name, DDS* dds)
{
    // Look in memory cache if it's initialized
    DDS* cached_datadds_ptr = 0;
    if (datadds_cache && (cached_datadds_ptr = static_cast<DDS*>(datadds_cache->get(dataset_name)))) {
        // copy the cached DDS into the BES response object. 
        BESDEBUG(NC_NAME, prolog << "DataDDS Cached hit for : " << dataset_name << endl);
        *dds = *cached_datadds_ptr; // Copy the referenced object
    }
    else {
        if (!container_name.empty()) dds->container_name(container_name);
        dds->filename(dataset_name);

        nc_read_dataset_variables(*dds, dataset_name);

        if (datadds_cache) {
            // add a copy
            BESDEBUG(NC_NAME, prolog << "DataDDS added to the cache for : " << dataset_name << endl);
            datadds_cache->add(new DDS(*dds), dataset_name);
        }
    }
}


bool NCRequestHandler::nc_build_dds(BESDataHandlerInterface & dhi)
{

    BES_STOPWATCH_START_DHI(NC_NAME, prolog + "Timer", &dhi);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        // If there's no value for this set in the conf file, look at the context
        // and set the default behavior based on the protocol version clients say
        // they will accept.
        if (NCRequestHandler::_show_shared_dims_set == false) {
            bool context_found = false;
            string context_value = BESContextManager::TheManager()->get_context("xdap_accept", context_found);
            if (context_found) {
                BESDEBUG(NC_NAME, prolog << "xdap_accept: " << context_value << endl);
                if (version_ge(context_value, 3.2))
                    NCRequestHandler::_show_shared_dims = false;
                else
                    NCRequestHandler::_show_shared_dims = true;
            }
        }

        string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
        DDS *dds = bdds->get_dds();

        // Build a DDS in the empty DDS object
        string filename = dhi.container->access();
        get_dds_with_attributes(filename, container_name, dds);

        bdds->set_constraint(dhi);
        bdds->clear_container();
    }
    catch (BESError &e) {
        throw e;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "Unknown exception caught building DDS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool NCRequestHandler::nc_build_data(BESDataHandlerInterface & dhi)
{
    BES_STOPWATCH_START_DHI(NC_NAME, prolog + "Timer", &dhi);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        if (NCRequestHandler::_show_shared_dims_set == false) {
            bool context_found = false;
            string context_value = BESContextManager::TheManager()->get_context("xdap_accept", context_found);
            if (context_found) {
                BESDEBUG(NC_NAME, prolog << "xdap_accept: " << context_value << endl);
                if (version_ge(context_value, 3.2))
                    NCRequestHandler::_show_shared_dims = false;
                else
                    NCRequestHandler::_show_shared_dims = true;
            }
        }

        string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
        DDS *dds = bdds->get_dds();

        // Build a DDS in the empty DDS object,don't include attributes here. KY 10/30/19
        get_dds_without_attributes(dhi.container->access(), container_name, dds);

        bdds->set_constraint(dhi);
        BESDEBUG(NC_NAME, prolog << "Data ACCESS build_data(): set the including attribute flag to false: "<<dhi.container->access() << endl);
        bdds->set_ia_flag(false);
        bdds->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "Unknown exception caught building DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool NCRequestHandler::nc_build_dmr(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(NC_NAME, prolog + "Timer", &dhi);

    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bdmr = dynamic_cast<BESDMRResponse &>(*response);

    // Because this code does not yet know how to build a DMR directly, use
    // the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
    // First step, build the 'full DDS'
    string dataset_name = dhi.container->access();

    // Get the DMR made by the BES in the BES/dap/BESDMRResponseHandler, make sure there's a
    // factory we can use and then dump the DAP2 variables and attributes in using the
    // BaseType::transform_to_dap4() method that transforms individual variables
    DMR *dmr = bdmr.get_dmr();

	try {
        DMR* cached_dmr_ptr = 0;
        if (dmr_cache && (cached_dmr_ptr = static_cast<DMR*>(dmr_cache->get(dataset_name)))) {
            // copy the cached DMR into the BES response object
            BESDEBUG(NC_NAME, prolog << "DMR Cached hit for : " << dataset_name << endl);
            *dmr = *cached_dmr_ptr; // Copy the referenced object
            dmr->set_request_xml_base(bdmr.get_request_xml_base());
        }
        else {
#if 0
            // this version builds and caches the DDS/DAS info.
            BaseTypeFactory factory;
            DDS dds(&factory, name_path(dataset_name), "3.2");

            // This will get the DDS, either by building it or from the cache
            get_dds_with_attributes(dataset_name, "", &dds);

            D4BaseTypeFactory MyD4TypeFactory;
            dmr->set_factory(&MyD4TypeFactory);
            dmr->build_using_dds(dds);
#else
            // This version builds a DDS only to build the resulting DMR. The DDS is
            // not cached. It does look in the DDS cache, just in case...
            D4BaseTypeFactory MyD4TypeFactory;                                                              
            dmr->set_factory(&MyD4TypeFactory);

            DDS *dds_ptr = 0;
            if (dds_cache && (dds_ptr = static_cast<DDS*>(dds_cache->get(dataset_name)))) {
                // Use the cached DDS; Assume that all cached DDS objects hold DAS info too
                BESDEBUG(NC_NAME, prolog << "DDS Cached hit (while building DMR) for : " << dataset_name << endl);

                dmr->build_using_dds(*dds_ptr);
            }
            else {
                // Build a throw-away DDS; don't bother to cache it. DMR's don't support
                // containers.
                BaseTypeFactory factory;
                DDS dds(&factory, name_path(dataset_name), "3.2");

                dds.filename(dataset_name);
                nc_read_dataset_variables(dds, dataset_name);

                DAS das;

                nc_read_dataset_attributes(das, dataset_name);
                Ancillary::read_ancillary_das(das, dataset_name);

                dds.transfer_attributes(&das);
                dmr->build_using_dds(dds);
            }
#endif

            if (dmr_cache) {
                // add a copy
                BESDEBUG(NC_NAME, prolog << "DMR added to the cache for : " << dataset_name << endl);
                dmr_cache->add(new DMR(*dmr), dataset_name);
            }
        }

        // Instead of fiddling with the internal storage of the DHI object,
        // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
        // methods to set the constraints. But, why? Ans: from Patrick is that
        // in the 'container' mode of BES each container can have a different
        // CE.
        bdmr.set_dap4_constraint(dhi);
        bdmr.set_dap4_function(dhi);
    }
	catch (InternalErr &e) {
		throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (Error &e) {
		throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
	}
	catch (...) {
		throw BESDapError("Caught unknown error build NC DMR response", true, unknown_error, __FILE__, __LINE__);
	}

	return true;
}

bool NCRequestHandler::nc_build_help(BESDataHandlerInterface & dhi)
{
    BES_STOPWATCH_START_DHI(NC_NAME, prolog + "Timer", &dhi);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    map < string, string, std::less<> > attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    list < string > services;
    BESServiceRegistry::TheRegistry()->services_handled(NC_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

bool NCRequestHandler::nc_build_version(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(NC_NAME, prolog + "Timer", &dhi);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *> (response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}

void NCRequestHandler::add_attributes(BESDataHandlerInterface &dhi) {

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);
    DDS *dds = bdds->get_dds();
    string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
    string dataset_name = dhi.container->access();
    DAS* das = 0;
    if (das_cache && (das = static_cast<DAS*>(das_cache->get(dataset_name)))) {
        BESDEBUG(NC_NAME, prolog << "DAS Cached hit for : " << dataset_name << endl);
        dds->transfer_attributes(das); // no need to cop the cached DAS
    }
    else {
        das = new DAS;
        // This looks at the 'use explicit containers' prop, and if true
        // sets the current container for the DAS.
        if (!container_name.empty()) das->container_name(container_name);

        // Here we will check if we can generate DAS by parsing from MDS
        if(true == get_use_mds()) {

            bes::GlobalMetadataStore *mds=bes::GlobalMetadataStore::get_instance();
            bool valid_mds = true;
            if(NULL == mds)
                valid_mds = false;
            else if(false == mds->cache_enabled())
                valid_mds = false;
            if(true ==valid_mds) {
                string rel_file_path = dhi.container->get_relative_name();
                // Obtain the DAS lock in MDS
                bes::GlobalMetadataStore::MDSReadLock mds_das_lock = mds->is_das_available(rel_file_path);
                if(mds_das_lock()) {
                    BESDEBUG(NC_NAME, prolog << "Using MDS to generate DAS in the data response for file " << dataset_name << endl);
                    mds->parse_das_from_mds(das,rel_file_path);
                }
                else {//Don't fail, still build das from the NC APIs
                    nc_read_dataset_attributes(*das, dataset_name);
                }
                mds_das_lock.clearLock();
            }
            else {
                nc_read_dataset_attributes(*das, dataset_name);
            }
        }
        else {//Cannot parse from MDS, still build the attributes from NC APIs.
            nc_read_dataset_attributes(*das, dataset_name);
        }
        Ancillary::read_ancillary_das(*das, dataset_name);

        dds->transfer_attributes(das);

        // Only free the DAS if it's not added to the cache
        if (das_cache) {
            // add a copy
            BESDEBUG(NC_NAME, prolog << "DAS added to the cache for : " << dataset_name << endl);
            das_cache->add(das, dataset_name);
        }
        else {
            delete das;
        }
    }
    BESDEBUG(NC_NAME, prolog << "Data ACCESS in add_attributes(): set the including attribute flag to true: "<<dataset_name << endl);
    bdds->set_ia_flag(true);
    return;
}
