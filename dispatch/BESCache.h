// BESCache.h

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

#ifndef BESCache_h_
#define BESCache_h_ 1

#include <algorithm>
#include <map>
#include <string>
#include <sstream>

#include "BESObj.h"

class BESKeys ;

static const unsigned int MAX_LOCK_RETRY_MS = 5000;	// in microseconds
static const unsigned int MAX_LOCK_TRIES = 16;

/** @brief Implementation of a caching mechanism.
 *
 * The caching mechanism simply allows the user to create a cache. Cached
 * files are typically specified by full path. The file name is changed by
 * changing all slashes to pound signs (#), the ending extension is removed,
 * and the specified prefix is prepended to the name of the cached file.
 *
 * The purge method removes the oldest accessed files until the size of all
 * files is less than that specified by the size in the constructors.
 */
class BESCache : public BESObj
{
public:

  /** for filename -> filesize map below */
  struct cache_entry
  {
      string name ;
      unsigned long long size ;
  };

  /** Sugar for the multimap of entries sorted with
    older files first. */
  typedef std::multimap<double, cache_entry, std::greater<double> > CacheFilesByAgeMap;

  /** Helper class for info on the cache directory */
    struct CacheDirInfo
    {
      CacheDirInfo()
      : _total_cache_files_size(0ULL)
      , _num_files_in_cache(0ULL)
      , _contents()
      {}

      ~CacheDirInfo()
      {
        clear();
      }

      void clear()
      {
        _total_cache_files_size = 0ULL;
        _num_files_in_cache = 0ULL;
        _contents.clear();
      }

      unsigned long long get_avg_size() const
      {
        return ( (_num_files_in_cache > 0) ?
            ( _total_cache_files_size / _num_files_in_cache) :
            (0ULL) );
      }

      std::string toString() const
      {
        std::ostringstream oss;
        oss << "Numfiles: " << _num_files_in_cache << ""
            " Total size: " << _total_cache_files_size;
        return oss.str();
      }

      unsigned long long _total_cache_files_size;
      unsigned long long _num_files_in_cache;
      BESCache::CacheFilesByAgeMap _contents;
    };  // struct CacheDirInfo


private:
    // slashes are replaced with this char to make cache file.
    static const char           BES_CACHE_CHAR = '#';

    string 			_cache_dir ;
    string 			_prefix ;
    unsigned long long 		_cache_size_in_megs ;
    int				_lock_fd ;

    void                        check_ctor_params();
				BESCache() {}
public:
    				BESCache( const string &cache_dir,
					  const string &prefix,
					  unsigned long long sizeInMegs ) ;
    				BESCache( BESKeys &keys,
					  const string &cache_dir_key,
					  const string &prefix_key,
					  const string &size_key ) ;
    virtual			~BESCache() {}

    virtual bool		lock( unsigned int retry_ms,
                                      unsigned int num_tries ) ;
    virtual bool		unlock() ;

    virtual bool		is_cached( const string &src, string &target ) ;
    virtual void		purge( ) ;

    string			cache_dir( ) const { return _cache_dir ; }
    string                      match_prefix() const { return _prefix + BES_CACHE_CHAR; };
    string			prefix( ) const { return _prefix ; }
    unsigned long long		cache_size( ) const  { return _cache_size_in_megs ; }

    void                        collect_cache_dir_info(
                                  BESCache::CacheDirInfo& cd_info //output
                                  ) const;

    virtual void		dump( ostream &strm ) const ;
};

#endif // BESCache_h_

