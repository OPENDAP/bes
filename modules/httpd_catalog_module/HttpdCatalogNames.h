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

#ifndef I_HTTPD_CATALOG_NAME_H
#define I_HTTPD_CATALOG_NAME_H 1

#define HTTPD_CATALOG "httpd"

#define HTTPD_CATALOG_NAME "RemoteResources"

// These are the names of the be keys used to configure the handler.
#define HTTPD_CATALOG_COLLECTIONS "Httpd_Catalog.Collections"

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

#define MODULE HTTPD_CATALOG

#endif // I_HTTPD_CATALOG_NAME_H
