// -*- mode: c++; c-basic-offset:4 -*-
//
// FoJsonRequestHandler.cc
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "W10nJsonRequestHandler.h"
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

/** @brief Constructor for FileOut NetCDF module
 *
 * This constructor adds functions to add to the build of a help request
 * and a version request to the BES.
 *
 * @param name The name of the request handler being added to the list
 * of request handlers
 */
W10nJsonRequestHandler::W10nJsonRequestHandler( const string &name )
    : BESRequestHandler( name )
{
    add_method( HELP_RESPONSE, W10nJsonRequestHandler::build_help ) ;
    add_method( VERS_RESPONSE, W10nJsonRequestHandler::build_version ) ;
}

/** @brief Any cleanup that needs to take place
 */
W10nJsonRequestHandler::~W10nJsonRequestHandler()
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
bool
W10nJsonRequestHandler::build_help( BESDataHandlerInterface &dhi )
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESInfo *info = dynamic_cast < BESInfo * >(response) ;
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    bool found = false ;
    string key = "W10n.Reference" ;
    string ref ;
    TheBESKeys::TheKeys()->get_value( key, ref, found ) ;
    if( ref.empty() )
	ref = "https://docs.opendap.org/index.php/BES_-_Modules_-_w10n_JSON" ;
    map<string, string, std::less<>> attrs ;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    attrs["reference"] = ref ;
    info->begin_tag( "module", &attrs ) ;
    info->end_tag( "module" ) ;

    return true ;
}

/** @brief add version information to a version response
 *
 * Adds the version of this module to the version response.
 *
 * @param dhi The data interface containing information for the current
 * request to the BES
 */
bool
W10nJsonRequestHandler::build_version( BESDataHandlerInterface &dhi )
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESVersionInfo *info = dynamic_cast < BESVersionInfo * >(response) ;
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);

    return true ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
W10nJsonRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "W10nJsonRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

