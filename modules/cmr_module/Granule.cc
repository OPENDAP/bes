// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cmr_module, A C++ MODULE that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2015 OPeNDAP, Inc.
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

/*
 * Granule.h
 *
 *  Created on: July, 13 2018
 *      Author: ndp
 */
#include "config.h"

#include <cstdlib>     /* atol */
#include <sstream>

#include "rjson_utils.h"
#include "BESDebug.h"

#include "CmrNames.h"
#include "CmrInternalError.h"
#include "CmrNotFoundError.h"
#include "Granule.h"


using namespace std;

#define prolog std::string("Granule::").append(__func__).append("() - ")


namespace cmr {


Granule::Granule(const nlohmann::json& granule_json)
{
    setId(granule_json);
    setName(granule_json);
    setSize(granule_json);
    setDataAccessUrl(granule_json);
    setMetadataAccessUrl(granule_json);
    setLastModifiedStr(granule_json);
}


void Granule::setName(const nlohmann::json& j_obj)
{
    this->d_name = j_obj[CMR_V2_TITLE_KEY].get<string>();
}


void Granule::setId(const nlohmann::json& j_obj)
{
    this->d_id = j_obj[CMR_GRANULE_ID_KEY].get<string>();
}


void Granule::setSize(const nlohmann::json& j_obj)
{
    this->d_size_str = j_obj[CMR_GRANULE_SIZE_KEY];
}


/**
  * Sets the last modified time of the granule as a string.
  * @param go
  */
void Granule::setLastModifiedStr(const nlohmann::json& go)
{
    this->d_last_modified_time = go[CMR_GRANULE_LMT_KEY].get<string>();
}


/**
 * Internal method that retrieves the "links" array from the Granule's object.
 */
const nlohmann::json& Granule::get_links_array(const nlohmann::json& go)
{
    auto &links = go[CMR_GRANULE_LINKS_KEY];
    if(links.is_null()){
        string msg = prolog + "ERROR: Failed to locate the value '"+CMR_GRANULE_LINKS_KEY+"' in object.";
        BESDEBUG(MODULE, prolog << msg << endl << go.get<string>());
        throw CmrNotFoundError(msg, __FILE__, __LINE__);
    }

    if(!links.is_array()){
        stringstream msg;
        msg << "ERROR: The '" << CMR_GRANULE_LINKS_KEY << "' object is NOT an array!";
        throw CmrInternalError(msg.str(), __FILE__, __LINE__);
    }
    return links;
}



/**
 * Sets the data access URL for the dataset granule.
 */
void Granule::setDataAccessUrl(const nlohmann::json& go)
{
    const auto& links = get_links_array(go);
    for(auto &link : links){
        string rel = link[CMR_GRANULE_LINKS_REL].get<string>();
        if(rel == CMR_GRANULE_LINKS_REL_DATA_ACCES){
            this->d_data_access_url = link[CMR_GRANULE_LINKS_HREF];
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate granule data access link (";
    msg << CMR_GRANULE_LINKS_REL_DATA_ACCES << "), :(";
    throw CmrInternalError(msg.str(), __FILE__, __LINE__);
}


/**
 * Sets the metadata access URL for the dataset granule.
 */
void Granule::setMetadataAccessUrl(const nlohmann::json& go)
{
    const auto &links = get_links_array(go);
    for(auto &link : links){
        string rel = link[CMR_GRANULE_LINKS_REL].get<string>();
        if(rel == CMR_GRANULE_LINKS_REL_METADATA_ACCESS){
            this->d_metadata_access_url = link[CMR_GRANULE_LINKS_HREF].get<string>();
            return;
        }
    }
    stringstream msg;
    msg << "ERROR: Failed to locate granule metadata access link (";
    msg << CMR_GRANULE_LINKS_REL_METADATA_ACCESS << "), :(";
    throw CmrInternalError(msg.str(), __FILE__, __LINE__);
}




bes::CatalogItem *Granule::getCatalogItem(BESCatalogUtils *d_catalog_utils){
    bes::CatalogItem *item = new bes::CatalogItem();
    item->set_type(bes::CatalogItem::leaf);
    item->set_name(getName());
    item->set_lmt(getLastModifiedStr());
    item->set_size(getSize());
    item->set_is_data(d_catalog_utils->is_data(item->get_name()));
    if(!getDataAccessUrl().empty()) {
        item->set_dap_data_access_url(getDataAccessUrl());
    }
    return item;
}



} //namespace cmr
