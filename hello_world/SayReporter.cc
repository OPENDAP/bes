// SayReporter.cc

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
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "SayReporter.h"
#include "TheBESKeys.h"
#include "BESLogException.h"
#include "SampleResponseNames.h"

SayReporter::SayReporter()
    : BESReporter(),
      _file_buffer( 0 )
{
    bool found = false ;
    _log_name = TheBESKeys::TheKeys()->get_key( "Say.LogName", found );
    if( _log_name == "" )
    {
	throw BESLogException( "can not determine Say log name", __FILE__, __LINE__ ) ;
    }
    else
    {
	_file_buffer = new ofstream( _log_name.c_str(), ios::out | ios::app ) ;
	if( !(*_file_buffer) )
	{
	    string s = "can not open Say log file " + _log_name ;;
	    throw BESLogException( s, __FILE__, __LINE__ ) ;
	} 
    }
}

SayReporter::~SayReporter()
{
    if( _file_buffer )
    {
	delete _file_buffer ;
	_file_buffer = 0 ;
    }
}

void
SayReporter::report( const BESDataHandlerInterface &dhi )
{
    const time_t sctime = time( NULL ) ;
    const struct tm *sttime = localtime( &sctime ) ; 
    char zone_name[10] ;
    strftime( zone_name, sizeof( zone_name ), "%Z", sttime ) ;
    char *b = asctime( sttime ) ;
    *(_file_buffer) << "[" << zone_name << " " ;
    for(register int j = 0; b[j] != '\n'; j++ )
	*(_file_buffer) << b[j] ;
    *(_file_buffer) << "] " ;

    string say_what ;
    string say_to ;
    BESDataHandlerInterface::data_citer i = dhi.data_c().find( SAY_WHAT ) ;
    if( i != dhi.data_c().end() )
    {
	say_what = (*i).second ;
    }
    i = dhi.data_c().find( SAY_TO ) ;
    if( i != dhi.data_c().end() )
    {
	say_to = (*i).second ;
	*(_file_buffer) << "\"" << say_what << "\" said to \"" << say_to << "\""
			<< endl ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the containers stored in this volatile list.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
SayReporter::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SayReporter::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "Say log name: " << _log_name << endl ;
    BESIndent::UnIndent() ;
}

