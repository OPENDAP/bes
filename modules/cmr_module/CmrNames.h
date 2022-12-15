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

#define CMR_MAX_PAGE_SIZE 2000
#define CMR_PROVIDERS_LEGACY_API_ENDPOINT "legacy-services/rest/providers"
#define CMR_COLLECTIONS_SEARCH_API_ENDPOINT "search/collections.umm_json"
#define CMR_GRANULES_SEARCH_API_ENDPOINT "search/granules.json"
#define CMR_GRANULES_SEARCH_API_UMM_ENDPOINT "search/granules.umm_json"

#define CMR_META_KEY "meta"
#define CMR_CONCEPT_ID_KEY "concept-id"
#define CMR_UMM_KEY "umm"
#define CMR_HITS_KEY "hits"


#define CMR_PROVIDER_KEY "provider"
#define CMR_PROVIDER_ID_KEY "provider_id"
#define CMR_PROVIDER_DESCRIPTION_OF_HOLDINGS_KEY "description_of_holdings"
#define CMR_PROVIDER_ORGANIZATION_NAME_KEY "organization_name"
#define CMR_PROVIDER_CONTACTS_KEY "contacts"
#define CMR_PROVIDER_REST_ONLY_KEY "rest_only"

#define CMR_COLLECTION_ABSTRACT_KEY "Abstract"
#define CMR_COLLECTION_ENTRY_TITLE_KEY "EntryTitle"
#define CMR_COLLECTION_SHORT_NAME_KEY "ShortName"

#define CMR_TEMPORAL_NAVIGATION_FACET_KEY "temporal"

#define CMR_V2_TEMPORAL_FACET_TITLE_VALUE "Temporal"
#define CMR_V2_FEED_KEY "feed"
#define CMR_V2_ENTRY_KEY "entry"
#define CMR_V2_FACETS_KEY "facets"
#define CMR_V2_HAS_CHILDREN_KEY "has_children"
#define CMR_V2_CHILDREN_KEY "children"
#define CMR_V2_TITLE_KEY "title"
#define CMR_V2_YEAR_TITLE_VALUE "Year"
#define CMR_V2_MONTH_TITLE_VALUE "Month"
#define CMR_V2_DAY_TITLE_VALUE "Day"
#define CMR_V2_TEMPORAL_TITLE_VALUE "Temporal"

#define CMR_UMM_ITEMS_KEY "items"
#define CMR_UMM_UMM_KEY "umm"
#define CMR_UMM_META_KEY "meta"
#define CMR_UMM_NAME_KEY "name"
#define CMR_UMM_RELATED_URLS_KEY "RelatedUrls"
#define CMR_UMM_GRANULE_UR_KEY "GranuleUR"
#define CMR_UMM_CONCEPT_ID_KEY "concept-id"
#define CMR_UMM_DATA_GRANULE_KEY "DataGranule"
#define CMR_UMM_ARCHIVE_AND_DIST_INFO_KEY "ArchiveAndDistributionInformation"
#define CMR_UMM_SIZE_KEY "Size"
#define CMR_UMM_SIZE_UNIT_KEY "SizeUnit"
#define CMR_UMM_REVISION_DATE_KEY "revision-date"
#define CMR_UMM_URL_KEY "URL"
#define CMR_UMM_TYPE_KEY "Type"
#define CMR_UMM_TYPE_GET_DATA_VALUE "GET DATA"
#define CMR_UMM_TYPE_USE_SERVICE_API_VALUE "USE SERVICE API"
#define CMR_UMM_SUBTYPE_KEY "Subtype"
#define CMR_UMM_SUBTYPE_KEY_OPENDAP_DATA_VALUE "OPENDAP DATA"


#define CMR_UMM_DESCRIPTION_KEY "Description"

#define CMR_GRANULE_ID_KEY "id"
#define CMR_GRANULE_SIZE_KEY "granule_size"
#define CMR_GRANULE_LMT_KEY "updated"
#define CMR_GRANULE_LINKS_KEY "links"
#define CMR_GRANULE_LINKS_REL_DATA_ACCESS     "http://esipfed.org/ns/fedsearch/1.1/data#"
#define CMR_GRANULE_LINKS_REL_METADATA_ACCESS "http://esipfed.org/ns/fedsearch/1.1/data#"
#define CMR_GRANULE_LINKS_REL_SERVICE "http://esipfed.org/ns/fedsearch/1.1/service#"
#define CMR_GRANULE_LINKS "links"
#define CMR_GRANULE_LINKS_REL "rel"
#define CMR_GRANULE_LINKS_HREFLANG "hreflang"
#define CMR_GRANULE_LINKS_HREF "href"

#define CMR_ITEMS_KEY "items"
#define CMR_RELATED_URLS_KEY "RelatedUrls"
#define CMR_SUBTYPE_KEY "Subtype"
#define CMR_RELATED_URLS_SUBTYPE_OPENDAP_DATA "OPENDAP DATA"



#define MODULE CMR_NAME

#endif // E_CmrNames_H
