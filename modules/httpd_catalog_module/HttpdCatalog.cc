// HttpdCatalog.cc
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of BES httpd_catalog_module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cstring>
#include <cerrno>

#include <sstream>
#include <cassert>

#include <memory>
#include <algorithm>
#include <map>

#include <BESUtil.h>
#include <BESCatalogUtils.h>
#include <BESCatalogEntry.h>

#include <CatalogNode.h>
#include <CatalogItem.h>

#include <BESInfo.h>
#include <BESContainerStorageList.h>
#include <BESFileContainerStorage.h>
#include <BESLog.h>

#include <BESInternalError.h>
#include <BESForbiddenError.h>
#include <BESNotFoundError.h>
#include <BESSyntaxUserError.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#include "HttpdCatalogNames.h"
#include "HttpdCatalog.h"
#include "HttpdDirScraper.h"

using namespace bes;
using namespace std;

#define prolog std::string("HttpdCatalog::").append(__func__).append("() - ")

namespace httpd_catalog {

/**
 * @brief A catalog based on scraping the directory pages produced by httpd .
 *
 * See HttpdDirScraper.cc for more.
 *
 * HttpdCatalog is BESCatalog specialized for Apache httpd catalog pages.
 * *
 * @param name The name of the catalog.
 * @see BESCatalogUtils
 */
HttpdCatalog::HttpdCatalog(const string &catalog_name) :
		BESCatalog(catalog_name) {
	bool found = false;
	vector<string> httpd_catalogs;
	TheBESKeys::TheKeys()->get_values(HTTPD_CATALOG_COLLECTIONS, httpd_catalogs,
			found);
	if (!found) {
		throw BESInternalError(
				string("The httpd_catalog module must define at least one catalog name using the key; '")
						+ HTTPD_CATALOG_COLLECTIONS + "'", __FILE__, __LINE__);
	}

	vector<string>::iterator it;
	for (it = httpd_catalogs.begin(); it != httpd_catalogs.end(); it++) {
		string catalog_entry = *it;
		int index = catalog_entry.find(":");
		if (index > 0) {
			string name = catalog_entry.substr(0, index);
			string url = catalog_entry.substr(index + 1);
			BESDEBUG(MODULE,
					prolog << "name: '" << name << "'  url: " << url << endl);
			d_httpd_catalogs.insert(pair<string, string>(name, url));
		} else {
			throw BESInternalError(
					string("The configuration entry for the ")
							+ HTTPD_CATALOG_COLLECTIONS
							+ " was incorrectly formatted. entry: "
							+ catalog_entry, __FILE__, __LINE__);
		}
	}

#if 0
	string default_type_match_key = "BES.Catalog.catalog.TypeMatch";
	string catalog_type_match_key = "BES.Catalog."+catalog_name+".TypeMatch";
	string type_match_key = catalog_type_match_key;
	vector<string> type_match_v;
	TheBESKeys::TheKeys()->get_values(type_match_key, type_match_v, found);
	if(!found) {
		type_match_key = default_type_match_key;
		TheBESKeys::TheKeys()->get_values(type_match_key, type_match_v, found);
		if(!found) {
			throw BESInternalError("ERROR: Failed to located either the '"+catalog_type_match_key+
					"' or the '"+default_type_match_key+"' BES keys." , __FILE__, __LINE__);
		}
	}

	for(it=type_match_v.begin(); it!=type_match_v.end(); it++) {
		string typeMatch_entry = *it;
		int index = typeMatch_entry.find(":");
		if(index>0) {
			string name = typeMatch_entry.substr(0,index);
			string regex = typeMatch_entry.substr(index+1);
			BESDEBUG(MODULE, prolog << "name: '" << name << "'  regex: " << regex << endl);
			d_typematch.insert( pair<string,string>(name,regex));
		}
		else {
			throw BESInternalError(string("The configuration entry for the ") + type_match_key +
					" was incorrectly formatted. entry: "+typeMatch_entry, __FILE__,__LINE__);
		}
	}
#endif

}

/**
 * @brief Produces the bes::CatalogNode for the string ppath.
 *
 * This includes dealing with the named Httpd_Catalog.Collections
 * part of the path and providing catalog page with the list of Collections
 * for the empty path.
 *
 */
bes::CatalogNode *
HttpdCatalog::get_node(const string &ppath) const
{
    string time_now = BESUtil::get_time(0,false);
    bes::CatalogNode *node;

    string path = ppath;
    while(path.length()>0 && path[0]=='/')
        path = path.substr(1);

    if(path.empty() || path=="/"){
        node = new CatalogNode("/");
        node->set_lmt(time_now);
        node->set_catalog_name(HTTPD_CATALOG_NAME);
        map<string, string>::const_iterator  it  = d_httpd_catalogs.begin();

        while(it!=d_httpd_catalogs.end()){
            CatalogItem *collection = new CatalogItem();
            collection->set_name(it->first);
            collection->set_type(CatalogItem::node);
            node->add_node(collection);
            it++;
        }
    }
    else {
#if 0
        vector<string> path_elements = BESUtil::split(path);
        BESDEBUG(MODULE, prolog << "path_elements.size(): " << path_elements.size()  << " path: '" << path << "'"<< endl);

        string collection = path_elements[0];
        BESDEBUG(MODULE, prolog << "Checking for collection: " << collection << " d_httpd_catalogs.size(): "
            << d_httpd_catalogs.size() << endl);

        map<string,string>::const_iterator it = d_httpd_catalogs.find(collection);
        if(it == d_httpd_catalogs.end()){
            throw BESNotFoundError("The httpd_catalog does not contain a collection named '"+collection+"'",__FILE__,__LINE__);
        }
        BESDEBUG(MODULE, prolog << "The httpd_catalog collection " << collection << " is valid." << endl);

        string url = it->second;
        string remote_relative_path = path.substr(collection.length());
        BESDEBUG(MODULE, prolog << "remote_relative_path: " << remote_relative_path << endl);

        string remote_target_url;
        if(remote_relative_path == "/" || remote_relative_path.empty())
            remote_target_url = url;
        else
            remote_target_url = BESUtil::pathConcat(url,remote_relative_path);

        BESDEBUG(MODULE, prolog << "remote_target_url: " << remote_target_url << endl);
#endif

        string remote_target_url = path_to_access_url(path);

        HttpdDirScraper hds;
        node = hds.get_node(remote_target_url,path);
        node->set_lmt(time_now);
        node->set_catalog_name(HTTPD_CATALOG_NAME);

    }
    return node;
}

/**
 * Takes a path which begins with the name of an HttpdCatalog collection and returns the associated
 * access url for the referenced thingy.
 *
 * The \arg path is a slash separated series of tokens. The first token
 * names the httpd_catalog collection, the following tokens are assumed to be a path
 * on the remote resource and are treated as a single unit. If there is no slash in
 * the path (i.e., it consists of a single token with no separator), then the collection
 * name is that token and the remainder is '/'.
 *
 * @param path The path to convert.
 * @returns the access URL for the path on the named remote resource.
 * @throws BESNotFoundError when there is no matching collection found.
 */
string HttpdCatalog::path_to_access_url(string path) const
{
    map<string,string>::const_iterator it;

    // the special case where there is no separator an 'path' is the name of the collection
    if (path.find('/') == string::npos) {
        it = d_httpd_catalogs.find(path);
        if (it == d_httpd_catalogs.end())
            throw BESNotFoundError("The httpd_catalog does not contain a collection named '" + path + "'", __FILE__, __LINE__);
        return it->second;  // it->second is the URL bound to the Collection in the bes.conf file
    }

    // path contains a '/'
    string::size_type i = path.find('/');
    string collection = path.substr(0, i-1);

    it = d_httpd_catalogs.find(collection);
    if (it == d_httpd_catalogs.end())
        throw BESNotFoundError("The httpd_catalog does not contain a collection named '" + collection + "'", __FILE__, __LINE__);

    string remote_relative_path = path.substr(i);

    if(remote_relative_path == "/" || remote_relative_path.empty())
        return it->second;
    else
        return BESUtil::pathConcat(it->second, remote_relative_path);

#if 0
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path_elements.size(): " << path_elements.size() << " path: '" << path << "'"<< endl);

    string collection = path_elements[0];
    BESDEBUG(MODULE, prolog << "Checking for collection: " << collection << " d_httpd_catalogs.size(): "
        << d_httpd_catalogs.size() << endl);

    map<string,string>::const_iterator it = d_httpd_catalogs.find(collection);
    if(it == d_httpd_catalogs.end()) {
        throw BESNotFoundError("The httpd_catalog does not contain a collection named '"+collection+"'",__FILE__,__LINE__);
    }
    BESDEBUG(MODULE, prolog << "The httpd_catalog collection " << collection << " is valid." << endl);

    string url = it->second;

    string remote_relative_path = path.substr(collection.length()+1);
    BESDEBUG(MODULE, prolog << "remote_relative_path: " << remote_relative_path << endl);

    string access_url;
    if(remote_relative_path == "/" || remote_relative_path.empty())
    access_url = url;
    else
    access_url = BESUtil::pathConcat(url,remote_relative_path);

    BESDEBUG(MODULE, prolog << "remote_target_url: " << access_url << endl);

    return access_url;
#endif

}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void HttpdCatalog::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog << "(" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "catalog utilities: " << endl;
    BESIndent::Indent();
    get_catalog_utils()->dump(strm);
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}

} // namespace httpd_catalog
