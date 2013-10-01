// BESCache.cc

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

#include "config.h"

#include <unistd.h>  // for unlink
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include <cstring>
#include <cerrno>
#include <iostream>
#include <sstream>

#include "BESCache.h"
#include "TheBESKeys.h"
#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESDebug.h"

using std::string;
using std::multimap ;
using std::pair ;
using std::greater ;
using std::endl ;

// conversion factor
static const unsigned long long BYTES_PER_MEG = 1048576ULL;

// Max cache size in megs, so we can check the user input and warn.
// 2^64 / 2^20 == 2^44
static const unsigned long long MAX_CACHE_SIZE_IN_MEGABYTES = (1ULL << 44);

void 
BESCache::check_ctor_params()
{
    if( _cache_dir.empty() )
    {
	string err = "The cache directory was not specified, must be non-empty";
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    struct stat buf;
    int statret = stat( _cache_dir.c_str(), &buf ) ;
    if( statret != 0 || ! S_ISDIR(buf.st_mode) )
    {
	string err = "The cache directory " + _cache_dir + " does not exist" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    if( _prefix.empty() )
    {
	string err = "The cache file prefix was not specified, must be non-empty" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    if( _cache_size_in_megs <= 0 )
    {
	string err = "The cache size was not specified, must be non-zero" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    // If the user specifies a cache that is too large,
    // it is a user exception and we should tell them.
    // Actually, this may not work since by this
    // time we may have already overflowed the variable...
    if( _cache_size_in_megs > MAX_CACHE_SIZE_IN_MEGABYTES )
      {
        _cache_size_in_megs = MAX_CACHE_SIZE_IN_MEGABYTES ;
        std::ostringstream msg;
        msg << "The specified cache size was larger than the max cache size of: "
            << MAX_CACHE_SIZE_IN_MEGABYTES;
        throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
      }

    BESDEBUG( "bes", "BES Cache: directory " << _cache_dir
		     << ", prefix " << _prefix
		     << ", max size " << _cache_size_in_megs << endl ) ;
}

/** @brief Constructor that takes as arguments the values of the cache dir,
 * the file prefix, and the cache size
 *
 * @param cache_dir directory where the files are cached
 * @param prefix prefix used to prepend to the resulting cached file
 * @param size cache max size in megabytes (1 == 1048576 bytes)
 * @throws BESSyntaxUserError if cache_dir or prefix is 
 * empty, if size is 0, or if the cache directory does not exist.
 */
BESCache::BESCache( const string &cache_dir,
		    const string &prefix,
		    unsigned long long sizeInMegs )
    : _cache_dir( cache_dir ),
      _prefix( prefix ),
      _cache_size_in_megs( sizeInMegs ),
      _lock_fd( -1 )
{
    check_ctor_params(); // Throws BESSyntaxUserError on error.
}

/** @brief Constructor that takes as arguments keys to the cache directory,
 * file prefix, and size of the cache to be looked up a configuration file
 *
 * The keys specified are looked up in the specified keys object. If not
 * found or not set correctly then an exception is thrown. I.E., if the
 * cache directory is empty, the size is zero, or the prefix is empty.
 *
 * @param keys BESKeys object used to look up the keys
 * @param cache_dir_key key to look up in the keys file to find cache dir
 * @param prefix_key key to look up in the keys file to find the cache prefix
 * @param size_key key to look up in the keys file to find the cache size
 * @throws BESSyntaxUserError if keys not set, cache dir or prefix empty,
 * size is 0, or if cache dir does not exist.
 */
BESCache::BESCache( BESKeys &keys,
		    const string &cache_dir_key,
		    const string &prefix_key,
		    const string &size_key )
    : _cache_size_in_megs( 0 ),
      _lock_fd( -1 )
{
    bool found = false ;
    keys.get_value( cache_dir_key, _cache_dir, found ) ;
    if( !found )
    {
	string err = "The cache directory key " + cache_dir_key
	             + " was not found in the BES configuration file" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    keys.get_value( prefix_key, _prefix, found ) ;
    if( !found )
    {
	string err = "The prefix key " + prefix_key
	             + " was not found in the BES configuration file" ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    string cache_size_str ;
    keys.get_value( size_key, cache_size_str, found ) ;
    if( !found )
    {
	string err = "The size key " + size_key
	             + " was not found in the BES configuration file" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    std::istringstream is( cache_size_str ) ;
    is >> _cache_size_in_megs ;

    check_ctor_params(); // Throws BESSyntaxUserError on error.
}

/** @brief lock the cache using a file lock
 *
 * if the cache has not already been locked, lock it using a file lock.
 *
 * @throws BESInternalError if the cache is already locked
 */
bool
BESCache::lock( unsigned int retry, unsigned int num_tries )
{
    // make sure we aren't retrying too many times
    if( num_tries > MAX_LOCK_TRIES )
	num_tries = MAX_LOCK_TRIES ;
    if( retry > MAX_LOCK_RETRY_MS )
	retry = MAX_LOCK_RETRY_MS ;

    bool got_lock = true ;
    if( _lock_fd == -1 )
    {
	string lock_file = _cache_dir + "/lock" ;
	unsigned int tries = 0 ;
	_lock_fd = open( lock_file.c_str(),
			 O_CREAT | O_EXCL,
			 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ;
	while( _lock_fd < 0 && got_lock )
	{
	    tries ++ ;
	    if( tries > num_tries )
	    {
		_lock_fd = -1 ;
		got_lock = false ;
	    }
	    else
	    {
		usleep( retry ) ;
		_lock_fd = open( lock_file.c_str(),
				 O_CREAT | O_EXCL,
				 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ;
	    }
	}
    }
    else
    {
	// This would be a programming error, or we've gotten into a
	// situation where the lock is lost. Lock has been called on the
	// same cache object twice in a row without an unlock being called.
	string err = "The cache dir " + _cache_dir + " is already locked" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    return got_lock ;
}

/** @brief unlock the cache
 *
 * If the cache is locked, unlock it using the stored file lock descriptor
 *
 * @throws BESInternalError if the cache is not already locked
 */
bool
BESCache::unlock()
{
    // if we call unlock twice in a row, does it matter? I say no, just say
    // that it is unlocked.
    bool unlocked = true ;
    if( _lock_fd != -1 )
    {
	string lock_file = _cache_dir + "/lock" ;
	close( _lock_fd ) ;
	(void)unlink( lock_file.c_str() ) ;
    }

    _lock_fd = -1 ;

    return unlocked ;
}

/** @brief Determine if the file specified by src is cached
 *
 * The src is the full name of the file to be cached. We are assuming that
 * the file name passed has an extension on the end that will be stripped
 * once the file is cached. In other words, if the full path to the file
 * name is /usr/lib/data/fnoc1.nc.gz (a compressed netcdf file) then the
 * resulting file name set in target will be set to 
 * \#&lt;prefix&gt;\#usr\#lib\#data\#fnoc1.nc.
 *
 * @param src src file that will be cached eventually
 * @param target set to the resulting cached file
 * @return true if the file is cached already
 */
bool
BESCache::is_cached( const string &src, string &target )
{
    bool is_it = true ;
    string tmp_target = src ;

    // Create the file that would be created in the cache directory
    //echo ${infile} | sed 's/^\///' | sed 's/\//#/g' | sed 's/\(.*\)\..*$/\1/g'
    if( tmp_target.at(0) == '/' )
    {
	tmp_target = src.substr( 1, tmp_target.length() - 1 ) ;
    }
    string::size_type slash = 0 ;
    while( ( slash = tmp_target.find( '/' ) ) != string::npos )
    {
	tmp_target.replace( slash, 1, 1, BES_CACHE_CHAR ) ;
    }
    string::size_type last_dot = tmp_target.rfind( '.' ) ;
    if( last_dot != string::npos )
    {
	tmp_target = tmp_target.substr( 0, last_dot ) ;
    }

    target = _cache_dir + "/" + _prefix + BES_CACHE_CHAR + tmp_target ;

    // Determine if the target file is already in the cache or not
    struct stat buf;
    int statret = stat( target.c_str(), &buf ) ;
    if( statret != 0 )
    {
	is_it = false ;
    }

    return is_it ;
}




/** @brief Check to see if the cache size exceeds the size specified in the
 * constructor and purge older files until size is less
 *
 * Usually called prior to caching a new file, this method will purge any
 * files, oldest to newest, if the current size of the cache exceeds the
 * size of the cache specified in the constructor.
 *
 */
void
BESCache::purge( )
{
    // Fill in contents and get the info
    CacheDirInfo cd_info;
    collect_cache_dir_info(cd_info);
    unsigned long long avg_size = cd_info.get_avg_size();

    // These are references in the refactor, probably would make
    // sense to add these calls below to the info, but...
    unsigned long long& size = cd_info._total_cache_files_size;
    unsigned long long& num_files_in_cache = cd_info._num_files_in_cache;
    BESCache::CacheFilesByAgeMap& contents = cd_info._contents;

    BESDEBUG( "bes", "cache size = " << size << endl ) ;
    BESDEBUG( "bes", "avg size = " << avg_size << endl ) ;
    BESDEBUG( "bes", "num files in cache = "
			 << num_files_in_cache << endl ) ;
    if( BESISDEBUG( "bes" ) )
      {
        BESDEBUG( "bes", endl << "BEFORE" << endl ) ;
        CacheFilesByAgeMap::iterator ti = contents.begin() ;
        CacheFilesByAgeMap::iterator te = contents.end() ;
        for( ; ti != te; ti++ )
          {
            BESDEBUG( "bes", (*ti).first << ": " << (*ti).second.name << ": size " << (*ti).second.size << endl ) ;
          }
        BESDEBUG( "bes", endl ) ;
      }


    // if the size of files is greater than max allowed then we need to
    // purge the cache directory. Keep going until the size is less than
    // the max.
    // [Maybe change this to size + (fraction of max_size) > max_size?
    // jhrg 5/9/07]
    unsigned long long max_size_in_bytes = _cache_size_in_megs * BYTES_PER_MEG ; // Bytes/Meg
    while( (size+avg_size) > max_size_in_bytes )
      {
        // Grab the first which is the oldest
        // in terms of access time.
        CacheFilesByAgeMap::iterator i = contents.begin() ;

        // if we've deleted all entries, exit the loop
        if( i == contents.end() )
          {
            break;
          }

        // Otherwise, remove the file with unlink
        BESDEBUG( "bes", "BESCache::purge - removing "
            << (*i).second.name << endl ) ;
        // unlink rather than remove in case the file is in use
        // by a forked BES process
        if( unlink( (*i).second.name.c_str() ) != 0 )
          {
            char *s_err = strerror( errno ) ;
            string err = "Unable to remove the file "
                + (*i).second.name
                + " from the cache: " ;
            if( s_err )
              {
                err.append( s_err ) ;
              }
            else
              {
                err.append( "Unknown error" ) ;
              }
            throw BESInternalError( err, __FILE__, __LINE__ ) ;
          }

        size -= (*i).second.size ;
        contents.erase( i ) ;
      }

    if( BESISDEBUG( "bes" ) )
      {
        BESDEBUG( "bes", endl << "AFTER" << endl ) ;
        CacheFilesByAgeMap::iterator ti = contents.begin() ;
        CacheFilesByAgeMap::iterator te = contents.end() ;
        for( ; ti != te; ti++ )
          {
            BESDEBUG( "bes", (*ti).first << ": " << (*ti).second.name << ": size " << (*ti).second.size << endl ) ;
          }
      }
}

// Local RAII helper class to be sure the DIR
// is closed in the face of exceptions using RAII
struct DIR_Wrapper
{
  DIR_Wrapper(const std::string& dir_name)
  {
    _dip = opendir(dir_name.c_str());
  }

  ~DIR_Wrapper()
  {
    close();
  }

  DIR* get() const { return _dip; }

  void close()
  {
    if (_dip)
      {
        closedir(_dip);
        _dip = NULL;
      }
  }

  // data rep
  DIR* _dip;
};

void
BESCache::collect_cache_dir_info(
    BESCache::CacheDirInfo& cd_info // output
    ) const
{
  // start fresh
  cd_info.clear();

  time_t curr_time = time( NULL ) ; // grab the current time so we can
                                        // determine the oldest file

  DIR_Wrapper dip = DIR_Wrapper( _cache_dir );
  if (! (dip.get()) )
    {
      string err = "Unable to open cache directory " + _cache_dir ;
      throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
  else // got a dir entry so count up the cached files
    {
      struct stat buf;
      struct dirent *dit;
      // go through the cache directory and collect all of the files that
      // start with the matching prefix
      while( ( dit = readdir( dip.get() ) ) != NULL )
        {
          string dirEntry = dit->d_name ;
          if( dirEntry.compare( 0, _prefix.length(), _prefix ) == 0)
            {
              // Now that we have found a match we want to get the size of
              // the file and the last access time from the file.
              string fullPath = _cache_dir + "/" + dirEntry ;
              int statret = stat( fullPath.c_str(), &buf ) ;
              if( statret == 0 )
                {
                  cd_info._total_cache_files_size += buf.st_size ;

                  // Find out how old the file is
                  time_t file_time = buf.st_atime ;

                  // I think we can use the access time without the diff,
                  // since it's the relative ages that determine when to
                  //         delete a file. Good idea to use the access time so
                  // recently used (read) files will linger. jhrg 5/9/07
                  double time_diff = difftime( curr_time, file_time ) ;
                  cache_entry entry ;
                  entry.name = fullPath ;
                  entry.size = buf.st_size ;
                  cd_info._contents.insert( pair<double, cache_entry>( time_diff, entry ) );
                  }
              cd_info._num_files_in_cache++ ;
            }
        }
    }

  dip.close();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this cache.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESCache::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCache::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "cache dir: " << _cache_dir << endl ;
    strm << BESIndent::LMarg << "prefix: " << _prefix << endl ;
    strm << BESIndent::LMarg << "size (mb): " << _cache_size_in_megs << endl ;
    BESIndent::UnIndent() ;
}

