// BESSilentInfo.cc

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

#include "BESSilentInfo.h"

/** @brief constructs a BESSilentInfo object for the specified type.
 *
 * @param otype type of data represented by this response object
 */
BESSilentInfo::BESSilentInfo( )
    : BESInfo( )
{
}

BESSilentInfo::~BESSilentInfo()
{
}

/** @brief begin the informational response
 *
 * Because this is silent, there is nothing to do
 *
 * @param response_name name of the response represented by the information
 */
void
BESSilentInfo::begin_response( const string &response_name )
{
    BESInfo::begin_response( response_name ) ;
}

/** @brief add tagged information to the inforamtional response
 *
 * @param tag_name name of the tag to add to the infroamtional response
 * @param tag_data information describing the tag
 */
void
BESSilentInfo::add_tag( const string &tag_name,
			const string &tag_data,
			map<string,string> *attrs )
{
}

/** @brief begin a tagged part of the information, information to follow
 *
 * @param tag_name name of the tag to begin
 */
void
BESSilentInfo::begin_tag( const string &tag_name ,
			  map<string,string> *attrs )
{
    BESInfo::begin_tag( tag_name ) ;
}

/** @brief end a tagged part of the informational response
 *
 * If the named tag is not the current tag then an error is thrown.
 *
 * @param tag_name name of the tag to end
 */
void
BESSilentInfo::end_tag( const string &tag_name )
{
    BESInfo::end_tag( tag_name ) ;
}

/** @brief add data to the inforamtional object
 *
 * because this is a silent response, nothing is added
 *
 * @param s information to be ignored
 */
void
BESSilentInfo::add_data( const string &s )
{
}

/** @brief add a space to the informational response
 *
 * because this is a silent response, nothing is added
 *
 * @param num_spaces number of spaces to add
 */
void
BESSilentInfo::add_space( unsigned long num_spaces )
{
}

/** @brief add a line break to the information
 *
 * because this is a silent response, nothing is added
 *
 * @param s information to be ignored
 */
void
BESSilentInfo::add_break( unsigned long num_breaks )
{
}

/** @brief ignore data from a file to the informational object.
 *
 * @param key Key from the initialization file specifying the file to be
 * @param name naem information to add to error messages
 * loaded.
 */
void
BESSilentInfo::add_data_from_file( const string &key, const string &name )
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
BESSilentInfo::add_exception( const string &type, BESException &e )
{
}

void
BESSilentInfo::transmit( BESTransmitter *transmitter,
		         BESDataHandlerInterface &dhi )
{
}

/** @brief ignore printing the information
 *
 * @param out output to this file descriptor if information buffered.
 */
void
BESSilentInfo::print( FILE *out )
{
}

