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

#include "config.h"

#include <sstream>

using std::ostringstream;
using std::istringstream;

#include "FONcRequestHandler.h" // for the keys

#include "FONcTransform.h"
#include "FONcUtils.h"
#include "FONcBaseType.h"
#include "FONcAttributes.h"

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

#include "DapFunctionUtils.h"

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
        _ncid(0), _dds(0)
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
        _ncid(0), _dmr(0)
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
}

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
    FONcUtils::reset();

    // Convert the DMR into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    
    BESDEBUG("fonc", "Coming into transform_dap4() "<< endl);

    bool support_group = check_group_support();
    if(true == support_group) {
        int stax = -1;
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
        stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        if (stax != NC_NOERR) 
            FONcUtils::handle_error(stax, "File out netcdf, unable to open: " + _localfile, __FILE__, __LINE__);
        
        D4Group* root_grp = _dmr->root();
        map<string,int>fdimname_to_id;
        transform_dap4_group(root_grp,true,_ncid,fdimname_to_id);
        stax = nc_close(_ncid);
        if (stax != NC_NOERR)
            FONcUtils::handle_error(stax, "File out netcdf, unable to close: " + _localfile, __FILE__, __LINE__);

    }
    else 
        transform_dap4_no_group();

    return;
#if 0
    D4Group* root_grp = _dmr->root();
    D4Dimensions *root_dims = root_grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
        //cout <<"dim size is: "<<(*di)->size()<<endl;
        //cout <<"dim fully_qualified_name is: "<<(*di)->fully_qualified_name()<<endl;
    }
    Constructor::Vars_iter vi = root_grp->var_begin();
    Constructor::Vars_iter ve = root_grp->var_end();
#if 0
    for (D4Group::Vars_iter i = root_grp->var_begin(), e = root_grp->var_end(); i != e; ++i) {
        BESDEBUG("fonc", "transform_dap4() - "<< (*i)->name() <<endl);
        if ((*i)->send_p()) {
            BESDEBUG("fonc", "transform_dap4() inside send_p - "<< (*i)->name() <<endl);
            //(*i)->intern_data();
        }
    }
#endif

#if 0
    DDS::Vars_iter vi = _dds->var_begin();
    DDS::Vars_iter ve = _dds->var_end();
#endif
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Converting variable '" << v->name() << "'" << endl);

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

    if(root_grp->grp_begin() == root_grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);
   for (D4Group::groupsIter gi = root_grp->grp_begin(), ge = root_grp->grp_end(); gi != ge; ++gi) 
       BESDEBUG("fonc", "FONcTransform::transform_dap4() - group name:  " << (*gi)->name() << endl);

#if 0
           vector<BaseType *> *d2_vars = (*gi)->transform_to_dap2(group_attrs);
           if (d2_vars) {
                          for (vector<BaseType *>::iterator i = d2_vars->begin(), e = d2_vars->end(); i != e; ++i) {
                                          results->push_back(*i);
                                                           }
                                              }
                           delete d2_vars;


#endif

    // Open the file for writing
    int stax;
    if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
        if (FONcRequestHandler::classic_model){
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file in classic mode. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
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
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Defining variable:  " << fbt->name() << endl);
            fbt->set_is_dap4(true);
            fbt->define(_ncid);
        }

        if(FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            D4Group* root_grp=_dmr->root();
            D4Attributes*d4_attrs = root_grp->attributes();
            BESDEBUG("fonc", "FONcTransform::transform_dap4() handle GLOBAL DAP4 attributes "<< d4_attrs <<endl);
#if 0
            for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
                string name = (*ii)->name();
                BESDEBUG("fonc", "FONcTransform::transform_dap4() GLOBAL attribute name is "<<name <<endl);
            }
#endif
            //    AttrTable &globals = root_grp->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Adding Global Attributes" << endl) ;
            bool is_netCDF_enhanced = false;
            if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_dap4_attributes(_ncid, NC_GLOBAL, d4_attrs, "", "",is_netCDF_enhanced);
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
#endif
}

void FONcTransform::transform_dap4_no_group() {

    D4Group* root_grp = _dmr->root();
    D4Dimensions *root_dims = root_grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
        //cout <<"dim size is: "<<(*di)->size()<<endl;
        //cout <<"dim fully_qualified_name is: "<<(*di)->fully_qualified_name()<<endl;
    }
    Constructor::Vars_iter vi = root_grp->var_begin();
    Constructor::Vars_iter ve = root_grp->var_end();
#if 0
    for (D4Group::Vars_iter i = root_grp->var_begin(), e = root_grp->var_end(); i != e; ++i) {
        BESDEBUG("fonc", "transform_dap4() - "<< (*i)->name() <<endl);
        if ((*i)->send_p()) {
            BESDEBUG("fonc", "transform_dap4() inside send_p - "<< (*i)->name() <<endl);
            //(*i)->intern_data();
        }
    }
#endif

#if 0
    DDS::Vars_iter vi = _dds->var_begin();
    DDS::Vars_iter ve = _dds->var_end();
#endif
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Converting variable '" << v->name() << "'" << endl);

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

    if(root_grp->grp_begin() == root_grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);
   for (D4Group::groupsIter gi = root_grp->grp_begin(), ge = root_grp->grp_end(); gi != ge; ++gi) 
       BESDEBUG("fonc", "FONcTransform::transform_dap4() - group name:  " << (*gi)->name() << endl);

#if 0
           vector<BaseType *> *d2_vars = (*gi)->transform_to_dap2(group_attrs);
           if (d2_vars) {
                          for (vector<BaseType *>::iterator i = d2_vars->begin(), e = d2_vars->end(); i != e; ++i) {
                                          results->push_back(*i);
                                                           }
                                              }
                           delete d2_vars;
                               


#endif

    // Open the file for writing
    int stax;
    if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
        if (FONcRequestHandler::classic_model){
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file in classic mode. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4|NC_CLASSIC_MODEL, &_ncid);
        }
        else {
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-4 cache file. fileName:  " << _localfile << endl);
            stax = nc_create(_localfile.c_str(), NC_CLOBBER|NC_NETCDF4, &_ncid);
        }
    }
    else {
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - Opening NetCDF-3 cache file. fileName:  " << _localfile << endl);
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
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Defining variable:  " << fbt->name() << endl);
            fbt->set_is_dap4(true);
            fbt->define(_ncid);
        }

        if(FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            D4Group* root_grp=_dmr->root();
            D4Attributes*d4_attrs = root_grp->attributes();
            BESDEBUG("fonc", "FONcTransform::transform_dap4() handle GLOBAL DAP4 attributes "<< d4_attrs <<endl);
#if 0
            for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
                string name = (*ii)->name();
                BESDEBUG("fonc", "FONcTransform::transform_dap4() GLOBAL attribute name is "<<name <<endl);
            }
#endif
            //    AttrTable &globals = root_grp->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Adding Global Attributes" << endl) ;
            bool is_netCDF_enhanced = false;
            if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_dap4_attributes(_ncid, NC_GLOBAL, d4_attrs, "", "",is_netCDF_enhanced);
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

void FONcTransform::transform_dap4_group(D4Group* grp,bool is_root_grp,int par_grp_id,map<string,int>&fdimname_to_id ) {

    int grp_id = -1;
        int stax = -1;
        if(is_root_grp == true) 
            grp_id = _ncid;
        else
            stax = nc_def_grp(par_grp_id,(*grp).name().c_str(),&grp_id);
     
    D4Dimensions *root_dims = grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
        int g_dimid = -1;
        stax = nc_def_dim(grp_id,(*di)->name().c_str(),(*di)->size(),&g_dimid);
        fdimname_to_id[(*di)->fully_qualified_name()] = g_dimid; 
        //fdimname_to_id.push_back(temp_dimname_to_id);
        //cout <<"dim size is: "<<(*di)->size()<<endl;
        //cout <<"dim fully_qualified_name is: "<<(*di)->fully_qualified_name()<<endl;
    }
//#endif
    Constructor::Vars_iter vi = grp->var_begin();
    Constructor::Vars_iter ve = grp->var_end();
#if 0
    for (D4Group::Vars_iter i = root_grp->var_begin(), e = root_grp->var_end(); i != e; ++i) {
        BESDEBUG("fonc", "transform_dap4() - "<< (*i)->name() <<endl);
        if ((*i)->send_p()) {
            BESDEBUG("fonc", "transform_dap4() inside send_p - "<< (*i)->name() <<endl);
            //(*i)->intern_data();
        }
    }
#endif

#if 0
    DDS::Vars_iter vi = _dds->var_begin();
    DDS::Vars_iter ve = _dds->var_end();
#endif

    vector<FONcBaseType *> fonc_vars_in_grp;
    for (; vi != ve; vi++) {
        if ((*vi)->send_p()) {
            BaseType *v = *vi;

            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Converting variable '" << v->name() << "'" << endl);

            // This is a factory class call, and 'fg' is specialized for 'v'
            //FONcBaseType *fb = FONcUtils::convert(v,FONcTransform::_returnAs,FONcRequestHandler::classic_model);
            FONcBaseType *fb = FONcUtils::convert(v,RETURNAS_NETCDF4,false,fdimname_to_id);
#if 0
            fb->setVersion( FONcTransform::_returnAs );
            if ( FONcTransform::_returnAs == RETURNAS_NETCDF4 ) {
                if (FONcRequestHandler::classic_model)
                    fb->setNC4DataModel("NC4_CLASSIC_MODEL");
                else 
                    fb->setNC4DataModel("NC4_ENHANCED");
            }
#endif
            //_fonc_vars.push_back(fb);
            fonc_vars_in_grp.push_back(fb);

            vector<string> embed;
            fb->convert(embed,true);
        }
    }

    if(grp->grp_begin() == grp->grp_end()) 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - No group  " <<  endl);
    else 
        BESDEBUG("fonc", "FONcTransform::transform_dap4() - has group  " <<  endl);

#if 0
           vector<BaseType *> *d2_vars = (*gi)->transform_to_dap2(group_attrs);
           if (d2_vars) {
                          for (vector<BaseType *>::iterator i = d2_vars->begin(), e = d2_vars->end(); i != e; ++i) {
                                          results->push_back(*i);
                                                           }
                                              }
                           delete d2_vars;
                               


#endif

    try {
        // Here we will be defining the variables of the netcdf and
        // adding attributes. To do this we must be in define mode.
        // TO CHECK: for netCDF4 group, this may NOT be necessary.
        //nc_redef(_ncid);

        // For each converted FONc object, call define on it to define
        // that object to the netcdf file. This also adds the attributes
        // for the variables to the netcdf file
#if 0
        int grp_id = -1;
        int stax = -1;
        if(is_root_grp == true) 
            grp_id = _ncid;
        else
            stax = nc_def_grp(par_grp_id,(*grp).name().c_str(),&grp_id);
            
    D4Dimensions *root_dims = grp->dims();
    for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
        BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
        BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
        BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
        BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
        int g_dimid = -1;
        stax = nc_def_dim(grp_id,(*di)->name().c_str(),(*di)->size(),&g_dimid);
        fdimname_to_id[(*di)->fully_qualified_name()] = g_dimid; 
        //fdimname_to_id.push_back(temp_dimname_to_id);
        //cout <<"dim size is: "<<(*di)->size()<<endl;
        //cout <<"dim fully_qualified_name is: "<<(*di)->fully_qualified_name()<<endl;
    }
#endif
 
        vector<FONcBaseType *>::iterator i = fonc_vars_in_grp.begin();
        vector<FONcBaseType *>::iterator e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Defining variable:  " << fbt->name() << endl);
            fbt->set_is_dap4(true);
            fbt->define(grp_id);
        }

        // TOODOO: tackle attribute later. KY
        if(FONcRequestHandler::no_global_attrs == false) {
            // Add any global attributes to the netcdf file
            D4Group* root_grp=_dmr->root();
            D4Attributes*d4_attrs = root_grp->attributes();
            BESDEBUG("fonc", "FONcTransform::transform_dap4() handle GLOBAL DAP4 attributes "<< d4_attrs <<endl);
#if 0
            for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end(); ii != ee; ++ii) {
                string name = (*ii)->name();
                BESDEBUG("fonc", "FONcTransform::transform_dap4() GLOBAL attribute name is "<<name <<endl);
            }
#endif
            //    AttrTable &globals = root_grp->get_attr_table();
            BESDEBUG("fonc", "FONcTransform::transform_dap4() - Adding Global Attributes" << endl) ;
            bool is_netCDF_enhanced = false;
            if(FONcTransform::_returnAs == RETURNAS_NETCDF4 && FONcRequestHandler::classic_model==false)
                is_netCDF_enhanced = true;
            FONcAttributes::add_dap4_attributes(_ncid, NC_GLOBAL, d4_attrs, "", "",is_netCDF_enhanced);
        }

        // We are done defining the variables, dimensions, and
        // attributes of the netcdf file. End the define mode.
        // CHECK: no need to call the end define code for netCDF-4 .
        //int stax = nc_enddef(_ncid);

        // Check error for nc_enddef. Handling of HDF failures
        // can be detected here rather than later.  KY 2012-10-25
#if 0
        if (stax != NC_NOERR) {
            FONcUtils::handle_error(stax, "File out netcdf, unable to end the define mode: " + _localfile, __FILE__, __LINE__);
        }
#endif

        // Write everything out
        //i = _fonc_vars.begin();
        //e = _fonc_vars.end();
        i = fonc_vars_in_grp.begin();
        e = fonc_vars_in_grp.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = *i;
            BESDEBUG("fonc", "FONcTransform::transform() - Writing data for variable in group:  " << fbt->name() << endl);
            //fbt->write(_ncid);
            fbt->write(grp_id);
        }

   for (D4Group::groupsIter gi = grp->grp_begin(), ge = grp->grp_end(); gi != ge; ++gi) {
       BESDEBUG("fonc", "FONcTransform::transform_dap4() in group  - group name:  " << (*gi)->name() << endl);
       transform_dap4_group(*gi,false,grp_id,fdimname_to_id);
   }

//STOP HERE.


    }
    catch (BESError &e) {
        (void) nc_close(_ncid); // ignore the error at this point
        throw;
    }



}

bool FONcTransform::check_group_support() {
    if(RETURNAS_NETCDF4 == FONcTransform::_returnAs && false == FONcRequestHandler::classic_model && 
       (_dmr->root()->grp_begin()!=_dmr->root()->grp_end())) 
        return true; 
    else 
        return false;
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


