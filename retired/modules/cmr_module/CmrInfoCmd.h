// -*- mode: c++; c-basic-offset:4 -*-
//
// CmrInfoCmd.h
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
#include <string>
#include <ctime>

class CmrInfo {
public:

    // HTTP status of CMR response.
    int d_status;

    // The (dynamically generated) path of the granule
    string d_object_id;

    // The location URL of the granule (could be S3, could be httpd, somewhere)
    string d_location_url;

    // Size of granule
    unsigned long long size;

    // Last modified time
    time_t d_last_modified;

    string d_user_id;

    string d_user_token;

public:
    CmrInfo(string object_id, string user_id, string user_token) :
        d_status(0),
        d_object_id(object_id),
        d_location_url(0),
        size(0),
        d_last_modified(-1),
        d_user_id(user_id),
        d_user_token(user_token) { }

    ~CmrInfo(){ }

};


bool get_info(CmrInfo &cmr_info);

