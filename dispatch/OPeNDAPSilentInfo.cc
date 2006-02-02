// OPeNDAPSilentInfo.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

#include "OPeNDAPSilentInfo.h"

/** @brief constructs a OPeNDAPSilentInfo object for the specified type.
 *
 * @param otype type of data represented by this response object
 */
OPeNDAPSilentInfo::OPeNDAPSilentInfo( )
    : DODSInfo( unknown_type )
{
    _buffered = false ;
}

OPeNDAPSilentInfo::~OPeNDAPSilentInfo()
{
}

/** @brief no response is constructed, so do nothing
 *
 * @param s information to be ignored
 */
void
OPeNDAPSilentInfo::add_data( const string &s )
{
}

/** @brief ignore data from a file to the informational object.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name naem information to add to error messages
 * loaded.
 */
void
OPeNDAPSilentInfo::add_data_from_file( const string &key, const string &name )
{
}

/** @brief ignore exception data to this informational object. If buffering is
 * not set then the information is output directly to the output stream.
 *
 * @param type type of the exception received
 * @param msg the error message
 * @param file file name of where the error was sent
 * @param line line number in the file where the error was sent
 */
void
OPeNDAPSilentInfo::add_exception( const string &type,
				  const string &msg,
				  const string &file,
				  int line )
{
}

/** @brief ignore printing the information
 *
 * @param out output to this file descriptor if information buffered.
 */
void
OPeNDAPSilentInfo::print(FILE *out)
{
}

// $Log: OPeNDAPSilentInfo.cc,v $
