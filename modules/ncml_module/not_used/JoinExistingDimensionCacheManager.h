///////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2011 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#if 0 // THis is in progress, so commented out to avoid compilation bugs.
#ifndef __NCML_MODULE__JOIN_EXISTING_DIMENSION_CACHE_MANAGER_H__
#define __NCML_MODULE__JOIN_EXISTING_DIMENSION_CACHE_MANAGER_H__

#include <memory>
#include <string>

namespace agg_util
{
    class AggMemberDataset;
};

namespace ncml_module
{
    class JoinExistingDimensionCacheManager
    {
    public:

        /** Create a cache manager which saves files into the specified
         * absolute path cacheDir.
         * Will throw if cacheDir is not a valid dir
         * (ie write permission, exists, etc) */
        JoinExistingDimensionCacheManager(const std::string& cacheDir);
        ~JoinExistingDimensionCacheManager();

        /** Construct a cache object to handle caching
         * for this manager.
         */
        static std::unique_ptr<JoinExistingDimensionCache> makeCacheInstance(const std::string& sourceFile);

    }; // class JoinExistingDimensionCacheManager

    class JoinExistingDimensionCache
    {
        friend class JoinExistingDimensionCacheManager;

    private:
        // only the manager can make them
        JoinExistingDimensionCache(const std::string sourcePath, const std::string& cacheDir);

    public:
        ~JoinExistingDimensionCache();

        /** Is there a cache file associated with the
         * source file?
         * */
        bool doesCacheFileExist() const;

        /** Check the modification times on the cache
         * file and return whether it is newer than the
         * source file.
         */
        bool isCacheFileFresh() const;

        /////////////////////////////////////////////////////////////////
    private:// data rep

        std::string _sourceFilename;// source to be cached, set in ctor
        std::string _cacheFilename;// cache file associated with _sourceFilename
        std::string _tempCacheFilename;// temp name for cache file for writing it before copy

        // @var Mod time for the source file
        // @var Mod time for the cache file

        // Locks?

    }; // class JoinExistingDimensionCache

}

#endif /* __NCML_MODULE__JOIN_EXISTING_DIMENSION_CACHE_MANAGER_H__ */
#endif // 0
