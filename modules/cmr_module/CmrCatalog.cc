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


#include "BESInfo.h"
#include "BESContainerStorageList.h"
#include "BESFileContainerStorage.h"
#include "BESLog.h"

#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESSyntaxUserError.h"

#include "TheBESKeys.h"
#include "BESDebug.h"

#include "CatalogNode.h"
#include "CatalogItem.h"

#include "CmrApi.h"
#include "CmrNames.h"
#include "CmrCatalog.h"

using namespace bes;
using namespace std;

#define prolog std::string("CmrCatalog::").append(__func__).append("() - ")

namespace cmr {

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
CmrCatalog::CmrCatalog(const std::string &name /* = “CMR” */) : BESCatalog(name) {
    bool found = false;
    TheBESKeys::TheKeys()->get_values(CMR_COLLECTIONS_KEY, d_collections, found);
    if(!found){
        throw BESInternalError(string("The CMR module must define at least one collection name using the key; '") + CMR_COLLECTIONS_KEY
                               + "'", __FILE__, __LINE__);
    }

    found = false;
    TheBESKeys::TheKeys()->get_values(CMR_FACETS_KEY, d_facets, found);
    if(!found){
        throw BESInternalError(string("The CMR module must define at least one facet name using the key; '") + CMR_COLLECTIONS_KEY
                               + "'", __FILE__, __LINE__);
    }
}

bes::CatalogNode * CmrCatalog::get_providers_node() const
{
    CmrApi cmrApi;
    bes::CatalogNode *node;
    string epoch_time = BESUtil::get_time(0,false);

    node = new CatalogNode("/");
    node->set_lmt(epoch_time);
    node->set_catalog_name(CMR_CATALOG_NAME);
    map<string, unique_ptr<Provider>> providers;
    cmrApi.get_opendap_providers(providers);
    for (const auto &provider : providers ) {
        auto *collection = new CatalogItem();
        collection->set_name(provider.second->id());
        collection->set_description(provider.second->description_of_holding());
        collection->set_type(CatalogItem::node);
        node->add_node(collection);
    }
    return node;
}

bes::CatalogNode *CmrCatalog::get_collections_node(const string &path, const string &provider_id) const
{
    CmrApi cmrApi;
    string epoch_time = BESUtil::get_time(0,false);


    map<string, unique_ptr<Collection>> collections;
    cmrApi.get_opendap_collections(provider_id, collections);
    if(collections.empty()){
        stringstream msg;
        msg << "The provider " << provider_id << " does contain any OPeNDAP enabled collections.";
        throw BESNotFoundError(msg.str(),__FILE__,__LINE__);
    }

    auto *catalog_node = new CatalogNode(path);
    catalog_node->set_lmt(epoch_time);
    catalog_node->set_catalog_name(CMR_CATALOG_NAME);
    for (const auto &collection : collections ) {
        auto *catalog_item = new CatalogItem();
        catalog_item->set_name(collection.second->id());
        catalog_item->set_description(collection.second->abstract());
        catalog_item->set_type(CatalogItem::node);
        catalog_node->add_node(catalog_item);
    }
    return catalog_node;
}

bes::CatalogNode *
CmrCatalog::get_facets_node(const std::string &path, const std::string &collection_id) const {
    BESDEBUG(MODULE, prolog << "Building facet list for collection: " << collection_id << endl);
    string epoch_time = BESUtil::get_time(0,false);
    auto node = new CatalogNode(path);
    node->set_lmt(epoch_time);
    node->set_catalog_name(CMR_CATALOG_NAME);
    for(const auto & d_facet : d_facets){
        auto *catalogItem = new CatalogItem();
        catalogItem->set_name(d_facet);
        catalogItem->set_type(CatalogItem::node);
        catalogItem->set_lmt(epoch_time);
        BESDEBUG(MODULE, prolog << "Adding facet: " << d_facet << endl);
        node->add_node(catalogItem);
    }
    return node;
}

bes::CatalogNode *
CmrCatalog::get_temporal_facet_nodes(const string &path, const vector<string> &path_elements, const string &collection_id) const
{
    BESDEBUG(MODULE, prolog << "Found Temporal Facet"<< endl);
    CmrApi cmrApi;
    string epoch_time = BESUtil::get_time(0,false);
    auto node = new CatalogNode(path);
    node->set_lmt(epoch_time);
    node->set_catalog_name(CMR_CATALOG_NAME);


    switch( path_elements.size()){

        case 0: // The path ends at temporal facet, so we need the year nodes.
        {
            vector<string> years;

            BESDEBUG(MODULE, prolog << "Getting year nodes for collection: " << collection_id<< endl);
            cmrApi.get_years(collection_id, years);
            for(const auto & year : years){
                auto *catalogItem = new CatalogItem();
                catalogItem->set_type(CatalogItem::node);
                catalogItem->set_name(year);
                catalogItem->set_is_data(false);
                catalogItem->set_lmt(epoch_time);
                catalogItem->set_size(0);
                node->add_node(catalogItem);
            }
        }
            break;

        case 1:  // The path ends at years facet, so we need the month nodes.
        {
            const string &year = path_elements[0];
            string day;
            vector<string> months;

            BESDEBUG(MODULE, prolog << "Getting month nodes for collection: " << collection_id << " year: " << year << endl);
            cmrApi.get_months(collection_id, year, months);
            for(const auto & month : months){
                auto *catalogItem = new CatalogItem();
                catalogItem->set_type(CatalogItem::node);
                catalogItem->set_name(month);
                catalogItem->set_is_data(false);
                catalogItem->set_lmt(epoch_time);
                catalogItem->set_size(0);
                node->add_node(catalogItem);
            }
        }
            break;

        case 2:  // The path ends at months facet, so we need the day nodes.
        {
            const string &year = path_elements[0];
            const string &month = path_elements[1];
            vector<string> days;

            BESDEBUG(MODULE, prolog << "Getting day nodes for collection: " << collection_id << " year: " << year << " month: " << month << endl);
            cmrApi.get_days(collection_id, year, month, days);
            for(const auto &day : days){
                auto *catalogItem = new CatalogItem();
                catalogItem->set_type(CatalogItem::node);
                catalogItem->set_name(day);
                catalogItem->set_is_data(false);
                catalogItem->set_lmt(epoch_time);
                catalogItem->set_size(0);
                node->add_node(catalogItem);
            }
        }
            break;

        case 3:  // The path ends at the days facet, so we need the granule nodes.
        {
            const string &year = path_elements[0];
            const string &month = path_elements[1];
            const string &day = path_elements[2];
            BESDEBUG(MODULE, prolog << "Getting granule leaves for collection: " << collection_id << " year: " << year << " month: " << month <<  " day: " << day << endl);
            vector<unique_ptr<GranuleUMM>> granules;
            cmrApi.get_granules_umm(collection_id, year, month, day, granules);
            for(const auto &granule : granules){
                node->add_leaf(granule->getCatalogItem(get_catalog_utils()));
            }
        }
            break;

        case 4: // Looks like they are trying to get a particular granule...
        {
            // http://localhost:8080/opendap/CMR/EEDTEST/C1245618475-EEDTEST/temporal/2020/01/05/GPM_3IMERGHH.06%3A3B-HHR.MS.MRG.3IMERG.20200105-S000000-E002959.0000.V06B.HDF5.dmr.html
            // provider_id: EEDTEST
            // collection_conept_id: C1245618475-EEDTEST
            // temporal/year/month/day
            // granule??: GPM_3IMERGHH.06%3A3B-HHR.MS.MRG.3IMERG.20200105-S000000-E002959.0000.V06B.HDF5
            const string &year = path_elements[0];
            const string &month = path_elements[1];
            const string &day = path_elements[2];
            const string &granule_id = path_elements[3];
            BESDEBUG(MODULE, prolog << "Request resolved to leaf granule/dataset name,  collection: " << collection_id << " year: " << year
                                    << " month: " << month <<  " day: " << day << " granule: " << granule_id << endl);
            auto granule = cmrApi.get_granule(collection_id,year,month,day,granule_id);
            if(granule){
                auto *granuleItem = new CatalogItem();
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
            throw BESSyntaxUserError("CmrCatalog: The path '"+path+"' does not describe a valid temporal facet search.",__FILE__,__LINE__);
        }
    }

    return node;
}

/**
 * path_elements.size()==0  path: / (providers node - providers with OPeNDAP serviced collections)
 * path_elements.size()==1  path: /provider_id/ (collections node - OPeNDAP serviced collections for provider_name)
 * path_elements.size()==2  path: /provider_id/collection_concept_id/ (facets node)
 * path_elements.size()==3  path: /provider_id/collection_concept_id/temporal/ (years node)
 * path_elements.size()==4  path: /provider_id/collection_concept_id/years/ (months node)
 * path_elements.size()==5  path: /provider_id/collection_concept_id/years/months/ (days node)
 * path_elements.size()==6  path: /provider_id/collection_concept_id/years/months/days/ (granules node)
 * path_elements.size()==7  path: /provider_id/collection_concept_id/years/months/days/granule_concept_id (IFH)
 *
 * @param ppath
 * @return
 */
bes::CatalogNode *
CmrCatalog::get_node(const string &ppath) const
{
    const string path = BESUtil::normalize_path(ppath,true, false);
    vector<string> path_elements = BESUtil::split(path);
    BESDEBUG(MODULE, prolog << "path: '" << path << "'   path_elements.size(): " << path_elements.size() << endl);

    BESUtil::get_time(0,false);

    // Not sure why this is being "cleaned" but it must have been a thing - ndp 11/9/22
    for (auto & path_element : path_elements) {
        if (path_element == "-")
            path_element = "";
    }

    string provider_id;

    switch(path_elements.size()){
        case 0: {
            // path_elements.size()==0  path: / (providers node - providers with OPeNDAP serviced collections)
            return get_providers_node();
        }
        case 1: {
            // path_elements.size()==1  path: /provider_id/ (collections node - OPeNDAP serviced collections for provider_name)
            provider_id = path_elements[0];
            return get_collections_node(path, provider_id);
        }
        default:
            break;
    }

    // If we are here, we know the path_elements vector is not empty and that it has MORE than
    // three members. So we set provider_id and the collection_id to the first two values.
    provider_id = path_elements[0];
    path_elements.erase(path_elements.begin());

    string collection_id = path_elements[0];
    path_elements.erase(path_elements.begin());

    return get_temporal_facet_nodes(path, path_elements,collection_id);
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * this catalog directory.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void CmrCatalog::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << prolog << "(" << (void *) this << ")" << endl;
    BESIndent::Indent();

    strm << BESIndent::LMarg << "catalog utilities: " << endl;
    BESIndent::Indent();
    get_catalog_utils()->dump(strm);
    BESIndent::UnIndent();
    BESIndent::UnIndent();
}

} // namespace cmr
