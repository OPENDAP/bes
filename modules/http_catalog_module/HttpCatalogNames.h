// HttpCatalogNames.h

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

#ifndef http_catalog_names_h_
#define http_catalog_names_h_ 1

#define HTTP_CATALOG "HttpCatalog"

#define HTTP_CATALOG_NAME "HTTP_Catalog"

// These are the names of the be keys used to configure the handler.
#define HTTP_CATALOG_COLLECTIONS "HTTP_Catalog.Collections"
#define HTTP_CATALOG_FACETS "HTTP_Catalog.Facets"

#define HTTP_CATALOG_WHITELIST "HTTP_Catalog.Whitelist"
#define HTTP_CATALOG_MIMELIST "HTTP_Catalog.MimeTypes"
#define HTTP_CATALOG_PROXYPROTOCOL "HTTP_Catalog.ProxyProtocol"
#define HTTP_CATALOG_PROXYHOST "HTTP_Catalog.ProxyHost"
#define HTTP_CATALOG_PROXYPORT "HTTP_Catalog.ProxyPort"
#define HTTP_CATALOG_PROXYAUTHTYPE "HTTP_Catalog.ProxyAuthType"
#define HTTP_CATALOG_PROXYUSER "HTTP_Catalog.ProxyUser"
#define HTTP_CATALOG_PROXYPASSWORD "HTTP_Catalog.ProxyPassword"
#define HTTP_CATALOG_PROXYUSERPW "HTTP_Catalog.ProxyUserPW"
#define HTTP_CATALOG_USE_INTERNAL_CACHE "HTTP_Catalog.UseInternalCache"

#define MODULE HTTP_CATALOG

#endif // http_catalog_names_h_
