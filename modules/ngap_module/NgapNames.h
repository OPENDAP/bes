// NgapContainer.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2020 OPeNDAP, Inc.
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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>

#ifndef E_NgapNames_H
#define E_NgapNames_H 1

#define NGAP_NAME "ngap"

#define NGAP_CACHE_TERMINAL_URLS_KEY "NGAP.cache.terminal.urls"

#define DATA_ACCESS_URL_KEY "OPeNDAP_DMRpp_DATA_ACCESS_URL"
#define NGAP_INJECT_DATA_URL_KEY "NGAP.inject_data_urls"
#define NGAP_CMR_HOSTNAME_KEY "NGAP.cmr_host_url"
#define NGAP_CMR_SEARCH_ENDPOINT_PATH_KEY "NGAP.cmr_search_endpoint_path"

#define MODULE NGAP_NAME

#endif // E_NgapNames_H
