// BESCatalogDirectory.cc

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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cstring>
#include <cerrno>
#include <sstream>

using std::stringstream ;
using std::endl ;

#include "BESCatalogDirectory.h"
#include "BESCatalogUtils.h"
#include "BESInfo.h"
#include "BESResponseNames.h"
#include "BESCatalogUtils.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"
#include "BESLog.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESDebug.h"

void bes_add_stat_info( map<string,string> &props,
			struct stat &buf,
			const string &node ) ;
void bes_get_stat_info( BESCatalogDirectory::bes_dir_entry &entry,
			struct stat &buf,
			const string &node);

BESCatalogDirectory::BESCatalogDirectory( const string &name )
    : BESCatalog( name )
{
    _utils = BESCatalogUtils::Utils( name ) ;
}

BESCatalogDirectory::~BESCatalogDirectory( )
{
}

void
BESCatalogDirectory::show_catalog( const string &node,
                                   const string &coi,
				   BESInfo *info )
{
    // remove any trailing slash
#if 0
    // Replaced the code below since this was causing a memory access error
    // flagged by valgrind when node was "/".2/25/09 jhrg
    string use_node = node ;
    if( node != "" )
    {
	string::size_type stopat = node.length() - 1 ;
	while( node[stopat] == '/' )
	{
	    stopat-- ;
	}
	use_node = use_node.substr( 0, stopat + 1 ) ;
    }
#else
    string use_node = node;
    // use_node should only end in '/' is that's the only character in which
    // case there's no need to call find()
    if (!node.empty() && node != "/") {
        string::size_type pos = use_node.find_last_not_of("/");
        use_node = use_node.substr(0, pos+1);
    }

    // This takes care of bizarre cases like "///" where use_node would be
    // empty after the substring call.
    if (use_node.empty())
        use_node = "/";

    BESDEBUG("bes", "use_node: " << use_node << endl )
#endif
    string rootdir = _utils->get_root_dir() ;
    string fullnode = rootdir ;
    if( !use_node.empty() )
    {
	fullnode = fullnode + "/" + use_node ;
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

    // This will throw the appropriate exception (Forbidden or Not Found).
    // Checks to make sure the different elements of the path are not
    // symbolic links if follow_sym_links is set to false, and checks to
    // make sure have permission to access node and the node exists.
    BESUtil::check_path( use_node, rootdir, _utils->follow_sym_links() ) ;

    // Is this node a directory?
    DIR *dip = opendir( fullnode.c_str() ) ;
    if( dip != NULL )
    {
	// The node is a directory

	// if the directory requested is in the exclude list then we won't
	// let the user see it.
	if( _utils->exclude( basename ) )
	{
	    closedir( dip ) ;
	    string error = "You do not have permission to view the node "
	                   + use_node ;
	    throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
	}
	struct stat cbuf ;
	int statret = stat( fullnode.c_str(), &cbuf ) ;
	int my_errno = errno ;
	if( statret == 0 )
	{
	    map<string,string> props ;
	    props["node"] = "true" ;
	    props["catalog"] = get_catalog_name() ;
	    if( use_node == "" )
	    {
		bes_add_stat_info( props, cbuf, "/" ) ;
	    }
	    else
	    {
		bes_add_stat_info( props, cbuf, use_node ) ;
	    }

	    struct dirent *dit;
	    unsigned int cnt = 0 ;
	    struct stat buf;
	    struct stat lbuf;

	    map<string,bes_dir_entry> dir_list ;
	    while( ( dit = readdir( dip ) ) != NULL )
	    {
		string dirEntry = dit->d_name ;
		if( dirEntry != "." && dirEntry != ".." )
		{
		    string fullPath = fullnode + "/" + dirEntry ;

		    // if follow_sym_links is true then continue with
		    // the checking. If false, first see if the entry is
		    // a symbolic link. If it is, do not include in the
		    // listing for this node. If not, then continue
		    // checking the entry.
		    bool continue_checking = true ;
		    if( _utils->follow_sym_links() == false )
		    {
#if 0
		        int lstatret = lstat( fullPath.c_str(), &lbuf ) ;
#endif
		        (void)lstat( fullPath.c_str(), &lbuf ) ;
			if( S_ISLNK( lbuf.st_mode ) )
			{
			    continue_checking = false ;
			}
		    }

		    if( continue_checking )
		    {
			// look at the mode and determine if this is a
			// directory or a regular file. If it is not
			// accessible, the stat fails, is not a directory
			// or regular file, then simply do not include it.
			statret = stat( fullPath.c_str(), &buf ) ;
			if ( statret == 0 && S_ISDIR( buf.st_mode ) )
			{
			    if( _utils->exclude( dirEntry ) == false )
			    {
				cnt++ ;
				if( coi == CATALOG_RESPONSE )
				{
				    bes_dir_entry entry ;
				    entry.collection = true ;
				    bes_get_stat_info( entry, buf, dirEntry ) ;
				    dir_list[dirEntry] = entry ;
				}
			    }
			}
			else if ( statret == 0 && S_ISREG( buf.st_mode ) )
			{
			    if( _utils->include( dirEntry ) )
			    {
				cnt++ ;
				if( coi == CATALOG_RESPONSE )
				{
				    bes_dir_entry entry ;
				    entry.collection = false ;
				    isData( fullPath, entry.services ) ;
				    bes_get_stat_info( entry, buf, dirEntry ) ;
				    dir_list[dirEntry] = entry ;
				}
			    }
			}
		    }
		}
	    }
	    stringstream sscnt ;
	    sscnt << cnt ;
	    props["count"] = sscnt.str() ;
	    info->begin_tag( "dataset", &props ) ;

	    // Now iterate through the entry list and add it to info. This
	    // will add it in alpha order
	    if( coi == CATALOG_RESPONSE )
	    {
		map<string,bes_dir_entry>::iterator i = dir_list.begin() ;
		map<string,bes_dir_entry>::iterator e = dir_list.end() ;
		for( ; i != e; i++ )
		{
		    map<string,string> attrs ;
		    if( (*i).second.collection )
			attrs["node"] = "true" ;
		    else
			attrs["node"] = "false" ;
		    attrs["catalog"] = get_catalog_name() ;
		    attrs["name"] = (*i).second.name ;
		    attrs["size"] = (*i).second.size ;
		    string dt = (*i).second.mod_date + "T"
				+ (*i).second.mod_time ;
		    attrs["lastModified"] = dt ;
		    info->begin_tag( "dataset", &attrs ) ;

		    list<string>::const_iterator si =
			(*i).second.services.begin() ;
		    list<string>::const_iterator se =
			(*i).second.services.end() ;
		    for( ; si != se; si++ )
		    {
			info->add_tag( "serviceRef", (*si) ) ;
		    }
		    info->end_tag( "dataset" ) ;
		}
	    }
	    closedir( dip ) ;
	    info->end_tag( "dataset" ) ;
	}
	else
	{
	    closedir( dip ) ;
	    // ENOENT means that the path or part of the path does not exist
	    if( my_errno == ENOENT )
	    {
		string error = "Node " + use_node + " does not exist" ;
		char *s_err = strerror( my_errno ) ;
		if( s_err )
		{
		    error = s_err ;
		}
		throw BESNotFoundError( error, __FILE__, __LINE__ ) ;
	    }
	    // any other error means that access is denied for some reason
	    else
	    {
		string error = "Access denied for node " + use_node ;
		char *s_err = strerror( my_errno ) ;
		if( s_err )
		{
		    error = error + s_err ;
		}
		throw BESNotFoundError( error, __FILE__, __LINE__ ) ;
	    }
	}
    }
    else
    {
	// if the node is not in the include list then the requester does
	// not have access to that node
	if( _utils->include( basename ) )
	{
	    struct stat buf;
	    int statret = 0 ;
	    if( _utils->follow_sym_links() == false )
	    {
		/*statret =*/(void)lstat( fullnode.c_str(), &buf ) ;
		if( S_ISLNK( buf.st_mode ) )
		{
		    string error = "You do not have permission to access node "
		                   + use_node ;
		    throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
		}
	    }
	    statret = stat( fullnode.c_str(), &buf ) ;
	    if ( statret == 0 && S_ISREG( buf.st_mode ) )
	    {
		map<string,string> attrs ;
		attrs["node"] = "false" ;
		attrs["catalog"] = get_catalog_name() ;
		bes_add_stat_info( attrs, buf, node ) ;
		info->begin_tag( "dataset", &attrs ) ;

		list<string> services ;
		isData( node, services ) ;
		list<string>::const_iterator si = services.begin() ;
		list<string>::const_iterator se = services.end() ;
		for( ; si != se; si++ )
		{
		    info->add_tag( "serviceRef", (*si) ) ;
		}

		info->end_tag( "dataset" ) ;
	    }
	    else if( statret == 0 )
	    {
		string error = "You do not have permission to access "
		               + use_node ;
		throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
	    }
	    else
	    {
		// ENOENT means that the path or part of the path does not
		// exist
		if( errno == ENOENT )
		{
		    string error = "Node " + use_node + " does not exist" ;
		    char *s_err = strerror( errno ) ;
		    if( s_err )
		    {
			error = s_err ;
		    }
		    throw BESNotFoundError( error, __FILE__, __LINE__ ) ;
		}
		// any other error means that access is denied for some reason
		else
		{
		    string error = "Access denied for node " + use_node ;
		    char *s_err = strerror( errno ) ;
		    if( s_err )
		    {
			error = error + s_err ;
		    }
		    throw BESNotFoundError( error, __FILE__, __LINE__ ) ;
		}
	    }
	}
	else
	{
	    string error = "You do not have permission to access " + use_node ;
	    throw BESForbiddenError( error, __FILE__, __LINE__ ) ;
	}
    }
}

void
bes_add_stat_info( map<string,string> &props,
		   struct stat &buf,
		   const string &node )
{
    BESCatalogDirectory::bes_dir_entry entry ;
    bes_get_stat_info( entry, buf, node ) ;
    props["name"] = entry.name ;
    props["size"] = entry.size ;
    string dt = entry.mod_date + "T" + entry.mod_time ;
    props["lastModified"] = dt ;
}

void
bes_get_stat_info( BESCatalogDirectory::bes_dir_entry &entry,
		   struct stat &buf, const string &node )
{
    entry.name = node ;

    off_t sz = buf.st_size ;
    stringstream ssz ;
    ssz << sz ;
    entry.size = ssz.str() ;

    // %T = %H:%M:%S
    // %F = %Y-%m-%d
    time_t mod = buf.st_mtime ;
    struct tm *stm = gmtime( &mod ) ;
    char mdate[64] ;
    strftime( mdate, 64, "%Y-%m-%d", stm ) ;
    char mtime[64] ;
    strftime( mtime, 64, "%T", stm ) ;

    stringstream sdt ;
    sdt << mdate ;
    entry.mod_date = sdt.str() ;

    stringstream stt ;
    stt << mtime ;
    entry.mod_time = stt.str() ;
}

bool
BESCatalogDirectory::isData( const string &inQuestion,
			     list<string> &services )
{
    BESContainerStorage *store =
	BESContainerStorageList::TheList()->find_persistence( get_catalog_name() ) ;
    if( !store )
	return false ;

    BESContainerStorageCatalog *cat_store =
	dynamic_cast<BESContainerStorageCatalog *>( store ) ;
    if( !cat_store )
	return false ;

    return cat_store->isData( inQuestion, services ) ;
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

