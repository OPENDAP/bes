// CmrNames.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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

#ifndef E_CmrNames_H
#define E_CmrNames_H 1

#define CMR_NAME "cmr"

#define CMR_CATALOG_NAME "CMR"

// These are the names of the be keys used to configure the handler.
#define CMR_COLLECTIONS_KEY "CMR.Collections"
#define CMR_FACETS_KEY "CMR.Facets"
#define CMR_HOST_URL_KEY "CMR.host.url"
#define DEFAULT_CMR_HOST_URL "https://cmr.earthdata.nasa.gov/"

#define CMR_PROVIDERS_LEGACY_API_ENDPOINT "legacy-services/rest/providers.json"
#define CMR_COLLECTIONS_SEARCH_API_ENDPOINT "search/collections.umm_json"
#define CMR_GRANULES_SEARCH_API_ENDPOINT "search/granules.json"

#define CMR_PROVIDER_KEY "provider"
#define CMR_PROVIDER_ID_KEY "provider_id"
#define CMR_PROVIDER_DESCRIPTION_OF_HOLDINGS_KEY "description_of_holdings"
#define CMR_PROVIDER_ORGANIZATION_NAME_KEY "organization_name"
#define CMR_PROVIDER_CONTACTS_KEY "contacts"
#define CMR_PROVIDER_REST_ONLY_KEY "rest_only"

#define MODULE CMR_NAME

#endif // E_CmrNames_H
