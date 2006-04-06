// DirectoryCatalog.cc

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

#include "sys/types.h"
#include "sys/stat.h"
#include "dirent.h"
#include "stdio.h"

#include <sstream>

using std::stringstream ;

#include "DirectoryCatalog.h"
#include "TheDODSKeys.h"
#include "DODSInfo.h"
#include "DODSResponseException.h"
#include "DODSResponseNames.h"
#include "GNURegex.h"

DirectoryCatalog::DirectoryCatalog( const string &name )
{
    bool found = false ;
    string key = (string)"Catalog." + name + ".RootDirectory" ;
    _rootDir = TheDODSKeys::TheKeys()->get_key( key, found ) ;
    if( !found || _rootDir == "" )
    {
	string serr = "DirectoryCatalog - unable to load root directory key "
		      + key + " from initialization file" ;
	DODSResponseException e( serr ) ;
	throw e ;
    }

    DIR *dip = opendir( _rootDir.c_str() ) ;
    if( dip == NULL )
    {
	string serr = "DirectoryCatalog - unable to load root directory "
	              + _rootDir ;
	DODSResponseException e( serr ) ;
	throw e ;
    }
    closedir( dip ) ;

    key = (string)"Catalog." + name + ".Exclude" ;
    string e_str = TheDODSKeys::TheKeys()->get_key( key, found ) ;
    if( found && e_str != "" )
    {
	buildList( _exclude, e_str ) ;
    }

    key = (string)"Catalog." + name + ".Include" ;
    string i_str = TheDODSKeys::TheKeys()->get_key( key, found ) ;
    if( found && i_str != "" )
    {
	buildList( _include, i_str ) ;
    }
}

DirectoryCatalog::~DirectoryCatalog( )
{
}

bool
DirectoryCatalog::show_catalog( const string &node,
                                const string &coi,
				DODSInfo *info )
{
    string fullnode ;
    if( node == "" )
    {
	fullnode = _rootDir ;
    }
    else
    {
	fullnode = _rootDir + "/" + node ;
    }
    DIR *dip = opendir( fullnode.c_str() ) ;
    if( dip != NULL )
    {
	struct stat cbuf ;
	stat( fullnode.c_str(), &cbuf ) ;
	info->add_data( "        <dataset thredds_collection=\"true\">\n" ) ;
	if( node == "" )
	{
	    add_stat_info( info, cbuf, "/", "        " ) ;
	}
	else
	{
	    add_stat_info( info, cbuf, node, "        " ) ;
	}

	struct dirent *dit;
	unsigned int cnt = 0 ;
	while( ( dit = readdir( dip ) ) != NULL )
	{
	    string dirEntry = dit->d_name ;
	    if( dirEntry != "." && dirEntry != ".." && include( dirEntry ) )
	    {
		cnt++ ;
	    }
	}

	stringstream sscnt ;
	sscnt << "            <count>" << cnt << "</count>" << endl ;
	info->add_data( sscnt.str() ) ;

	if( coi == CATALOG_RESPONSE )
	{
	    rewinddir( dip ) ;

	    while( ( dit = readdir( dip ) ) != NULL )
	    {
		struct stat buf;
		string dirEntry = dit->d_name ;
		if( dirEntry != "." && dirEntry != ".." && include( dirEntry ) )
		{
		    string fullPath = fullnode + "/" + dirEntry ;
		    stat( fullPath.c_str(), &buf ) ;

		    // look at the mode and determine if this is a directory
		    if ( S_ISDIR( buf.st_mode ) )
		    {
			info->add_data( "            <dataset thredds_collection=\"true\">\n" ) ;
			add_stat_info( info, buf, dirEntry, "            " ) ;
			info->add_data( "            </dataset>\n" ) ;
		    }
		    else if ( S_ISREG( buf.st_mode ) )
		    {
			info->add_data( "            <dataset thredds_collection=\"false\">\n" );
			add_stat_info( info, buf, dirEntry, "            " ) ;
			info->add_data( "            </dataset>\n" );
		    }
		}
	    }
	}
	closedir( dip ) ;
	info->add_data( "        </dataset>\n" ) ;
    }
    else
    {
	struct stat buf;
	int statret = stat( fullnode.c_str(), &buf ) ;
	if ( statret == 0 && S_ISREG( buf.st_mode ) )
	{
	    info->add_data( "        <dataset thredds_collection=\"false\">\n" ) ;
	    add_stat_info( info, buf, node, "        " ) ;
	    info->add_data( "        </dataset>\n" ) ;
	}
	else
	{
	    return false ;
	}
    }

    return true ;
}

bool
DirectoryCatalog::include( const string &inQuestion )
{
    bool toInclude = true ;
    // First check the file against the exclude list. If there is a
    // match (the node should be excluded) then check the node against
    // the include list. If there is a match with the include list then
    // include the node (return true).
    list<string>::iterator e_iter = _exclude.begin() ;
    list<string>::iterator e_end = _exclude.end() ;
    for( ; e_iter != e_end; e_iter++ )
    {
	string reg = *e_iter ;
	Regex reg_expr( reg.c_str() ) ;
	if( reg_expr.match( inQuestion.c_str(), inQuestion.length() ) != -1)
	{
	    toInclude = false ;
	}
    }

    if( toInclude == false )
    {
	list<string>::iterator i_iter = _include.begin() ;
	list<string>::iterator i_end = _include.end() ;
	for( ; i_iter != i_end; i_iter++ )
	{
	    string reg = *i_iter ;
	    Regex reg_expr( reg.c_str() ) ;
	    if( reg_expr.match( inQuestion.c_str(), inQuestion.length() ) != -1)
	    {
		toInclude = true ;
	    }
	}
    }
    return toInclude ;
}

void
DirectoryCatalog::buildList( list<string> &theList, const string &listStr )
{
    string::size_type str_begin = 0 ;
    string::size_type str_end = listStr.length() ;
    string::size_type semi = 0 ;
    bool done = false ;
    while( done == false )
    {
	semi = listStr.find( ";", str_begin ) ;
	if( semi == -1 )
	{
	    string s = (string)"Catalog type match malformed, no semicolon, "
		       "looking for type:regexp;[type:regexp;]" ;
	    DODSResponseException pe ;
	    pe.set_error_description( s ) ;
	    throw pe;
	}
	else
	{
	    string a_member = listStr.substr( str_begin, semi-str_begin ) ;
	    str_begin = semi+1 ;
	    if( semi == str_end-1 )
	    {
		done = true ;
	    }
	    theList.push_back( a_member ) ;
	}
    }
}

void
DirectoryCatalog::add_stat_info( DODSInfo *info,
				 struct stat &buf,
				 const string &node,
				 const string &indent )
{
    string newindent = indent + "    " ;
    stringstream snm ;
    snm << newindent << "<name>" << node << "</name>\n" ;
    info->add_data( snm.str() ) ;

    off_t sz = buf.st_size ;
    stringstream ssz ;
    ssz << newindent << "<size>" << sz << "</size>\n" ;
    info->add_data( ssz.str() ) ;

    // %T = %H:%M:%S
    // %F = %Y-%m-%d
    time_t mod = buf.st_mtime ;
    struct tm *stm = gmtime( &mod ) ;
    char mdate[64] ;
    strftime( mdate, 64, "%F", stm ) ;
    char mtime[64] ;
    strftime( mtime, 64, "%T", stm ) ;
    string lm = newindent + "<lastmodified>\n" ;
    info->add_data( lm ) ;
    stringstream sdt ;
    sdt << newindent << "    <date>" << mdate << "</date>\n" ;
    info->add_data( sdt.str() ) ;
    stringstream stt ;
    stt << newindent << "    <time>" << mtime << "</time>\n" ;
    info->add_data( stt.str() ) ;
    lm = newindent + "</lastmodified>\n" ;
    info->add_data( lm ) ;
}

// $Log: DirectoryCatalog.cc,v $

