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

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESVersionInfo.h>
#include <BESContainer.h>
#include <ObjMemCache.h>

#include <BESDMRResponse.h>

#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESLog.h>
#include <TheBESKeys.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDebug.h>
#include <BESStopWatch.h>

#include "DmrppNames.h"
//#include "DMRpp.h"
#include "DmrppTypeFactory.h"
//#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"
//#include "DmrppMetadataStore.h"
#include "CredentialsManager.h"

using namespace bes;
using namespace libdap;
using namespace std;

#define MODULE_NAME "dmrpp_module"
#ifndef MODULE_VERSION
#define MODULE_VERSION "unset"      // Set this in the Makefile.am
#endif

#define prolog std::string("DmrppRequestHandler::").append(__func__).append("() - ")

#define USE_DMZ_TO_MANAGE_XML 1

namespace dmrpp {

ObjMemCache *DmrppRequestHandler::das_cache = 0;
ObjMemCache *DmrppRequestHandler::dds_cache = 0;
ObjMemCache *DmrppRequestHandler::dmr_cache = 0;

shared_ptr<DMZ> DmrppRequestHandler::dmz(nullptr);

// This is used to maintain a pool of reusable curl handles that enable connection
// reuse. jhrg
CurlHandlePool *DmrppRequestHandler::curl_handle_pool = 0;

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

    CredentialsManager::theCM()->load_credentials();

    if (!curl_handle_pool)
        curl_handle_pool = new CurlHandlePool(d_max_transfer_threads);

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
    catch (BESError &e) {
        throw e;
    }
    catch (InternalErr &e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), file, line);
    }
    catch (Error &e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), file, line);
    }
    catch (std::exception &e) {
        BESInternalFatalError(string("C++ exception: ").append(e.what()), file, line);
    }
    catch (...) {
        throw BESInternalFatalError("Unknown exception caught building DAP4 Data response", file, line);
    }


void DmrppRequestHandler::build_dmr_from_file(BESContainer *container, DMR* dmr)
{
    string data_pathname = container->access();

    dmr->set_filename(data_pathname);
    dmr->set_name(name_path(data_pathname));

#if USE_DMZ_TO_MANAGE_XML
    dmz = shared_ptr<DMZ>(new DMZ);

    // Enable adding the DMZ to the BaseTypes built by the factory
    DmrppTypeFactory BaseFactory(dmz);
    dmr->set_factory(&BaseFactory);

    dmz->parse_xml_doc(data_pathname);
    dmz->build_thin_dmr(dmr);
#else
    DmrppTypeFactory BaseFactory;   // Use the factory for this handler's types
    dmr->set_factory(&BaseFactory);

    DmrppParserSax2 parser;
    ifstream in(data_pathname.c_str(), ios::in);
    parser.intern(in, dmr);

    dmr->set_factory(0);
#endif
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

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(response);
    if (!bdmr) throw BESInternalError("Cast error, expected a BESDMRResponse object.", __FILE__, __LINE__);

    try {
        build_dmr_from_file(dhi.container, bdmr->get_dmr());

        dmz->load_all_attributes(bdmr->get_dmr());

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
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "timer" , dhi.data[REQUEST_ID]);

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(response);
    if (!bdmr) throw BESInternalError("Cast error, expected a BESDMRResponse object.", __FILE__, __LINE__);

    try {
        build_dmr_from_file(dhi.container, bdmr->get_dmr());

        // We don't need all the attributes, so use the lazy-load feature implemented
        // using overloads of the BaseType::set_send_p() method.
        dmz->load_global_attributes(bdmr->get_dmr());

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
 * @brief Build a DDS that is loaded with attributes
 * @tparam T BESResponseObject specialization (BESDataDDSResponse, BESDDSResponse)
 * @param dhi The Data handler interface object
 * @param bdds A pointer to a BESDDSResponse or BESDataDDSResponse
 */
template <class T>
void DmrppRequestHandler::get_dds_from_dmr_or_cache(BESDataHandlerInterface &dhi, T *bdds) {
    string container_name_str = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name() : "";

    DDS *dds = bdds->get_dds();
    if (!container_name_str.empty()) dds->container_name(container_name_str);
    string accessed = dhi.container->access();

    // Look in memory cache, if it's initialized
    DDS *cached_dds_ptr = 0;
    if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(accessed)))) {
        BESDEBUG(MODULE, prolog << "DDS Cached hit for : " << accessed << endl);
        *dds = *cached_dds_ptr;
    }
    else {
        DMR dmr;
        build_dmr_from_file(dhi.container, &dmr);
        dmz->load_all_attributes(&dmr);

        delete dds;                         // delete the current one;
        dds = dmr.getDDS();                 // assign the new one.

        // Stuff it into the response.
        bdds->set_dds(dds);

        // Cache it, if the cache is active.
        if (dds_cache) {
            dds_cache->add(new DDS(*dds), accessed);
        }
    }
}

/**
 * Produce a DAP2 Data Response (.dods) response from a DMRPP file.
 */
bool DmrppRequestHandler::dap_build_dap2data(BESDataHandlerInterface & dhi)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "timer" , dhi.data[REQUEST_ID]);

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds) throw BESInternalError("Cast error, expected a BESDataDDSResponse object.", __FILE__, __LINE__);

    try {
        get_dds_from_dmr_or_cache<BESDataDDSResponse>(dhi, bdds);
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
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "timer" , dhi.data[REQUEST_ID]);

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(response);
    if (!bdds) throw BESInternalError("Cast error, expected a BESDDSResponse object.", __FILE__, __LINE__);

    try {
        get_dds_from_dmr_or_cache<BESDDSResponse>(dhi, bdds);

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
 * Produce a DAP2 DAS response from a DMRPP data set.
 *
 */
bool DmrppRequestHandler::dap_build_das(BESDataHandlerInterface & dhi)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start(prolog + "timer" , dhi.data[REQUEST_ID]);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(response);
    if (!bdas) throw BESInternalError("Cast error, expected a BESDASResponse object.", __FILE__, __LINE__);

    try {
        string container_name_str = bdas->get_explicit_containers() ? dhi.container->get_symbolic_name() : "";

        DAS *das = bdas->get_das();
        if (!container_name_str.empty()) das->container_name(container_name_str);
        string accessed = dhi.container->access();

        // Look in memory cache (if it's initialized)
        DAS *cached_das_ptr = 0;
        if (das_cache && (cached_das_ptr = static_cast<DAS*>(das_cache->get(accessed)))) {
            // copy the cached DAS into the BES response object
            *das = *cached_das_ptr;
        }
        else {
            DMR dmr;
            build_dmr_from_file(dhi.container, &dmr);
            dmz->load_all_attributes(&dmr);

            // Get a DDS from the DMR, getDDS() allocates all new objects. Use unique_ptr
            // to ensure this is deleted. jhrg 11/12/21
            // TODO Add a getDAS() method to DMR so we don't have to go the long way?
            //  Or not and drop the DAP2 stuff until the code is higher up the chain?
            //  jhrg 11/12/21
            unique_ptr<DDS> dds(dmr.getDDS());

            // Load the BESDASResponse DAS from the DDS
            dds->get_das(das);
            Ancillary::read_ancillary_das(*das, accessed);
            
            // Add to cache if cache is active
            if (das_cache) {
                // copy because the BES deletes the DAS held by the DHI.
                // TODO Change the DHI to use shared_ptr objects. I think ... jhrg 11/12/21
                das_cache->add(new DAS(*das), accessed);
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
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(MODULE, services);
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
