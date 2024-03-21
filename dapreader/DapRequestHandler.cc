// DapRequestHandler.cc

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
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

#include "DapRequestHandler.h"

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESTextInfo.h>
#include <BESDapNames.h>

#include <BESDataDDSResponse.h>
#include <BESDDSResponse.h>
#include <BESDASResponse.h>
#include <BESDMRResponse.h>

#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESDebug.h>

#include <libdap/BaseTypeFactory.h>
#include <test/TestTypeFactory.h>
#include <libdap/D4BaseTypeFactory.h>
#include <test/D4TestTypeFactory.h>
#include <test/TestCommon.h>

#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Connect.h>
#include <libdap/D4ParserSax2.h>

#include <libdap/Ancillary.h>
#include <libdap/Connect.h>
#include <libdap/Response.h>
#include <libdap/InternalErr.h>
#include <libdap/mime_util.h>

using namespace libdap;

int test_variable_sleep_interval = 0;

bool DapRequestHandler::d_use_series_values = true;
bool DapRequestHandler::d_use_series_values_set = false;

bool DapRequestHandler::d_use_test_types = true;
bool DapRequestHandler::d_use_test_types_set = false;

const string module = "dapreader";

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
    return pos != string::npos && pos + extension.size() == data_source.size();
}

DapRequestHandler::DapRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_method(DAS_RESPONSE, dap_build_das);
    add_method(DDS_RESPONSE, dap_build_dds);
    add_method(DATA_RESPONSE, dap_build_data);

    add_method(DMR_RESPONSE, dap_build_dmr);
    add_method(DAP4DATA_RESPONSE, dap_build_dap4data);

    add_method(VERS_RESPONSE, dap_build_vers);
    add_method(HELP_RESPONSE, dap_build_help);

    read_key_value("DR.UseTestTypes", d_use_test_types, d_use_test_types_set);
    read_key_value("DR.UseSeriesValues", d_use_series_values, d_use_series_values_set);
}

/** Read values from a DAP2 or DAP4 response
 *
 * @param accessed File that holds a 'frozen' DAP2 or DAP4 data response
 * @param dds Value-result parameter; this DDS is loaded with new variables
 * with values from the DAP2/4 data response.
 */
void DapRequestHandler::load_dds_from_data_file(const string &accessed, DDS &dds)
{
    BESDEBUG("dapreader", "In DapRequestHandler::load_dds_from_data_file; accessed: " << accessed << endl);

    TestTypeFactory t_factory;
    BaseTypeFactory b_factory;
    if (d_use_test_types)
        dds.set_factory(&t_factory);   
        //valgrind shows the leaking caused by the following line. KY 2019-12-12
        //dds.set_factory(new TestTypeFactory);   // DDS deletes the factory
    else
        dds.set_factory(&b_factory);
        //valgrind shows the leaking caused by the following line. KY 2019-12-12
        //dds.set_factory(new BaseTypeFactory);

    unique_ptr<Connect> url(new Connect(accessed));
    Response r(fopen(accessed.c_str(), "r"), 0);
    if (!r.get_stream()) throw Error(string("The input source: ") + accessed + string(" could not be opened"));
    url->read_data_no_mime(dds, &r);

    unique_ptr<DAS> das(new DAS);
    Ancillary::read_ancillary_das(*das, accessed);

    if (das->get_size() > 0) dds.transfer_attributes(das.get());

    // This is needed for the values read to show up. Without it the default
    // behavior of the TestTypes will take over and the values from the data files
    // will be ignored.
    for (auto i = dds.var_begin(), e = dds.var_end(); i != e; i++) {
        (*i)->set_read_p(true);
    }
}

/** Given a .dds, .dods or .data file, build a DDS/DataDDS
 *
 * @param accessed The name of the Data file, a DAP2/DAP4 data response
 * @param use_containers True if the 'explicit containers' context is set
 * @param dds Value-result parameter. This should be the DDS bound to the
 * BESDDSResponseObject or BESDDSDataResponseObject
 */
void DapRequestHandler::build_dds_from_file(const string &accessed, bool explicit_containers, DDS *dds)
{
    BESDEBUG("dapreader", "In DapRequestHandler::build_dds_from_file; accessed: " << accessed << endl);

    if (extension_match(accessed, ".dds") && d_use_test_types) {
        dds->set_factory(new TestTypeFactory);
        dds->parse(accessed);   // This sets the dataset name based on what's in the file

        DAS *das = new DAS;
        Ancillary::read_ancillary_das(*das, accessed);

        if (das->get_size() > 0) dds->transfer_attributes(das);
    }
    else if (extension_match(accessed, ".dods") || extension_match(accessed, ".data")) {
        if (explicit_containers) {
            BESDEBUG("dapreader", "In DapRequestHandler::build_dds_from_file; in container code" << endl);
            DDS local_dds(0);

            // This function reads from a .dods, ..., 'frozen response' and loads
            // the values into a DDS's variables. It then merges the Attributes read
            // from a matching .das file into those variables. The code in Connect
            // that reads the values is not 'container safe' so we use this function
            // to read value into a 'local dds' and then transfer its variables to
            // the real BESDDSResponseObject, which is the DDS passed to this function
            load_dds_from_data_file(accessed, local_dds);

            // Transfer variables just read into BESDDSResponse/BESDataDDSResponse's DDS
            for (DDS::Vars_iter i = local_dds.var_begin(), e = local_dds.var_end(); i != e; i++) {
                dds->add_var((*i)); // copy the variables; figure out how to not copy them
            }

            dds->set_dataset_name(name_path(accessed));
        }
        else {
            BESDEBUG("dapreader", "In DapRequestHandler::build_dds_from_file; in plain code" << endl);
            // In the non-container case, reading the values is pretty straightforward
            load_dds_from_data_file(accessed, *dds);
        }

        dds->filename(accessed);
    }
    else {
        throw Error("The dapreader module can only return DDS/DODS responses for files ending in .dods, .data or .dds");
    }

    BESDEBUG("dapreader2", "DDS/DDX in DapRequestHandler::build_dds_from_file: ");
    if (BESDebug::IsSet("dapreader2")) dds->print_xml(*(BESDebug::GetStrm()), false);
}

void DapRequestHandler::build_dmr_from_file(const string& accessed, bool explicit_containers, DMR* dmr)
{
    BESDEBUG("dapreader", "In DapRequestHandler::build_dmr_from_file; accessed: " << accessed << endl);

    dmr->set_filename(accessed);
    dmr->set_name(name_path(accessed));

    D4TestTypeFactory TestFactory;
    D4BaseTypeFactory BaseFactory;
    if (d_use_test_types) {
        dmr->set_factory(&TestFactory);
    }
    else {
        dmr->set_factory(&BaseFactory);
    }

    if ((extension_match(accessed, ".dmr") || extension_match(accessed, ".xml")) && d_use_test_types) {
        D4ParserSax2 parser;
        ifstream in(accessed.c_str(), ios::in);
        parser.intern(in, dmr);
    }
    else if (extension_match(accessed, ".dap")) {
        unique_ptr<D4Connect> url(new D4Connect(accessed));
        fstream f(accessed.c_str(), std::ios_base::in);
        if (!f.is_open() || f.bad() || f.eof()) throw Error((string) ("Could not open: ") + accessed);

        Response r(&f, 0);
        // use the read_data...() method because we need to process the special
        // binary glop in the data responses.
        url->read_data_no_mime(*dmr, r);
    }
    else if (extension_match(accessed, ".dds") || extension_match(accessed, ".dods")
            || extension_match(accessed, ".data")) {

        unique_ptr<DDS> dds(new DDS(0 /*factory*/));

        build_dds_from_file(accessed, explicit_containers, dds.get());

        dmr->build_using_dds(*dds);
    }
    else {
        dmr->set_factory(nullptr);
        throw Error("The dapreader module can only return DMR/DAP responses for files ending in .dmr, .xml or .dap");
    }

    dmr->set_factory(nullptr);
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
bool DapRequestHandler::dap_build_dmr(BESDataHandlerInterface &dhi)
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
bool DapRequestHandler::dap_build_dap4data(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_dap4data..." << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(response);
    if (!bdmr) throw BESInternalError("BESDMRResponse cast error", __FILE__, __LINE__);

    try {
        DMR *dmr = bdmr->get_dmr();
        build_dmr_from_file(dhi.container->access(), bdmr->get_explicit_containers(), dmr);

        if (d_use_series_values) {
            dmr->root()->set_read_p(false);

            TestCommon *tc = dynamic_cast<TestCommon*>(dmr->root());
            if (tc)
                tc->set_series_values(true);
            else
                throw Error("In the reader handler: Could not set UseSeriesValues");
        }

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
 * This method will look at the extension on the input file and assume
 * that if it's .das, that file should be read and used to build the DAS
 * object. If it's .data or .dods, then the _ancillary_ das file will be used.
 * @param dhi
 * @return
 */
bool DapRequestHandler::dap_build_das(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(response);
    if (!bdas) throw BESInternalError("DAS cast error", __FILE__, __LINE__);
    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();
        string accessed = dhi.container->access();

        if (extension_match(accessed, ".das")) {
            das->parse(accessed);
        }
        else if (extension_match(accessed, ".dods") || extension_match(accessed, ".data")) {
            Ancillary::read_ancillary_das(*das, accessed);
        }
        else {
            throw Error(
                    "The dapreader module can only return DAS responses for files ending in .das or .dods/.data.\nIn the latter case there must be an ancillary das file present.");
        }

        bdas->clear_container();
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
        throw BESInternalFatalError("Unknown exception caught building DAS", __FILE__, __LINE__);
    }

    return true;
}


bool DapRequestHandler::dap_build_dds(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_dds..." << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(response);
    if (!bdds) throw BESInternalError("DDS cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        build_dds_from_file(dhi.container->access(), bdds->get_explicit_containers(), bdds->get_dds());

        bdds->set_constraint(dhi);
        bdds->clear_container();
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
        throw BESInternalFatalError("Unknown exception caught building DDS", __FILE__, __LINE__);
    }

    BESDEBUG(module, "Exiting dap_build_dds..." << endl);

    return true;
}

bool DapRequestHandler::dap_build_data(BESDataHandlerInterface &dhi)
{
    BESDEBUG(module, "Entering dap_build_data..." << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds) throw BESInternalError("DDS cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        build_dds_from_file(dhi.container->access(), bdds->get_explicit_containers(), bdds->get_dds());

        bdds->set_constraint(dhi);
        bdds->clear_container();
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
        throw BESInternalFatalError("Unknown exception caught building a data response", __FILE__, __LINE__);
    }

    BESDEBUG(module, "Exiting dap_build_data..." << endl);

    return true;
}

#if 0
void DapRequestHandler::add_attributes(BESDataHandlerInterface &dhi) {

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();
    string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
    string dataset_name = dhi.container->access();
    DAS* das = 0;
    {
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
            BESDEBUG(NC_NAME, "DAS added to the cache for : " << dataset_name << endl);
            das_cache->add(das, dataset_name);
        }
        else {
            delete das;
        }
    }

    BESDEBUG(NC_NAME, "Data ACCESS in add_attributes(): set the including attribute flag to true: "<<dataset_name << endl);
    bdds->set_ia_flag(true);
    return;
}
#endif

bool DapRequestHandler::dap_build_vers(BESDataHandlerInterface &dhi)
{
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    info->add_module(DAPREADER_PACKAGE, DAPREADER_VERSION);
    return true;
}

bool DapRequestHandler::dap_build_help(BESDataHandlerInterface &dhi)
{
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw BESInternalFatalError("Expected a BESVersionInfo instance.", __FILE__, __LINE__);

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string, std::less<>> attrs;
    attrs["name"] = DAPREADER_PACKAGE /* PACKAGE_NAME */;
    attrs["version"] = DAPREADER_VERSION /* PACKAGE_VERSION */;
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

void DapRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DapRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

