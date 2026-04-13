//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#include "config.h"

#include <memory>

#include <libdap/DMR.h>
#include <libdap/DataDDS.h>

#include <libdap/mime_util.h>
#include <libdap/D4BaseTypeFactory.h>

#include "NCMLRequestHandler.h"

#include <BESConstraintFuncs.h>
#include <BESContainerStorage.h>
#include <BESContainerStorageList.h>
#include <BESDapNames.h>
#include "BESDataDDSResponse.h"
#include <BESDataNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDMRResponse.h>

#include <BESDebug.h>
#include "BESStopWatch.h"
#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESError.h>
#include <BESRequestHandlerList.h>
#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESServiceRegistry.h>
#include <BESTextInfo.h>
#include <BESUtil.h>
#include <BESVersionInfo.h>
#include <TheBESKeys.h>

#include "DDSLoader.h"

#include "NCMLDebug.h"
#include "NCMLUtil.h"
#include "NCMLParser.h"
#include "NCMLResponseNames.h"
#include "SimpleLocationParser.h"

using namespace agg_util;
using namespace ncml_module;
using namespace libdap;

#define MODULE "ncml"
#define prolog std::string("NCMLRequestHandler::").append(__func__).append("() - ")


bool NCMLRequestHandler::_global_attributes_container_name_set = false;
string NCMLRequestHandler::_global_attributes_container_name;

NCMLRequestHandler::NCMLRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_method(DAS_RESPONSE, NCMLRequestHandler::ncml_build_das);
    add_method(DDS_RESPONSE, NCMLRequestHandler::ncml_build_dds);
    add_method(DATA_RESPONSE, NCMLRequestHandler::ncml_build_data);

    add_method(DMR_RESPONSE, NCMLRequestHandler::ncml_build_dmr);
    add_method(DAP4DATA_RESPONSE, NCMLRequestHandler::ncml_build_dmr);

    add_method(VERS_RESPONSE, NCMLRequestHandler::ncml_build_vers);
    add_method(HELP_RESPONSE, NCMLRequestHandler::ncml_build_help);

    if (NCMLRequestHandler::_global_attributes_container_name_set == false) {
        bool key_found = false;
        string value;
        TheBESKeys::TheKeys()->get_value("NCML.GlobalAttributesContainerName", value, key_found);
        if (key_found) {
            // It was set in the conf file
            NCMLRequestHandler::_global_attributes_container_name_set = true;

            NCMLRequestHandler::_global_attributes_container_name = value;
        }
    }
}

NCMLRequestHandler::~NCMLRequestHandler()
{
}

#if 0
// Not used. jhrg 4/16/14

// This is the original example from Patrick or James for loading local file within the BES...
// Still used by DataDDS call, but the other callbacks use DDSLoader
// to get a brandy new DDX response.
// @see DDSLoader
bool NCMLRequestHandler::ncml_build_redirect(BESDataHandlerInterface &dhi, const string& location)
{
    // The current container in dhi is a reference to the ncml file.
    // Need to parse the ncml file here and get the list of locations
    // that we will be using. Any constraints defined?

    // do this for each of the locations retrieved from the ncml file.
    // If there are more than one locations in the ncml then we can't
    // set the context for dap_format to dap2. This will create a
    // structure for each of the locations in the resulting dap object.
    string sym_name = dhi.container->get_symbolic_name();
    BESContainerStorageList *store_list = BESContainerStorageList::TheList();
    BESContainerStorage *store = store_list->find_persistence("catalog");
    if (!store) {
        throw BESInternalError("couldn't find the catalog storage", __FILE__, __LINE__);
    }
    // this will throw an exception if the location isn't found in the
    // catalog. Might want to catch this. Wish the add returned the
    // container object created. Might want to change it.
    string new_sym = sym_name + "_location1";
    store->add_container(new_sym, location, "");

    BESContainer *container = store->look_for(new_sym);
    if (!container) {
        throw BESInternalError("couldn't find the container" + sym_name, __FILE__, __LINE__);
    }
    BESContainer *ncml_container = dhi.container;
    dhi.container = container;

    // this will throw an exception if there is a problem building the
    // response for this container. Might want to catch this
    BESRequestHandlerList::TheList()->execute_current(dhi);

    // clean up
    dhi.container = ncml_container;
    store->del_container(new_sym);

    return true;
}
#endif

// Here we load the DDX response with by hijacking the current dhi via DDSLoader
// and hand it to our parser to load the ncml, load the DDX for the location,
// apply ncml transformations to it, then return the modified DDS.
bool NCMLRequestHandler::ncml_build_das(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(MODULE, prolog + "Timer", &dhi);

    string filename = dhi.container->access();

    // Any exceptions winding through here will cause the loader and parser dtors
    // to clean up dhi state, etc.
    DDSLoader loader(dhi);
    NCMLParser parser(loader);
    unique_ptr<BESDapResponse> loaded_bdds = parser.parse(filename, DDSLoader::eRT_RequestDDX);

    // Now fill in the desired DAS response object from the DDS
    DDS* dds = NCMLUtil::getDDSFromEitherResponse(loaded_bdds.get());
    VALID_PTR(dds);

    BESDASResponse *bdas = dynamic_cast<BESDASResponse *>(dhi.response_handler->get_response_object());
    VALID_PTR(bdas);

    // Copy the modified DDS attributes into the DAS response object!
    DAS *das = bdas->get_das();

    if (dds->get_dap_major() < 4)
        NCMLUtil::hackGlobalAttributesForDAP2(dds->get_attr_table(),
            NCMLRequestHandler::get_global_attributes_container_name());

    NCMLUtil::populateDASFromDDS(das, *dds);

    // loaded_bdds destroys itself.
    return true;
}

bool NCMLRequestHandler::ncml_build_dds(BESDataHandlerInterface &dhi)
{
#if 0
    // original version 8/13/15
    BES_STOPWATCH_START_DHI(MODULE, prolog + "Timer", &dhi);

    string filename = dhi.container->access();

    // Any exceptions winding through here will cause the loader and parser dtors
    // to clean up dhi state, etc.
    unique_ptr<BESDapResponse> loaded_bdds(0);
    {
        DDSLoader loader(dhi);
        NCMLParser parser(loader);
        loaded_bdds = parser.parse(filename, DDSLoader::eRT_RequestDDX);
    }
    if (!loaded_bdds.get()) {
        throw BESInternalError("Null BESDDSResonse in ncml DDS handler.", __FILE__, __LINE__);
    }

    // Poke the handed down original response object with the loaded and modified one.
    DDS* dds = NCMLUtil::getDDSFromEitherResponse(loaded_bdds.get());
    VALID_PTR(dds);
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds_out = dynamic_cast<BESDDSResponse *>(response);
    VALID_PTR(bdds_out);
    DDS *dds = bdds_out->get_dds();
    VALID_PTR(dds);

    if (dds->get_dap_major() < 4)
        NCMLUtil::hackGlobalAttributesForDAP2(dds->get_attr_table(),
            NCMLRequestHandler::get_global_attributes_container_name());

    // If we just use DDS::operator=, we get into trouble with copied
    // pointers, bashing of the dataset name, etc etc so I specialize the copy for now.
    NCMLUtil::copyVariablesAndAttributesInto(dds_out, *dds);

    // Apply constraints to the result
    // See comment below. jhrg 8/12/15 dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
    bdds_out->set_constraint(dhi);

    // Also copy in the name of the original ncml request
    // TODO @HACK Not sure I want just the basename for the filename,
    // but since the DDS/DataDDS response fills the dataset name with it,
    // Our bes-testsuite fails since we get local path info in the dataset name.
    dds->filename(name_path(filename));
    dds->set_dataset_name(name_path(filename));

    return true;
#endif

    BES_STOPWATCH_START_DHI(MODULE, prolog + "Timer", &dhi);

    string filename = dhi.container->access();

    // it better be a data response!
    BESDDSResponse* ddsResponse = dynamic_cast<BESDDSResponse *>(dhi.response_handler->get_response_object());
    NCML_ASSERT_MSG(ddsResponse,
        "NCMLRequestHandler::ncml_build_data(): expected BESDDSResponse* but didn't get it!!");

    // Block it up to force cleanup of DHI.
    {
        DDSLoader loader(dhi);
        NCMLParser parser(loader);
        parser.parseInto(filename, DDSLoader::eRT_RequestDDX, ddsResponse);
    }

    DDS *dds = ddsResponse->get_dds();
    VALID_PTR(dds);

    if (dds->get_dap_major() < 4)
        NCMLUtil::hackGlobalAttributesForDAP2(dds->get_attr_table(),
            NCMLRequestHandler::get_global_attributes_container_name());

    // Apply constraints to the result
    // See comment below. jhrg 8/12/15 dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
    ddsResponse->set_constraint(dhi);

    // Also copy in the name of the original ncml request
    dds->filename(name_path(filename));
    dds->set_dataset_name(name_path(filename));

    return true;
}

bool NCMLRequestHandler::ncml_build_data(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(MODULE, prolog + "Timer", &dhi);

    string filename = dhi.container->access();

    // it better be a data response!
    BESDataDDSResponse* dataResponse = dynamic_cast<BESDataDDSResponse *>(dhi.response_handler->get_response_object());
    NCML_ASSERT_MSG(dataResponse,
        "NCMLRequestHandler::ncml_build_data(): expected BESDataDDSResponse* but didn't get it!!");

    // Block it up to force cleanup of DHI.
    {
        DDSLoader loader(dhi);
        NCMLParser parser(loader);
        parser.parseInto(filename, DDSLoader::eRT_RequestDataDDS, dataResponse);
    }

    // Apply constraints to the result

    // dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();
    // Replaced the above with the code below. P West said, a while ago, that using set_constraint
    // was better because BES containers would be supported. Not sure if that's a factor in this
    // code... jhrg 8/12/15
    dataResponse->set_constraint(dhi);

    // Also copy in the name of the original ncml request
    DDS* dds = NCMLUtil::getDDSFromEitherResponse(dataResponse);
    VALID_PTR(dds);

    dds->filename(name_path(filename));
    dds->set_dataset_name(name_path(filename));

    return true;
}

bool NCMLRequestHandler::ncml_build_dmr(BESDataHandlerInterface &dhi)
{
    BES_STOPWATCH_START_DHI(MODULE, prolog + "Timer", &dhi);

    // Because this code does not yet know how to build a DMR directly, use
    // the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
    // First step, build the 'full DDS'
    string data_path = dhi.container->access();

    DDS *dds = 0;	// This will be deleted when loaded_bdds goes out of scope.
    unique_ptr<BESDapResponse> loaded_bdds;
    try {
        DDSLoader loader(dhi);
        NCMLParser parser(loader);
        loaded_bdds = parser.parse(data_path, DDSLoader::eRT_RequestDDX);
        if (!loaded_bdds.get()) throw BESInternalError("Null BESDDSResonse in the NCML DDS handler.", __FILE__, __LINE__);
        dds = NCMLUtil::getDDSFromEitherResponse(loaded_bdds.get());
        VALID_PTR(dds);
        dds->filename(data_path);
        dds->set_dataset_name(data_path);
    }
    catch (InternalErr &e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (Error &e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e){
        throw;
    }
    catch (...) {
        throw BESDapError("Caught unknown error build ** DMR response", true, unknown_error, __FILE__, __LINE__);
    }

    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bdmr = dynamic_cast<BESDMRResponse &>(*response);

    // Get the DMR made by the BES in the BES/dap/BESDMRResponseHandler, make sure there's a
    // factory we can use and then dump the DAP2 variables and attributes in using the
    // BaseType::transform_to_dap4() method that transforms individual variables
    DMR *dmr = bdmr.get_dmr();
    dmr->set_factory(new D4BaseTypeFactory);
    dmr->build_using_dds(*dds);

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bdmr.set_dap4_constraint(dhi);
    bdmr.set_dap4_function(dhi);

    return true;
}

bool NCMLRequestHandler::ncml_build_vers(BESDataHandlerInterface &dhi)
{
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");

    info->add_module(MODULE_NAME, MODULE_VERSION);
    return true;
}

bool NCMLRequestHandler::ncml_build_help(BESDataHandlerInterface &dhi)
{
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info) throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map<string, string, std::less<>> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;

    list<string> services;
    BESServiceRegistry::TheRegistry()->services_handled(ncml_module::ModuleConstants::NCML_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    //info->add_data_from_file( "NCML.Help", "NCML Help" ) ;
    info->add_data("Please consult the online documentation at " + ncml_module::ModuleConstants::DOC_WIKI_URL);
    info->end_tag("module");

    return true;
}

void NCMLRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NCMLRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

