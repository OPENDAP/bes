// BESCatalogUtils.cc

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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cerrno>
#include <iostream>
#include <sstream>
#include <list>
#include <cstring>

#include "BESCatalogUtils.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESRegex.h"
#include "BESUtil.h"
#include "BESInfo.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorage.h"
#include "BESCatalogEntry.h"

using namespace std;

/**
 * @brief Initialize the file system catalog utilities
 *
 * Use parameters in the bes.conf file to configure a BES catalog
 * that reads data from a file system. This will set the root directory
 * of the catalog, regular expressions to exclude and include entries,
 * regular expressions to identify data, and whether the catalog should
 * follow symbolic links. The bes.conf key names are (N == catalog name):
 *
 * BES.Catalog.N.RootDirectory
 * BES.Catalog.N.Exclude
 * BES.Catalog.N.Include
 * BES.Catalog.N.TypeMatch
 * BES.Catalog.N.FollowSymLinks
 *
 * @note The RootDirectory and TypeMatch keys must be present for any
 * catalog N.
 *
 * @param n The name of the catalog.
 * @param strict True (the default) means that the RootDirectory and TypeMatch
 * must be defined; false causes this constructor to supply placeholder values.
 * This feature was added for tests that run without the bes.conf file.
 */
BESCatalogUtils::BESCatalogUtils(const string &n, bool strict) :
    d_name(n), d_follow_syms(false)
{
    string key = "BES.Catalog." + n + ".RootDirectory";
    bool found = false;
    TheBESKeys::TheKeys()->get_value(key, d_root_dir, found);
    if (strict && (!found || d_root_dir == "")) {
        string s = key + " not defined in BES configuration file";
        throw BESSyntaxUserError(s, __FILE__, __LINE__);
    }

    if(d_root_dir != "UNUSED"){
        // TODO access() or stat() would test for existence faster. jhrg 2.25.18
        DIR *dip = opendir(d_root_dir.c_str());
        if (dip == NULL) {
            string serr = "BESCatalogDirectory - root directory " + d_root_dir + " does not exist";
            throw BESNotFoundError(serr, __FILE__, __LINE__);
        }
        closedir(dip);
    }

    found = false;
    key = (string) "BES.Catalog." + n + ".Exclude";
    vector<string> vals;
    TheBESKeys::TheKeys()->get_values(key, vals, found);
    vector<string>::iterator ei = vals.begin();
    vector<string>::iterator ee = vals.end();
    for (; ei != ee; ei++) {
        string e_str = (*ei);
        if (!e_str.empty() && e_str != ";") BESUtil::explode(';', e_str, d_exclude);
    }

    key = (string) "BES.Catalog." + n + ".Include";
    vals.clear();
    TheBESKeys::TheKeys()->get_values(key, vals, found);
    vector<string>::iterator ii = vals.begin();
    vector<string>::iterator ie = vals.end();
    for (; ii != ie; ii++) {
        string i_str = (*ii);
        if (!i_str.empty() && i_str != ";") BESUtil::explode(';', i_str, d_include);
    }

    key = "BES.Catalog." + n + ".TypeMatch";
    list<string> match_list;
    vals.clear();
    TheBESKeys::TheKeys()->get_values(key, vals, found);
    if (strict && (!found || vals.size() == 0)) {
        string s = key + " not defined in key file";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    vector<string>::iterator vi = vals.begin();
    vector<string>::iterator ve = vals.end();
    for (; vi != ve; vi++) {
        BESUtil::explode(';', (*vi), match_list);
    }

    list<string>::iterator mli = match_list.begin();
    list<string>::iterator mle = match_list.end();
    for (; mli != mle; mli++) {
        if (!((*mli).empty()) && *(mli) != ";") {
            list<string> amatch;
            BESUtil::explode(':', (*mli), amatch);
            if (amatch.size() != 2) {
                string s = (string) "Catalog type match malformed, " + "looking for type:regexp;[type:regexp;]";
                throw BESInternalError(s, __FILE__, __LINE__);
            }
            list<string>::iterator ami = amatch.begin();
            handler_regex newval;
            newval.handler = (*ami);
            ami++;
            newval.regex = (*ami);
            d_match_list.push_back(newval);
        }
    }

    key = (string) "BES.Catalog." + n + ".FollowSymLinks";
    string s_str;
    TheBESKeys::TheKeys()->get_value(key, s_str, found);
    s_str = BESUtil::lowercase(s_str);
    if (s_str == "yes" || s_str == "on" || s_str == "true") {
        d_follow_syms = true;
    }
}

/**
 * @brief Should this file/directory be included in the catalog?
 *
 * First check if the file should be included (matches at least one
 * regex on the include list). If there are no regexes on the include
 * list, that means include everything. Then test the exclude list.
 * If there are no regexes on the exclude list, exclude nothing.
 *
 * @param inQuestion File or directory in question
 * @return True if it should be included, false if not.
 */
bool BESCatalogUtils::include(const string &inQuestion) const
{
    bool toInclude = false;

    // First check the file against the include list. If the file should be
    // included then check the exclude list to see if there are exceptions
    // to the include list.
    if (d_include.size() == 0) {
        toInclude = true;
    }
    else {
        list<string>::const_iterator i_iter = d_include.begin();
        list<string>::const_iterator i_end = d_include.end();
        for (; i_iter != i_end; i_iter++) {
            string reg = *i_iter;
            if (!reg.empty()) {
                try {
                    // must match exactly, meaning result is = to length of string
                    // in question
                    BESRegex reg_expr(reg.c_str());
                    if (reg_expr.match(inQuestion.c_str(), inQuestion.size())
                        == static_cast<int>(inQuestion.size())) {
                        toInclude = true;
                    }
                }
                catch (BESError &e) {
                    string serr = (string) "Unable to get catalog information, "
                        + "malformed Catalog Include parameter " + "in bes configuration file around " + reg + ": "
                        + e.get_message();
                    throw BESInternalError(serr, __FILE__, __LINE__);
                }
            }
        }
    }

    if (toInclude == true) {
        if (exclude(inQuestion)) {
            toInclude = false;
        }
    }

    return toInclude;
}

/**
 * @brief Should this file/directory be excluded in the catalog?
 *
 * @see BESCatalogUtils::include
 * @param inQuestion The file or directory name in question
 * @return True if the file/directory should be excluded, false if not.
 */
bool BESCatalogUtils::exclude(const string &inQuestion) const
{
    list<string>::const_iterator e_iter = d_exclude.begin();
    list<string>::const_iterator e_end = d_exclude.end();
    for (; e_iter != e_end; e_iter++) {
        string reg = *e_iter;
        if (!reg.empty()) {
            try {
                BESRegex reg_expr(reg.c_str());
                if (reg_expr.match(inQuestion.c_str(), inQuestion.size()) == static_cast<int>(inQuestion.size())) {
                    return true;
                }
            }
            catch (BESError &e) {
                string serr = (string) "Unable to get catalog information, " + "malformed Catalog Exclude parameter "
                    + "in bes configuration file around " + reg + ": " + e.get_message();
                throw BESInternalError(serr, __FILE__, __LINE__);
            }
        }
    }
    return false;
}

/**
 * @note Look at using get_handler_name() or is_data() instead
 *
 * @return The beginning of the handler match regex list.
 */
BESCatalogUtils::match_citer BESCatalogUtils::match_list_begin() const
{
    return d_match_list.begin();
}

/**
 * @note Look at using get_handler_name() or is_data() instead
 *
 * @return The end of the handler match regex list.
 */
BESCatalogUtils::match_citer BESCatalogUtils::match_list_end() const
{
    return d_match_list.end();
}

/**
 *
 * @param dip
 * @param fullnode
 * @param use_node
 * @param
 * @param entry
 * @param dirs_only
 * @return
 */
unsigned int BESCatalogUtils::get_entries(DIR *dip, const string &fullnode, const string &use_node,
    BESCatalogEntry *entry, bool dirs_only)
{
    unsigned int cnt = 0;

    struct stat cbuf;
    int statret = stat(fullnode.c_str(), &cbuf);
    if (statret != 0) {
        if (errno == ENOENT) { // ENOENT means that the path or part of the path does not exist
            char *s_err = strerror(errno);
            throw BESNotFoundError((s_err) ? string(s_err) : string("Node ") + use_node + " does not exist", __FILE__,
                __LINE__);
        }
        // any other error means that access is denied for some reason
        else {
            char *s_err = strerror(errno);
            throw BESNotFoundError((s_err) ? string(s_err) : string("Access denied for node ") + use_node, __FILE__,
                __LINE__);
        }
    }

    struct dirent *dit;
    while ((dit = readdir(dip)) != NULL) {
        string dirEntry = dit->d_name;
        if (dirEntry == "." || dirEntry == "..") {
            continue;
        }

        string fullPath = fullnode + "/" + dirEntry;

        // Skip this dir entry if it is a sym link and follow links is false
         if (follow_sym_links() == false) {
            struct stat lbuf;
            (void) lstat(fullPath.c_str(), &lbuf);
            if (S_ISLNK(lbuf.st_mode))
                 continue;
         }

        // look at the mode and determine if this is a
        // directory or a regular file. If it is not
        // accessible, the stat fails, is not a directory
        // or regular file, then simply do not include it.
        struct stat buf;
        statret = stat(fullPath.c_str(), &buf);
        if (statret == 0 && S_ISDIR(buf.st_mode)) {
            if (exclude(dirEntry) == false) {
                BESCatalogEntry *curr_entry = new BESCatalogEntry(dirEntry, entry->get_catalog());

                bes_add_stat_info(curr_entry, buf);

                entry->add_entry(curr_entry);

                // we don't go further than this, so we need
                // to add a blank node here so that we know
                // it's a node (collection)
                BESCatalogEntry *blank_entry = new BESCatalogEntry(".blank", entry->get_catalog());
                curr_entry->add_entry(blank_entry);
            }
        }
        else if (statret == 0 && S_ISREG(buf.st_mode)) {
            if (!dirs_only && include(dirEntry)) {
                BESCatalogEntry *curr_entry = new BESCatalogEntry(dirEntry, entry->get_catalog());
                bes_add_stat_info(curr_entry, buf);

                list<string> services;
                // TODO use the d_utils object? jhrg 2.26.18
                isData(fullPath, d_name, services);
                curr_entry->set_service_list(services);

                bes_add_stat_info(curr_entry, buf);

                entry->add_entry(curr_entry);
            }
        }
    } // end of the while loop

    // TODO this always return zero. FIXME jhrg 2.26.18
    return cnt;
}

void BESCatalogUtils::display_entry(BESCatalogEntry *entry, BESInfo *info)
{
    string defcatname = BESCatalogList::TheCatalogList()->default_catalog_name();

    // start with the external entry
    map<string, string> props;
    if (entry->get_catalog() == defcatname) {
        props["name"] = entry->get_name();
    }
    else {
        string name = entry->get_catalog() + "/";
        if (entry->get_name() != "/") {
            name = name + entry->get_name();
        }
        props["name"] = name;
    }
    props["catalog"] = entry->get_catalog();
    props["size"] = entry->get_size();
    props["lastModified"] = entry->get_mod_date() + "T" + entry->get_mod_time();
    if (entry->is_collection()) {
        props["node"] = "true";
        ostringstream strm;
        strm << entry->get_count();
        props["count"] = strm.str();
    }
    else {
        props["node"] = "false";
    }
    info->begin_tag("dataset", &props);

    list<string> services = entry->get_service_list();
    if (services.size()) {
        list<string>::const_iterator si = services.begin();
        list<string>::const_iterator se = services.end();
        for (; si != se; si++) {
            info->add_tag("serviceRef", (*si));
        }
    }
}

/**
 * @brief Find the handler name that will process \arg item
 *
 * Using the TypeMatch regular expressions for the Catalog that
 * holds this instance of CatalogUtils, find a handler that can
 * process \arg item.
 *
 * @note Use `BESServiceRegistry::TheRegistry()->services_handled(...);`
 * to get a list of the services provided by the handler.
 *
 * @param item The item to be handled
 * @return The handler name. The empty string if the item cannot be
 * processed by any handler.
 */
std::string
BESCatalogUtils::get_handler_name(const std::string &item) const
{
    for (auto i = match_list_begin(), e = match_list_end(); i != e; ++i) {
        BESRegex expr((*i).regex.c_str());
        if (expr.match(item.c_str(), item.size()) == (int)item.size()) {
            return (*i).handler;
        }
    }

    return "";
}

/**
 * @brief is there a handler that can process this \arg item
 *
 * Using the TypeMatch regular expressions for the Catalog that
 * holds this instance of CatalogUtils, find a handler that can
 * process \arg item.
 *
 * @param item The item to be handled
 * @return The handler name. The empty string if the item cannot be
 * processed by any handler.
 * @see get_handler_name()
 */
bool
BESCatalogUtils::is_data(const std::string &item) const
{
    for (auto i = match_list_begin(), e = match_list_end(); i != e; ++i) {
        BESRegex expr((*i).regex.c_str());
        if (expr.match(item.c_str(), item.size()) == (int)item.size()) {
            return true;
        }
    }
    return false;
}

/**
 * Add info about a node to an BESCatalogEntry object. This method calls stat(2)
 * and passes the result to bes_add_stat_into().
 *
 * @param entry The BESCatalogEntry object to modify
 * @param fullnode
 */
void BESCatalogUtils::bes_add_stat_info(BESCatalogEntry *entry, const string &fullnode)
{
    struct stat cbuf;
    int statret = stat(fullnode.c_str(), &cbuf);
    if (statret == 0) {
        bes_add_stat_info(entry, cbuf);
    }
}

void BESCatalogUtils::bes_add_stat_info(BESCatalogEntry *entry, struct stat &buf)
{
    off_t sz = buf.st_size;
    entry->set_size(sz);

    // %T = %H:%M:%S
    // %F = %Y-%m-%d
    time_t mod = buf.st_mtime;
    struct tm stm{};
    gmtime_r(&mod, &stm);
    char mdate[64];
    strftime(mdate, 64, "%Y-%m-%d", &stm);
    char mtime[64];
    strftime(mtime, 64, "%T", &stm);

    ostringstream sdt;
    sdt << mdate;
    entry->set_mod_date(sdt.str());

    ostringstream stt;
    stt << mtime;
    entry->set_mod_time(stt.str());
}

bool BESCatalogUtils::isData(const string &inQuestion, const string &catalog, list<string> &services)
{
    BESContainerStorage *store = BESContainerStorageList::TheList()->find_persistence(catalog);
    if (!store) return false;

    return store->isData(inQuestion, services);
}

void BESCatalogUtils::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESCatalogUtils::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "root directory: " << d_root_dir << endl;

    if (d_include.size()) {
        strm << BESIndent::LMarg << "include list:" << endl;
        BESIndent::Indent();
        list<string>::const_iterator i_iter = d_include.begin();
        list<string>::const_iterator i_end = d_include.end();
        for (; i_iter != i_end; i_iter++) {
            if (!(*i_iter).empty()) {
                strm << BESIndent::LMarg << *i_iter << endl;
            }
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "include list: empty" << endl;
    }

    if (d_exclude.size()) {
        strm << BESIndent::LMarg << "exclude list:" << endl;
        BESIndent::Indent();
        list<string>::const_iterator e_iter = d_exclude.begin();
        list<string>::const_iterator e_end = d_exclude.end();
        for (; e_iter != e_end; e_iter++) {
            if (!(*e_iter).empty()) {
                strm << BESIndent::LMarg << *e_iter << endl;
            }
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "exclude list: empty" << endl;
    }

    if (d_match_list.size()) {
        strm << BESIndent::LMarg << "type matches:" << endl;
        BESIndent::Indent();
        BESCatalogUtils::match_citer i = d_match_list.begin();
        BESCatalogUtils::match_citer ie = d_match_list.end();
        for (; i != ie; i++) {
            handler_regex match = (*i);
            strm << BESIndent::LMarg << match.handler << " : " << match.regex << endl;
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "    type matches: empty" << endl;
    }

    if (d_follow_syms) {
        strm << BESIndent::LMarg << "    follow symbolic links: on" << endl;
    }
    else {
        strm << BESIndent::LMarg << "    follow symbolic links: off" << endl;
    }

    BESIndent::UnIndent();
}

#if 0
BESCatalogUtils *
BESCatalogUtils::Utils(const string &cat_name)
{
    BESCatalogUtils *utils = BESCatalogUtils::_instances[cat_name];
    if (!utils) {
        utils = new BESCatalogUtils(cat_name);
        BESCatalogUtils::_instances[cat_name] = utils;
    }
    return utils;
}
#endif


#if 0
// Added 12/24/12
void BESCatalogUtils::delete_all_catalogs()
{
    map<string, BESCatalogUtils*>::iterator i = BESCatalogUtils::_instances.begin();
    map<string, BESCatalogUtils*>::iterator e = BESCatalogUtils::_instances.end();
    while (i != e) {
        delete (*i++).second;
    }
}

#endif

