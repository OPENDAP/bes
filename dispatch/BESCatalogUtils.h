// BESCatalogUtils.h

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

#ifndef S_BESCatalogUtils_h
#define S_BESCatalogUtils_h 1

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <list>
#include <map>
#include <string>
#include <vector>

#include "BESObj.h"
#include "BESUtil.h"

class BESInfo;
class BESCatalogEntry;

#if 0
// Refactored the 'singleton list of CatalogUtils out/ jhrg 7/27/18

// This seems odd - to have a singleton that is actually a map of
// instances and have that a member of (each of?) the instances...
// I think this should be a member of a catalog, not a singleton with
// a list of classes. jhrg 2.25.18
#endif

class BESCatalogUtils : public BESObj {
private:
#if 0
	static std::map<std::string, BESCatalogUtils *> _instances;
#endif

    std::string d_name;               ///< The name of the catalog
    std::string d_root_dir;           ///< The pathname of the root directory
    std::list<std::string> d_exclude; ///< list of regexes; exclude matches
    std::list<std::string> d_include; ///< include regexes
    bool d_follow_syms;               ///< Follow file system symbolic links?

    /// This identifies handlers to the things they can read. It is used
    /// to determine which catalog items can be treated as 'data.'
    struct handler_regex {
        std::string handler;
        std::string regex;
    };

    std::vector<handler_regex> d_match_list; ///< The list of types & regexes

    using match_citer = std::vector<handler_regex>::const_iterator;
    BESCatalogUtils::match_citer match_list_begin() const;
    BESCatalogUtils::match_citer match_list_end() const;

    BESCatalogUtils() : d_follow_syms(false) {}

    static void bes_add_stat_info(BESCatalogEntry *entry, struct stat &buf);

public:
    explicit BESCatalogUtils(const std::string &name, bool strict = true);

    ~BESCatalogUtils() override = default;

    /**
     * @brief Get the root directory of the catalog
     *
     * @return The pathname that is the root of the 'catalog'
     */
    const std::string &get_root_dir() const { return d_root_dir; }

    bool follow_sym_links() const { return d_follow_syms; }

    virtual bool include(const std::string &inQuestion) const;
    virtual bool exclude(const std::string &inQuestion) const;

    std::string get_handler_name(const std::string &item) const;
    bool is_data(const std::string &item) const;

    // TODO remove these once we no longer need showCatalog. jhrg 7/27/18
#if 1
    virtual unsigned int get_entries(DIR *dip, const std::string &fullnode, const std::string &use_node,
                                     BESCatalogEntry *entry, bool dirs_only);

    static void display_entry(BESCatalogEntry *entry, BESInfo *info);

    static void bes_add_stat_info(BESCatalogEntry *entry, const std::string &fullnode);

    static bool isData(const std::string &inQuestion, const std::string &catalog, std::list<std::string> &services);
#endif

    void dump(std::ostream &strm) const override;
};

#endif // S_BESCatalogUtils_h
