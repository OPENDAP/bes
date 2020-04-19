// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of httpd_catalog_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.
//
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

#ifndef I_HTTP_NAME_H
#define I_HTTP_NAME_H 1

#define HTTP_WHITELIST "Http.Whitelist"
#define HTTP_MIMELIST "Http.MimeTypes"
#define HTTP_PROXYPROTOCOL "Http.ProxyProtocol"
#define HTTP_PROXYHOST "Http.ProxyHost"
#define HTTP_PROXYPORT "Http.ProxyPort"
#define HTTP_PROXYAUTHTYPE "Http.ProxyAuthType"
#define HTTP_PROXYUSER "Http.ProxyUser"
#define HTTP_PROXYPASSWORD "Http.ProxyPassword"
#define HTTP_PROXYUSERPW "Http.ProxyUserPW"
#define HTTP_USE_INTERNAL_CACHE "Http.UseInternalCache"
// Could be used
#define HTTP_DIR_KEY = "Http.Cache.dir";
#define HTTP_PREFIX_KEY = "Http.Cache.prefix";
#define HTTP_SIZE_KEY = "Http.Cache.size";

#endif // I_HTTP_NAME_H

#ifndef I_HTTPD_CATALOG_NAME_H
#define I_HTTPD_CATALOG_NAME_H 1

#define HTTPD_CATALOG "httpd"

// This name  appears at the top level of the Hyrax catalog:
#define HTTPD_CATALOG_NAME "RemoteResources"

// These are the names of the bes keys used to configure the handler.
#define HTTPD_CATALOG_COLLECTIONS "Httpd_Catalog.Collections"
#if 0
#define HTTPD_CATALOG_WHITELIST "Httpd_Catalog.Whitelist"
#define HTTPD_CATALOG_MIMELIST "Httpd_Catalog.MimeTypes"
#define HTTPD_CATALOG_PROXYPROTOCOL "Httpd_Catalog.ProxyProtocol"
#define HTTPD_CATALOG_PROXYHOST "Httpd_Catalog.ProxyHost"
#define HTTPD_CATALOG_PROXYPORT "Httpd_Catalog.ProxyPort"
#define HTTPD_CATALOG_PROXYAUTHTYPE "Httpd_Catalog.ProxyAuthType"
#define HTTPD_CATALOG_PROXYUSER "Httpd_Catalog.ProxyUser"
#define HTTPD_CATALOG_PROXYPASSWORD "Httpd_Catalog.ProxyPassword"
#define HTTPD_CATALOG_PROXYUSERPW "Httpd_Catalog.ProxyUserPW"
#define HTTPD_CATALOG_USE_INTERNAL_CACHE "Httpd_Catalog.UseInternalCache"
#endif
//#define MODULE HTTPD_CATALOG

#endif // I_HTTPD_CATALOG_NAME_H


#ifndef E_CmrNames_H
#define E_CmrNames_H 1

#define CMR_NAME "cmr"

#define CMR_CATALOG_NAME "CMR"

// These are the names of the be keys used to configure the handler.
#define CMR_COLLECTIONS "CMR.Collections"
#define CMR_FACETS "CMR.Facets"

#if 0
#define CMR_WHITELIST "Cmr.Whitelist"
#define CMR_MIMELIST "Cmr.MimeTypes"
#define CMR_PROXYPROTOCOL "Cmr.ProxyProtocol"
#define CMR_PROXYHOST "Cmr.ProxyHost"
#define CMR_PROXYPORT "Cmr.ProxyPort"
#define CMR_PROXYAUTHTYPE "Cmr.ProxyAuthType"
#define CMR_PROXYUSER "Cmr.ProxyUser"
#define CMR_PROXYPASSWORD "Cmr.ProxyPassword"
#define CMR_PROXYUSERPW "Cmr.ProxyUserPW"
#define CMR_USE_INTERNAL_CACHE "Cmr.UseInternalCache"
#endif
//#define MODULE CMR_NAME

#endif // E_CmrNames_H

#ifndef E_NgapNames_H
#define E_NgapNames_H 1

#define NGAP_NAME "ngap"

// #define NGAP_CATALOG_NAME "NGAP"

// These are the names of the be keys used to configure the handler.
// #define NGAP_COLLECTIONS "NGAP.Collections"
// #define NGAP_FACETS "NGAP.Facets"
#if 0
#define NGAP_WHITELIST "NGAP.Whitelist"
#define NGAP_MIMELIST "NGAP.MimeTypes"
#define NGAP_PROXYPROTOCOL "NGAP.ProxyProtocol"
#define NGAP_PROXYHOST "NGAP.ProxyHost"
#define NGAP_PROXYPORT "NGAP.ProxyPort"
#define NGAP_PROXYAUTHTYPE "NGAP.ProxyAuthType"
#define NGAP_PROXYUSER "NGAP.ProxyUser"
#define NGAP_PROXYPASSWORD "NGAP.ProxyPassword"
#define NGAP_PROXYUSERPW "NGAP.ProxyUserPW"
#define NGAP_USE_INTERNAL_CACHE "NGAP.UseInternalCache"
#endif
#define DATA_ACCESS_URL_KEY "OPeNDAP_DMRpp_DATA_ACCESS_URL"
#define NGAP_INJECT_DATA_URL_KEY "NGAP.inject_data_urls"
#define NGAP_CMR_HOSTNAME_KEY "NGAP.cmr_host_url"
#define NGAP_CMR_SEARCH_ENDPOINT_PATH_KEY "NGAP.cmr_search_endpoint_path"

//#define MODULE NGAP_NAME

#endif // E_NgapNames_H

#ifndef E_GatewayResponseNames_H
#define E_GatewayResponseNames_H 1

#define Gateway_NAME "gateway"

// These are the names of the be keys used to configure the handler.
#if 0
#define Gateway_WHITELIST "Gateway.Whitelist"
#define Gateway_MIMELIST "Gateway.MimeTypes"
#define Gateway_PROXYPROTOCOL "Gateway.ProxyProtocol"
#define Gateway_PROXYHOST "Gateway.ProxyHost"
#define Gateway_PROXYPORT "Gateway.ProxyPort"
#define Gateway_PROXYAUTHTYPE "Gateway.ProxyAuthType"
#define Gateway_PROXYUSER "Gateway.ProxyUser"
#define Gateway_PROXYPASSWORD "Gateway.ProxyPassword"
#define Gateway_PROXYUSERPW "Gateway.ProxyUserPW"
#define Gateway_USE_INTERNAL_CACHE "Gateway.UseInternalCache"
#endif
//#define MODULE Gateway_NAME

#endif // E_GatewayResponseNames_H