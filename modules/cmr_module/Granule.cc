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
#include <stdlib.h>     /* atol */

#include "rjson_utils.h"
#include "BESDebug.h"

#include "CmrNames.h"
#include "CmrError.h"
#include "Granule.h"


using namespace std;

#define prolog std::string("Granule::").append(__func__).append("() - ")


namespace cmr {

string granule_LINKS_REL_DATA_ACCES = "http://esipfed.org/ns/fedsearch/1.1/data#";
string granule_LINKS_REL_METADATA_ACCESS = "http://esipfed.org/ns/fedsearch/1.1/data#";
string granule_LINKS = "links";
string granule_LINKS_REL= "rel";
string granule_LINKS_HREFLANG = "hreflang";
string granule_LINKS_HREF = "href";
string granule_SIZE = "granule_size";
string granule_LMT = "updated";


std::string Granule::getStringProperty(const std::string &name){
    rjson_utils rju;
    return rju.getStringValue(d_granule_obj,name);
}


/**
 * Returns th size of the Granule as a string.
 */
std::string Granule::getSizeStr(){
    return getStringProperty(granule_SIZE);
}

size_t Granule::getSize(){
    return atol(getSizeStr().c_str());
}


/**
 * Returns the last modified time of the granule as a string.
 */
std::string Granule::getLastModifiedStr(){
    return getStringProperty(granule_LMT);
}

/**
 * Internal method that retrieves the "links" array from the Granule's object.
 */
const rapidjson::Value& Granule::get_links_array(){

    rapidjson::Value::ConstMemberIterator itr = d_granule_obj.FindMember(granule_LINKS.c_str());
    bool result = itr != d_granule_obj.MemberEnd();
    string msg = prolog + (result?"Located":"FAILED to locate") + " the value '"+granule_LINKS+"' in object.";
    BESDEBUG(MODULE, msg << endl);
    if(!result){
        throw CmrError("ERROR: Failed to located '"+granule_LINKS+"' section for CMRGranule!",__FILE__,__LINE__);
    }
    const rapidjson::Value& links = itr->value;
    if(!links.IsArray())
        throw CmrError("ERROR: The '"+granule_LINKS+"' object is NOT an array!",__FILE__,__LINE__);

    return links;
}

/**
 * Returns the data access URL for the dataset granule.
 */
std::string Granule::getDataAccessUrl(){
    rjson_utils rju;

    const rapidjson::Value& links = get_links_array();
    for (rapidjson::SizeType i = 0; i < links.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& link = links[i];
        string rel = rju.getStringValue(link,granule_LINKS_REL);
        if(rel == granule_LINKS_REL_DATA_ACCES){
            string data_access_url = rju.getStringValue(link,granule_LINKS_HREF);
            return data_access_url;
        }
    }
    throw CmrError("ERROR: Failed to locate granule data access link ("+granule_LINKS_REL_DATA_ACCES+"). :(",__FILE__,__LINE__);
}

/**
 * Returns the metadata access URL for the dataset granule.
 */
std::string Granule::getMetadataAccessUrl(){
    rjson_utils rju;

    const rapidjson::Value& links = get_links_array();
    for (rapidjson::SizeType i = 0; i < links.Size(); i++) { // Uses SizeType instead of size_t
        const rapidjson::Value& link = links[i];
        string rel = rju.getStringValue(link,granule_LINKS_REL);
        if(rel == granule_LINKS_REL_METADATA_ACCESS){
            string data_access_url = rju.getStringValue(link,granule_LINKS_HREF);
            return data_access_url;
        }
    }
    throw CmrError("ERROR: Failed to locate granule metadata access link ("+granule_LINKS_REL_METADATA_ACCESS+"). :(",__FILE__,__LINE__);
}


bes::CatalogItem *Granule::getCatalogItem(BESCatalogUtils *d_catalog_utils){
    bes::CatalogItem *item = new bes::CatalogItem();
    item->set_type(bes::CatalogItem::leaf);
    item->set_name(getName());
    item->set_lmt(getStringProperty("updated"));
    item->set_size(getSize());
    item->set_is_data(d_catalog_utils->is_data(item->get_name()));
    return item;
}



} //namespace cmr
