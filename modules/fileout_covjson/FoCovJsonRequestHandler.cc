// -*- mode: c++; c-basic-offset:4 -*-
//
// FoCovJsonRequestHandler.cc
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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

#include "FoCovJsonRequestHandler.h"
#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESDataNames.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include "config.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

bool FoCovJsonRequestHandler::_may_ignore_z_axis   = true;
bool FoCovJsonRequestHandler::_simple_geo   = true;

// Borrow from the HDF5 handler
bool FoCovJsonRequestHandler::obtain_beskeys_info(const string & key, bool &has_key) {

    bool ret_value = false;
    string doset ="";
    TheBESKeys::TheKeys()->get_value( key, doset, has_key ) ;
    if(has_key) {
        const string dosettrue ="true";
        const string dosetyes = "yes";
        doset = BESUtil::lowercase(doset) ;
        ret_value = (dosettrue == doset  || dosetyes == doset);
    }
    return ret_value;

}

/** @brief Constructor for FileOut Coverage JSON module
 *
 * This constructor adds functions to add to the build of a help request
 * and a version request to the BES.
 *
 * @param name The name of the request handler being added to the list
 * of request handlers
 */
FoCovJsonRequestHandler::FoCovJsonRequestHandler(const string &name) :
    BESRequestHandler(name)
{
    add_handler( HELP_RESPONSE, FoCovJsonRequestHandler::build_help);
    add_handler( VERS_RESPONSE, FoCovJsonRequestHandler::build_version);
    bool has_key = false;
    bool key_value = obtain_beskeys_info("FoCovJson.MAY_IGNORE_Z_AXIS",has_key);
    if (has_key) 
        _may_ignore_z_axis = key_value;   
    key_value = obtain_beskeys_info("FoCovJson.SIMPLE_GEO",has_key);
    if (has_key) 
        _simple_geo = key_value;  

#if 0
if(_may_ignore_z_axis == true) 
std::cerr<<"IGNORE mode "<<endl;
else
std::cerr<<"Strict mode "<<endl;
#endif
}

/** @brief Any cleanup that needs to take place
 */
FoCovJsonRequestHandler::~FoCovJsonRequestHandler()
{
}

/** @brief adds help information for FileOut Coverage JSON to a help request
 *
 * Adds information to a help request to the BES regarding a file out
 * netcdf response. Included in this information is a link to a
 * docs.opendap.org page that describes fileout netcdf.
 *
 * @param dhi The data interface containing information for the current
 * request to the BES
 * @throws BESInternalError if the response object is not an
 * informational response object.
 */
bool FoCovJsonRequestHandler::build_help(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);

    bool found = false;
    string key = "FoCovJson.Reference";
    string ref;
    TheBESKeys::TheKeys()->get_value(key, ref, found);
    if (ref.empty()) ref = "https://docs.opendap.org/index.php/BES_-_Modules_-_FileOut_COVJSON";
    map<string, string, std::less<>> attrs;
    attrs["name"] = MODULE_NAME;
    attrs["version"] = MODULE_VERSION;

    attrs["reference"] = ref;
    info->begin_tag("module", &attrs);
    info->end_tag("module");

    return true;
}

/** @brief add version information to a version response
 *
 * Adds the version of this module to the version response.
 *
 * @param dhi The data interface containing information for the current
 * request to the BES
 */
bool FoCovJsonRequestHandler::build_version(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FoCovJsonRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FoCovJsonRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

