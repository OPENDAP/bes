// FONcRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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

#include <string>
#include <sstream>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include <BESDebug.h>
#include <BESUtil.h>

#include "FONcRequestHandler.h"
#include "FONcNames.h"

std::string FONcRequestHandler::temp_dir;
bool FONcRequestHandler::use_compression = true;
bool FONcRequestHandler::use_shuffle = true;
bool FONcRequestHandler::float_write_opt = true;
unsigned long long  FONcRequestHandler::float_write_opt_buffer_size = 536870912;
#if 0
float FONcRequestHandler::float_write_opt_comp_ratio;
#endif
bool FONcRequestHandler::use_contiguous_storage = false;
bool FONcRequestHandler::classic_model = false;
bool FONcRequestHandler::reduce_dim = false;
bool FONcRequestHandler::no_global_attrs = false;

// The request_max_size_kb equal = 0 means the request_max_size is unlimited.
unsigned long long FONcRequestHandler::request_max_size_kb = 0;
bool FONcRequestHandler::nc3_classic_format = false;

using namespace std;

/** @brief Constructor for FileOut NetCDF module
 *
 * This constructor adds functions to add to the build of a help request
 * and a version request to the BES.
 *
 * @param name The name of the request handler being added to the list
 * of request handlers
 */
FONcRequestHandler::FONcRequestHandler( const string &name )
: BESRequestHandler( name )
{
    add_method( HELP_RESPONSE, FONcRequestHandler::build_help ) ;
    add_method( VERS_RESPONSE, FONcRequestHandler::build_version ) ;

    if (FONcRequestHandler::temp_dir.empty()) {
        FONcRequestHandler::temp_dir = TheBESKeys::read_string_key(FONC_TEMP_DIR_KEY, FONC_TEMP_DIR);
    }

    // Updated calls to use TheBESKeys. kln 04/10/26
    FONcRequestHandler::use_compression = TheBESKeys::read_bool_key(FONC_USE_COMP_KEY, FONC_USE_COMP);
    FONcRequestHandler::use_shuffle = TheBESKeys::read_bool_key(FONC_USE_SHUFFLE_KEY, FONC_USE_SHUFFLE);
    FONcRequestHandler::float_write_opt = TheBESKeys::read_bool_key(FONC_FLOAT_WRITE_OPT_KEY, FONC_FLOAT_WRITE_OPT);
    FONcRequestHandler::float_write_opt_buffer_size = TheBESKeys::read_uint64_key(FONC_FLOAT_WRITE_OPT_BUFFER_SIZE_KEY, FONC_FLOAT_WRITE_OPT_BUFFER_SIZE);
#if 0
    FONcRequestHandler::float_write_opt_comp_ratio = TheBESKeys::read_float_key(FONC_FLOAT_WRITE_OPT_KEY, FONC_FLOAT_WRITE_OPT_COMP_RATIO);
#endif

    FONcRequestHandler::use_contiguous_storage = TheBESKeys::read_bool_key(FONC_USE_CONTIGUOUS_KEY, FONC_USE_CONTIGUOUS);
    FONcRequestHandler::classic_model = TheBESKeys::read_bool_key(FONC_CLASSIC_MODEL_KEY, FONC_CLASSIC_MODEL);
    FONcRequestHandler::reduce_dim = TheBESKeys::read_bool_key(FONC_REDUCE_DIM_KEY, FONC_REDUCE_DIM);
    FONcRequestHandler::no_global_attrs = TheBESKeys::read_bool_key(FONC_NO_GLOBAL_ATTRS_KEY, FONC_NO_GLOBAL_ATTRS);
    FONcRequestHandler::request_max_size_kb = TheBESKeys::read_ulong_key(FONC_REQUEST_MAX_SIZE_KB_KEY, FONC_REQUEST_MAX_SIZE_KB);
    FONcRequestHandler::nc3_classic_format = TheBESKeys::read_bool_key(FONC_NC3_CLASSIC_FORMAT_KEY, FONC_NC3_CLASSIC_FORMAT);

    BESDEBUG("fonc", "FONcRequestHandler::temp_dir: " << FONcRequestHandler::temp_dir << endl);
    BESDEBUG("fonc", "FONcRequestHandler::use_compression: " << FONcRequestHandler::use_compression << endl);
    BESDEBUG("fonc", "FONcRequestHandler::use_shuffle: " << FONcRequestHandler::use_shuffle << endl);
    BESDEBUG("fonc", "FONcRequestHandler::float_write_opt: " << FONcRequestHandler::float_write_opt << endl);
    BESDEBUG("fonc", "FONcRequestHandler::float_write_opt_buffer_size: " << FONcRequestHandler::float_write_opt_buffer_size << endl);

    BESDEBUG("fonc", "FONcRequestHandler::use_contiguous_storage: " << FONcRequestHandler::use_contiguous_storage << endl);
    BESDEBUG("fonc", "FONcRequestHandler::classic_model: " << FONcRequestHandler::classic_model << endl);
    BESDEBUG("fonc", "FONcRequestHandler::reduce_dim: " << FONcRequestHandler::reduce_dim << endl);
    BESDEBUG("fonc", "FONcRequestHandler::turn_off_global_attrs: " << FONcRequestHandler::no_global_attrs << endl);
    BESDEBUG("fonc", "FONcRequestHandler::request_max_size_kb: " << FONcRequestHandler::request_max_size_kb << endl);
    BESDEBUG("fonc", "FONcRequestHandler::nc3_classic_format " << FONcRequestHandler::nc3_classic_format << endl);
}

/** @brief Any cleanup that needs to take place
 */
FONcRequestHandler::~FONcRequestHandler()
{
}

/** @brief adds help information for FileOut NetCDF to a help request
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
bool FONcRequestHandler::build_help(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info) throw BESInternalError("cast error", __FILE__, __LINE__);

    bool found = false;
    string key = "FONc.Reference";
    string ref;
    TheBESKeys::TheKeys()->get_value(key, ref, found);
    if (ref.empty()) ref = "https://docs.opendap.org/index.php/BES_-_Modules_-_FileOut_Netcdf";
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
bool FONcRequestHandler::build_version(BESDataHandlerInterface &dhi)
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
void
FONcRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcRequestHandler::dump - ("
    << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}
