// DODSRequesthandlerList.cc

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

#include "DODSReporterList.h"
#include "DODSReporter.h"

DODSReporterList *DODSReporterList::_instance = 0 ;

DODSReporterList::DODSReporterList()
{
}

DODSReporterList::~DODSReporterList()
{
    DODSReporter *reporter = 0 ;
    DODSReporterList::Reporter_iter i = _reporter_list.begin() ;
    for( ; i != _reporter_list.end(); i++ )
    {
	reporter = (*i).second ;
	if( reporter ) delete reporter ;
	_reporter_list.erase( i ) ;
    }
}

bool
DODSReporterList::add_reporter( string reporter_name,
			      DODSReporter *reporter_object )
{
    if( find_reporter( reporter_name ) == 0 )
    {
	_reporter_list[reporter_name] = reporter_object ;
	return true ;
    }
    return false ;
}

DODSReporter *
DODSReporterList::remove_reporter( string reporter_name )
{
    DODSReporter *ret = 0 ;
    DODSReporterList::Reporter_iter i ;
    i = _reporter_list.find( reporter_name ) ;
    if( i != _reporter_list.end() )
    {
	ret = (*i).second;
	_reporter_list.erase( i ) ;
    }
    return ret ;
}

DODSReporter *
DODSReporterList::find_reporter( string reporter_name )
{
    DODSReporterList::Reporter_citer i ;
    i = _reporter_list.find( reporter_name ) ;
    if( i != _reporter_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

void
DODSReporterList::report( const DODSDataHandlerInterface &dhi )
{
    DODSReporter *reporter = 0 ;
    DODSReporterList::Reporter_iter i = _reporter_list.begin() ;
    for( ; i != _reporter_list.end(); i++ )
    {
	reporter = (*i).second ;
	if( reporter ) reporter->report( dhi ) ;
    }
}

DODSReporterList *
DODSReporterList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new DODSReporterList ;
    }
    return _instance ;
}

// $Log: DODSReporterList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
