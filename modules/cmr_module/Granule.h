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



class Granule {
private:
    rapidjson::Value& d_granule_obj;

public:

    Granule(rapidjson::Value& granule_obj){
        d_granule_obj = granule_obj;
    }




};



} // namespace cmr

#endif /* MODULES_CMR_MODULE_GRANULE_H_ */



