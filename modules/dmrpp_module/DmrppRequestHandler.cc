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

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESTextInfo.h>
#include <BESDapNames.h>

#include <BESDMRResponse.h>

#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDebug.h>

#include <DMR.h>
#include <D4Group.h>

#include <InternalErr.h>
#include <mime_util.h>  // for name_path

#include "DmrppTypeFactory.h"
#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"

using namespace libdap;
using namespace std;

const string module = "dmrpp";

#if 0
static void read_key_value(const std::string &key_name, bool &key_value, bool &is_key_set)
{
    if (is_key_set == false) {
        bool key_found = false;
        string doset;
        TheBESKeys::TheKeys()->get_value(key_name, doset, key_found);
        if (key_found) {
            // It was set in the conf file
            is_key_set = true;

            doset = BESUtil::lowercase(doset);
            key_value = (doset == "true" || doset == "yes");
        }
    }
}

static bool extension_match(const string &data_source, const string &extension)
{
    string::size_type pos = data_source.rfind(extension);
    return pos != string::npos && pos + extension.length() == data_source.length();
}
#endif

/**
 * Here we register all of our handler functions so that the BES Dispatch machinery
 * knows what kinds of things we handle.
 */
DmrppRequestHandler::DmrppRequestHandler(const string &name) :
        BESRequestHandler(name)
{
#if DAP2
    add_handler(DAS_RESPONSE, dap_build_das);
    add_handler(DDS_RESPONSE, dap_build_dds);
    add_handler(DATA_RESPONSE, dap_build_data);
#endif

    add_handler(DMR_RESPONSE, dap_build_dmr);
    add_handler(DAP4DATA_RESPONSE, dap_build_dap4data);

    add_handler(VERS_RESPONSE, dap_build_vers);
    add_handler(HELP_RESPONSE, dap_build_help);

#if 0
    read_key_value("DR.UseTestTypes", d_use_test_types, d_use_test_types_set);
    read_key_value("DR.UseSeriesValues", d_use_series_values, d_use_series_values_set);
#endif
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
    parser.intern(in, dmr);

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

