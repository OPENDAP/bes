// DmrppRequestHandler.cc

// Copyright (c) 2016 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.

#include "config.h"

#include <string>
#include <memory>
#include <sstream>

#include <curl/curl.h>
#include <stdlib.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESVersionInfo.h>
#include <BESTextInfo.h>

#include <Ancillary.h>
#include <ObjMemCache.h>

#include <BESDMRResponse.h>

#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDebug.h>
#include <BESStopWatch.h>

#include <DMR.h>
#include <D4Group.h>
#include <DAS.h>

#include <InternalErr.h>
#include <mime_util.h>  // for name_path

#include "DmrppTypeFactory.h"
#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"

using namespace libdap;
using namespace std;

#define NC_NAME "nc"

namespace dmrpp {

const string module = "dmrpp";

// These are not used. See the netcdf handler for an example of their use.
// jhrg 4/24/18
ObjMemCache *DmrppRequestHandler::das_cache = 0;
ObjMemCache *DmrppRequestHandler::dds_cache = 0;
ObjMemCache *DmrppRequestHandler::dmr_cache = 0;

// This is used to maintain a pool of reusable curl handles that enable connection
// reuse. jhrg
CurlHandlePool *DmrppRequestHandler::curl_handle_pool = 0;

bool DmrppRequestHandler::d_use_parallel_transfers = true;

static void read_key_value(const std::string &key_name, bool &key_value)
{
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    if (key_found) {
        value = BESUtil::lowercase(value);
        key_value = (value == "true" || value == "yes");
    }
}

/* **
 * Here we register all of our handler functions so that the BES Dispatch machinery
 * knows what kinds of things we handle.
 */
DmrppRequestHandler::DmrppRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_handler(DMR_RESPONSE, dap_build_dmr);
    add_handler(DAP4DATA_RESPONSE, dap_build_dap4data);
    add_handler(DAS_RESPONSE, dap_build_das);
    add_handler(DDS_RESPONSE, dap_build_dds);
    add_handler(DATA_RESPONSE, dap_build_dap2data);

    add_handler(VERS_RESPONSE, dap_build_vers);
    add_handler(HELP_RESPONSE, dap_build_help);

    read_key_value("DMRPP.UseParallelTransfers", d_use_parallel_transfers);

    if (!curl_handle_pool)
    curl_handle_pool = new CurlHandlePool();

    curl_global_init(CURL_GLOBAL_DEFAULT);
}

DmrppRequestHandler::~DmrppRequestHandler()
{
    delete curl_handle_pool;
    curl_global_cleanup();
}

void DmrppRequestHandler::build_dmr_from_file(const string& accessed, bool /*explicit_containers*/, DMR* dmr)
{
    BESDEBUG(module, "In DmrppRequestHandler::build_dmr_from_file; accessed: " << accessed << endl);

    dmr->set_filename(accessed);
    dmr->set_name(name_path(accessed));

    DmrppTypeFactory BaseFactory;   // Use the factory for this handler's types
    dmr->set_factory(&BaseFactory);

    DmrppParserSax2 parser;
    ifstream in(accessed.c_str(), ios::in);

    parser.intern(in, dmr, BESDebug::IsSet(module));

    dmr->set_factory(0);

    BESDEBUG(module, "Exiting build_dmr_from_file..." << endl);
}

/**
 * Given a request for the DMR response, look at the data source and
 * parse it's DMR/XML information. If the data source is a .dmr or .xml
 * file, assume that's all the data source contains and that the plain
 * DMR parser can be used. If the data source is a .dap file, assume it
 * is a DAP4 data response that has been dumped to a file, sans MIME
 * headers. Use the code in libdap::Connect to read the DMR.
 *
 * @param dhi
 * @return
 */
bool DmrppRequestHandler::dap_build_dmr(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_dmr..." << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(response);
    if (!bdmr) throw BESInternalError("BESDMRResponse cast error", __FILE__, __LINE__);

    try {
        build_dmr_from_file(dhi.container->access(), bdmr->get_explicit_containers(), bdmr->get_dmr());

        bdmr->set_dap4_constraint(dhi);
        bdmr->set_dap4_function(dhi);
    }
    catch (BESError &e) {
        throw e;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalFatalError("Unknown exception caught building a DMR", __FILE__, __LINE__);
    }

    BESDEBUG(module, "Leaving dap_build_dmr..." << endl);

    return true;
}

// This method sets the stage for the BES DAP service to return a data
// response. Unlike the DAP2 data response returned by this module, the
// data are not read from a 'freeze-dried' DAP data response. Instead
// they are generated by the D4TestTypeFactory types. So, for now, asking
// for a DAP4 data response from this handler w/o setting UseTestTypes
// is an error.
bool DmrppRequestHandler::dap_build_dap4data(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_dap4data..." << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(response);
    if (!bdmr) throw BESInternalError("BESDMRResponse cast error", __FILE__, __LINE__);

    try {
        DMR *dmr = bdmr->get_dmr();
        build_dmr_from_file(dhi.container->access(), bdmr->get_explicit_containers(), dmr);

        bdmr->set_dap4_constraint(dhi);
        bdmr->set_dap4_function(dhi);
    }
    catch (BESError &e) {
        throw e;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalFatalError("Unknown exception caught building DAP4 Data response", __FILE__, __LINE__);
    }

    BESDEBUG(module, "Leaving dap_build_dap4data..." << endl);

    return false;
}

/**
 * Produce a DAP2 Data Response (.dods) response from a DMRPP file.
 */
bool DmrppRequestHandler::dap_build_dap2data(BESDataHandlerInterface & dhi)
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("DmrppRequestHandler::dap_build_dap2data()", dhi.data[REQUEST_ID]);

    BESDEBUG(module, __func__ << "() - BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds) throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        string container_name_str = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name() : "";

        DDS *dds = bdds->get_dds();
        if (!container_name_str.empty()) dds->container_name(container_name_str);
        string accessed = dhi.container->access();

        // Look in memory cache, if it's initialized
        DDS *cached_dds_ptr = 0;
        if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(accessed)))) {
            // copy the cached DAS into the BES response object
            BESDEBUG(module, "DDS Cached hit for : " << accessed << endl);
            *dds = *cached_dds_ptr;
            bdds->set_constraint(dhi);
        }
        else {
            // Not in cache, make one...
            DMR *dmr = new DMR();
            build_dmr_from_file(dhi.container->access(), true, dmr);

            // delete the current one;
            delete dds;
            dds = 0;

            // assign the new one.
            dds = dmr->getDDS();

            // Stuff it into the response.
            bdds->set_dds(dds);
            bdds->set_constraint(dhi);

            // Cache it, if the cache is active.
            if (dds_cache) {
                // add a copy
                BESDEBUG(module, __func__ << "() - " << "DDS added to the cache for : " << accessed << endl);
                dds_cache->add(new DDS(*dds), accessed);
            }
        }
        bdds->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
        __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
        __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "unknown exception caught building DDS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    BESDEBUG(module, "DmrppRequestHandler::dap_build_dds() - END" << endl);
    return true;
}

/**
 * Produce a DAP2 DDS response from a DMRPP file.
 */
bool DmrppRequestHandler::dap_build_dds(BESDataHandlerInterface & dhi)
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("DmrppRequestHandler::dap_build_dds()", dhi.data[REQUEST_ID]);

    BESDEBUG(module, __func__ << "() - BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(response);
    if (!bdds) throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        string container_name_str = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name() : "";

        DDS *dds = bdds->get_dds();
        if (!container_name_str.empty()) dds->container_name(container_name_str);
        string accessed = dhi.container->access();

        // Look in memory cache, if it's initialized
        DDS *cached_dds_ptr = 0;
        if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(accessed)))) {
            // copy the cached DAS into the BES response object
            BESDEBUG(module, "DDS Cached hit for : " << accessed << endl);
            *dds = *cached_dds_ptr;
        }
        else {
            // Not in cache, make one...
            DMR *dmr = new DMR();
            build_dmr_from_file(dhi.container->access(), true, dmr);

            // delete the current one;
            delete dds;
            dds = 0;

            // assign the new one.
            dds = dmr->getDDS();

            // Stuff it into the response.
            bdds->set_dds(dds);
            bdds->set_constraint(dhi);

            // Cache it, if the cache is active.
            if (dds_cache) {
                // add a copy
                BESDEBUG(module, __func__ << "() - " << "DDS added to the cache for : " << accessed << endl);
                dds_cache->add(new DDS(*dds), accessed);
            }
        }
        bdds->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
        __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
        __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "unknown exception caught building DDS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    BESDEBUG(module, "DmrppRequestHandler::dap_build_dds() - END" << endl);
    return true;
}

/**
 * Produce a DAP2 DAS response from a DMRPP data set.
 *
 */
bool DmrppRequestHandler::dap_build_das(BESDataHandlerInterface & dhi)
{
    BESStopWatch sw;
    if (BESISDEBUG(TIMING_LOG)) sw.start("DmrppRequestHandler::dap_build_das()", dhi.data[REQUEST_ID]);

    BESDEBUG(module, __func__ << "() - BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(response);
    if (!bdas) throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        string container_name_str = bdas->get_explicit_containers() ? dhi.container->get_symbolic_name() : "";

        DAS *das = bdas->get_das();
        if (!container_name_str.empty()) das->container_name(container_name_str);
        string accessed = dhi.container->access();
        BESDEBUG(module, __func__ << "() - accessed: "<< accessed << endl);

        // Look in memory cache (if it's initialized)
        DAS *cached_das_ptr = 0;
        if (das_cache && (cached_das_ptr = static_cast<DAS*>(das_cache->get(accessed)))) {
            // copy the cached DAS into the BES response object
            BESDEBUG(module, __func__ << "() - DAS Cached hit for : " << accessed << endl);
            *das = *cached_das_ptr;
        }
        else {
            BESDEBUG(module, __func__ << "() - Building DAS From DMR++ " << endl);
            // Not in cache, better make one!
            // 1) Build a DMR
            DMR *dmr = new DMR();
            build_dmr_from_file(dhi.container->access(), true, dmr);
            BESDEBUG(module, __func__ << "() - DMR has been built." << endl);

            // Get a DDS from the DMR
            DDS *dds = dmr->getDDS();
            BESDEBUG(module, __func__ << "() - DDS retrieved from DMR." << endl);
            // Print the DDS
            if (BESDebug::IsSet(module)) {
                BESDEBUG(module, __func__ << "() - DDS: " << endl);
                dds->print(*BESDebug::GetStrm());
            }

            // Load the BESDASResponse DAS from the DDS
            dds->get_das(das);
            // Print the DAS
            if (BESDebug::IsSet(module)) {
                BESDEBUG(module, __func__ << "() -  " << "DAS: " << endl);
                das->print(*(BESDebug::GetStrm()), false);
            }

            delete dds;
            delete dmr;

            Ancillary::read_ancillary_das(*das, accessed);
            // Add to cache if cache is active
            if (das_cache) {
                // add a copy
                BESDEBUG(module, "DAS added to the cache for : " << accessed << endl);
                das_cache->add(new DAS(*das), accessed);
            }
        }
        bdas->clear_container();
    }
    catch (BESError &e) {
        throw;
    }
    catch (InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
        __FILE__, __LINE__);
        throw ex;
    }
    catch (Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
        __FILE__, __LINE__);
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }
    catch (...) {
        string s = "unknown exception caught building DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    BESDEBUG(module, __func__ << "() - END" << endl);
    return true;
}

#if DAP2
/**
 * This method will look at the extension on the input file and assume
 * that if it's .das, that file should be read and used to build the DAS
 * object. If it's .data or .dods, then the _ancillary_ das file will be used.
 * @param dhi
 * @return
 */
bool DmrppRequestHandler::dap_build_das(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_das..." << endl);

    BESDEBUG(module, "Leaving dap_build_das..." << endl);

    return true;
}

bool DmrppRequestHandler::dap_build_dds(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_dds..." << endl);

    BESDEBUG(module, "Exiting dap_build_dds..." << endl);

    return true;
}

bool DmrppRequestHandler::dap_build_data(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_data..." << endl);

    BESDEBUG(module, "Exiting dap_build_data..." << endl);

    return true;
}
#endif

bool DmrppRequestHandler::dap_build_vers(BESDataHandlerInterface &dhi)
{
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);
    return true;
}

bool DmrppRequestHandler::dap_build_help(BESDataHandlerInterface &dhi)
{
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string> attrs;
    attrs["name"] = MODULE_NAME /* PACKAGE_NAME */;
    attrs["version"] = MODULE_VERSION /* PACKAGE_VERSION */;
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(module, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

void DmrppRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DmrppRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

} // namespace dmrpp
