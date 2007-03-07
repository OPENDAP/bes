// BESCatalogDirectory.cc

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

#include "sys/types.h"
#include "sys/stat.h"
#include "dirent.h"
#include "stdio.h"

#include <sstream>

using std::stringstream ;
using std::endl ;

#include "BESCatalogDirectory.h"
#include "BESCatalogUtils.h"
#include "BESInfo.h"
#include "BESResponseException.h"
#include "BESResponseNames.h"
#include "BESCatalogUtils.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"

BESCatalogDirectory::BESCatalogDirectory( const string &name )
    : BESCatalog( name )
{
    try
    {
	_utils = BESCatalogUtils::Utils( name ) ;
    }
    catch( BESException &e )
    {
	throw BESResponseException( e.get_message(), e.get_file(), e.get_line() ) ;
    }
}

BESCatalogDirectory::~BESCatalogDirectory( )
{
}

bool
BESCatalogDirectory::show_catalog( const string &node,
                                const string &coi,
				BESInfo *info )
{
    // remove any trailing slash
    string fullnode = node ;
    if( node != "" )
    {
	string::size_type stopat = node.length() - 1 ;
	while( node[stopat] == '/' )
	{
	    stopat-- ;
	}
	fullnode = fullnode.substr( 0, stopat + 1 ) ;
    }

    if( fullnode == "" )
    {
	fullnode = _utils->get_root_dir() ;
    }
    else
    {
	fullnode = _utils->get_root_dir() + "/" + fullnode ;
    }
    string basename ;
    string::size_type slash = fullnode.rfind( "/" ) ;
    if( slash != string::npos )
    {
	basename = fullnode.substr( slash+1, fullnode.length() - slash ) ;
    }
    else
    {
	basename = fullnode ;
    }
    DIR *dip = opendir( fullnode.c_str() ) ;
    if( dip != NULL )
    {
	// if the directory requested is in the exclude list then we won't
	// let the user see it.
	if( _utils->exclude( basename ) )
	{
	    return false ;
	}
	struct stat cbuf ;
	stat( fullnode.c_str(), &cbuf ) ;
	map<string,string> a1 ;
	a1["thredds_collection"] = "\"true\"" ;
	a1["isData"] = "\"false\"" ;
	info->begin_tag( "dataset", &a1 ) ;
	if( node == "" )
	{
	    add_stat_info( info, cbuf, "/" ) ;
	}
	else
	{
	    add_stat_info( info, cbuf, node ) ;
	}

	struct dirent *dit;
	unsigned int cnt = 0 ;
	struct stat buf;
	while( ( dit = readdir( dip ) ) != NULL )
	{
	    string dirEntry = dit->d_name ;
	    if( dirEntry != "." && dirEntry != ".." )
	    {
		// look at the mode and determine if this is a directory
		string fullPath = fullnode + "/" + dirEntry ;
		stat( fullPath.c_str(), &buf ) ;
		if ( S_ISDIR( buf.st_mode ) )
		{
		    if( _utils->exclude( dirEntry ) == false )
		    {
			cnt++ ;
		    }
		}
		else if ( S_ISREG( buf.st_mode ) )
		{
		    if( _utils->include( dirEntry ) )
		    {
			cnt++ ;
		    }
		}
	    }
	}

	stringstream sscnt ;
	sscnt << cnt ;
	info->add_tag( "count", sscnt.str() ) ;

	if( coi == CATALOG_RESPONSE )
	{
	    rewinddir( dip ) ;

	    while( ( dit = readdir( dip ) ) != NULL )
	    {
		string dirEntry = dit->d_name ;
		if( dirEntry != "." && dirEntry != ".." )
		{
		    // look at the mode and determine if this is a directory
		    string fullPath = fullnode + "/" + dirEntry ;
		    stat( fullPath.c_str(), &buf ) ;
		    if ( S_ISDIR( buf.st_mode ) )
		    {
			if( _utils->exclude( dirEntry ) == false )
			{
			    map<string,string> a2 ;
			    a2["thredds_collection"] = "\"true\"" ;
			    a2["isData"] = "\"false\"" ;
			    info->begin_tag( "dataset", &a2 ) ;
			    add_stat_info( info, buf, dirEntry ) ;
			    info->end_tag( "dataset" ) ;
			}
		    }
		    else if ( S_ISREG( buf.st_mode ) )
		    {
			if( _utils->include( dirEntry ) )
			{
			    map<string,string> a3 ;
			    a3["thredds_collection"] = "\"false\"" ;
			    list<string> provides ;
			    if( isData( dirEntry, provides ) )
				a3["isData"] = "\"true\"" ;
			    else
				a3["isData"] = "\"false\"" ;
			    info->begin_tag( "dataset", &a3 ) ;
			    add_stat_info( info, buf, dirEntry ) ;
			    info->end_tag( "dataset" ) ;
			}
		    }
		}
	    }
	}
	closedir( dip ) ;
	info->end_tag( "dataset" ) ;
    }
    else
    {
	// if the node is in the include list then continue, else the node
	// requested is not included and we return false.
	if( _utils->include( basename ) )
	{
	    struct stat buf;
	    int statret = stat( fullnode.c_str(), &buf ) ;
	    if ( statret == 0 && S_ISREG( buf.st_mode ) )
	    {
		map<string,string> a4 ;
		a4["thredds_collection"] = "\"false\"" ;
		list<string> provides ;
		if( isData( node, provides ) )
		    a4["isData"] = "\"true\"" ;
		else
		    a4["isData"] = "\"false\"" ;
		info->begin_tag( "dataset", &a4 ) ;
		add_stat_info( info, buf, node ) ;
		info->end_tag( "dataset" ) ;
	    }
	    else
	    {
		return false ;
	    }
	}
	else
	{
	    return false ;
	}
    }

    return true ;
}

void
BESCatalogDirectory::add_stat_info( BESInfo *info,
				    struct stat &buf,
				    const string &node )
{
    info->add_tag( "name", node ) ;

    off_t sz = buf.st_size ;
    stringstream ssz ;
    ssz << sz ;
    info->add_tag( "size", ssz.str() ) ;

    // %T = %H:%M:%S
    // %F = %Y-%m-%d
    time_t mod = buf.st_mtime ;
    struct tm *stm = gmtime( &mod ) ;
    char mdate[64] ;
    strftime( mdate, 64, "%F", stm ) ;
    char mtime[64] ;
    strftime( mtime, 64, "%T", stm ) ;

    info->begin_tag( "lastmodified" ) ;

    stringstream sdt ;
    sdt << mdate ;
    info->add_tag( "date", sdt.str() ) ;

    stringstream stt ;
    stt << mtime ;
    info->add_tag( "time", stt.str() ) ;

    info->end_tag( "lastmodified" ) ;
}

bool
BESCatalogDirectory::isData( const string &inQuestion,
			     list<string> &provides )
{
    BESContainerStorage *store =
	BESContainerStorageList::TheList()->find_persistence( get_catalog_name() ) ;
    if( !store )
	return false ;

    BESContainerStorageCatalog *cat_store =
	dynamic_cast<BESContainerStorageCatalog *>(store ) ;
    if( !cat_store )
	return false ;

    return cat_store->isData( inQuestion, provides ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCatalogDirectory::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCatalogDirectory::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;

    strm << BESIndent::LMarg << "catalog utilities: " << endl ;
    BESIndent::Indent() ;
    _utils->dump( strm ) ;
    BESIndent::UnIndent() ;
    BESIndent::UnIndent() ;
}

