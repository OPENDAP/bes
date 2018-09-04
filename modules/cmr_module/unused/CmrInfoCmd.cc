// -*- mode: c++; c-basic-offset:4 -*-
//
// CmrInfoCmd.cc
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

#include "CmrInfoCmd.h"



bool get_granule_info(CmrInfo &cmr_info){

    string uid = cmr_info.d_user_id;
    string utoke = cmr_info.d_user_token;
    string o_id = cmr_info.d_object_id;

    if(o_id.compare("data/nc/coads_climatology.nc")) {
        // Not the same :(
        cmr_info.d_location_url="/errors/error404.jsp";
        cmr_info.d_status = 404;
        cmr_info.size = 0;
        cmr_info.d_last_modified = time(0);
        return false;
    }
    else {
        // String match. Woot. :)
        cmr_info.d_location_url="http://balto.opendap.org/opendap/data/nc/coads_climatology.nc";
        cmr_info.d_status = 200;
        cmr_info.size = 3114044;
        cmr_info.d_last_modified = time(0);
        return true;
    }
}
