// BESReporterList.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

#include <mutex>

#include "BESReporterList.h"
#include "BESReporter.h"

using std::endl;
using std::ostream;
using std::string;

BESReporterList::BESReporterList() {}

bool
BESReporterList::add_reporter( string reporter_name,
			       BESReporter *reporter_object )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    if( find_reporter( reporter_name ) == 0 )
    {
	_reporter_list[reporter_name] = reporter_object ;
	return true ;
    }
    return false ;
}

BESReporter *
BESReporterList::remove_reporter( string reporter_name )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESReporter *ret = 0 ;
    BESReporterList::Reporter_iter i ;
    i = _reporter_list.find( reporter_name ) ;
    if( i != _reporter_list.end() )
    {
	ret = (*i).second;
	_reporter_list.erase( i ) ;
    }
    return ret ;
}

BESReporter *
BESReporterList::find_reporter( string reporter_name )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESReporterList::Reporter_citer i ;
    i = _reporter_list.find( reporter_name ) ;
    if( i != _reporter_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

void
BESReporterList::report( BESDataHandlerInterface &dhi )
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    BESReporter *reporter = 0 ;
    BESReporterList::Reporter_iter i = _reporter_list.begin() ;
    for( ; i != _reporter_list.end(); i++ )
    {
	reporter = (*i).second ;
	if( reporter ) reporter->report( dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESReporterList::dump( ostream &strm ) const
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);

    strm << BESIndent::LMarg << "BESReporterList::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _reporter_list.size() )
    {
	strm << BESIndent::LMarg << "registered reporters:" << endl ;
	BESIndent::Indent() ;
	BESReporterList::Reporter_citer i = _reporter_list.begin() ;
	BESReporterList::Reporter_citer ie = _reporter_list.end() ;
	for( ; i != ie; i++ )
	{
	    strm << BESIndent::LMarg << "reporter: " << (*i).first << endl ;
	    BESIndent::Indent() ;
	    BESReporter *reporter = (*i).second ;
	    reporter->dump( strm ) ;
	    BESIndent::UnIndent() ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "registered reporters: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

BESReporterList *
BESReporterList::TheList()
{
    static BESReporterList list;
    return &list;
}

