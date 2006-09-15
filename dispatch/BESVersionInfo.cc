// BESVersionInfo.cc

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifdef __GNUG__
#pragma implementation
#endif

#include "BESVersionInfo.h"
#include "BESInfoList.h"
#include "BESHandlerException.h"

/** @brief constructs a basic text information response object.
 *
 * Uses the default OPeNDAP.Info.Buffered key in the dods initialization file to
 * determine whether the information should be buffered or not.
 *
 * @see BESXMLInfo
 * @see DODSResponseObject
 */
BESVersionInfo::BESVersionInfo()
    : BESInfo(),
      _indap( false ),
      _inbes( false ),
      _inhandler( false ),
      _info( 0 )
{
    _info = BESInfoList::TheList()->build_info() ;
}

BESVersionInfo::~BESVersionInfo()
{
}

void
BESVersionInfo::beginDAPVersion( )
{
    if( _indap || _inbes || _inhandler )
    {
	throw BESHandlerException( "Attempting to begin DAP version information while already adding DAP, BES, or Handler info", __FILE__, __LINE__ ) ;
    }
    _indap = true ;
    _info->begin_tag( "DAP" ) ;
}

void
BESVersionInfo::addDAPVersion( const string &v )
{
    if( !_indap )
    {
	throw BESHandlerException( "Attempting to add DAP version information while not in DAP tag", __FILE__, __LINE__ ) ;
    }
    _info->add_tag( "version", v ) ;
}

void
BESVersionInfo::endDAPVersion( )
{
    if( !_indap )
    {
	throw BESHandlerException( "Attempting to end DAP version information while not in DAP tag", __FILE__, __LINE__ ) ;
    }
    _indap = false ;
    _info->end_tag( "DAP" ) ;
}

void
BESVersionInfo::beginBESVersion( )
{
    if( _indap || _inbes || _inhandler )
    {
	throw BESHandlerException( "Attempting to begin BES version information while already adding DAP, BES, or Handler info", __FILE__, __LINE__ ) ;
    }
    _inbes = true ;
    _info->begin_tag( "BES" ) ;
}

void
BESVersionInfo::addBESVersion( const string &n, const string &v )
{
    if( !_inbes )
    {
	throw BESHandlerException( "Attempting to add BES version information while not in BES tag", __FILE__, __LINE__ ) ;
    }
    _info->begin_tag( "lib" ) ;
    _info->add_tag( "name", n ) ;
    _info->add_tag( "version", v ) ;
    _info->end_tag( "lib" ) ;
}

void
BESVersionInfo::endBESVersion( )
{
    if( !_inbes )
    {
	throw BESHandlerException( "Attempting to end BES version information while not in BES tag", __FILE__, __LINE__ ) ;
    }
    _inbes = false ;
    _info->end_tag( "BES" ) ;
}

void
BESVersionInfo::beginHandlerVersion( )
{
    if( _indap || _inbes || _inhandler )
    {
	throw BESHandlerException( "Attempting to begin Handler version information while already adding DAP, BES, or Handler info", __FILE__, __LINE__ ) ;
    }
    _inhandler = true ;
    _info->begin_tag( "Handlers" ) ;
}

void
BESVersionInfo::addHandlerVersion( const string &n, const string &v )
{
    if( !_inhandler )
    {
	throw BESHandlerException( "Attempting to add Handler version information while not in Handler tag", __FILE__, __LINE__ ) ;
    }
    _info->begin_tag( "lib" ) ;
    _info->add_tag( "name", n ) ;
    _info->add_tag( "version", v ) ;
    _info->end_tag( "lib" ) ;
}

void
BESVersionInfo::endHandlerVersion( )
{
    if( !_inhandler )
    {
	throw BESHandlerException( "Attempting to end Handler version information while not in Handler tag", __FILE__, __LINE__ ) ;
    }
    _inhandler = true ;
    _info->end_tag( "Handlers" ) ;
}

