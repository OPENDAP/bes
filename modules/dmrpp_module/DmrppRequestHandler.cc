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

#include "NgapOwnedContainer.h"

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
using namespace ngap;

// These should be set in the Makefile.am. jhrg 11/23/24
#ifndef MODULE_NAME
#define MODULE_NAME "dmrpp_module"
#endif

#ifndef MODULE_VERSION
#define MODULE_VERSION "unset"      // Set this in the Makefile.am
#endif

#define prolog std::string("DmrppRequestHandler::").append(__func__).append("() - ")
#define dmrpp_cache "dmrpp:cache"

namespace dmrpp {

unique_ptr<ObjMemCache> DmrppRequestHandler::das_cache{nullptr};
unique_ptr<ObjMemCache> DmrppRequestHandler::dds_cache{nullptr};

shared_ptr<DMZ> DmrppRequestHandler::dmz{nullptr};

// This is used to maintain a pool of reusable curl handles that enable connection
// reuse. I tried making this a unique_ptr, but that caused tests to fail because
// the DmrppRequestHandler dtor did not free this object every time the handler
// was instantiated. That's not an issue for the BES, but it was/is for unit tests
// that make and delete many of these objects. jhrg 11/22/24
CurlHandlePool *DmrppRequestHandler::curl_handle_pool{nullptr};

// These now only affect the DDS and DAS ObjMemCaches; the DMR++
// is cached in the NGAP module. Once issues with the DMR++ object's
// copy constructor are solved, the ObjMemCache can be used again.
// jhrg 9/26/23
bool DmrppRequestHandler::d_use_object_cache = true;
unsigned int DmrppRequestHandler::d_object_cache_entries = 100;
double DmrppRequestHandler::d_object_cache_purge_level = 0.2;

bool DmrppRequestHandler::d_use_transfer_threads = false;
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

bool DmrppRequestHandler::is_netcdf4_enhanced_response = false;
bool DmrppRequestHandler::is_netcdf4_classic_response = false;

// The direct IO feature is turned on by default.
bool DmrppRequestHandler::disable_direct_io = false;

bool DmrppRequestHandler::use_buffer_chunk = true;
// There are methods in TheBESKeys that should be used instead of these.
// jhrg 9/26/23
static void read_key_value(const std::string &key_name, bool &key_value) {
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    if (key_found) {
        value = BESUtil::lowercase(value);
        key_value = (value == "true" || value == "yes");
    }
}

static void read_key_value(const std::string &key_name, unsigned int &key_value) {
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    if (key_found) {
        istringstream iss(value);
        iss >> key_value;
    }
}

static void read_key_value(const std::string &key_name, unsigned long long &key_value) {
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    if (key_found) {
        istringstream iss(value);
        iss >> key_value;
    }
}

static void read_key_value(const std::string &key_name, double &key_value) {
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
    if (DmrppRequestHandler::d_use_transfer_threads) {
        msg << "Enabled. max_transfer_threads: " << DmrppRequestHandler::d_max_transfer_threads << endl;
    }
    else {
        msg << "Disabled." << endl;
    }

    INFO_LOG(msg.str());
    msg.str(std::string());

    read_key_value(DMRPP_USE_COMPUTE_THREADS_KEY, d_use_compute_threads);
    read_key_value(DMRPP_MAX_COMPUTE_THREADS_KEY, d_max_compute_threads);
    msg << prolog << "Concurrent Compute Threads: ";
    if (DmrppRequestHandler::d_use_compute_threads) {
        msg << "Enabled. max_compute_threads: " << DmrppRequestHandler::d_max_compute_threads << endl;
    }
    else {
        msg << "Disabled." << endl;
    }

    INFO_LOG(msg.str());
    msg.str(std::string());

    // DMRPP_CONTIGUOUS_CONCURRENT_THRESHOLD_KEY
    read_key_value(DMRPP_CONTIGUOUS_CONCURRENT_THRESHOLD_KEY, d_contiguous_concurrent_threshold);
    msg << prolog << "Contiguous Concurrency Threshold: " << d_contiguous_concurrent_threshold << " bytes." << endl;
    INFO_LOG(msg.str());

    // Whether the default direct IO feature is disabled. Read the key in.
    read_key_value(DMRPP_DISABLE_DIRECT_IO, disable_direct_io);

    read_key_value(DMRPP_USE_BUFFER_CHUNK, use_buffer_chunk);

    // Check the value of FONc.ClassicModel to determine if this response is a netCDF-4 classic from fileout netCDF
    // This must be done here since direct IO flag for individual variables  should NOT be set for netCDF-4 classic response.
    read_key_value(DMRPP_USE_CLASSIC_IN_FILEOUT_NETCDF, is_netcdf4_classic_response);

#if !HAVE_CURL_MULTI_API
    if (DmrppRequestHandler::d_use_transfer_threads)
        ERROR_LOG("The DMR++ handler is configured to use parallel transfers, but the libcurl Multi API is not present, defaulting to serial transfers");
#endif

    if (!curl_handle_pool) {
        curl_handle_pool = new CurlHandlePool();
        curl_handle_pool->initialize();
    }

    // This can be set to true using the bes conf file; the default value is false
    read_key_value(DMRPP_USE_OBJECT_CACHE_KEY, d_use_object_cache);
    if (d_use_object_cache) {
        read_key_value(DMRPP_OBJECT_CACHE_ENTRIES_KEY, d_object_cache_entries);
        read_key_value(DMRPP_OBJECT_CACHE_PURGE_LEVEL_KEY, d_object_cache_purge_level);
        // The default value of these is nullptr
        dds_cache = make_unique<ObjMemCache>(d_object_cache_entries, d_object_cache_purge_level);
        das_cache = make_unique<ObjMemCache>(d_object_cache_entries, d_object_cache_purge_level);
    }

    // This and the matching cleanup function can be called many times as long as
    // they are called in balanced pairs. jhrg 9/3/20
    // TODO 10/8/21 move this into the http at the top level of the BES. That is, all
    //  calls to this should be moved out of handlers and handlers can/should
    //  assume that curl is present and init'd. jhrg
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

DmrppRequestHandler::~DmrppRequestHandler() {
    delete curl_handle_pool;
    // generally, this is not necessary, but for this to be used in the unit tests, where the DmrppRequestHandler
    // is made and destroyed many times, it is necessary. That is because the curl handle pool is a static pointer.
    // jhrg 11/22/24
    curl_handle_pool = nullptr;
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
 * This method builds the DMR++ in memory using the DMZ parser and a DMR++ document
 * that has been either read from a local file, a remote file or from a local cache.
 * When the contents are cached, they are cached as a string, and not a file. We did
 * that to avoid issues with multiple processes and threads and our existing cache
 * code. However, caching the DMR++ XML content as a string is also very performant.
 *
 * @note caching the DMR++ as a string may be only a stop-gap measure since caching
 * the DMR++ object would be faster still. jhrg 9/25/23
 *
 * @param container When run in the NASA Cloud, this is likely a NgapContainer; it can
 * be any container that references a DMR++ XML file. In the NGAP case, the server will
 * use the RemoteResources class to pull the DMR++ document into the local host.
 * @param request_xml_base The base URL for the request. Used when pulling the DMR++
 * from a cache.
 * @param dmr Value-result parameter. The DMR either built from the DMR++ XML file or
 * copied from the object cache.
 */
void DmrppRequestHandler::get_dmrpp_from_container_or_cache(BESContainer *container, DMR *dmr) {
    try {
        auto ngo_cont = dynamic_cast<ngap::NgapOwnedContainer*>(container);
        // If the container is an NGAP container (or maybe other types in the future),
        // the DMR++ itself might be returned as a string by access(). If the container
        // did not come from the NGAP handler, the return value might be a string that
        // names a file on the local host. jhrg 10/19/23
        string container_attributes = container->get_attributes();
        if (container_attributes == "as-string") {
            dmr->set_filename(container_attributes);
            dmr->set_name(name_path(container_attributes));

            // this shared_ptr is held by the DMRpp BaseType instances
            dmz = make_shared<DMZ>();

            // Enable adding the DMZ to the BaseTypes built by the factory
            DmrppTypeFactory factory(dmz);
            dmr->set_factory(&factory);

            string dmrpp_content = ngo_cont->alt_access();

            dmz->parse_xml_string(dmrpp_content);

            dmz->build_thin_dmr(dmr);

            BESDEBUG("dmrpp","Before calling set_up_all_direct_io_flags"<<endl);
            if (DmrppRequestHandler::is_netcdf4_enhanced_response == true &&
                DmrppRequestHandler::disable_direct_io == false) { 
                BESDEBUG("dmrpp","calling set_up_all_direct_io_flags"<<endl);
                bool global_dio_flag = dmz->set_up_all_direct_io_flags_phase_1(dmr);
                if (global_dio_flag) {
                    BESDEBUG("dmrpp","global_dio_flags is true."<<endl);
                    dmz->set_up_all_direct_io_flags_phase_2(dmr);
                }
            }

            dmz->load_all_attributes(dmr);
        }
        else {
            string data_pathname = container->access();
            dmr->set_filename(data_pathname);
            dmr->set_name(name_path(data_pathname));
            BESDEBUG(dmrpp_cache, prolog << "DMR Cache miss for : " << container->get_real_name() << endl);

            // this shared_ptr is held by the DMRpp BaseType instances
            dmz = make_shared<DMZ>();

            // Enable adding the DMZ to the BaseTypes built by the factory
            DmrppTypeFactory factory(dmz);
            dmr->set_factory(&factory);

            dmz->parse_xml_doc(data_pathname);

            dmz->build_thin_dmr(dmr);
            BESDEBUG("dmrpp","Before calling set_up_all_direct_io_flags: second"<<endl);
            if (DmrppRequestHandler::is_netcdf4_enhanced_response == true &&
                DmrppRequestHandler::disable_direct_io == false) { 
                BESDEBUG("dmrpp","calling set_up_all_direct_io_flags: second"<<endl);
                bool global_dio_flag = dmz->set_up_all_direct_io_flags_phase_1(dmr);
                if (global_dio_flag) {
                    BESDEBUG("dmrpp","global_dio_flags is true."<<endl);
                    dmz->set_up_all_direct_io_flags_phase_2(dmr);
                }
            }
 
            dmz->load_all_attributes(dmr);
        }
    }
    catch (...) {
        handle_exception(__FILE__, __LINE__);
    }
}

/**
 * @brief Build a DDS that is loaded with attributes
 *
 * @note The type of T should only ever be BESDDSResponse or BESDataDDSResponse!
 *
 * @tparam T BESResponseObject specialization (limited to BESDataDDSResponse, BESDDSResponse)
 * @param dhi The Data handler interface object
 * @param bdds A pointer to a BESDDSResponse or BESDataDDSResponse
 */
template<class T>
void DmrppRequestHandler::get_dds_from_dmr_or_cache(BESContainer *container, T *bdds) {
    string container_name_str = bdds->get_explicit_containers() ? container->get_symbolic_name() : "";

    // Since this must be a BESDDSResponse or BESDataDDSResponse, we know that a DDS
    // can be accessed.
    DDS *dds = bdds->get_dds();
    if (!container_name_str.empty()) dds->container_name(container_name_str);

    // Inserted new code here
    string filename = container->get_real_name();
    const DDS *cached_dds = nullptr;
    if (dds_cache && (cached_dds = dynamic_cast<DDS *>(dds_cache->get(filename)))) {
        BESDEBUG(dmrpp_cache, prolog << "DDS Cache hit for : " << filename << endl);
        // copy the cached DMR into the BES response object
        *dds = *cached_dds; // Copy the cached object
    }
    else {
        BESDEBUG(dmrpp_cache, prolog << "DDS Cache miss for : " << filename << endl);
        DMR dmr;
        get_dmrpp_from_container_or_cache(container, &dmr);

        delete dds;                         // delete the current one;
        dds = dmr.getDDS();                 // assign the new one.

        // Stuff it into the response so that the BESDDSResponse instances manages the storage.
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
bool DmrppRequestHandler::dap_build_dmr(BESDataHandlerInterface &dhi) {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdmr = dynamic_cast<BESDMRResponse *>(dhi.response_handler->get_response_object());
    if (!bdmr) throw BESInternalError("Cast error, expected a BESDMRResponse object.", __FILE__, __LINE__);

    try {
        get_dmrpp_from_container_or_cache(dhi.container, bdmr->get_dmr());

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
bool DmrppRequestHandler::dap_build_dap4data(BESDataHandlerInterface &dhi) {
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    auto bdmr = dynamic_cast<BESDMRResponse *>(dhi.response_handler->get_response_object());
    if (!bdmr) throw BESInternalError("Cast error, expected a BESDMRResponse object.", __FILE__, __LINE__);

    try {
        bool is_netcdf4_response = (dhi.data["return_command"] == "netcdf-4");

        DmrppRequestHandler::is_netcdf4_enhanced_response = is_netcdf4_response;
        if (DmrppRequestHandler::is_netcdf4_enhanced_response &&
            DmrppRequestHandler::is_netcdf4_classic_response)
            DmrppRequestHandler::is_netcdf4_enhanced_response = false;

        BESDEBUG(MODULE,
                 prolog << "netcdf4_enhanced_response: " << DmrppRequestHandler::is_netcdf4_enhanced_response << endl);

        BESDEBUG(MODULE, prolog << "netcdf4_classic_response: "
                                << (is_netcdf4_response && DmrppRequestHandler::is_netcdf4_classic_response) << endl);

        get_dmrpp_from_container_or_cache(dhi.container, bdmr->get_dmr());

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
bool DmrppRequestHandler::dap_build_dap2data(BESDataHandlerInterface &dhi) {
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
bool DmrppRequestHandler::dap_build_dds(BESDataHandlerInterface &dhi) {
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
bool DmrppRequestHandler::dap_build_das(BESDataHandlerInterface &dhi) {
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
        if (das_cache && (cached_das = static_cast<DAS *>(das_cache->get(filename)))) {
            BESDEBUG(dmrpp_cache, prolog << "DAS Cache hit for : " << filename << endl);
            // copy the cached DAS into the BES response object
            *das = *cached_das;
        }
        else {
            BESDEBUG(dmrpp_cache, prolog << "DAS Cache miss for : " << filename << endl);
            DMR dmr;
            get_dmrpp_from_container_or_cache(dhi.container, &dmr);

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


bool DmrppRequestHandler::dap_build_vers(BESDataHandlerInterface &dhi) {
    auto info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);
    return true;
}

bool DmrppRequestHandler::dap_build_help(BESDataHandlerInterface &dhi) {
    auto info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string, std::less<>> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(MODULE, services);
    if (!services.empty()) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

void DmrppRequestHandler::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "DmrppRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

} // namespace dmrpp
