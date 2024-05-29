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
 * rjson_utils.h
 *
 *  Created on: July, 17 2018
 *      Author: ndp
 */

#ifndef MODULES_NGAP_MODULE_RJSONUTILS_H_
#define MODULES_NGAP_MODULE_RJSONUTILS_H_

#include <string>
#include <vector>
#include "rapidjson/document.h"

namespace ngap {


    class rjson_utils {
    public:
        void getJsonDoc(const std::string &url, rapidjson::Document &d);
        std::string getStringValue(const rapidjson::Value& object, const std::string &name);
        // bool getBooleanValue(const rapidjson::Value& object, const std::string name);
        std::string jsonDocToString(rapidjson::Document &d);
    };



} // namespace ngap

#endif /* MODULES_NGAP_MODULE_RJSONUTILS_H_ */
