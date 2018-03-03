// GatewayResponseNames.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of gateway_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Patrick West <pwest@ucar.edu>
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

#ifndef E_GatewayResponseNames_H
#define E_GatewayResponseNames_H 1

#define Gateway_NAME "gateway"

// These are the names of the be keys used to configure the handler.

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

#endif // E_GatewayResponseNames_H
