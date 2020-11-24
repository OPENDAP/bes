// FONgRequestHandler.cc

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include <gdal.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>

#include <TheBESKeys.h>

#include "config.h"

#include "FONgRequestHandler.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

// Added hrg 3/20/19
bool FONgRequestHandler::d_use_byte_for_geotiff_bands = true;

/** @brief Constructor for FileOut GDAL module
 *
 * This constructor adds functions to add to the build of a help request
 * and a version request to the BES.
 *
 * @param name The name of the request handler being added to the list
 * of request handlers
 */
FONgRequestHandler::FONgRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_method(HELP_RESPONSE, FONgRequestHandler::build_help);
    add_method(VERS_RESPONSE, FONgRequestHandler::build_version);

    FONgRequestHandler::d_use_byte_for_geotiff_bands = TheBESKeys::TheKeys()->read_bool_key("FONg.GeoTiff.band.type.byte", true);

    GDALAllRegister();
    CPLSetErrorHandler(CPLQuietErrorHandler);
}

/** @brief Any cleanup that needs to take place
 */
FONgRequestHandler::~FONgRequestHandler()
{
}

/** @brief adds help information for FileOut GDAL to a help request
 *
 * Adds information to a help request to the BES regarding a file out
 * geotiff response. Included in this information is a link to a
 * docs.opendap.org page that describes fileout geotiff.
 *
 * @param dhi The data interface containing information for the current
 * request to the BES
 * @throws BESInternalError if the response object is not an
 * informational response object.
 */
bool FONgRequestHandler::build_help(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    bool found = false;
    string key = "FONg.Reference";
    string ref;
    TheBESKeys::TheKeys()->get_value(key, ref, found);
    if (ref.empty())
        ref = "https://docs.opendap.org/index.php/BES_-_Modules_-_FileOut_GDAL";

    map<string, string> attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;

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
bool FONgRequestHandler::build_version(BESDataHandlerInterface &dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(response);
    if (!info)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONgRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONgRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

