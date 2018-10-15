// -*- mode: c++; c-basic-offset:4 -*-
//
// CMRCatalog.cc
//
// This file is part of BES cmr_module
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

#include "BESUtil.h"
#include "BESCatalogUtils.h"
#include "BESCatalogEntry.h"

#include "CatalogNode.h"
#include "CatalogItem.h"

#include "BESInfo.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorageCatalog.h"
#include "BESLog.h"

#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESSyntaxUserError.h"

#include "TheBESKeys.h"
#include "BESDebug.h"

#include "HttpCatalogNames.h"
#include "HttpCatalog.h"

using namespace bes;
using namespace std;

#define prolog std::string("HttpCatalog::").append(__func__).append("() - ")

namespace http_catalog {

/**
 * @brief A catalog based on NASA's CMR system
 *
 * CMRCatalog is BESCatalog specialized for  NASA's CMR system.
 *
 * @note Access to the host's file system is made using BESCatalogUtils,
 * which is initialized using the catalog name.
 *
 * @param name The name of the catalog.
 * @see BESCatalogUtils
 */
HttpCatalog::HttpCatalog(const std::string &name /* = “CMR” */) : BESCatalog(name) {
    bool found = false;
    TheBESKeys::TheKeys()->get_values(CMR_COLLECTIONS, d_collections, found);
    if(!found){
        throw BESInternalError(string("The CMR module must define at least one collection name using the key; '")+CMR_COLLECTIONS
            +"'", __FILE__, __LINE__);
    }

    found = false;
    TheBESKeys::TheKeys()->get_values(CMR_FACETS, d_facets, found);
    if(!found){
        throw BESInternalError(string("The CMR module must define at least one facet name using the key; '")+CMR_COLLECTIONS
            +"'", __FILE__, __LINE__);
    }
}

HttpCatalog::~HttpCatalog()
{
}
bes::CatalogNode *
HttpCatalog::get_node(const string &path) const
{
    return get_node_NEW(path);
}


bes::CatalogNode *
HttpCatalog::get_node_NEW(const string &ppath) const
{
    string path = BESUtil::normalize_path(ppath,true, false);
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'   path_elements.size(): " << path_elements.size() << endl);

    string epoch_time = BESUtil::get_time(0,false);

    HttpCatalogApi cmrApi;
    bes::CatalogNode *node;

    if(path_elements.empty()){
        node = new CatalogNode("/");
        node->set_lmt(epoch_time);
        node->set_catalog_name(CMR_CATALOG_NAME);
        for(size_t i=0; i<d_collections.size() ; i++){
            CatalogItem *collection = new CatalogItem();
            collection->set_name(d_collections[i]);
            collection->set_type(CatalogItem::node);
            node->add_node(collection);
        }
    }
    else {
        for(size_t i=0; i< path_elements.size() ;i++){
            if(path_elements[i]=="-")
                path_elements[i] = "";
        }

        string collection = path_elements[0];
        BESDEBUG(MODULE, prolog << "Checking for collection: " << collection << " d_collections.size(): " << d_collections.size() << endl);
        bool valid_collection = false;
        for(size_t i=0; i<d_collections.size() && !valid_collection ; i++){
            if(collection == d_collections[i])
                valid_collection = true;
        }
        if(!valid_collection){
            throw BESNotFoundError("The CMR catalog does not contain a collection named '"+collection+"'",__FILE__,__LINE__);
        }
        BESDEBUG(MODULE, prolog << "Collection " << collection << " is valid." << endl);
        if(path_elements.size() >1){
            string facet = path_elements[1];
            bool valid_facet = false;
            for(size_t i=0; i<d_facets.size() && !valid_facet ; i++){
                if(facet == d_facets[i])
                    valid_facet = true;
            }
            if(!valid_facet){
                throw BESNotFoundError("The CMR collection '"+collection+"' does not contain a facet named '"+facet+"'",__FILE__,__LINE__);
            }

            if(facet=="temporal"){
                BESDEBUG(MODULE, prolog << "Found Temporal Facet"<< endl);
                node = new CatalogNode(path);
                node->set_lmt(epoch_time);
                node->set_catalog_name(CMR_CATALOG_NAME);


                switch( path_elements.size()){

                case 2: // The path ends at temporal facet, so we need the years.
                {
                    vector<string> years;

                    BESDEBUG(MODULE, prolog << "Getting year nodes for collection: " << collection<< endl);
                    cmrApi.get_years(collection, years);
                    for(size_t i=0; i<years.size() ; i++){
                        CatalogItem *collection = new CatalogItem();
                        collection->set_type(CatalogItem::node);
                        collection->set_name(years[i]);
                        collection->set_is_data(false);
                        collection->set_lmt(epoch_time);
                        collection->set_size(0);
                        node->add_node(collection);
                    }
                }
                break;

                case 3:
                {
                    string year = path_elements[2];
                    string month("");
                    string day("");
                    vector<string> months;

                    BESDEBUG(MODULE, prolog << "Getting month nodes for collection: " << collection << " year: " << year << endl);
                    cmrApi.get_months(collection, year, months);
                    for(size_t i=0; i<months.size() ; i++){
                        CatalogItem *collection = new CatalogItem();
                        collection->set_type(CatalogItem::node);
                        collection->set_name(months[i]);
                        collection->set_is_data(false);
                        collection->set_lmt(epoch_time);
                        collection->set_size(0);
                        node->add_node(collection);
                    }
                }
                break;

                case 4:
                {
                    string year = path_elements[2];
                    string month = path_elements[3];
                    string day("");
                    vector<string> days;

                    BESDEBUG(MODULE, prolog << "Getting day nodes for collection: " << collection << " year: " << year << " month: " << month << endl);
                    cmrApi.get_days(collection, year, month, days);
                    for(size_t i=0; i<days.size() ; i++){
                        CatalogItem *collection = new CatalogItem();
                        collection->set_type(CatalogItem::node);
                        collection->set_name(days[i]);
                        collection->set_is_data(false);
                        collection->set_lmt(epoch_time);
                        collection->set_size(0);
                        node->add_node(collection);
                    }
                }
                break;

                case 5:
                {
                    string year = path_elements[2];
                    string month = path_elements[3];
                    string day = path_elements[4];
                    BESDEBUG(MODULE, prolog << "Getting granule leaves for collection: " << collection << " year: " << year << " month: " << month <<  " day: " << day << endl);
                    vector<Granule *> granules;
                    cmrApi.get_granules(collection, year, month, day, granules);
                    for(size_t i=0; i<granules.size() ; i++){
                        node->add_leaf(granules[i]->getCatalogItem(get_catalog_utils()));
                    }
                }
                break;

                case 6:
                {
                    string year = path_elements[2];
                    string month = path_elements[3];
                    string day = path_elements[4];
                    string granule_id = path_elements[5];
                    BESDEBUG(MODULE, prolog << "Request resolved to leaf granule/dataset name,  collection: " << collection << " year: " << year
                        << " month: " << month <<  " day: " << day << " granule: " << granule_id << endl);
                    Granule *granule = cmrApi.get_granule(collection,year,month,day,granule_id);
                    if(granule){
                        CatalogItem *granuleItem = new CatalogItem();
                        granuleItem->set_type(CatalogItem::leaf);
                        granuleItem->set_name(granule->getName());
                        granuleItem->set_is_data(true);
                        granuleItem->set_lmt(granule->getLastModifiedStr());
                        granuleItem->set_size(granule->getSize());
                        node->set_leaf(granuleItem);
                    }
                    else {
                        throw BESNotFoundError("No such resource: "+path,__FILE__,__LINE__);
                    }
                }
                break;

                default:
                {
                    throw BESSyntaxUserError("HttpCatalog: The path '"+path+"' does not describe a valid temporal facet search.",__FILE__,__LINE__);
                }
                break;
                }

            }
            else {
                throw BESNotFoundError("The CMR catalog only supports temporal faceting.",__FILE__,__LINE__);
            }
        }
        else {
            BESDEBUG(MODULE, prolog << "Building facet list for collection: " << collection << endl);
            node = new CatalogNode(path);
            node->set_lmt(epoch_time);
            node->set_catalog_name(CMR_CATALOG_NAME);
            for(size_t i=0; i<d_facets.size() ; i++){
                CatalogItem *collection = new CatalogItem();
                collection->set_name(d_facets[i]);
                collection->set_type(CatalogItem::node);
                collection->set_lmt(epoch_time);
                BESDEBUG(MODULE, prolog << "Adding facet: " << d_facets[i] << endl);
                node->add_node(collection);
            }
        }
    }
    return node;
}


// path must start with a '/'. By this class it will be interpreted as a
// starting at the CatalogDirectory instance's root directory. It may either
// end in a '/' or not.
//
// If it is not a directory - that is an error. (return null or throw?)
//
// Item names are relative
/**
 * @brief Get a CatalogNode for the given path in the current catalog
 *
 * This is similar to show_catalog() but returns a simpler response. The
 * \arg path must start with a slash and is used as a suffix to the Catalog's
 * root directory.
 *
 * @param path The pathname for the node; must start with a slash
 * @return A CatalogNode instance or null if there is no such path in the
 * current catalog.
 * @throw BESInternalError If the \arg path is not a directory
 * @throw BESForbiddenError If the \arg path is explicitly excluded by the
 * bes.conf file
 */
bes::CatalogNode *
HttpCatalog::get_node_OLD(const string &ppath) const
{
    string path = BESUtil::normalize_path(ppath,true, false);
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'   path_elements.size(): " << path_elements.size() << endl);

    string epoch_time = BESUtil::get_time(0,false);

    HttpCatalogApi cmrApi;
    bes::CatalogNode *node;

    if(path_elements.empty()){
        node = new CatalogNode("/");
        node->set_lmt(epoch_time);
        node->set_catalog_name(CMR_CATALOG_NAME);
        for(size_t i=0; i<d_collections.size() ; i++){
            CatalogItem *collection = new CatalogItem();
            collection->set_name(d_collections[i]);
            collection->set_type(CatalogItem::node);
            node->add_node(collection);
        }
    }
    else {
        string collection = path_elements[0];
        BESDEBUG(MODULE, prolog << "Checking for collection: " << collection << " d_collections.size(): " << d_collections.size() << endl);
        bool valid_collection = false;
        for(size_t i=0; i<d_collections.size() && !valid_collection ; i++){
            if(collection == d_collections[i])
                valid_collection = true;
        }
        if(!valid_collection){
            throw BESNotFoundError("The CMR catalog does not contain a collection named '"+collection+"'",__FILE__,__LINE__);
        }
        BESDEBUG(MODULE, prolog << "Collection " << collection << " is valid." << endl);
        if(path_elements.size() >1){
            string facet = path_elements[1];
            bool valid_facet = false;
            for(size_t i=0; i<d_facets.size() && !valid_facet ; i++){
                if(facet == d_facets[i])
                    valid_facet = true;
            }
            if(!valid_facet){
                throw BESNotFoundError("The CMR collection '"+collection+"' does not contain a facet named '"+facet+"'",__FILE__,__LINE__);
            }

            if(facet=="temporal"){
                BESDEBUG(MODULE, prolog << "Found Temporal Facet"<< endl);
                node = new CatalogNode(path);
                node->set_lmt(epoch_time);
                node->set_catalog_name(CMR_CATALOG_NAME);


                switch( path_elements.size()){
                case 2: // The path ends at temporal facet, so we need the years.
                {
                    vector<string> years;

                    BESDEBUG(MODULE, prolog << "Getting year nodes for collection: " << collection<< endl);
                    cmrApi.get_years(collection, years);
                    for(size_t i=0; i<years.size() ; i++){
                        CatalogItem *collection = new CatalogItem();
                        collection->set_type(CatalogItem::node);
                        collection->set_name(years[i]);
                        collection->set_is_data(false);
                        collection->set_lmt(epoch_time);
                        collection->set_size(0);
                        node->add_node(collection);
                    }
                }
                    break;
                case 3:
                {
                    string year = path_elements[2];
                    string month("");
                    string day("");
                    vector<string> months;

                    BESDEBUG(MODULE, prolog << "Getting month nodes for collection: " << collection << " year: " << year << endl);
                    cmrApi.get_months(collection, year, months);
                    for(size_t i=0; i<months.size() ; i++){
                        CatalogItem *collection = new CatalogItem();
                        collection->set_type(CatalogItem::node);
                        collection->set_name(months[i]);
                        collection->set_is_data(false);
                        collection->set_lmt(epoch_time);
                        collection->set_size(0);
                        node->add_node(collection);
                    }
                }
                    break;
                case 4:
                {
                    string year = path_elements[2];
                    string month = path_elements[3];
                    string day("");
                    vector<string> days;

                    BESDEBUG(MODULE, prolog << "Getting day nodes for collection: " << collection << " year: " << year << " month: " << month << endl);
                    cmrApi.get_days(collection, year, month, days);
                    for(size_t i=0; i<days.size() ; i++){
                        CatalogItem *collection = new CatalogItem();
                        collection->set_type(CatalogItem::node);
                        collection->set_name(days[i]);
                        collection->set_is_data(false);
                        collection->set_lmt(epoch_time);
                        collection->set_size(0);
                        node->add_node(collection);
                    }
                }
                    break;
                case 5:
                {
                    string year = path_elements[2];
                    string month = path_elements[3];
                    string day = path_elements[4];
                    BESDEBUG(MODULE, prolog << "Getting granule leaves for collection: " << collection << " year: " << year << " month: " << month <<  " day: " << day << endl);
                    vector<Granule *> granules;
                    cmrApi.get_granules(collection, year, month, day, granules);
                    for(size_t i=0; i<granules.size() ; i++){
                        node->add_leaf(granules[i]->getCatalogItem(get_catalog_utils()));
                    }
                }
                    break;
                default:
                    throw BESSyntaxUserError("HttpCatalog: The path '"+path+"' does not describe a valid temporal facet search.",__FILE__,__LINE__);
                    break;
                }
            }
            else {
                throw BESNotFoundError("The CMR catalog only supports temporal faceting.",__FILE__,__LINE__);
            }
        }
        else {
            BESDEBUG(MODULE, prolog << "Building facet list for collection: " << collection << endl);
            node = new CatalogNode(path);
            node->set_lmt(epoch_time);
            node->set_catalog_name(CMR_CATALOG_NAME);
            for(size_t i=0; i<d_facets.size() ; i++){
                CatalogItem *collection = new CatalogItem();
                collection->set_name(d_facets[i]);
                collection->set_type(CatalogItem::node);
                collection->set_lmt(epoch_time);
                BESDEBUG(MODULE, prolog << "Adding facet: " << d_facets[i] << endl);
                node->add_node(collection);
            }
        }
    }
    return node;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void HttpCatalog::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog << "(" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "catalog utilities: " << endl;
    BESIndent::Indent();
    get_catalog_utils()->dump(strm);
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}

} // namespace http_catalog
