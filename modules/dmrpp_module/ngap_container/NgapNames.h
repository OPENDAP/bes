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

// This could be a 'global' debug key. jhrg 9/20/23
constexpr static auto NGAP_CACHE = "cache";
constexpr static auto NGAP_NAME = "ngap";

constexpr static auto DATA_ACCESS_URL_KEY = "OPeNDAP_DMRpp_DATA_ACCESS_URL";
constexpr static auto MISSING_DATA_ACCESS_URL_KEY = "OPeNDAP_DMRpp_SC_DATA_ACCESS_URL";
constexpr static auto NGAP_INJECT_DATA_URL_KEY = "NGAP.inject_data_urls";
constexpr static auto NGAP_CMR_HOSTNAME_KEY = "NGAP.cmr_host_url";
constexpr static auto NGAP_CMR_SEARCH_ENDPOINT_PATH_KEY = "NGAP.cmr_search_endpoint_path";

constexpr static auto NGAP_PROVIDERS_KEY = "/providers/";
constexpr static auto NGAP_COLLECTIONS_KEY = "/collections/";
constexpr static auto NGAP_CONCEPTS_KEY = "/concepts/";
constexpr static auto NGAP_GRANULES_KEY = "/granules/";

constexpr static auto DEFAULT_CMR_ENDPOINT_URL = "https://cmr.earthdata.nasa.gov";
constexpr static auto DEFAULT_CMR_SEARCH_ENDPOINT_PATH = "/search/granules.umm_json_v1_4";
constexpr static auto CMR_URL_TYPE_GET_DATA = "GET DATA";

constexpr static auto USE_CMR_CACHE = "NGAP.UseCMRCache";
constexpr static auto CMR_CACHE_THRESHOLD = "NGAP.CMRCacheSize.Items";
constexpr static auto CMR_CACHE_SPACE = "NGAP.CMRCachePurge.Items";

constexpr static auto USE_DMRPP_CACHE = "NGAP.UseDMRppCache";
constexpr static auto DMRPP_CACHE_THRESHOLD = "NGAP.DMRppCacheSize.Items";
constexpr static auto DMRPP_CACHE_SPACE = "NGAP.DMRppCachePurge.Items";

constexpr static auto DMRPP_FILE_CACHE_THRESHOLD = "NGAP.DMRppFileCacheSize.MB"; // in MB
constexpr static auto DMRPP_FILE_CACHE_SPACE = "NGAP.DMRppFileCachePurge.MB";    // in MB
constexpr static auto DMRPP_FILE_CACHE_DIR = "NGAP.DMRppFileCacheDir";

constexpr static auto DATA_SOURCE_LOCATION = "NGAP.DataSourceLocation";
constexpr static auto USE_OPENDAP_BUCKET = "NGAP.UseOPeNDAPBucket";

  // These are used only in NgapApiTest.cc. jhrg 9/28/23
#define CMR_PROVIDER "provider"
#define CMR_ENTRY_TITLE "entry_title"
#define CMR_COLLECTION_CONCEPT_ID "collection_concept_id"
#define CMR_GRANULE_UR "granule_ur"

constexpr static auto MODULE = "dmrpp";

#endif // E_NgapNames_H
