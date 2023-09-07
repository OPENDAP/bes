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

#include <libdap/Ancillary.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/DAS.h>

#include <libdap/InternalErr.h>
#include <libdap/mime_util.h>  // for name_path

#include "BESResponseHandler.h"
#include "BESResponseNames.h"
#include "BESDapNames.h"
#include "BESDataNames.h"
#include "BESDASResponse.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESVersionInfo.h"
#include "BESContainer.h"
#include "ObjMemCache.h"

#include "BESDMRResponse.h"

#include "BESConstraintFuncs.h"
#include "BESServiceRegistry.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "TheBESKeys.h"

#include "BESDapError.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
#include "BESStopWatch.h"

#include "DapUtils.h"

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "DmrppNames.h"
#include "DmrppTypeFactory.h"
#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"
#include "CredentialsManager.h"

using namespace bes;
using namespace http;
using namespace libdap;
using namespace std;

#define MODULE_NAME "dmrpp_module"
#ifndef MODULE_VERSION
#define MODULE_VERSION "unset"      // Set this in the Makefile.am
#endif

#define prolog std::string("DmrppRequestHandler::").append(__func__).append("() - ")
#define dmrpp_cache "dmrpp:cache"

#define USE_DMZ_TO_MANAGE_XML 1

namespace dmrpp {

unique_ptr<ObjMemCache> DmrppRequestHandler::das_cache{nullptr};
unique_ptr<ObjMemCache> DmrppRequestHandler::dds_cache{nullptr};
unique_ptr<ObjMemCache> DmrppRequestHandler::dmr_cache{nullptr};

shared_ptr<DMZ> DmrppRequestHandler::dmz(nullptr);

// This is used to maintain a pool of reusable curl handles that enable connection
// reuse. jhrg
CurlHandlePool *DmrppRequestHandler::curl_handle_pool = nullptr;

bool DmrppRequestHandler::d_use_object_cache = true;
unsigned int DmrppRequestHandler::d_object_cache_entries = 100;
float DmrppRequestHandler::d_object_cache_purge_level = 0.2;

bool DmrppRequestHandler::d_use_transfer_threads = true;
unsigned int DmrppRequestHandler::d_max_transfer_threads = 8;

bool DmrppRequestHandler::d_use_compute_threads = true;
unsigned int DmrppRequestHandler::d_max_compute_threads = 8;

// Default minimum value is 2MB: 2 * (1024*1024)
unsigned long long DmrppRequestHandler::d_contiguous_concurrent_threshold = DMRPP_DEFAULT_CONTIGUOUS_CONCURRENT_THRESHOLD;

// This behavior mirrors the SAX2 parser behavior where the software doesn't require that
// a variable actually have chunks (that is, some variable might not have any data).
// We could make this a run-time option if needed. jhrg 11/4/21
bool DmrppRequestHandler::d_require_chunks = false;

// See the comment in the header for more about this kludge. jhrg 11/9/21
bool DmrppRequestHandler::d_emulate_original_filter_order_behavior = false;

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

static void read_key_value(const std::string &key_name, unsigned int &key_value)
{
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    if (key_found) {
        istringstream iss(value);
        iss >> key_value;
    }
}
static void read_key_value(const std::string &key_name, unsigned long long &key_value)
{
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    if (key_found) {
        istringstream iss(value);
        iss >> key_value;
    }
}

/**
 * Here we register all of our handler functions so that the BES Dispatch machinery
 * knows what kinds of things we handle.
 */
DmrppRequestHandler::DmrppRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_method(DMR_RESPONSE, dap_build_dmr);
    add_method(DAP4DATA_RESPONSE, dap_build_dap4data);
    add_method(DAS_RESPONSE, dap_build_das);
    add_method(DDS_RESPONSE, dap_build_dds);
    add_method(DATA_RESPONSE, dap_build_dap2data);

    add_method(VERS_RESPONSE, dap_build_vers);
    add_method(HELP_RESPONSE, dap_build_help);

    stringstream msg;
    read_key_value(DMRPP_USE_TRANSFER_THREADS_KEY, d_use_transfer_threads);
    read_key_value(DMRPP_MAX_TRANSFER_THREADS_KEY, d_max_transfer_threads);
    msg << prolog << "Concurrent Transfer Threads: ";
    if(DmrppRequestHandler::d_use_transfer_threads){
        msg << "Enabled. max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl;
    }
    else{
        msg << "Disabled." << endl;
    }

    INFO_LOG(msg.str() );
    msg.str(std::string());

    read_key_value(DMRPP_USE_COMPUTE_THREADS_KEY, d_use_compute_threads);
    read_key_value(DMRPP_MAX_COMPUTE_THREADS_KEY, d_max_compute_threads);
    msg << prolog << "Concurrent Compute Threads: ";
    if(DmrppRequestHandler::d_use_compute_threads){
        msg << "Enabled. max_compute_threads: " << DmrppRequestHandler::d_max_compute_threads << endl;
    }
    else{
        msg << "Disabled." << endl;
    }

    INFO_LOG(msg.str() );
    msg.str(std::string());

    // DMRPP_CONTIGUOUS_CONCURRENT_THRESHOLD_KEY
    read_key_value(DMRPP_CONTIGUOUS_CONCURRENT_THRESHOLD_KEY, d_contiguous_concurrent_threshold);
    msg << prolog << "Contiguous Concurrency Threshold: " << d_contiguous_concurrent_threshold << " bytes." << endl;
    INFO_LOG(msg.str() );

#if !HAVE_CURL_MULTI_API
    if (DmrppRequestHandler::d_use_transfer_threads)
        ERROR_LOG("The DMR++ handler is configured to use parallel transfers, but the libcurl Multi API is not present, defaulting to serial transfers");
#endif

    if (!curl_handle_pool)
        curl_handle_pool = new CurlHandlePool(d_max_transfer_threads);

    // dmr_cache = new ObjMemCache(100, 0.2);
    dmr_cache = make_unique<ObjMemCache>(100, 0.2);
    dds_cache = make_unique<ObjMemCache>(100, 0.2);
    das_cache = make_unique<ObjMemCache>(100, 0.2);

    // This and the matching cleanup function can be called many times as long as
    // they are called in balanced pairs. jhrg 9/3/20
    // TODO 10/8/21 move this into the http at the top level of the BES. That is, all
    //  calls to this should be moved out of handlers and handlers can/should
    //  assume that curl is present and init'd. jhrg
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

DmrppRequestHandler::~DmrppRequestHandler()
{
    delete curl_handle_pool;
    curl_global_cleanup();
}

/**
 * @brief function-try-block to reduce catch() clause duplication
 * @param file
 * @param line
 */
void
handle_exception(const string &file, int line)
    try {
        throw;
    }
    catch (const BESError &e) {
        throw;
    }
    catch (const InternalErr &e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), file, line);
    }
    catch (const Error &e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), file, line);
    }
    catch (const std::exception &e) {
        throw BESInternalFatalError(string("C++ exception: ").append(e.what()), file, line);
    }
    catch (...) {
        throw BESInternalFatalError("Unknown exception caught building DAP4 Data response", file, line);
    }

/**
 * @brief Get (maybe, if it's remote), parse, and build a DMR from a DMR++ XML file.
 *
 * @param container When run in the NASA Cloud, this is likely a NgapContainer; It can
 * be any container that references a DMR++ XML file. In the NGAP case, the server will
 * use the RemoteResources class to pull the DMR++ document into the local host.
 * @param request_xml_base The base URL for the request. Used when pulling the DMR++
 * from a cache.
 * @param dmr Value-result parameter. The DMR either built from the DMR++ XML file or
 * copied from the object cache.
 */
void DmrppRequestHandler::get_dmrpp_from_container_or_cache(BESContainer *container,
                                                            const string &request_xml_base,
                                                            DMR *dmr)
{
    try {
        string filename = container->get_real_name();
        DMR* cached_dmr = nullptr;
        if (dmr_cache && (cached_dmr = dynamic_cast<DMR*>(dmr_cache->get(filename)))) {
            BESDEBUG(dmrpp_cache, prolog << "DMR Cache hit for : " << filename << endl);
            // copy the cached DMR into the BES response object
            *dmr = *cached_dmr; // Copy the cached object
            dmr->set_request_xml_base(request_xml_base);
        }
        else {  // Not cached (or maybe no cache)
            BESDEBUG(dmrpp_cache, prolog << "DMR Cache miss for : " << filename << endl);
            string data_pathname = container->access();

            dmr->set_filename(data_pathname);
            dmr->set_name(name_path(data_pathname));

            // this shared_ptr is held by the DMRpp BaseType instances
            dmz = shared_ptr<DMZ>(new DMZ);

            // Enable adding the DMZ to the BaseTypes built by the factory
            DmrppTypeFactory factory(dmz);
            dmr->set_factory(&factory);

            dmz->parse_xml_doc(data_pathname);
            dmz->build_thin_dmr(dmr);

            dmz->load_all_attributes(dmr);

            if (dmr_cache) {
                // Cache a copy of the DMR.
                dmr_cache->add(new DMR(*dmr), filename);
            }
        }
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }
}

/**
 * @brief Build a DDS that is loaded with attributes
 * @tparam T BESResponseObject specialization (limited to BESDataDDSResponse, BESDDSResponse)
 * @param dhi The Data handler interface object
 * @param bdds A pointer to a BESDDSResponse or BESDataDDSResponse
 */
template <class T>
void DmrppRequestHandler::get_dds_from_dmr_or_cache(BESContainer *container, T *bdds) {
    string container_name_str = bdds->get_explicit_containers() ? container->get_symbolic_name() : "";

    DDS *dds = bdds->get_dds();
    if (!container_name_str.empty()) dds->container_name(container_name_str);

    // Inserted new code here
    string filename = container->get_real_name();
    DDS *cached_dds = nullptr;
    if (dds_cache && (cached_dds = dynamic_cast<DDS *>(dds_cache->get(filename)))) {
        BESDEBUG(dmrpp_cache, prolog << "DDS Cache hit for : " << filename << endl);
        // copy the cached DMR into the BES response object
        *dds = *cached_dds; // Copy the cached object
    }
    else {
        BESDEBUG(dmrpp_cache, prolog << "DDS Cache miss for : " << filename << endl);
        DMR dmr;
        get_dmrpp_from_container_or_cache(container, bdds->get_request_xml_base(), &dmr);

        delete dds;                         // delete the current one;
        dds = dmr.getDDS();                 // assign the new one.

        // Stuff it into the response.
        bdds->set_dds(dds);

        // Cache it, if the cache is active.
        if (dds_cache) {
            dds_cache->add(new DDS(*dds), filename);
        }
    }
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
 * @return Always returns true
 * @throw BESError, libdap::InternalErr, libdap::Error
 */
bool DmrppRequestHandler::dap_build_dmr(BESDataHandlerInterface &dhi)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdmr = dynamic_cast<BESDMRResponse *>(dhi.response_handler->get_response_object());
    if (!bdmr) throw BESInternalError("Cast error, expected a BESDMRResponse object.", __FILE__, __LINE__);

    try {
        get_dmrpp_from_container_or_cache(dhi.container, bdmr->get_request_xml_base(), bdmr->get_dmr());

        bdmr->set_dap4_constraint(dhi);
        bdmr->set_dap4_function(dhi);
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
    return true;
}

/**
 * @brief Build a DAP4 data response. Adds timing to dap_build_dmr()
 * @param dhi
 * @return
 */
bool DmrppRequestHandler::dap_build_dap4data(BESDataHandlerInterface &dhi)
{
#ifndef NDEBUG
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "timer" , dhi.data[REQUEST_ID]);
#endif

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdmr = dynamic_cast<BESDMRResponse *>(dhi.response_handler->get_response_object());
    if (!bdmr) throw BESInternalError("Cast error, expected a BESDMRResponse object.", __FILE__, __LINE__);

    try {
        get_dmrpp_from_container_or_cache(dhi.container, bdmr->get_request_xml_base(), bdmr->get_dmr());

        bdmr->set_dap4_constraint(dhi);
        bdmr->set_dap4_function(dhi);
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
    return true;
}


/**
 * Produce a DAP2 Data Response (.dods) response from a DMRPP file.
 */
bool DmrppRequestHandler::dap_build_dap2data(BESDataHandlerInterface & dhi)
{
#ifndef NDEBUG
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "timer" , dhi.data[REQUEST_ID]);
#endif

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdds = dynamic_cast<BESDataDDSResponse *>(dhi.response_handler->get_response_object());
    if (!bdds) throw BESInternalError("Cast error, expected a BESDataDDSResponse object.", __FILE__, __LINE__);

    try {
        get_dds_from_dmr_or_cache<BESDataDDSResponse>(dhi.container, bdds);

        bdds->set_constraint(dhi);
        bdds->clear_container();
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
    return true;
}


/**
 * Produce a DAP2 DDS response from a DMRPP file.
 */
bool DmrppRequestHandler::dap_build_dds(BESDataHandlerInterface & dhi)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdds = dynamic_cast<BESDDSResponse *>(dhi.response_handler->get_response_object());
    if (!bdds) throw BESInternalError("Cast error, expected a BESDDSResponse object.", __FILE__, __LINE__);

    try {
        get_dds_from_dmr_or_cache<BESDDSResponse>(dhi.container, bdds);

        bdds->set_constraint(dhi);
        bdds->clear_container();
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
    return true;
}

/**
 * Produce a DAP2 DAS response from a DMRPP data set. This is a little messy.
 *
 */
bool DmrppRequestHandler::dap_build_das(BESDataHandlerInterface & dhi)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdas = dynamic_cast<BESDASResponse *>(dhi.response_handler->get_response_object());
    if (!bdas) throw BESInternalError("Cast error, expected a BESDASResponse object.", __FILE__, __LINE__);

    try {
        string container_name = bdas->get_explicit_containers() ? dhi.container->get_symbolic_name() : "";

        DAS *das = bdas->get_das();
        if (!container_name.empty()) das->container_name(container_name);

        string filename = dhi.container->get_real_name();
        // Look in memory cache (if it's initialized)
        const DAS *cached_das = nullptr;
        if (das_cache && (cached_das = static_cast<DAS*>(das_cache->get(filename)))) {
            BESDEBUG(dmrpp_cache, prolog << "DAS Cache hit for : " << filename << endl);
            // copy the cached DAS into the BES response object
            *das = *cached_das;
        }
        else {
            BESDEBUG(dmrpp_cache, prolog << "DAS Cache miss for : " << filename << endl);
            DMR dmr;
            get_dmrpp_from_container_or_cache(dhi.container, bdas->get_request_xml_base(), &dmr);

            // Get a DDS from the DMR, getDDS() allocates all new objects. Use unique_ptr
            // to ensure this is deleted. jhrg 11/12/21
            unique_ptr<DDS> dds(dmr.getDDS());

            dds->mark_all(true);
            dap_utils::throw_for_dap4_typed_vars_or_attrs(dds.get(), __FILE__, __LINE__);

            // Load the BESDASResponse DAS from the DDS
            dds->get_das(das);

            // I'm not sure that this makes much sense, but there could be a local DMR++ and
            // it could have ancillary das info.
            Ancillary::read_ancillary_das(*das, filename);
            
            // Add to cache if cache is active
            if (das_cache) {
                 das_cache->add(new DAS(*das), filename);
            }
        }

        bdas->clear_container();
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }

    BESDEBUG(MODULE, prolog << "END" << endl);
    return true;
}


bool DmrppRequestHandler::dap_build_vers(BESDataHandlerInterface &dhi)
{
    auto info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);
    return true;
}

bool DmrppRequestHandler::dap_build_help(BESDataHandlerInterface &dhi)
{
    auto info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(MODULE, services);
    if (!services.empty()){
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
