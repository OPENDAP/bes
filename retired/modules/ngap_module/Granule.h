// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ MODULE that can be loaded in to
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
 *  Created on: July, 14 2018
 *      Author: ndp
 */
#ifndef MODULES_NGAP_MODULE_GRANULE_H_
#define MODULES_NGAP_MODULE_GRANULE_H_

#include <string>
#include <vector>
#include "rapidjson/document.h"
#include "CatalogItem.h"
#include "BESCatalogUtils.h"


namespace ngap {

    class Granule {
    private:
        const rapidjson::Value& get_links_array(const rapidjson::Value& go);

        std::string d_name;
        std::string d_id;
        std::string d_data_access_url;
        std::string d_metadata_access_url;
        std::string d_size_str;
        std::string d_last_modified_time;

        void setName(const rapidjson::Value& granule_obj);
        void setId(const rapidjson::Value& granule_obj);
        void setDataAccessUrl(const rapidjson::Value& granule_obj);
        void setMetadataAccessUrl(const rapidjson::Value& granule_obj);
        void setSize(const rapidjson::Value& granule_obj);
        void setLastModifiedStr(const rapidjson::Value& granule_obj);

    public:
        Granule(const rapidjson::Value& granule_obj);

        std::string getName(){ return d_name; }
        std::string getId(){ return d_id; }
        std::string getDataAccessUrl() { return d_data_access_url; }
        std::string getMetadataAccessUrl(){ return d_metadata_access_url; }
        std::string getSizeStr(){ return d_size_str; }
        std::string getLastModifiedStr() { return d_last_modified_time; }
        size_t getSize(){ return atol(getSizeStr().c_str()); }

        bes::CatalogItem *getCatalogItem(BESCatalogUtils *d_catalog_utils);
    };

} // namespace ngap

#endif /* MODULES_NGAP_MODULE_GRANULE_H_ */



