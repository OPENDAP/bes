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
 *  Created on: July, 14 2018
 *      Author: ndp
 */
#ifndef MODULES_CMR_MODULE_GRANULE_H_
#define MODULES_CMR_MODULE_GRANULE_H_

#include <string>
#include <vector>
#include "rapidjson/document.h"


namespace cmr {

class CmrLink {
    bool inherited;
    std::string rel;
    std::string type;
    std::string hreflang;
    std::string href;
};



class Granule {
private:


public:
    std::string producer_granule_id;
    std::vector<std::string> boxes;
    std::string time_start;
    std::string updated;
    std::string dataset_id;
    std::string data_center;
    std::string title;
    std::string coordinate_system;
    std::string day_night_flag;
    std::string time_end;
    std::string id;
    std::string original_format;
    std::string granule_size;
    bool browse_flag;
    std::string collection_concept_id;
    bool online_access_flag;

    std::vector<CmrLink &> links;

    Granule():
        producer_granule_id(""),
        time_start(""),
        updated(""),
        dataset_id(""),
        data_center(""),
        title(""),
        coordinate_system(""),
        day_night_flag(""),
        time_end(""),
        id(""),
        original_format(""),
        granule_size(""),
        browse_flag(false),
        collection_concept_id(""),
        online_access_flag(false)
    {

    }

    Granule(const rapidjson::Value& granule_obj);

};



} // namespace cmr

#endif /* MODULES_CMR_MODULE_GRANULE_H_ */



