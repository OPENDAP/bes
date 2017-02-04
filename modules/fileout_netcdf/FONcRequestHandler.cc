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
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include <BESDebug.h>
#include <BESUtil.h>

#include "FONcRequestHandler.h"

#define FONC_TEMP_DIR "/tmp"
#define FONC_TEMP_DIR_KEY "FONc.Tempdir"

// I think this should be true always. I'm leaving it in for now
// but the code does not use the key. Maybe this will be used when
// there is more comprehensive support for DAP4. jhrg 11/30/15
#define FONC_BYTE_TO_SHORT true
#define FONC_BYTE_TO_SHORT_KEY "FONc.ByteToShort"

#define FONC_USE_COMP true
#define FONC_USE_COMP_KEY "FONc.UseCompression"

#define FONC_CHUNK_SIZE 4096
#define FONC_CHUNK_SIZE_KEY "FONc.ChunkSize"

#define FONC_CLASSIC_MODEL true
#define FONC_CLASSIC_MODEL_KEY "FONc.ClassicModel"

string FONcRequestHandler::temp_dir;
bool FONcRequestHandler::byte_to_short;
bool FONcRequestHandler::use_compression;
int FONcRequestHandler::chunk_size;
bool FONcRequestHandler::classic_model;

using namespace std;

/**
 * Look at the BES configuration keys and see if key_name is included
 * and, if so, what its value is.
 *
 * @param key_name The key to loop up
 * @param key Value result parameter that takes on the value of the key
 * @param default_value
 * @see read_key_value() - other impls
 */
static void read_key_value(const string &key_name, bool &key, const bool default_value)
{
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    // 'key' holds the string value at this point if key_found is true
    if (key_found) {
        value = BESUtil::lowercase(value);
        key = (value == "true" || value == "yes");
    }
    else {
        key = default_value;
    }
}

static void read_key_value(const string &key_name, string &key, const string &default_value)
{
    bool key_found = false;
    TheBESKeys::TheKeys()->get_value(key_name, key, key_found);
    // 'key' holds the string value at this point if key_found is true
    if (key_found) {
        if (key[key.length() - 1] == '/') key.erase(key.length() - 1);
    }
    else {
        key = default_value;
    }
}

static void read_key_value(const string &key_name, int &key, const int default_value)
{
    bool key_found = false;
    string value;
    TheBESKeys::TheKeys()->get_value(key_name, value, key_found);
    // 'key' holds the string value at this point if key_found is true
    if (key_found) {
        istringstream iss(value);
        iss >> key;
        if (iss.eof() || iss.bad() || iss.fail()) key = default_value;
    }
    else {
        key = default_value;
    }
}


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
    add_handler( HELP_RESPONSE, FONcRequestHandler::build_help ) ;
    add_handler( VERS_RESPONSE, FONcRequestHandler::build_version ) ;

    if (FONcRequestHandler::temp_dir.empty()) {
        read_key_value(FONC_TEMP_DIR_KEY, FONcRequestHandler::temp_dir, FONC_TEMP_DIR);
    }

    // Not currently used. jhrg 11/30/15
    read_key_value(FONC_BYTE_TO_SHORT_KEY, FONcRequestHandler::byte_to_short, FONC_BYTE_TO_SHORT);

    read_key_value(FONC_USE_COMP_KEY, FONcRequestHandler::use_compression, FONC_USE_COMP);

    read_key_value(FONC_CHUNK_SIZE_KEY, FONcRequestHandler::chunk_size, FONC_CHUNK_SIZE);

    read_key_value(FONC_CLASSIC_MODEL_KEY, FONcRequestHandler::classic_model, FONC_CLASSIC_MODEL);

    BESDEBUG("fonc", "FONcRequestHandler::temp_dir: " << FONcRequestHandler::temp_dir << endl);
    BESDEBUG("fonc", "FONcRequestHandler::byte_to_short: " << FONcRequestHandler::byte_to_short << endl);
    BESDEBUG("fonc", "FONcRequestHandler::use_compression: " << FONcRequestHandler::use_compression << endl);
    BESDEBUG("fonc", "FONcRequestHandler::chunk_size: " << FONcRequestHandler::chunk_size << endl);
    BESDEBUG("fonc", "FONcRequestHandler::classic_model: " << FONcRequestHandler::classic_model << endl);
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
    if (ref.empty()) ref = "http://docs.opendap.org/index.php/BES_-_Modules_-_FileOut_Netcdf";
    map<string, string> attrs;
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

