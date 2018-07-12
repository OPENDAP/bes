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

#ifndef E_CmrResponseNames_H
#define E_CmrResponseNames_H 1

#define Cmr_NAME "cmr"

// These are the names of the be keys used to configure the handler.

#define Cmr_WHITELIST "Cmr.Whitelist"
#define Cmr_MIMELIST "Cmr.MimeTypes"
#define Cmr_PROXYPROTOCOL "Cmr.ProxyProtocol"
#define Cmr_PROXYHOST "Cmr.ProxyHost"
#define Cmr_PROXYPORT "Cmr.ProxyPort"
#define Cmr_PROXYAUTHTYPE "Cmr.ProxyAuthType"
#define Cmr_PROXYUSER "Cmr.ProxyUser"
#define Cmr_PROXYPASSWORD "Cmr.ProxyPassword"
#define Cmr_PROXYUSERPW "Cmr.ProxyUserPW"
#define Cmr_USE_INTERNAL_CACHE "Cmr.UseInternalCache"

#endif // E_CmrResponseNames_H
