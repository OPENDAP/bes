// BESUncompressManager.cc

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

#include <sstream>

using std::istringstream ;

#include "BESUncompressManager.h"
#include "BESUncompressGZ.h"
#include "BESUncompressBZ2.h"
#include "BESUncompressZ.h"
#include "BESCache.h"
#include "BESContainerStorageException.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

BESUncompressManager *BESUncompressManager::_instance = 0 ;

/** @brief constructs an uncompression manager adding gz and bz2
 * uncompression methods by default.
 */
BESUncompressManager::BESUncompressManager()
{
    add_method( "gz", BESUncompressGZ::uncompress ) ;
    add_method( "bz2", BESUncompressBZ2::uncompress ) ;
    add_method( "z", BESUncompressZ::uncompress ) ;

    bool found = false ;
    string key = "BES.Uncompress.Retry" ;
    string val = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( !found || val.empty() )
    {
	_retry = 2 ;
    }
    else
    {
	istringstream is( val ) ;
	is >> _retry ;
    }

    key = "BES.Uncompress.NumTries" ;
    val = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( !found || val.empty() )
    {
	_num_tries = 10 ;
    }
    else
    {
	istringstream is( val ) ;
	is >> _num_tries ;
    }
}

/** @brief add a uncompress method to the list
 *
 * This method actually adds to the list a static method that knows how to
 * uncompress a particular type of file. For example, a .gz or .bz2 file.
 *
 * @param name name of the method to add to the list
 * @param method the static function that uncompress the particular type of file
 * @return true if successfully added, false if it already exists
 */
bool
BESUncompressManager::add_method( const string &name,
			      	  p_bes_uncompress method )
{
    BESUncompressManager::UCIter i ;
    i = _uncompress_list.find( name ) ;
    if( i == _uncompress_list.end() )
    {
	_uncompress_list[name] = method ;
	return true ;
    }
    return false ;
}

/** @brief removes a uncompress method from the list
 *
 * The static method that knows how to uncompress the specified type of file
 * is removed from the list.
 *
 * @param name name of the method to remove
 * @return true if successfully removed, false if it doesn't exist in the list
 */
bool
BESUncompressManager::remove_method( const string &name )
{
    BESUncompressManager::UIter i ;
    i = _uncompress_list.find( name ) ;
    if( i != _uncompress_list.end() )
    {
	_uncompress_list.erase( i ) ;
	return true ;
    }
    return false ;
}

/** @brief returns the uncompression method specified
 *
 * This method looks up the uncompression method with the given name and
 * returns that method.
 *
 * @param name name of the uncompression method to find
 * @return the function of type p_bes_uncompress
 */
p_bes_uncompress
BESUncompressManager::find_method( const string &name )
{
    BESUncompressManager::UCIter i ;
    i = _uncompress_list.find( name ) ;
    if( i != _uncompress_list.end() )
    {
	return (*i).second ;
    }
    return 0 ;
}

/** @brief returns the comma separated list of all uncompression methods
 * currently registered.
 *
 * @return comma separated list of uncompression method names
 */
string
BESUncompressManager::get_method_names()
{
    string ret ;
    bool first_name = true ;
    BESUncompressManager::UCIter i = _uncompress_list.begin() ;
    for( ; i != _uncompress_list.end(); i++ )
    {
	if( !first_name )
	    ret += ", " ;
	ret += (*i).first ;
	first_name = false ;
    }
    return ret ;
}

/** @brief find the method that can uncompress the specified src and pass
 * control to that method.
 *
 * @param src file to be uncompressed
 * @param cache BESCache object to uncompress the src file in
 * @return full path to the uncompressed file
 */
string
BESUncompressManager::uncompress( const string &src, BESCache &cache )
{
    BESDEBUG( "BESUncompressManager::uncompress - src = " << src << endl )
    string::size_type dot = src.rfind( "." ) ;
    if( dot != string::npos )
    {
	string ext = src.substr( dot+1, src.length() - dot ) ;
	// Why fold the extension to lowercase? jhrg 5/9/07
	for( int i = 0; i < ext.length(); i++ )
	{
	    ext[i] = tolower( ext[i] ) ;
	}

	// if we find the method for this file then use it. If we don't find
	// it then assume that the file is not compressed and simply return
	// the src file at the end of the method.
	p_bes_uncompress p = find_method( ext ) ;
	if( p )
	{
	    // the file is compressed so we either need to uncompress it or
	    // we need to tell if it is already cached. To do this, lock the
	    // cache so no one else can do anything
	    if( cache.lock( _retry, _num_tries ) )
	    {
		try
		{
		    // before calling uncompress on the file, see if the file
		    // has already been cached. If it has, then simply return
		    // the target, no need to cache.
		    BESDEBUG( "BESUncompressManager::uncompress - is cached? " \
			      << src << endl )
		    string target ;
		    if( cache.is_cached( src, target ) )
		    {
			BESDEBUG( "BESUncompressManager::uncompress - " \
			          << "is cached " << target << endl )
			cache.unlock() ;
			return target ;
		    }

		    // the file is not cached, so we need to uncompress the
		    // file.  First determine if there is enough space in
		    // the cache to uncompress the file
		    BESDEBUG( "BESUncompressManager::uncompress - " \
		              << "purging cache" << endl )
		    cache.purge() ;

		    // Now that we have some room ... uncompress the file
		    BESDEBUG( "BESUncompressManager::uncompress - " \
		              << "uncompress to " << target \
			      << " using " << ext << " uncompression" \
			      << endl )

		    // we are now done in the cahce, unlock it
		    cache.unlock() ;

		    // How about having the p_bes_uncompress() return bool
		    // since we already know the name of the target? Or
		    // return void since it throws on error? jhrg 5/9/07
		    return p( src, target ) ;
		}
		catch( BESException &e )
		{
		    // a problem in the cache, unlock it and re-throw the
		    // exception
		    cache.unlock() ;
		    throw e ;
		}
		catch( ... )
		{
		    // an unknown problem in the cache, unlock it and throw a
		    // BES exception
		    cache.unlock() ;
		    string err = (string)"Problem working with the cache, "
		                 + "unknow error" ;
		    throw BESContainerStorageException( err, __FILE__,__LINE__);
		}
	    }
	    else
	    {
		string err = "Unable to lock the cache " 
		             + cache.cache_dir() ;
		throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    BESDEBUG( "BESUncompressManager::uncompress - not compressed " \
		      << endl )
	}
    }
    else
    {
	string err = "Unable to determine type of file from "
	             + src ;
	throw BESContainerStorageException( err, __FILE__, __LINE__ ) ;
    }

    return src ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with the names of the
 * registered uncompression methods.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESUncompressManager::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESUncompressManager::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _uncompress_list.size() )
    {
	strm << BESIndent::LMarg << "registered uncompression methods:" << endl;
	BESIndent::Indent() ;
	BESUncompressManager::UCIter i = _uncompress_list.begin() ;
	BESUncompressManager::UCIter ie = _uncompress_list.end() ;
	for( ; i != ie; i++ )
	{
	    strm << BESIndent::LMarg << (*i).first << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "registered uncompress methods: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

BESUncompressManager *
BESUncompressManager::TheManager()
{
    if( _instance == 0 )
    {
	_instance = new BESUncompressManager ;
    }
    return _instance ;
}
