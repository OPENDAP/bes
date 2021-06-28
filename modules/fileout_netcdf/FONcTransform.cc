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

#include "FONcRequestHandler.h" // for the keys

#include "FONcTransform.h"
#include "FONcUtils.h"
#include "FONcBaseType.h"
#include "FONcAttributes.h"
#include "FONcTransmitter.h"
#include "history_utils.h"

#include <DDS.h>
#include <DMR.h>
#include <D4Group.h>
#include <D4Attributes.h>
#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>

#include "DapFunctionUtils.h"

using std::ostringstream;
using std::istringstream;

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile, const string &ncVersion) :
        _ncid(0), _dds(nullptr), _dmr(nullptr), d_obj(nullptr), d_dhi(nullptr)
{
    if (!dds) {
        string s = (string) "File out netcdf, " + "null DDS passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (localfile.empty()) {
        string s = (string) "File out netcdf, " + "empty local file name passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    _localfile = localfile;
    _dds = dds;
    _returnAs = ncVersion;

    // if there is a variable, attribute, dimension name that is not
    // compliant with netcdf naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container();
    if (dhi.container) {
        FONcUtils::name_prefix = dhi.container->get_container_type() + "_";
    }
    else {
        FONcUtils::name_prefix = "nc_";
    }
}

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dmr DMR object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform(DMR *dmr, BESDataHandlerInterface &dhi, const string &localfile, const string &ncVersion) :
        _ncid(0), _dds(nullptr), _dmr(nullptr), d_obj(nullptr), d_dhi(nullptr)
{
    if (!dmr) {
        string s = (string) "File out netcdf, " + "null DDS passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (localfile.empty()) {
        string s = (string) "File out netcdf, " + "empty local file name passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    _localfile = localfile;
    _dmr = dmr;
    _returnAs = ncVersion;

    // if there is a variable, attribute, dimension name that is not
    // compliant with netcdf naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container();
    if (dhi.container) {
        FONcUtils::name_prefix = dhi.container->get_container_type() + "_";
    }
    else {
        FONcUtils::name_prefix = "nc_";
    }
}

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
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
                             const string &ncVersion) :
        _ncid(0), _dds(nullptr), d_obj(obj), d_dhi(dhi), _localfile(localfile), _returnAs(ncVersion)
{
    if (!d_obj) {
        string s = (string) "File out netcdf, " + "null BESResponseObject passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (_localfile.empty()) {
        string s = (string) "File out netcdf, " + "empty local file name passed to constructor";
        throw BESInternalError(s, __FILE__, __LINE__);
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
FONcTransform::~FONcTransform()
{
    bool done = false;
    while (!done) {
        vector<FONcBaseType *>::iterator i = _fonc_vars.begin();
        vector<FONcBaseType *>::iterator e = _fonc_vars.end();
        if (i == e) {
            done = true;
        }
        else {
            // These are the FONc types, not the actual ones
            FONcBaseType *b = (*i);
            delete b;
            _fonc_vars.erase(i);
        }
    }
    done = false;
    while (!done) {
        vector<FONcBaseType *>::iterator i = _total_fonc_vars_in_grp.begin();
        vector<FONcBaseType *>::iterator e = _total_fonc_vars_in_grp.end();
        if (i == e) {
            done = true;
        }
        else {
            // These are the FONc types, not the actual ones
            FONcBaseType *b = (*i);
            delete b;
            _total_fonc_vars_in_grp.erase(i);
        }
    }

}

// previous transform() fct, keep for rollback purposes. sbl 5.14.21
#if 0
/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FONcTransform::transform()
{
    FONcUtils::reset();

    // Convert the DDS into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    DDS::Vars_iter vi = _dds->var_begin();
    DDS::Vars_iter ve = _dds->var_end();
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            FONcBaseType *fb = FONcUtils::convert(v,FONcTransform::_returnAs,FONcRequestHandler::classic_model);
#if 0
            fb->setVersion( FONcTransform::_returnAs );
            if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
                if (FONcRequestHandler::classic_model)
                    fb->setNC4DataModel("NC4_CLASSIC_MODEL");
                else 
                    fb->setNC4DataModel("NC4_ENHANCED");
            }
#endif
            _fonc_vars.push_back(fb);
            vector<string> embed;
            fb->convert(embed);
        }
    }

    // Open the file for writing
    int stax;
    if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
        if (FONcRequestHandler::classic_model){
            BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-4 cache file in classic mode. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
    }

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
            BESDEBUG("fonc", "FONcTransform::transform() - Defining variable:  " << fbt->name() << endl);
            fbt->define(_ncid);
        }

        if(FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            AttrTable &globals = _dds->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform() - Adding Global Attributes" << endl << globals << endl);
            bool is_netCDF_enhanced = false;
            if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_attributes(_ncid, NC_GLOBAL, globals, "", "",is_netCDF_enhanced);
        }

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
        if (stax != NC_NOERR) {
            FONcUtils::handle_error(stax, "File out netcdf, unable to end the define mode: " + _localfile, __FILE__, __LINE__);
        }

        // Write everything out
        i = _fonc_vars.begin();
        e = _fonc_vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform() - Writing data for variable:  " << fbt->name() << endl);
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
#endif

/** @brief Transforms each of the variables of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void FONcTransform::transform_dap2(ostream &strm)
{
#if 0
    BESDapResponseBuilder responseBuilder;
    // Use the DDS from the ResponseObject along with the parameters
    // from the DataHandlerInterface to load the DDS with values.
    // Note that the BESResponseObject will manage the loaded_dds object's
    // memory. Make this a shared_ptr<>. jhrg 9/6/16
#endif
    // Now that we are ready to start reading the response data we
    // cancel any pending timeout alarm according to the configuration.
    BESUtil::conditional_timeout_cancel();

    BESDEBUG("fonc", "FONcTransmitter::send_data() - Reading data into DataDDS" << endl);

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


    // This function is used by all fileout modules and they need to include the attributes in data access.
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
        BESDEBUG("fonc","Found function(s) in CE: " << besDRB.get_btp_func_ce() << endl);

        BESDapFunctionResponseCache *responseCache = BESDapFunctionResponseCache::get_instance();

        ConstraintEvaluator func_eval;
        DDS *fdds = 0; // nulll_ptr
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
        // sent. If that is empty (there was only a function call) all
        // of the variables in the intermediate DDS (i.e., the function
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

    //throw_if_dap2_response_too_big(_dds); // TODO Fix this, sbl 5/6/21

    // Convert the DDS into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    for (auto vi = _dds->var_begin(), ve = _dds->var_end(); vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BESDEBUG("fonc", "FONcTransform::transform() - Converting variable '" << (*vi)->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for '*vi'
            FONcBaseType *fb = FONcUtils::convert((*vi), FONcTransform::_returnAs, FONcRequestHandler::classic_model);

            _fonc_vars.push_back(fb);
            vector<string> embed;
            fb->convert(embed);
        }
    }

    updateHistoryAttributes(_dds, d_dhi->data[POST_CONSTRAINT]);

    // Open the file for writing
    int stax;
    if (FONcTransform::_returnAs == RETURN_AS_NETCDF4) {
        if (FONcRequestHandler::classic_model) {
            BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-4 cache file in classic mode. fileName:  "
                    << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4 | NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc",
                     "FONcTransform::transform() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc", "FONcTransform::transform() - Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
    }

    int current_fill_prop_vaule;

    stax = nc_set_fill(_ncid, NC_NOFILL, &current_fill_prop_vaule);
    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to set fill to NC_NOFILL: " + _localfile, __FILE__, __LINE__);
    }

    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        nc_redef(_ncid);

        // For each converted FONc object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
        for (FONcBaseType *fbt: _fonc_vars) {
            BESDEBUG("fonc", "FONcTransform::transform() - Defining variable:  " << fbt->name() << endl);
            fbt->define(_ncid);
        }

        if (FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            AttrTable &globals = _dds->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform() - Adding Global Attributes" << endl << globals << endl);
            bool is_netCDF_enhanced = false;
            if (FONcTransform::_returnAs == RETURN_AS_NETCDF4 && FONcRequestHandler::classic_model == false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_attributes(_ncid, NC_GLOBAL, globals, "", "", is_netCDF_enhanced);
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
        // write file data
        uint64_t byteCount = 0;

        if (is_streamable()) {
        byteCount = BESUtil::file_to_stream_helper(_localfile, strm, byteCount);
        BESDEBUG("fonc", "FONcTransform::transform() - first write data to stream, count:  " << byteCount << endl);
        }

        for (FONcBaseType *fbt: _fonc_vars) {
            BESDEBUG("fonc", "FONcTransform::transform() - Writing data for variable:  " << fbt->name() << endl);

            fbt->set_dds(_dds);
            fbt->set_eval(&eval);

            fbt->write(_ncid);
            nc_sync(_ncid);


            if (is_streamable()) {
                // write the whats been written
                byteCount = BESUtil::file_to_stream_helper(_localfile, strm, byteCount);
                BESDEBUG("fonc", "FONcTransform::transform() - Writing data to stream, count:  " << byteCount << endl);
            }
        }

        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);

        byteCount = BESUtil::file_to_stream_helper(_localfile, strm, byteCount);
        BESDEBUG("fonc", "FONcTransform::transform() - after nc_close() count:  " << byteCount << endl);
    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }
}

/** @brief checks if a netcdf file is streamable
 *
 * /!\ WARNING /!\ DDS/DMR object must be correctly constructed for this function to work
 * checks if a netcdf file is to be returned as netcdf-4 and if so is not streamable
 * if file is returned as netcdf-3 then checks if the dds/dmr has a structure datatype
 * @return false if file returns as netcdf-4 OR has a structure datatype
 */
bool FONcTransform::is_streamable(){
    if (FONcTransform::_returnAs == RETURN_AS_NETCDF4){
        return false;
    }

    if (_dds != nullptr){
        return is_dds_streamable();
    }
    else {
        return is_dmr_streamable(_dmr->root());
    }
}

/** @brief checks if a DDS contains a Structure datatype in its variables
 *
 * checks the variable type for a structure datatype
 * @return false if the dds contains a structure datatype
 */
bool FONcTransform::is_dds_streamable(){
    for (auto var = _dds->var_begin(), varEnd = _dds->var_end(); var != varEnd; ++var) {
        if ((*var)->type() == dods_structure_c) {
            return false; // cannot be streamed
        }
    }
    return true;
}

/** checks if a DMR contains a Structure datatype in its variables
 *
 * checks the variable type for a structure datatype
 * @param group the D4Group holding the variables to search through
 * @return false if the dmr contains a structure datatype
 */
bool FONcTransform::is_dmr_streamable(D4Group *group){
    for (auto var = group->var_begin(), varEnd = group->var_end(); var != varEnd; ++var) {
        if ((*var)->type() == dods_structure_c)
            return false ; // cannot be streamed

        if ((*var)->type() == dods_group_c) {
            D4Group *g = dynamic_cast<D4Group *>(*var);
            if (g != nullptr && !is_dmr_streamable(g)) {
                return false;
            }
        }
    }
    return true;
}


/** @brief Transforms each of the variables of the DMR to the NetCDF
 * file
 *
 * For each variable in the DMR write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DMR.
 */
void FONcTransform::transform_dap4()
{
    BESUtil::conditional_timeout_cancel();
    BESDEBUG("fonc", "FONcTransform::transform_dap4() Reading data into DataDMR" << endl);

    FONcUtils::reset();

    d_dhi->first_container();

    BESDapResponseBuilder responseBuilder;
    _dmr = responseBuilder.setup_dap4_intern_data(d_obj, *d_dhi).release();

    BESDapResponseBuilder besDRB;

    besDRB.set_dataset_name(_dmr->filename());

    // Added set of DAP4 fields. jhrg 5/30/21
    besDRB.set_dap4ce(d_dhi->data[DAP4_CONSTRAINT]);
    besDRB.set_dap4function(d_dhi->data[DAP4_FUNCTION]);

    besDRB.set_async_accepted(d_dhi->data[ASYNC]);
    besDRB.set_store_result(d_dhi->data[STORE_RESULT]);

    // Convert the DMR into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.

    BESDEBUG("fonc", "Coming into transform_dap4() " << endl);

    // First check if this DMR has groups etc.
    bool support_group = check_group_support();

    if (true == support_group) {

        int stax = -1;
        BESDEBUG("fonc",
                 "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4, &_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);

        D4Group *root_grp = _dmr->root();

        // Declare the dimname to dimid map to handle netCDF-4 dimensions
        map<string, int> fdimname_to_id;

        // Generate a list of the groups in the final netCDF file. 
        // The attributes of these groups should be included.
        gen_included_grp_list(root_grp);

#if !NDEBUG
        for (std::set<string>::iterator it=_included_grp_names.begin(); it!=_included_grp_names.end(); ++it)
            BESDEBUG("fonc","included group list name is: "<<*it<<endl);
#endif
        // Build a global dimension name table for all variables if
        // the constraint is not empty!
        check_and_obtain_dimensions(root_grp, true);

        // Don't remove the following code, they are for debugging.
#if !NDEBUG
        map<string,unsigned long>:: iterator it;

        for(it=GFQN_dimname_to_dimsize.begin();it!=GFQN_dimname_to_dimsize.end();++it) {
            BESDEBUG("fonc", "Final GFQN dim name is: "<<it->first<<endl);
            BESDEBUG("fonc", "Final GFQN dim size is: "<<it->second<<endl);
        }

        for(it=VFQN_dimname_to_dimsize.begin();it!=VFQN_dimname_to_dimsize.end();++it) {
            BESDEBUG("fonc", "Final VFQN dim name is: "<<it->first<<endl);
            BESDEBUG("fonc", "Final VFQN dim size is: "<<it->second<<endl);
        }
#endif

        // DAP4 requires the DAP4 dimension sizes defined in the group should be changed
        // according to the corresponding variable sizes. Check section 8.6.2 at
        // https://docs.opendap.org/index.php/DAP4:_Specification_Volume_1
        //
        map<string, unsigned long>::iterator git, vit;
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
        vector<string> root_d4_dimname_list;
        for (git = GFQN_dimname_to_dimsize.begin(); git != GFQN_dimname_to_dimsize.end(); ++git) {
            string d4_temp_dimname = git->first.substr(1);
            //BESDEBUG("fonc", "d4_temp_dimname: "<<d4_temp_dimname<<endl);
            if (d4_temp_dimname.find('/') == string::npos)
                root_d4_dimname_list.push_back(d4_temp_dimname);
        }

#if !NDEBUG
        for(unsigned int i = 0; i <root_d4_dimname_list.size();i++)
            BESDEBUG("fonc", "root_d4 dim name is: "<<root_d4_dimname_list[i]<<endl);
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
                //BESDEBUG("fonc", "temp_suffix: "<<temp_suffix<<endl);
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
        for(unsigned int i = 0; i <root_dim_suffix_nums.size();i++)
            BESDEBUG("fonc", "root_dim_suffix_nums: "<<root_dim_suffix_nums[i]<<endl);


        for(it=GFQN_dimname_to_dimsize.begin();it!=GFQN_dimname_to_dimsize.end();++it) {
            BESDEBUG("fonc", "RFinal GFQN dim name is: "<<it->first<<endl);
            BESDEBUG("fonc", "RFinal GFQN dim size is: "<<it->second<<endl);
        }

        for(it=VFQN_dimname_to_dimsize.begin();it!=VFQN_dimname_to_dimsize.end();++it) {
            BESDEBUG("fonc", "RFinal VFQN dim name is: "<<it->first<<endl);
            BESDEBUG("fonc", "RFinal VFQN dim size is: "<<it->second<<endl);
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

    return;
}

/**
 * @brief Transform the DMR to a netCDF-4 file when there are no DAP4 groups.
 * This routine is similar to transform() that handles DAP2 objects.  However, DAP4 routines are needed.
 * So still keep a separate function. May combine this function with the transform()  in the future.
 */
void FONcTransform::transform_dap4_no_group()
{

    D4Group *root_grp = _dmr->root();
#if !NDEBUG
    D4Dimensions *root_dims = root_grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
    }
#endif
    Constructor::Vars_iter vi = root_grp->var_begin();
    Constructor::Vars_iter ve = root_grp->var_end();

    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_no_group() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            FONcBaseType *fb = FONcUtils::convert(v, FONcTransform::_returnAs, FONcRequestHandler::classic_model);
            _fonc_vars.push_back(fb);

            vector<string> embed;
            fb->convert(embed,true,false);
        }
    }

#if !NDEBUG
    if(root_grp->grp_begin() == root_grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);
   for (D4Group::groupsIter gi = root_grp->grp_begin(), ge = root_grp->grp_end(); gi != ge; ++gi) 
       BESDEBUG("fonc", "FONcTransform::transform_dap4() - group name:  " << (*gi)->name() << endl);
#endif

    updateHistoryAttributes(_dmr, d_dhi->data[POST_CONSTRAINT]);

    // Open the file for writing
    int stax = -1;
    if (FONcTransform::_returnAs == RETURN_AS_NETCDF4) {
        if (FONcRequestHandler::classic_model) {
            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_no_group() - Opening NetCDF-4 cache file in classic mode. fileName:  "
                             << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4 | NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_no_group() - Opening NetCDF-4 cache file. fileName:  " << _localfile
                                                                                                           << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER | NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc",
                 "FONcTransform::transform_dap4_no_group() - Opening NetCDF-3 cache file. fileName:  " << _localfile
                                                                                                       << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER, &_ncid);
    }

    if (stax != NC_NOERR) {
        FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
    }

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
            BESDEBUG("fonc", "FONcTransform::transform_dap4_no_group() - Defining variable:  " << fbt->name() << endl);
            //fbt->set_is_dap4(true);
            fbt->define(_ncid);
        }

        if (FONcRequestHandler::no_global_attrs == false) {

            // Add any global attributes to the netcdf file
            D4Group *root_grp = _dmr->root();
            D4Attributes *d4_attrs = root_grp->attributes();

            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_no_group() handle GLOBAL DAP4 attributes " << d4_attrs << endl);
#if !NDEBUG
            for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
                string name = (*ii)->name();
                BESDEBUG("fonc", "FONcTransform::transform_dap4() GLOBAL attribute name is "<<name <<endl);
            }
#endif
            bool is_netCDF_enhanced = false;
            if (FONcTransform::_returnAs == RETURN_AS_NETCDF4 && FONcRequestHandler::classic_model == false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_dap4_attributes(_ncid, NC_GLOBAL, d4_attrs, "", "", is_netCDF_enhanced);
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
            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_no_group() - Writing data for variable:  " << fbt->name() << endl);
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
                                         vector<int> &root_dim_suffix_nums)
{

    bool included_grp = false;

    if (_dmr->get_ce_empty()) {
        BESDEBUG("fonc",
                 "Check-get_ce_empty. FONcTransform::transform_dap4() in group  - group name:  " << grp->FQN() << endl);
        included_grp = true;
    }
        // Always include the root and its attributes.
    else if (is_root_grp == true)
        included_grp = true;
    else {
        // Check if this group is in the group list kept in the file.
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
 * @ brief The internal routine to transform DMR to netCDF-4 when there are groups.
 * @param grp
 * @param is_root_grp
 * @param par_grp_id
 * @param fdimname_to_id
 * @param rds_nums
 */
void FONcTransform::transform_dap4_group_internal(D4Group *grp,
                                                  bool is_root_grp,
                                                  int par_grp_id, map<string, int> &fdimname_to_id,
                                                  vector<int> &rds_nums)
{

    BESDEBUG("fonc", "transform_dap4_group_internal() - inside" << endl);
    int grp_id = -1;
    int stax = -1;

    updateHistoryAttributes(_dmr, d_dhi->data[POST_CONSTRAINT]);

    if (is_root_grp == true) 
        grp_id = _ncid;
    else {
        stax = nc_def_grp(par_grp_id, (*grp).name().c_str(), &grp_id);
        BESDEBUG("fonc", "transform_dap4_group_internal() - group name is " << (*grp).name() << endl);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to define group: " + _localfile, __FILE__, __LINE__);

    }

    D4Dimensions *grp_dims = grp->dims();
    for (D4Dimensions::D4DimensionsIter di = grp_dims->dim_begin(), de = grp_dims->dim_end(); di != de; ++di) {

#if !NDEBUG
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
#endif

        unsigned long dimsize = (*di)->size();

        // The dimension size may need to be updated because of the expression constraint.
        map<string, unsigned long>::iterator it;
        for (it = GFQN_dimname_to_dimsize.begin(); it != GFQN_dimname_to_dimsize.end(); ++it) {
            if (it->first == (*di)->fully_qualified_name())
                dimsize = it->second;
        }

        // Define dimension. 
        int g_dimid = -1;
        stax = nc_def_dim(grp_id, (*di)->name().c_str(), dimsize, &g_dimid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to define dimension: " + _localfile, __FILE__,
                                    __LINE__);
        // Save this dimension ID in a map.
        fdimname_to_id[(*di)->fully_qualified_name()] = g_dimid;
    }

    Constructor::Vars_iter vi = grp->var_begin();
    Constructor::Vars_iter ve = grp->var_end();

    vector<FONcBaseType *> fonc_vars_in_grp;
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_group() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            //FONcBaseType *fb = FONcUtils::convert(v,FONcTransform::_returnAs,FONcRequestHandler::classic_model);
            FONcBaseType *fb = FONcUtils::convert(v, RETURN_AS_NETCDF4, false, fdimname_to_id, rds_nums);

            fonc_vars_in_grp.push_back(fb);

            // This is needed to avoid the memory leak.
            _total_fonc_vars_in_grp.push_back(fb);

            vector<string> embed;
            fb->convert(embed, true,true);
        }
    }

#if !NDEBUG
    if(grp->grp_begin() == grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);
#endif


    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        //nc_redef(_ncid);

        vector<FONcBaseType *>::iterator i = fonc_vars_in_grp.begin();
        vector<FONcBaseType *>::iterator e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform_dap4_group() - Defining variable:  " << fbt->name() << endl);
            //fbt->set_is_dap4(true);
            fbt->define(grp_id);
        }

        bool is_netCDF_enhanced = false;
        if (FONcTransform::_returnAs == RETURN_AS_NETCDF4 && FONcRequestHandler::classic_model == false)
            is_netCDF_enhanced = true;


        bool add_attr = true;

        // Only the root attribute may be ignored.
        if (FONcRequestHandler::no_global_attrs == true && is_root_grp == true)
            add_attr = false;

        if (true == add_attr) {
            D4Attributes *d4_attrs = grp->attributes();
            BESDEBUG("fonc", "FONcTransform::transform_dap4_group() - Adding Group Attributes" << endl);
            // add dap4 group attributes.
            FONcAttributes::add_dap4_attributes(grp_id, NC_GLOBAL, d4_attrs, "", "", is_netCDF_enhanced);
        }

        // Write every variable in this group. 
        i = fonc_vars_in_grp.begin();
        e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc",
                     "FONcTransform::transform_dap4_group() - Writing data for variable:  " << fbt->name() << endl);
            //fbt->write(_ncid);
            fbt->write(grp_id);
        }

        // Now handle all the child groups.
        for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
            BESDEBUG("fonc", "FONcTransform::transform_dap4() in group  - group name:  " << (*gi)->name() << endl);
            transform_dap4_group(*gi, false, grp_id, fdimname_to_id, rds_nums);
        }

    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }

}


// Group support is only on when netCDF-4 is in enhanced model and there are groups in the DMR.
bool FONcTransform::check_group_support()
{
    if (RETURN_AS_NETCDF4 == FONcTransform::_returnAs && false == FONcRequestHandler::classic_model &&
        (_dmr->root()->grp_begin() != _dmr->root()->grp_end()))
        return true;
    else
        return false;
}

// Generate the final group list in the netCDF-4 file. Empty groups and their attributes will be removed.
void FONcTransform::gen_included_grp_list(D4Group *grp)
{
    bool grp_has_var = false;
    if (grp) {
        BESDEBUG("fnoc", "<coming to the D4 group  has name " << grp->name() << endl);
        BESDEBUG("fnoc", "<coming to the D4 group  has fullpath " << grp->FQN() << endl);

        if (grp->var_begin() != grp->var_end()) {

            BESDEBUG("fnoc", "<has the vars  " << endl);
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
            BESDEBUG("fonc", "obtain included groups  - group name:  " << (*gi)->name() << endl);
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

void FONcTransform::check_and_obtain_dimensions(D4Group *grp, bool is_root_grp)
{

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

void FONcTransform::check_and_obtain_dimensions_internal(D4Group *grp)
{

    // Remember the Group Fully Qualified dimension Name and the corresponding dimension size.
    D4Dimensions *grp_dims = grp->dims();
    if (grp_dims) {
        for (D4Dimensions::D4DimensionsIter di = grp_dims->dim_begin(), de = grp_dims->dim_end(); di != de; ++di) {

#if !NDEBUG
            BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
            BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
            BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
            BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
#endif
            unsigned long dimsize = (*di)->size();
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
                            BESDEBUG("fonc", "transform_dap4() check dim- dim name is: " << d4dim->name() << endl);
                            BESDEBUG("fonc", "transform_dap4() check dim- dim size is: " << d4dim->size() << endl);
                            BESDEBUG("fonc", "transform_dap4() check dim- fully_qualfied_dim name is: "
                                    << d4dim->fully_qualified_name() << endl);

                            unsigned long dimsize = t_a->dimension_size(dim_i, true);
#if !NDEBUG
                            BESDEBUG("fonc", "transform_dap4() check dim- final dim size is: "<< dimsize << endl);
#endif
                            pair<map<string, unsigned long>::iterator, bool> ret_it;
                            ret_it = VFQN_dimname_to_dimsize.insert(
                                    pair<string, unsigned long>(d4dim->fully_qualified_name(), dimsize));
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
    map<string,unsigned long>:: iterator it;
    for(it=GFQN_dimname_to_dimsize.begin();it!=GFQN_dimname_to_dimsize.end();++it) {
        BESDEBUG("fonc", "GFQN dim name is: "<<it->first<<endl);
        BESDEBUG("fonc", "GFQN dim size is: "<<it->second<<endl);
    }

    for(it=VFQN_dimname_to_dimsize.begin();it!=VFQN_dimname_to_dimsize.end();++it) {
        BESDEBUG("fonc", "VFQN dim name is: "<<it->first<<endl);
        BESDEBUG("fonc", "VFQN dim size is: "<<it->second<<endl);
    }

#endif

    // Go through all the descendent groups.
    for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
        BESDEBUG("fonc",
                 "FONcTransform::check_and_obtain_dimensions() in group  - group name:  " << (*gi)->name() << endl);
        check_and_obtain_dimensions(*gi, false);
    }

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
void FONcTransform::dump(ostream &strm) const
{
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


