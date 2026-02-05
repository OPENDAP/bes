// FONcTransform.cc

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>
//      kyang       Kent Yang <myang6@hdfgroup.org> (for DAP4/netCDF-4 enhancement)
//      slloyd      Samuel Lloyd <slloyd@opendap.org> (netCDF file streaming)

#include "config.h"

#include <sstream>

#include <netcdf.h>

#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/Structure.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>
#include <libdap/Sequence.h>

#include <BESResponseObject.h>
#include <BESDapResponseBuilder.h>
#include <BESDataHandlerInterface.h>
#include <BESUtil.h>
#include <TempFile.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDataDDSResponse.h>
#include <BESDMRResponse.h>
#include <BESRequestHandlerList.h>
#include <BESDapFunctionResponseCache.h>
#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>
#include "BESSyntaxUserError.h"
#include "RequestServiceTimer.h"

#include "DapFunctionUtils.h"
#include "DapUtils.h"

#include "FONcRequestHandler.h" // for the keys

#include "FONcTransform.h"
#include "FONcUtils.h"
#include "FONcBaseType.h"
#include "FONcAttributes.h"
#include "FONcTransmitter.h"
#include "history_utils.h"
#include "FONcNames.h"

using namespace libdap;
using namespace std;

#define MODULE "fonc"
#define prolog std::string("FONcTransform::").append(__func__).append("() - ")

#define FOUR_GB_IN_KB (4294967296/1024)
#define TWO_GB_IN_KB (2147483648/1024)
#define MSG_LABEL_CLASSIC_MODEL " (classic model)"
#define MSG_LABEL_SIXTYFOUR_BIT_MODEL " (64-bit offset model)"

/** @brief Constructor that creates transformation object from the specified BESResponseObject object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform(BESResponseObject *obj, BESDataHandlerInterface *dhi, const string &localfile,
                             const string &ncVersion)
                             : d_obj(obj), d_dhi(dhi), _localfile(localfile), _returnAs(ncVersion) {
    if (!d_obj) {
        throw BESInternalError("File out netcdf, null BESResponseObject passed to constructor", __FILE__, __LINE__);
    }
    if (_localfile.empty()) {
        throw BESInternalError("File out netcdf, empty local file name passed to constructor", __FILE__, __LINE__);
    }

    // if there is a variable, attribute, dimension name that is not
    // compliant with netcdf naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi->first_container();
    if (dhi->container) {
        FONcUtils::name_prefix = dhi->container->get_container_type() + "_";
    }
    else {
        FONcUtils::name_prefix = "nc_";
    }
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FONcTransform::~FONcTransform() {
    for (auto &b: _fonc_vars) {
        delete b;
    }
    for (auto &b: _total_fonc_vars_in_grp) {
        delete b;
    }
    // _dmr is not managed by the BESDMRResponse class in this code. However,
    // _dds still is. jhrg 8/13/21
    delete _dmr;
}

/**
 *
 * @param dap_version Dap version of the request 2 || 4)
 * @param return_encoding Should be netcdf-3|4 (classic)|{64-bit
 * @param config_max_response_size_kb
 * @param contextual_max_response_size_kb
 * @param ce
 * @return
 */
string FONcTransform::too_big_error_msg(
        const unsigned dap_version,
        const string &return_encoding,
        const unsigned long long dap2_response_size_kb,
        const unsigned long long  contextual_max_response_size_kb,
        const string &ce
){

    stringstream msg;

    msg << "Your request was for a (DAP"<< dap_version << " data model response) to be encoded as ";
    msg << return_encoding << ". ";
    msg << "The response to your specific request will produce a " << dap2_response_size_kb;
    msg <<  " kilobyte response. On this server the response size for your request is limited to ";
    msg << contextual_max_response_size_kb << " kilobytes. ";

    msg << "The server is configured to allow ";
    auto conf_max_request_size_kb =FONcRequestHandler::get_request_max_size_kb();
    if(conf_max_request_size_kb==0){
        msg << " responses of unlimited size. ";
    }
    else {
        msg << "responses as large as: " << conf_max_request_size_kb <<" kilobytes. ";
    }

    if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF3) {
        msg << "Additionally, the requested response encoding " << return_encoding << " is structurally limited to ";
        if (FONcRequestHandler::nc3_classic_format) {
            msg << TWO_GB_IN_KB << " kilobytes" << MSG_LABEL_CLASSIC_MODEL << ".";
        }
        else {
            msg << FOUR_GB_IN_KB << " kilobytes" << MSG_LABEL_SIXTYFOUR_BIT_MODEL << ".";
        }
        msg << "One thing to try would be to reissue the the request, but change the requested response encoding ";
        msg << "to NetCDF-4. This can be accomplished with the buttons in the Data Request Form, or by modifying ";
        msg << "the request URL by changing the terminal path suffix from \".nc\" to \".nc4\".  ";
    }

    if(ce.empty()){
        msg << "I've noticed that no constraint expression accompanied your request. ";
    } else {
        msg << "Your request employed the constraint expression: \"" << ce << "\" ";
    }
    msg << "You may also reduce the size of the request by choosing just the variable(s) you need and/or by ";
    msg << "using the DAP index based array sub-setting syntax to additionally limit the amount of data requested.";
    return msg.str();
}


/**
 * This helper function generates two pieces of information, the maximum request size in KB (max_request_size_kb) and
 * an error message parameter (return_encoding) that is used in building an error response
 * @param max_request_size_kb This returned value parameter will be set to the corrected max request size in kilobytes.
 * This may be adjusted from the configured state to accommodate formats like netcdf-3 (classic)  whose response size
 * limitations may be less than for the server's more general configuration.
 * @param return_encoding The error response message component is modified to be the netcdf-3|4 tag followed by an
 * optional netcdf model tag. example: "netcdf-3 (classic)"
 */
void FONcTransform::set_max_size_and_encoding(unsigned long long &max_request_size_kb, string &return_encoding){

    return_encoding.clear();

    // The following conditional accomplishes two things:
    // 1) It correctly controls the values of "max_request_size_kb" so that even if it's
    //    set to unlimited (aka 0) rational limits will be enforced based on the type of
    //    response coding that was requested.
    // 2) It constructs the string "return_encoding" for use in debugging and as
    //    a component of the "it's too big" error message.
    if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF3) {
        return_encoding = string(FONC_RETURN_AS_NETCDF3).append("-3 ");
        if (FONcRequestHandler::nc3_classic_format) {
            return_encoding += MSG_LABEL_CLASSIC_MODEL;
            if (max_request_size_kb == 0 || max_request_size_kb >= TWO_GB_IN_KB) {
                max_request_size_kb = TWO_GB_IN_KB - 1 /* kb */;
                BESDEBUG(MODULE, prolog << "Configured max request size was incompatible with NetCDF-3 classic format. " <<
                                        "Reset to: " << max_request_size_kb << endl);
            }
        }
        else {
            return_encoding += MSG_LABEL_SIXTYFOUR_BIT_MODEL;
            if (max_request_size_kb == 0 || max_request_size_kb >= FOUR_GB_IN_KB) {
                max_request_size_kb = FOUR_GB_IN_KB - 1 /* kb */;
                BESDEBUG(MODULE, prolog << "Configured max request size was incompatible with NetCDF-3 w/64-bit offset format. " <<
                                        "Reset to: " << max_request_size_kb << endl);
            }
        }
    }
    else {
        return_encoding = FONC_RETURN_AS_NETCDF4;
        if (FONcRequestHandler::nc3_classic_format) {
            return_encoding += MSG_LABEL_CLASSIC_MODEL;
        }
    }
    BESDEBUG(MODULE, prolog << "return_encoding: " << return_encoding << endl);
    BESDEBUG(MODULE, prolog << "max_request_size_kb: " << max_request_size_kb << endl);
}


/**
 * @brief convenience function for the response limit test.
 * The DDS stores the response size limit in Bytes even though the context
 * param uses KB. The DMR uses KB throughout.
 * @param dds
 */
void FONcTransform::throw_if_dap2_response_too_big(DDS *dds, const string &dap2_ce)
{
    string return_encoding;

    unsigned long long max_response_size_kb = FONcRequestHandler::get_request_max_size_kb();
    BESDEBUG(MODULE, prolog << "Configured max_request_size_kb: " << max_response_size_kb << endl);

    unsigned long long dap2_response_size_kb = dds->get_request_size_kb(true);
    BESDEBUG(MODULE, prolog << "dds->get_request_size_kb(): " << dap2_response_size_kb << endl);

    set_max_size_and_encoding(max_response_size_kb, return_encoding);

    // set the max request size in kilobytes for testing if the request is too large
    dds->set_response_limit_kb(max_response_size_kb);

    if (dds->too_big()) {
        string err_msg = too_big_error_msg(2,return_encoding,dap2_response_size_kb, max_response_size_kb, dap2_ce);
        throw BESSyntaxUserError(err_msg,__FILE__,__LINE__);
    }
}

/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FONcTransform::transform_dap2() {

    BESDEBUG(MODULE, prolog << "BEGIN" << endl);
    BESDEBUG(MODULE, prolog << "Reading data into DataDDS" << endl);

    FONcUtils::reset();

    d_dhi->first_container();

    auto bdds = dynamic_cast<BESDataDDSResponse *>(d_obj);
    if (!bdds) throw BESInternalFatalError("Expected a BESDataDDSResponse instance", __FILE__, __LINE__);

    _dds = bdds->get_dds();

    BESDapResponseBuilder besDRB;

    besDRB.set_dataset_name(_dds->filename());
    besDRB.set_ce(d_dhi->data[POST_CONSTRAINT]);
    besDRB.set_async_accepted(d_dhi->data[ASYNC]);
    besDRB.set_store_result(d_dhi->data[STORE_RESULT]);


    // This function is used by all fileout modules, and they need to include the attributes in data access.
    // So obtain the attributes if necessary. KY 2019-10-30
    if (bdds->get_ia_flag() == false) {
        BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler(
                d_dhi->container->get_container_type());
        besRH->add_attributes(*d_dhi);
    }

    ConstraintEvaluator &eval = bdds->get_ce();

    // Split constraint into two halves; stores the function and non-function parts in this instance.
    besDRB.split_ce(eval);
    // If there are functions, parse them and eval.
    // Use that DDS and parse the non-function ce
    // Serialize using the second ce and the second dds
    if (!besDRB.get_btp_func_ce().empty()) {
        BESDEBUG(MODULE,  prolog << "Found function(s) in CE: " << besDRB.get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = nullptr;
        if (responseCache && responseCache->can_be_cached(_dds, besDRB.get_btp_func_ce())) {
            fdds = responseCache->get_or_cache_dataset(_dds, besDRB.get_btp_func_ce());
        }
        else {
            func_eval.parse_constraint(besDRB.get_btp_func_ce(), *_dds);
            fdds = func_eval.eval_function_clauses(*_dds);
        }

        delete _dds;             // Delete so that we can ...
        bdds->set_dds(fdds);    // Transfer management responsibility
        _dds = fdds;

        // Server functions might mark (i.e. setting send_p) so variables will use their read()
        // methods. Clear that so the CE in d_dap2ce will control what is
        // sent. If that is empty (there was only a function call), all
        // variables in the intermediate DDS (i.e., the function
        // result) will be sent.
        _dds->mark_all(false);

        // Look for one or more top level Structures whose name indicates (by way of ending with
        // "_uwrap") that their contents should be moved to the top level.
        //
        // This is in support of a hack around the current API where server side functions
        // may only return a single DAP object and not a collection of objects. The name suffix
        // "_unwrap" is used as a signal from the function to the the various response
        // builders and transmitters that the representation needs to be altered before
        // transmission, and that in fact is what happens in our friend
        // promote_function_output_structures()
        promote_function_output_structures(_dds);
    }


    // evaluate the rest of the CE - the part that follows the function calls.
    eval.parse_constraint(besDRB.get_ce(), *_dds);

    _dds->tag_nested_sequences(); // Tag Sequences as Parent or Leaf node.

    throw_if_dap2_response_too_big(_dds, besDRB.get_ce());
    dap_utils::throw_for_dap4_typed_vars_or_attrs(_dds,__FILE__,__LINE__);

    // Convert the DDS into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    for (auto vi = _dds->var_begin(), ve = _dds->var_end(); vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BESDEBUG(MODULE,  prolog << "Converting variable '" << (*vi)->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for '*vi'
            FONcBaseType *fb = FONcUtils::convert((*vi), FONcTransform::_returnAs, FONcRequestHandler::classic_model);

            _fonc_vars.push_back(fb);
            vector <string> embed;
            fb->convert(embed);
        }
    }

    fonc_history_util::updateHistoryAttributes(_dds, d_dhi->data[POST_CONSTRAINT]);

    // Open the file for writing
    int stax = NC_NOERR;
    if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF4) {
        if (FONcRequestHandler::classic_model) {
            BESDEBUG(MODULE,  prolog << "Opening NetCDF-4 cache file in classic mode. fileName:  "
                    << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4 | NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG(MODULE, prolog << "Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG(MODULE,  prolog << "Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
        if (FONcRequestHandler::nc3_classic_format) 
            stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
        else 
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_64BIT_OFFSET, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, prolog + "Call to nc_create() failed for file: " + _localfile, __FILE__, __LINE__);
    }

    int current_fill_prop_vaule =-1;

    // TODO: we may need to evaluate if using NC_NOFILL is always the best way.
    stax = nc_set_fill(_ncid, NC_NOFILL, &current_fill_prop_vaule);
    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to set fill to NC_NOFILL: " + _localfile, __FILE__,
                                __LINE__);
    }

    try {

        // For each converted FONc object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
        for (FONcBaseType *fbt: _fonc_vars) {
            BESDEBUG(MODULE,  prolog << "Defining variable:  " << fbt->name() << endl);
            fbt->define(_ncid);
        }

        if (FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            AttrTable &globals = _dds->get_attr_table();
            BESDEBUG(MODULE,  prolog << "Adding Global Attributes" << endl << globals << endl);
            bool is_netCDF_enhanced = false;
            if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF4 && FONcRequestHandler::classic_model == false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_attributes(_ncid, NC_GLOBAL, globals, "", "", is_netCDF_enhanced);
            // We could add the json history directly to the netcdf file here. For now,
            // this code, which adds it to the global attribute table and then moves
            // those into the netcdf file, is working. There are two other places in the
            // file where this is true. Search for '***' jhrg 2/28/22
        }

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
        if (stax != NC_NOERR) {
            FONcUtils::handle_error(stax, "File out netcdf, unable to end the define mode: " + _localfile, __FILE__,
                                    __LINE__);
        }
        for (FONcBaseType *fbt: _fonc_vars) {
            BESDEBUG(MODULE,  prolog << "Writing data for variable:  " << fbt->name() << endl);

            fbt->set_dds(_dds);
            fbt->set_eval(&eval);

            fbt->write(_ncid);
            nc_sync(_ncid);
        }

        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);

    }
    catch (const BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }
}

/**
 * @brief Throws a BESSyntaxUserError if the size of the responses exceeds what is permitted or possible
 * .\
 * The DDS stores the response size limit in Bytes even though the context
 * param uses KB. The DMR uses KB throughout.
 * @param dds
 */
void FONcTransform::throw_if_dap4_response_too_big(DMR *dmr, const string &dap4_ce)
{
    unsigned long long max_response_size_kb = FONcRequestHandler::get_request_max_size_kb();
    BESDEBUG(MODULE, prolog << "Configured max_request_size_kb: " << max_response_size_kb << endl);

    unsigned long long req_size_kb = dmr->request_size_kb(true);
    BESDEBUG(MODULE, prolog << "dmr->get_request_size_kb(): " << req_size_kb << endl);

    string return_encoding;
    set_max_size_and_encoding(max_response_size_kb, return_encoding);

    // set the max request size in kilobytes for testing if the request is too large
    dmr->set_response_limit_kb(max_response_size_kb);

    if (dmr->too_big()) {
        string err_msg = too_big_error_msg(4,return_encoding,req_size_kb, max_response_size_kb, dap4_ce);
        throw BESSyntaxUserError(err_msg,__FILE__,__LINE__);
    }
}

/** @brief Transforms each of the variables of the DMR to the NetCDF
 * file
 *
 * For each variable in the DMR write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DMR.
 */
void FONcTransform::transform_dap4() {
    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);

    FONcUtils::reset();

    d_dhi->first_container();

    BESDapResponseBuilder responseBuilder;
    _dmr = responseBuilder.setup_dap4_intern_data(d_obj, *d_dhi).release();

    _dmr->set_response_limit_kb(FONcRequestHandler::get_request_max_size_kb());

    vector<string> inventory;
    bool d4_true = _dmr->is_dap4_projected(inventory);

    if (d4_true && _returnAs == "netcdf"){
        stringstream msg;
        msg << "This dataset contains variables and/or attributes whose data types are not compatible with the " << endl;
        msg << "NetCDF-3 data model. If your request includes any of variables represented by one of these " << endl;
        msg << "incompatible variables and/or attributes and you choose the “NetCDF-3” download encoding, " << endl;
        msg << "your request will FAIL. " << endl;
        msg << endl;
        msg << "You may also try constraining your request to omit the problematic data type(s), " << endl;
        msg << "or ask for a different encoding such as DAP4 binary or NetCDF-4." << endl;
        msg << "There are " << inventory.size() << " incompatible variables referenced in your request." << endl;
        msg << "Incompatible variables: " << endl;
        msg << endl;
        for(const auto &entry: inventory){
            msg << "    " << entry << endl;
        }
        throw BESSyntaxUserError(
                msg.str(),
                __FILE__,
                __LINE__);
    }

    throw_if_dap4_response_too_big(_dmr,responseBuilder.get_dap4ce() );

    BESDapResponseBuilder besDRB;

    besDRB.set_dataset_name(_dmr->filename());

    // Added set of DAP4 fields. jhrg 5/30/21
    besDRB.set_dap4ce(d_dhi->data[DAP4_CONSTRAINT]);
    besDRB.set_dap4function(d_dhi->data[DAP4_FUNCTION]);

    besDRB.set_async_accepted(d_dhi->data[ASYNC]);
    besDRB.set_store_result(d_dhi->data[STORE_RESULT]);

    // Here we need to check if we need to reduce the redundant dimension names.
    if (FONcRequestHandler::reduce_dim == true) {
        do_reduce_dim = check_reduce_dim();
        if (do_reduce_dim) 
            build_reduce_dim();
#if !NDEBUG        
        if (do_reduce_dim) 
            BESDEBUG(MODULE, prolog << "reduced dimensions" << endl);
        else
            BESDEBUG(MODULE, prolog << "Not reduced dimensions" << endl);
    
        if (do_reduce_dim) {
            D4Group *root_grp_debug = _dmr->root();
            for (auto &var:root_grp_debug->variables()) {
        
                if (var->type() == dods_array_c) {
                    auto t_a = dynamic_cast<Array *>(var);
                    Array::Dim_iter dim_i = t_a->dim_begin();
                    Array::Dim_iter dim_e = t_a->dim_end();
                    for (; dim_i != dim_e; dim_i++) {
                        BESDEBUG(MODULE, prolog << "CHANGED dim name: " << dim_i->name<<endl);
                    }
                }
            }
        
            D4Dimensions *root_dims = root_grp_debug->dims();
            for (D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
                BESDEBUG(MODULE,  prolog << "transform_dap4() - check dimensions" << endl);
                BESDEBUG(MODULE,  prolog << "transform_dap4() - dim name is: " << (*di)->name() << endl);
                BESDEBUG(MODULE,  prolog << "transform_dap4() - dim size is: " << (*di)->size() << endl);
                BESDEBUG(MODULE,  prolog << "transform_dap4() - fully_qualfied_dim name is: " << (*di)->fully_qualified_name() << endl);
            }
        }
 
#endif
    }
    // Check if direct_io_flag is set for any Array variables. If the global dio flag is false, we don't need to loop through
    // every variable to check if the direct IO can be applied. 
    if (FONC_RETURN_AS_NETCDF4 == FONcTransform::_returnAs && false == FONcRequestHandler::classic_model) {
        global_dio_flag = _dmr->get_global_dio_flag();

#if !NDEBUG
        if(global_dio_flag) {  
            BESDEBUG(MODULE, prolog << "global_dio_flag is true" << endl);
        }
        else 
            BESDEBUG(MODULE, prolog << "global_dio_flag is false" << endl);
#endif

    }

    // Convert the DMR into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.


    // First check if this DMR has groups etc.
    bool support_group = check_group_support();

    if (true == support_group) {

        int stax = -1;
        BESDEBUG(MODULE, prolog << "Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4, &_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, prolog + "Call to nc_create() failed for file: " + _localfile, __FILE__, __LINE__);

        D4Group *root_grp = _dmr->root();

        // Declare the dimname to dimid map to handle netCDF-4 dimensions
        map<string, int> fdimname_to_id;

        // Generate a list of the groups in the final netCDF file. 
        // The attributes of these groups should be included.
        gen_included_grp_list(root_grp);

#if !NDEBUG
        for (std::set<string>::iterator it = _included_grp_names.begin(); it != _included_grp_names.end(); ++it)
            BESDEBUG(MODULE,  prolog << "Included group list name is: " << *it << endl);
#endif
        // Build a global dimension name table for all variables if
        // the constraint is not empty!
        check_and_obtain_dimensions(root_grp, true);

        // Don't remove the following code, they are for debugging.
#if !NDEBUG
        map<string, int64_t>::iterator it;

        for (it = GFQN_dimname_to_dimsize.begin(); it != GFQN_dimname_to_dimsize.end(); ++it) {
            BESDEBUG(MODULE,  prolog << "Final GFQN dim name is: " << it->first << endl);
            BESDEBUG(MODULE,  prolog << "Final GFQN dim size is: " << it->second << endl);
        }

        for (it = VFQN_dimname_to_dimsize.begin(); it != VFQN_dimname_to_dimsize.end(); ++it) {
            BESDEBUG(MODULE,  prolog << "Final VFQN dim name is: " << it->first << endl);
            BESDEBUG(MODULE,  prolog << "Final VFQN dim size is: " << it->second << endl);
        }
#endif

        // DAP4 requires the DAP4 dimension sizes defined in the group should be changed
        // according to the corresponding variable sizes. Check section 8.6.2 at
        // https://docs.opendap.org/index.php/DAP4:_Specification_Volume_1
        //
        map<string, int64_t>::iterator git, vit;
        for (git = GFQN_dimname_to_dimsize.begin(); git != GFQN_dimname_to_dimsize.end(); ++git) {
            for (vit = VFQN_dimname_to_dimsize.begin(); vit != VFQN_dimname_to_dimsize.end(); ++vit) {
                if (git->first == vit->first) {
                    if (git->second != vit->second)
                        git->second = vit->second;
                    break;
                }
            }
        }

        // This part of code is to address the possible dimension name conflict
        // when variables in the constraint don't have dimension names. Fileout netCDF
        // adds the fake dimensions such as dim1, dim2...to these variables.
        // If these dimension names are used by
        // the file to be handled, the dimension conflict will corrupt the final output.
        // The idea is to find if there are any dimension names like dim1, dim2 ... 
        // under the root group.
        // We will remember them and not use these names as fake dimension names.
        //
        // Obtain the dim. names under the root group
        vector <string> root_d4_dimname_list;
        for (git = GFQN_dimname_to_dimsize.begin(); git != GFQN_dimname_to_dimsize.end(); ++git) {
            string d4_temp_dimname = git->first.substr(1);
            //BESDEBUG(MODULE,  prolog << "d4_temp_dimname: "<<d4_temp_dimname<<endl);
            if (d4_temp_dimname.find('/') == string::npos)
                root_d4_dimname_list.push_back(d4_temp_dimname);
        }

#if !NDEBUG
        for (unsigned int i = 0; i < root_d4_dimname_list.size(); i++)
            BESDEBUG(MODULE,  prolog << "root_d4 dim name is: " << root_d4_dimname_list[i] << endl);
#endif

        // Only remember the root dimension names that are like "dim1,dim2,..."
        vector<int> root_dim_suffix_nums;
        for (unsigned int i = 0; i < root_d4_dimname_list.size(); i++) {
            if (root_d4_dimname_list[i].size() < 4)
                continue;
            else if (root_d4_dimname_list[i].substr(0, 3) != "dim")
                continue;
            else {
                string temp_suffix = root_d4_dimname_list[i].substr(3);
                bool ignored_suffix = false;
                for (unsigned int j = 0; j < temp_suffix.size(); j++) {
                    if (!isdigit(temp_suffix[j])) {
                        ignored_suffix = true;
                        break;
                    }
                }
                if (ignored_suffix == true)
                    continue;
                else
                    root_dim_suffix_nums.push_back(atoi(temp_suffix.c_str()));
            }
        }

#if !NDEBUG
        for (unsigned int i = 0; i < root_dim_suffix_nums.size(); i++)
            BESDEBUG(MODULE,  prolog << "root_dim_suffix_nums: " << root_dim_suffix_nums[i] << endl);


        for (it = GFQN_dimname_to_dimsize.begin(); it != GFQN_dimname_to_dimsize.end(); ++it) {
            BESDEBUG(MODULE,  prolog << "RFinal GFQN dim name is: " << it->first << endl);
            BESDEBUG(MODULE,  prolog << "RFinal GFQN dim size is: " << it->second << endl);
        }

        for (it = VFQN_dimname_to_dimsize.begin(); it != VFQN_dimname_to_dimsize.end(); ++it) {
            BESDEBUG(MODULE,  prolog << "RFinal VFQN dim name is: " << it->first << endl);
            BESDEBUG(MODULE,  prolog << "RFinal VFQN dim size is: " << it->second << endl);
        }
#endif

        // Now we transform all the objects(including groups) to netCDF-4
        transform_dap4_group(root_grp, true, _ncid, fdimname_to_id, root_dim_suffix_nums);
        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);

    }
    else // No group, handle as the classic way
        transform_dap4_no_group();

    BESDEBUG(MODULE,  prolog << "END" << endl);

    return;
}

/**
 * @brief Transform the DMR to a netCDF-4 file when there are no DAP4 groups.
 * This routine is similar to transform() that handles DAP2 objects.  However, DAP4 routines are needed.
 * So still keep a separate function. May combine this function with the transform()  in the future.
 */
void FONcTransform::transform_dap4_no_group() {

    // Open the file for writing
    int stax = -1;
    if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF4) {
        if (FONcRequestHandler::classic_model) {
            BESDEBUG(MODULE, prolog << "Opening NetCDF-4 cache file in classic mode. fileName:  "
                             << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4 | NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG(MODULE, prolog << "Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG(MODULE, prolog << "Opening NetCDF-3 cache file. fileName:  " << _localfile <<endl);
        if (FONcRequestHandler::nc3_classic_format)                                                    
            stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
        else 
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_64BIT_OFFSET, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, prolog + "Call to nc_create() failed for file: " + _localfile, __FILE__, __LINE__);
    }

    D4Group *root_grp = _dmr->root();

    is_root_no_grp_unlimited_dim = obtain_unlimited_dimension_info(root_grp,root_no_grp_unlimited_dimnames);

    if(root_grp->has_enum_defs()) 
        gen_nc4_enum_type(root_grp,_ncid);
 
#if !NDEBUG
    D4Dimensions *root_dims = root_grp->dims();
    for (D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG(MODULE,  prolog << "transform_dap4() - check dimensions" << endl);
        BESDEBUG(MODULE,  prolog << "transform_dap4() - dim name is: " << (*di)->name() << endl);
        BESDEBUG(MODULE,  prolog << "transform_dap4() - dim size is: " << (*di)->size() << endl);
        BESDEBUG(MODULE,  prolog << "transform_dap4() - fully_qualfied_dim name is: " << (*di)->fully_qualified_name() << endl);
    }
#endif

    Constructor::Vars_iter vi = root_grp->var_begin();
    Constructor::Vars_iter ve = root_grp->var_end();

    // If the global_dio_flag is false, we cannot do direct IO for this file at all. No need to check every variable.
    // This is necessary since direct IO cannot apply to the current existing dmrpp files in the earth data cloud. KY 2023-12-11
    if (global_dio_flag == false) {
        for (; vi != ve; vi++) {
            if ((*vi)->send_p()) {
                BaseType *v = *vi;
    
                BESDEBUG(MODULE, prolog << "Converting variable '" << v->name() << "'" << endl);
                Array *t_a = dynamic_cast<Array *>(v);
                 BESDEBUG(MODULE, prolog << "FONC_array ratio:" << t_a->get_storage_size_ratio() <<endl);
    
                // This is a factory class call, and 'fg' is specialized for 'v'
                FONcBaseType *fb = FONcUtils::convert(v, FONcTransform::_returnAs, FONcRequestHandler::classic_model, GFQN_to_en_typeid_vec,root_no_grp_unlimited_dimnames);
                
                _fonc_vars.push_back(fb);
    
                vector <string> embed;
                // This call sets d_is_dap4 to true, and d_is_dap4_group to false. jhrg 2/14/24
                fb->convert(embed, true, false);
            }
        }
    }
    else {
        for (; vi != ve; vi++) {
            if ((*vi)->send_p()) {
                BaseType *v = *vi;
    
                BESDEBUG(MODULE, prolog << "Direct IO is on, Converting variable '" << v->name() << "'" << endl);
                 Array *t_a = dynamic_cast<Array *>(v);
                 BESDEBUG(MODULE, prolog << "FONC_array ratio:" << t_a->get_storage_size_ratio() <<endl);
    
                if (v->type() == dods_array_c) {
                    auto t_a = dynamic_cast<Array *>(v);
                    if (t_a->get_dio_flag()) {
                        set_constraint_var_dio_flag(t_a);
#if 0
                        bool var_has_unlim_dim = false;
                        if (is_root_no_grp_unlimited_dim) 
                            var_has_unlim_dim = check_var_unlimited_dimension(t_a,root_no_grp_unlimited_dimnames);
                        if (var_has_unlim_dim)
                            t_a->set_dio_flag(false);
                        else 
                            set_constraint_var_dio_flag(t_a);
#endif
                    }
    
                }
                // This is a factory class call, and 'fg' is specialized for 'v'
                FONcBaseType *fb = FONcUtils::convert(v, FONcTransform::_returnAs, FONcRequestHandler::classic_model, GFQN_to_en_typeid_vec,root_no_grp_unlimited_dimnames);
                
                _fonc_vars.push_back(fb);
    
                vector <string> embed;
                // This call sets d_is_dap4 to true, and d_is_dap4_group to false. jhrg 2/14/24
                fb->convert(embed, true, false);
            }
        }
    }

#if !NDEBUG
    if (root_grp->grp_begin() == root_grp->grp_end())
        BESDEBUG(MODULE,  prolog << "No group  " << endl);
    else
        BESDEBUG(MODULE,  prolog << "Has group  " << endl);
    for (D4Group::groupsIter gi = root_grp->grp_begin(), ge = root_grp->grp_end(); gi != ge; ++gi)
        BESDEBUG(MODULE,  prolog << "Group name:  " << (*gi)->name() << endl);
#endif

    fonc_history_util::updateHistoryAttributes(_dmr, d_dhi->data[POST_CONSTRAINT]);


    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        nc_redef(_ncid);

        // For each converted FONc object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
        vector<FONcBaseType *>::iterator i = _fonc_vars.begin();
        vector<FONcBaseType *>::iterator e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG(MODULE, prolog << "Defining variable:  " << fbt->name() << endl);
            //fbt->set_is_dap4(true);
            fbt->define(_ncid);
        }

        if (FONcRequestHandler::no_global_attrs == false) {

            // Add any global attributes to the netcdf file
            D4Group *root_grp = _dmr->root();
            D4Attributes *d4_attrs = root_grp->attributes();

            BESDEBUG(MODULE, prolog << "Handle GLOBAL DAP4 attributes " << d4_attrs << endl);
#if !NDEBUG
            for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end();
                 ii != ee; ++ii) {
                string name = (*ii)->name();
                BESDEBUG(MODULE, prolog << "GLOBAL attribute name is " << name << endl);
            }
#endif
            bool is_netCDF_enhanced = false;
            if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF4 && FONcRequestHandler::classic_model == false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_dap4_attributes(_ncid, NC_GLOBAL, d4_attrs, "", "", is_netCDF_enhanced);
            // *** Add the json history here
        }

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
        if (stax != NC_NOERR) {
            FONcUtils::handle_error(stax, "File out netcdf, unable to end the define mode: " + _localfile, __FILE__,
                                    __LINE__);
        }

        // Write everything out
        i = _fonc_vars.begin();
        e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmitting: " + fbt->name() , __FILE__, __LINE__);
            BESDEBUG(MODULE, prolog << "Writing data for variable:  " << fbt->name() << endl);
            fbt->write(_ncid);
        }

        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);
    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }

}

// Transform the DMR to a netCDF-4 file when there are DAP4 groups.
void FONcTransform::transform_dap4_group(D4Group *grp,
                                         bool is_root_grp,
                                         int par_grp_id, map<string, int> &fdimname_to_id,
                                         vector<int> &root_dim_suffix_nums) {

    bool included_grp = false;

    // Obtain the whole file.
    if (_dmr->get_ce_empty()) {
        BESDEBUG(MODULE, prolog << "In group  - group name:  " << grp->FQN() << endl);
        included_grp = true;
    }
    else if (is_root_grp == true)
        included_grp = true;
    else {
        // Check if this group is in the group list.
        set<string>::iterator iset;
        if (_included_grp_names.find(grp->FQN()) != _included_grp_names.end())
            included_grp = true;
    }

    // Call the internal routine to transform the DMR that has groups if this group is in the group list.. 
    // If this group is not in the group list, we know all its subgroups are also not in the list, just stop and return.
    if (included_grp == true)
        transform_dap4_group_internal(grp, is_root_grp, par_grp_id, fdimname_to_id, root_dim_suffix_nums);
    return;
}

/**
 * @brief The internal routine to transform DMR to netCDF-4 when there are groups.
 * @param grp
 * @param is_root_grp
 * @param par_grp_id
 * @param fdimname_to_id
 * @param rds_nums
 */
void FONcTransform::transform_dap4_group_internal(D4Group *d4_grp,
                                                  bool is_root_grp,
                                                  int par_nc4_grp_id, map<string, int> &fdimname_to_id,
                                                  vector<int> &rds_nums) {

    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);
    int nc4_grp_id = -1;
    int stax = -1;

    fonc_history_util::updateHistoryAttributes(_dmr, d_dhi->data[POST_CONSTRAINT]);

    if (is_root_grp == true) { 
        nc4_grp_id = _ncid;
        // handle enum type.
        if(d4_grp->has_enum_defs()) 
            gen_nc4_enum_type(d4_grp,nc4_grp_id);
    }
    else {
        // Here we need to check if there is any special character inside the group name.
        // If yes, we will replace that character to _ since nc_def_grp will fail if
        // there are special characters inside the group name. KY 2025-02-14 
        string grp_name = (*d4_grp).name();
        string new_grp_name = FONcUtils::id2netcdf(grp_name);
        stax = nc_def_grp(par_nc4_grp_id, new_grp_name.c_str(), &nc4_grp_id);
        BESDEBUG(MODULE,  prolog << "Group name is " << (*d4_grp).name() << endl);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to define group: " + _localfile, __FILE__, __LINE__);

        if(d4_grp->has_enum_defs()) {
            gen_nc4_enum_type(d4_grp,nc4_grp_id);

#if !NDEBUG 
            auto d4enum_defs= d4_grp->enum_defs();
            for (D4EnumDefs::D4EnumDefIter ed_i = d4enum_defs->enum_begin(), ed_e= d4enum_defs->enum_end(); ed_i != ed_e; ++ed_i) {

                BESDEBUG(MODULE, prolog <<"d4enumdef name: "<<(*ed_i)->name()<<endl);
                BESDEBUG(MODULE, prolog <<"d4enumdef type: "<<(*ed_i)->type()<<endl);

                for (D4EnumDef::D4EnumValueIter edv_i = (*ed_i)->value_begin(), edv_e = (*ed_i)->value_end(); edv_i != edv_e; ++edv_i) {

                    string edv_label = (*ed_i)->label(edv_i);
                    long long edv_value = (*ed_i)->value(edv_i);

                    BESDEBUG(MODULE, prolog <<"edv_label: "<<edv_label <<endl);
                    BESDEBUG(MODULE, prolog <<"edv_value: "<<edv_value<<endl);

                }
            } 
#endif
        }
    }

    // No need to check unlimited dims.
    vector<string> unlimited_dimnames;
    bool has_unlimited_dims =  obtain_unlimited_dimension_info(d4_grp,unlimited_dimnames);

    D4Dimensions *grp_dims = d4_grp->dims();
    for (D4Dimensions::D4DimensionsIter di = grp_dims->dim_begin(), de = grp_dims->dim_end(); di != de; ++di) {

#if !NDEBUG
        BESDEBUG(MODULE, prolog << "Check dimensions" << endl);
        BESDEBUG(MODULE, prolog << "Dim name is: " << (*di)->name() << endl);
        BESDEBUG(MODULE, prolog << "Dim size is: " << (*di)->size() << endl);
        BESDEBUG(MODULE, prolog << "Fully_qualfied_dim name is: " << (*di)->fully_qualified_name() << endl);
#endif

        int64_t dimsize = (*di)->size();

        // The dimension size may need to be updated because of the expression constraint.
        map<string, int64_t>::iterator it;
        for (it = GFQN_dimname_to_dimsize.begin(); it != GFQN_dimname_to_dimsize.end(); ++it) {
            if (it->first == (*di)->fully_qualified_name())
                dimsize = it->second;
        }

        // Define dimension. 
        int g_dimid = -1;
        
        if (has_unlimited_dims) {
            for (const auto &und:unlimited_dimnames) {
                if (und == (*di)->name()) { 
                    dimsize = NC_UNLIMITED;
                    break;
                }
            }
        }
        stax = nc_def_dim(nc4_grp_id, (*di)->name().c_str(), dimsize, &g_dimid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to define dimension: " + _localfile, __FILE__,
                                    __LINE__);
        // Save this dimension ID in a map.
        fdimname_to_id[(*di)->fully_qualified_name()] = g_dimid;
    }

    Constructor::Vars_iter vi = d4_grp->var_begin();
    Constructor::Vars_iter ve = d4_grp->var_end();

    vector < FONcBaseType * > fonc_vars_in_grp;

    // If the global_dio_flag is false, we cannot do direct IO for this file at all. No need to check every variable.
    // This is necessary since direct IO cannot apply to the current existing dmrpp files in the earth data cloud. KY 2023-12-11
    if (global_dio_flag == false) {
        for (; vi != ve; vi++) {
            if ((*vi)->send_p()) {
                BaseType *v = *vi;
    
                BESDEBUG(MODULE, prolog << "Converting variable '" << v->name() << "'" << endl);
    
                // This is a factory class call, and 'fg' is specialized for 'v'
                FONcBaseType *fb = FONcUtils::convert(v, FONC_RETURN_AS_NETCDF4, false, fdimname_to_id, rds_nums, GFQN_to_en_typeid_vec,root_no_grp_unlimited_dimnames);
    
                fonc_vars_in_grp.push_back(fb);
    
                // This is needed to avoid the memory leak.
                _total_fonc_vars_in_grp.push_back(fb);
    
                vector <string> embed;
                // This call sets d_is_dap4 to true, and d_is_dap4_group to true. jhrg 2/14/24
                fb->convert(embed, true, true);
            }
        }
    }
    else {
        for (; vi != ve; vi++) {
            if ((*vi)->send_p()) {
                BaseType *v = *vi;
    
                BESDEBUG(MODULE, prolog << "Converting variable '" << v->name() << "'" << endl);
    
                if (v->type() == dods_array_c) {
                    auto t_a = dynamic_cast<Array *>(v);
                    if (t_a->get_dio_flag()) { 
                        set_constraint_var_dio_flag(t_a);
#if 0
                        bool var_has_unlim_dim = false;
                        var_has_unlim_dim = check_var_unlimited_dimension(t_a,unlimited_dimnames);
                        if (var_has_unlim_dim)
                            t_a->set_dio_flag(false);
                        else 
                            set_constraint_var_dio_flag(t_a);
#endif
                    }
                }
    
                // This is a factory class call, and 'fg' is specialized for 'v'
                FONcBaseType *fb = FONcUtils::convert(v, FONC_RETURN_AS_NETCDF4, false, fdimname_to_id, rds_nums,GFQN_to_en_typeid_vec, root_no_grp_unlimited_dimnames);
    
                fonc_vars_in_grp.push_back(fb);
    
                // This is needed to avoid the memory leak.
                _total_fonc_vars_in_grp.push_back(fb);
    
                vector <string> embed;
                // This call sets d_is_dap4 to true, and d_is_dap4_group to true. jhrg 2/14/24
                fb->convert(embed, true, true);
            }
        }
    }

#if !NDEBUG
    if (d4_grp->grp_begin() == d4_grp->grp_end())
        BESDEBUG(MODULE, prolog << "No group" << endl);
    else
        BESDEBUG(MODULE, prolog << "Has group" << endl);
#endif

    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        //nc_redef(_ncid);

        vector<FONcBaseType *>::iterator i = fonc_vars_in_grp.begin();
        vector<FONcBaseType *>::iterator e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG(MODULE,  prolog << "Defining variable:  " << fbt->name() << endl);
            //fbt->set_is_dap4(true);
            fbt->define(nc4_grp_id);
        }

        bool is_netCDF_enhanced = false;
        if (FONcTransform::_returnAs == FONC_RETURN_AS_NETCDF4 && FONcRequestHandler::classic_model == false)
            is_netCDF_enhanced = true;

        bool add_attr = true;

        // Only the root attribute may be ignored.
        if (FONcRequestHandler::no_global_attrs == true && is_root_grp == true)
            add_attr = false;

        if (true == add_attr) {
            D4Attributes *d4_attrs = d4_grp->attributes();
            BESDEBUG(MODULE, prolog << "Adding Group Attributes" << endl);
            // add dap4 group attributes.
            FONcAttributes::add_dap4_attributes(nc4_grp_id, NC_GLOBAL, d4_attrs, "", "", is_netCDF_enhanced);
            // *** Add the json history here
        }

        // Write every variable in this group. 
        i = fonc_vars_in_grp.begin();
        e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmitting: " + fbt->name() , __FILE__, __LINE__);
            BESDEBUG(MODULE, prolog << "Writing data for variable:  " << fbt->name() << endl);
            fbt->write(nc4_grp_id);
        }

        // Now handle all the child groups.
        for (D4Group::groupsIter gi = d4_grp->grp_begin(), ge = d4_grp->grp_end(); gi != ge; ++gi) {
            BESDEBUG(MODULE, prolog << "In group:  " << (*gi)->name() << endl);
            transform_dap4_group(*gi, false, nc4_grp_id, fdimname_to_id, rds_nums);
        }

    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }
    BESDEBUG(MODULE,  prolog << "END" << endl);
}


// Group support is only on when netCDF-4 is in enhanced model and there are groups in the DMR.
bool FONcTransform::check_group_support() {
    if (FONC_RETURN_AS_NETCDF4 == FONcTransform::_returnAs && false == FONcRequestHandler::classic_model &&
        (_dmr->root()->grp_begin() != _dmr->root()->grp_end()))
        return true;
    else
        return false;
}

// Generate the final group list in the netCDF-4 file. Empty groups and their attributes will be removed.
void FONcTransform::gen_included_grp_list(D4Group *grp) {
    bool grp_has_var = false;
    if (grp) {
        BESDEBUG(MODULE,  prolog << "Processing D4 Group: " << grp->name() << " fullpath: " << grp->FQN() << endl);

        if (grp->var_begin() != grp->var_end()) {

            BESDEBUG(MODULE, prolog << "Has child variables" << endl);
            Constructor::Vars_iter vi = grp->var_begin();
            Constructor::Vars_iter ve = grp->var_end();

            for (; vi != ve; vi++) {

                // This variable is selected(in the local constraints).
                if ((*vi)->send_p()) {
                    grp_has_var = true;

                    //If a var in this group is selected, we need to include this group in the netcdf-4 file.
                    //We always include root attributes, so no need to obtain grp_names for the root.
                    if (grp->FQN() != "/")
                        _included_grp_names.insert(grp->FQN());
                    break;
                }
            }
        }
        // Loop through the subgroups to build up the list.
        for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
            BESDEBUG(MODULE, prolog << "Obtain included groups - group name:  " << (*gi)->name() << endl);
            gen_included_grp_list(*gi);
        }
    }

    // If this group is in the final list, all its ancestors(except root, since it is always selected),should also be included. 
    if (grp_has_var == true) {
        D4Group *temp_grp = grp;
        while (temp_grp) {
            if (temp_grp->get_parent()) {
                temp_grp = static_cast<D4Group *>(temp_grp->get_parent());
                if (temp_grp->FQN() != "/")
                    _included_grp_names.insert(temp_grp->FQN());
            }
            else
                temp_grp = 0;
        }
    }

}

void FONcTransform::check_and_obtain_dimensions(D4Group *grp, bool is_root_grp) {

    // We may not need to do this way,it may overkill.
    bool included_grp = false;

    if (_dmr->get_ce_empty())
        included_grp = true;
        // Always include the root attributes.
    else if (is_root_grp == true)
        included_grp = true;
    else {
        // Check if this group is in the group list kept in the file.
        set<string>::iterator iset;
        if (_included_grp_names.find(grp->FQN()) != _included_grp_names.end())
            included_grp = true;
    }

    if (included_grp == true)
        check_and_obtain_dimensions_internal(grp);
}

void FONcTransform::check_and_obtain_dimensions_internal(D4Group *grp) {

    // Remember the Group Fully Qualified dimension Name and the corresponding dimension size.
    D4Dimensions *grp_dims = grp->dims();
    if (grp_dims) {
        for (D4Dimensions::D4DimensionsIter di = grp_dims->dim_begin(), de = grp_dims->dim_end(); di != de; ++di) {

#if !NDEBUG
            BESDEBUG(MODULE, prolog << "Check dimensions" << endl);
            BESDEBUG(MODULE, prolog << "Dim name is: " << (*di)->name() << endl);
            BESDEBUG(MODULE, prolog << "Dim size is: " << (*di)->size() << endl);
            BESDEBUG(MODULE, prolog << "Fully qualfied dim name: " << (*di)->fully_qualified_name() << endl);
#endif
            int64_t dimsize = (*di)->size();
            if ((*di)->constrained()) {
                dimsize = ((*di)->c_stop() - (*di)->c_start()) / (*di)->c_stride() + 1;

            }
            GFQN_dimname_to_dimsize[(*di)->fully_qualified_name()] = dimsize;
        }
    }

    // The size of DAP4 dimension needs to be updated if the dimension size of a variable with the same dimension is 
    // different. So we also need to remember the Variable FQN dimension name and size. 
    // Check section 8.6.2 of DAP4 specification(https://docs.opendap.org/index.php/DAP4:_Specification_Volume_1)
    Constructor::Vars_iter vi = grp->var_begin();
    Constructor::Vars_iter ve = grp->var_end();
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            if ((*vi)->is_vector_type()) {
                Array *t_a = dynamic_cast<Array *>(*vi);
                Array::Dim_iter dim_i = t_a->dim_begin();
                Array::Dim_iter dim_e = t_a->dim_end();
                for (; dim_i != dim_e; dim_i++) {
                    if ((*dim_i).name != "") {
                        D4Dimension *d4dim = t_a->dimension_D4dim(dim_i);
                        if (d4dim) {
                            BESDEBUG(MODULE, prolog << "Check dim- dim name is: " << d4dim->name() << endl);
                            BESDEBUG(MODULE, prolog << "Check dim- dim size is: " << d4dim->size() << endl);
                            BESDEBUG(MODULE, prolog << "Check dim- fully_qualfied_dim name is: "
                                    << d4dim->fully_qualified_name() << endl);

                            int64_t dimsize = t_a->dimension_size_ll(dim_i, true);
#if !NDEBUG
                            BESDEBUG(MODULE, prolog << "Check dim- final dim size is: " << dimsize << endl);
#endif
                            pair<map<string, int64_t>::iterator, bool> ret_it;
                            ret_it = VFQN_dimname_to_dimsize.insert(
                                    pair<string, int64_t>(d4dim->fully_qualified_name(), dimsize));
                            if (ret_it.second == false && ret_it.first->second != dimsize) {
                                string err = "fileout_netcdf-4: dimension found with the same name, but different size";
                                throw BESInternalError(err, __FILE__, __LINE__);
                            }
                            //VFQN_dimname_to_dimsize[d4dim->fully_qualified_name()] = dimsize;
                        }
                        else
                            throw BESInternalError("Has dimension name but D4 dimension is NULL", __FILE__, __LINE__);
                    }
                    // No need to handle the case when the dimension name doesn't exist. This will be handled in FONcArray.cc.
                    // else { } 
                }
            }
        }
    }

#if !NDEBUG
    map<string, int64_t>::iterator it;
    for (it = GFQN_dimname_to_dimsize.begin(); it != GFQN_dimname_to_dimsize.end(); ++it) {
        BESDEBUG(MODULE, prolog << "GFQN dim name is: " << it->first << endl);
        BESDEBUG(MODULE, prolog << "GFQN dim size is: " << it->second << endl);
    }

    for (it = VFQN_dimname_to_dimsize.begin(); it != VFQN_dimname_to_dimsize.end(); ++it) {
        BESDEBUG(MODULE, prolog << "VFQN dim name is: " << it->first << endl);
        BESDEBUG(MODULE, prolog << "VFQN dim size is: " << it->second << endl);
    }

#endif

    // Go through all the descendent groups.
    for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
        BESDEBUG(MODULE,prolog << "In group:  " << (*gi)->name() << endl);
        check_and_obtain_dimensions(*gi, false);
    }

}
void FONcTransform::set_constraint_var_dio_flag(libdap::Array* t_a) const {

    // The last check to see if the direct io can be done is to check if
    // this array is subset. If yes, we cannot use direct IO.

    bool partial_subset_array = false;
    Array::Dim_iter di = t_a->dim_begin();
    Array::Dim_iter de = t_a->dim_end();
    for (; di != de; di++) {
        if (t_a->dimension_size_ll(di,true) != t_a->dimension_size_ll(di, false)) {
            partial_subset_array = true;
            break;
        }
    }
    if (partial_subset_array)
        t_a->set_dio_flag(false);

}

bool FONcTransform::check_reduce_dim() {

    bool ret_value = true;
    D4Group *root_grp = _dmr->root();
    ret_value = check_reduce_dim_internal(root_grp);
    return ret_value;
}

bool FONcTransform::check_reduce_dim_internal(D4Group*grp) {

    bool ret_value = true;
    D4Dimensions *grp_dims = grp->dims();

    // Check DAP4 dimensions
    for (D4Dimensions::D4DimensionsIter di = grp_dims->dim_begin(), de = grp_dims->dim_end(); di != de; ++di) {
        if((*di)->name().empty() == false) {
            ret_value = false;
            break;
        }
    }

    // Check DAP4 variables
    if (ret_value) {
        for (const auto &var:grp->variables()) {
            if (var->send_p()) {
                ret_value = check_var_dim(var);
                if (ret_value == false)
                    break;
            }
        }
    }
    // Check the children groups
    if (ret_value) {
        for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
            ret_value = check_reduce_dim_internal(*gi);
            if (ret_value ==false) 
                break;
        }
    }
    return ret_value;

}

bool FONcTransform::check_var_dim(BaseType *var) {

    bool ret_value = true;

    if (var->type() == dods_array_c) {

        auto t_a = dynamic_cast<Array *>(var);
        Array::Dim_iter dim_i = t_a->dim_begin();
        Array::Dim_iter dim_e = t_a->dim_end();
        for (; dim_i != dim_e; dim_i++) {
            if ((*dim_i).name != "") {
                ret_value = false;
                break;
            }
        }
    }
    return ret_value;
}

void FONcTransform::build_reduce_dim() {

    D4Group *root_grp = _dmr->root();
    build_reduce_dim_internal(root_grp, root_grp);
}

void FONcTransform::build_reduce_dim_internal(D4Group *grp, D4Group *root_grp) {

    for (auto &var:grp->variables()) {

        if (var->type() == dods_array_c && var->send_p()) {
            auto t_a = dynamic_cast<Array *>(var);

            unordered_map<int64_t,int> local_dsize_count;
 
            Array::Dim_iter dim_i = t_a->dim_begin();
            Array::Dim_iter dim_e = t_a->dim_end();
            for (; dim_i != dim_e; dim_i++) {

                if ((*dim_i).name == "") {
   
                    int64_t dimsize = t_a->dimension_size_ll(dim_i, true);

                    // We need to update the number of occurences this dim size is used in this array.
                    bool local_dsize_found = false;
                    if(local_dsize_count.find(dimsize)!=local_dsize_count.end()) {
                        int prev_count = local_dsize_count[dimsize];
                        local_dsize_count[dimsize] = prev_count +1;
                        local_dsize_found = true;
                    }
                    else  
                        local_dsize_count[dimsize] = 1;
                    
                    bool dim_name_exist = false;
                    auto it_sn=dimsize_to_dup_dimnames.find(dimsize);
                    if (it_sn !=dimsize_to_dup_dimnames.end()) {
                        vector<string>temp_dimnames = dimsize_to_dup_dimnames[dimsize];
                        if (local_dsize_found) {

                            int temp_local_dsize_count = local_dsize_count[dimsize];

                            // Now we need to create a new dim name for this dimension
                            // since for this size, the unique dimension names are used up.
                            if (temp_local_dsize_count > temp_dimnames.size()) {
                                string dim_name_suffix= to_string(reduced_dim_num);
                                (*dim_i).name ="dim" + dim_name_suffix;
                                reduced_dim_num++;
                                // Update the global map
                                temp_dimnames.push_back((*dim_i).name);
                                dimsize_to_dup_dimnames[dimsize]=temp_dimnames;
                            }
                            else {//Pick up the earliest created non-used dimension name.
                                (*dim_i).name = temp_dimnames[temp_local_dsize_count-1];
                                dim_name_exist = true;
                            }

                        }
                        else { // Use the first created dimension name for this dimension size.
                            (*dim_i).name = temp_dimnames[0];
                            dim_name_exist = true;
                        }
                    }
                    else { // This dimension has not been assigned a name in this file so far,assign it.
                        string dim_name_suffix= to_string(reduced_dim_num);
                        (*dim_i).name ="dim" + dim_name_suffix;
                        reduced_dim_num++;
                        vector<string>temp_dimnames;
                        temp_dimnames.push_back((*dim_i).name);
                        dimsize_to_dup_dimnames[dimsize] = temp_dimnames;
                    }

                    if (dim_name_exist) {

                        D4Dimensions *dims = root_grp->dims();
                        D4Dimension *d4_dim = dims->find_dim((*dim_i).name);
                        if(d4_dim == nullptr)
                            throw BESInternalError("D4 dimension cannot be found", __FILE__, __LINE__);
                        else 
                            (*dim_i).dim= d4_dim;
                    }
                    else {
                        // We need to add D4Dimension and group dimensions.
                         auto d4_dim0_unique = make_unique<D4Dimension>((*dim_i).name, dimsize);
                         (*dim_i).dim=d4_dim0_unique.get();
        
                         // The DAP4 group needs also to store these dimensions. 
                         D4Dimensions *dims = root_grp->dims();
                         dims->add_dim_nocopy(d4_dim0_unique.release());
                    }
                }
            }
        }
    }

    for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) 
        build_reduce_dim_internal(*gi,root_grp); 

}

bool FONcTransform::obtain_unlimited_dimension_info_helper(libdap::D4Attributes *d4_attrs, vector<string> &unlimited_dim_names){

    bool ret_value = false;

    if (d4_attrs->empty() == false) {
        for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end();
                     ii != ee; ++ii) {
            if ((*ii)->type() == attr_str_c && (*ii)->name()=="Unlimited_Dimension") {
                for (D4Attribute::D4AttributeIter vi = (*ii)->value_begin(), ve = (*ii)->value_end(); vi != ve; vi++) 
                      unlimited_dim_names.emplace_back(*vi);
                if (unlimited_dim_names.empty() == false)
                    ret_value = true;
                break;
            }
        }
    }
    
    return ret_value;
}

bool FONcTransform::obtain_unlimited_dimension_info(libdap::D4Group *d4_grp, vector<string> &unlimited_dim_names) {

    bool ret_value = false;
    D4Attributes *d4_attrs = d4_grp->attributes();
    if (d4_attrs->empty() == false) {
        for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end();
                     ii != ee; ++ii) {
            if ((*ii)->type() == attr_container_c && (*ii)->name()=="DODS_EXTRA") {
                D4Attributes *extra_attrs = (*ii)->attributes();
                ret_value = obtain_unlimited_dimension_info_helper(extra_attrs,unlimited_dim_names);
                if (ret_value == true)
                    break;
            }
        }
    }

    return ret_value; 
}

#if 0
bool FONcTransform::check_var_unlimited_dimension(libdap::Array *t_a, const vector<string> &unlimited_dimnames) {

    bool ret_value = false;
    for (const auto &und:unlimited_dimnames) {
        Array::Dim_iter dim_i = t_a->dim_begin();
        Array::Dim_iter dim_e = t_a->dim_end();
        for (; dim_i != dim_e; dim_i++) {
            if ((*dim_i).name == und) {
                ret_value = true;
                break;
            }
        }
        if (ret_value)
            break;
    }
    return ret_value;
 
}
#endif

void FONcTransform::gen_nc4_enum_type(libdap::D4Group *d4_grp,int nc4_grp_id) {

    string grp_fqn = d4_grp->FQN();
    D4EnumDefs * d4enum_defs = d4_grp->enum_defs();
    vector<pair<string,int>> en_type_id_vec;

    for (D4EnumDefs::D4EnumDefIter ed_i = d4enum_defs->enum_begin(), ed_e= d4enum_defs->enum_end(); ed_i != ed_e; ++ed_i) {

        int nc4_type_id = 0;
        nc_type nc4_enum_type = FONcUtils::dap4_int_float_type_to_nc4_type((*ed_i)->type());
        string nc4_enum_name = (*ed_i)->name();
        int stat = nc_def_enum(nc4_grp_id, nc4_enum_type,nc4_enum_name.c_str(), &nc4_type_id);
        if (stat != NC_NOERR)
            throw BESInternalError("nc_def_enum failed.", __FILE__, __LINE__);

        for (D4EnumDef::D4EnumValueIter edv_i = (*ed_i)->value_begin(), edv_e = (*ed_i)->value_end(); edv_i != edv_e; ++edv_i) {
            string edv_label = (*ed_i)->label(edv_i);
            long long edv_value = (*ed_i)->value(edv_i);
            switch(nc4_enum_type) {
                case NC_UBYTE:
                {
                    auto nc_enum_const = (uint8_t)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }
                    break;
                case NC_BYTE:
                {
                    auto nc_enum_const = (int8_t)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }
                    break;
                case NC_USHORT:
                {
                    auto nc_enum_const = (unsigned short)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }
                    break;
                case NC_SHORT:
                {
                    auto nc_enum_const = (short)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }
                    break;
                case NC_UINT:
                {
                    auto nc_enum_const = (unsigned int)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }
                    break;
                case NC_INT:
                {
                    auto nc_enum_const = (int)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }
                    break;

                case NC_UINT64:
                {
                    auto nc_enum_const = (uint64_t)edv_value;
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&nc_enum_const);
                }

                    break;
                case NC_INT64:
                    stat = nc_insert_enum(nc4_grp_id,nc4_type_id,edv_label.c_str(),(const void*)&edv_value);
                    break;
                default:
                    throw BESInternalError("Unsupported enum base type", __FILE__, __LINE__);
            }
            if (stat !=NC_NOERR)
                throw BESInternalError("nc_insert_enum failed.", __FILE__, __LINE__);

        }

        pair<string,int> en_type_id = make_pair(nc4_enum_name, nc4_type_id);
        en_type_id_vec.emplace_back(en_type_id);
        
    }
    GFQN_to_en_typeid_vec[grp_fqn] = en_type_id_vec;

}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FONc objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcTransform::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "FONcTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "ncid = " << _ncid << endl;
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    BESIndent::Indent();
    vector<FONcBaseType *>::const_iterator i = _fonc_vars.begin();
    vector<FONcBaseType *>::const_iterator e = _fonc_vars.end();
    for (; i != e; i++) {
        FONcBaseType *fbt = *i;
        fbt->dump(strm);
    }
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}


