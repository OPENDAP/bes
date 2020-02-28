// NgapResponseNames.h

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ngap_module, A C++ module that can be loaded in to
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

#ifndef E_NgapResponseNames_H
#define E_NgapResponseNames_H 1

#define Ngap_NAME "ngap"

// These are the names of the be keys used to configure the handler.

#define Ngap_WHITELIST "Ngap.Whitelist"
#define Ngap_MIMELIST "Ngap.MimeTypes"
#define Ngap_PROXYPROTOCOL "Ngap.ProxyProtocol"
#define Ngap_PROXYHOST "Ngap.ProxyHost"
#define Ngap_PROXYPORT "Ngap.ProxyPort"
#define Ngap_PROXYAUTHTYPE "Ngap.ProxyAuthType"
#define Ngap_PROXYUSER "Ngap.ProxyUser"
#define Ngap_PROXYPASSWORD "Ngap.ProxyPassword"
#define Ngap_PROXYUSERPW "Ngap.ProxyUserPW"
#define Ngap_USE_INTERNAL_CACHE "Ngap.UseInternalCache"

#endif // E_NgapResponseNames_H
