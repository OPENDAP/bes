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

#include <BaseTypeFactory.h>
#include <test/TestTypeFactory.h>
#include <D4BaseTypeFactory.h>
#include <test/D4TestTypeFactory.h>

#include <DMR.h>
#include <D4ParserSax2.h>

#include <Ancillary.h>
#include <Connect.h>
#include <Response.h>
#include <InternalErr.h>
#include <mime_util.h>

using namespace libdap;

int test_variable_sleep_interval = 0;

bool DapRequestHandler::d_use_series_values = true;
bool DapRequestHandler::d_use_series_values_set = false;

bool DapRequestHandler::d_use_test_types = true;
bool DapRequestHandler::d_use_test_types_set = false;

void DapRequestHandler::read_key_value(const std::string &key_name, bool &key_value, bool &is_key_set)
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

DapRequestHandler::DapRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_handler(DAS_RESPONSE, dap_build_das);
    add_handler(DDS_RESPONSE, dap_build_dds);
    add_handler(DATA_RESPONSE, dap_build_data);

    add_handler(DMR_RESPONSE, dap_build_dmr);
    add_handler(DAP4DATA_RESPONSE, dap_build_dap4data);

    add_handler(VERS_RESPONSE, dap_build_vers);
    add_handler(HELP_RESPONSE, dap_build_help);

    read_key_value("DR.UseTestTypes", d_use_test_types, d_use_test_types_set);
    read_key_value("DR.UseSeriesValues", d_use_series_values, d_use_series_values_set);
}

const string module = "dapreader";
// #include "XMLWriter.h"

// handle the DAP4 requests
bool DapRequestHandler::dap_build_dmr(BESDataHandlerInterface &dhi)
{
	BESDEBUG(module, "Entering dap_build_dmr..." << endl);

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse *bdmr = dynamic_cast<BESDMRResponse *>(response);
    if (!bdmr)
        throw BESInternalError("DMR cast error", __FILE__, __LINE__);

	try {
		// DMRs don't support containers yet - use groups? jhrg 11/6/13
		// bdmr->set_container(dhi.container->get_symbolic_name());
		DMR *dmr = bdmr->get_dmr();
		string accessed = dhi.container->access();
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

		D4ParserSax2 parser;
		ifstream in(accessed.c_str(), ios::in);
		parser.intern(in, dmr);
		dmr->set_factory(0);
#if 0
		XMLWriter xml;
		dmr->print_dap4(xml);
		cerr << "DMR: " << xml.get_doc() << endl;
#endif
		bdmr->set_constraint(dhi);
		// bdmr->clear_container(); FIXME see above
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

	BESDEBUG(module, "Leaving dap_build_dmr..." << endl);

    return true;
}

// FIXME Added DAP4 data response support
bool DapRequestHandler::dap_build_dap4data(BESDataHandlerInterface &)
{
	return false;
}

// Handle the DAP2 requests
bool DapRequestHandler::dap_build_das(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(response);
    if (!bdas)
        throw BESInternalError("DAS cast error", __FILE__, __LINE__);
    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();
        string accessed = dhi.container->access();
        das->parse(accessed);
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
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("DDS cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();
        string accessed = dhi.container->access();
        dds->filename(accessed);
        dds->set_dataset_name(name_path(accessed));

        if (d_use_test_types) {
			TestTypeFactory factory;
			dds->set_factory(&factory);
			dds->parse(accessed);
			dds->set_factory(0);
		}
		else {
			BaseTypeFactory factory;
			dds->set_factory(&factory);
			dds->parse(accessed);
			dds->set_factory(0);
		}

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        Ancillary::read_ancillary_das(*das, accessed);

        dds->transfer_attributes(das);

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

    return true;
}

bool DapRequestHandler::dap_build_data(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("DDS cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DataDDS *dds = bdds->get_dds();
        string accessed = dhi.container->access();
        dds->filename(accessed);
        dds->set_dataset_name(name_path(accessed));

        Connect *url = new Connect(accessed);
        Response *r = new Response(fopen(accessed.c_str(), "r"), 0);

        if (!r->get_stream())
            throw Error(string("The input source: ") + accessed + string(" could not be opened"));

        if (d_use_test_types) {
			TestTypeFactory factory;
			dds->set_factory(&factory);
			url->read_data_no_mime(*dds, r);
			dds->set_factory(0);
		}
		else {
			BaseTypeFactory factory;
			dds->set_factory(&factory);
			url->read_data_no_mime(*dds, r);
			dds->set_factory(0);
		}
#if 0
        BaseTypeFactory factory;
        dds->set_factory(&factory);
        dds->filename(accessed);
        dds->set_dataset_name(name_path(accessed));
#endif
#if 0
        Connect *url = new Connect(accessed);
        Response *r = new Response(fopen(accessed.c_str(), "r"), 0);

        if (!r->get_stream())
            throw Error(string("The input source: ") + accessed + string(" could not be opened"));

        url->read_data_no_mime(*dds, r);
        dds->set_factory(0);
#endif
        // mark everything as read.
        DDS::Vars_iter i = dds->var_begin();
        DDS::Vars_iter e = dds->var_end();
        for (; i != e; i++) {
            BaseType *b = (*i);
            b->set_read_p(true);
        }

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());
        Ancillary::read_ancillary_das(*das, accessed);
        dds->transfer_attributes(das);

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

    return true;
}

bool DapRequestHandler::dap_build_vers(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    info->add_module(DAPREADER_PACKAGE, DAPREADER_VERSION);
    return ret;
}

bool DapRequestHandler::dap_build_help(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string> attrs;
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled("dapreader", services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    //info->add_data_from_file( "Dap.Help", "Dap Help" ) ;
    info->end_tag("module");

    return ret;
}

void DapRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DapRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

