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

#ifndef E_DmrppNames_H
#define E_DmrppNames_H 1

#define DMRPP_NAME "dmrpp"

#define MODULE DMRPP_NAME

#define PARSER "dmrpp:dmz"
#define CREDS  "dmrpp:creds"
#define DMRPP_CURL  "dmrpp:curl"

#define DMRPP_USE_OBJECT_CACHE_KEY "DMRPP.UseObjectCache"
#define DMRPP_OBJECT_CACHE_ENTRIES_KEY "DMRPP.ObjectCacheEntries"
#define DMRPP_OBJECT_CACHE_PURGE_LEVEL_KEY "DMRPP.ObjectCachePurgeLevel"

#define DMRPP_USE_TRANSFER_THREADS_KEY "DMRPP.UseParallelTransfers"
#define DMRPP_MAX_TRANSFER_THREADS_KEY "DMRPP.MaxParallelTransfers"

#define DMRPP_USE_COMPUTE_THREADS_KEY "DMRPP.UseComputeThreads"
#define DMRPP_MAX_COMPUTE_THREADS_KEY "DMRPP.MaxComputeThreads"

#define DMRPP_USE_CLASSIC_IN_FILEOUT_NETCDF "FONc.ClassicModel"
#define DMRPP_DISABLE_DIRECT_IO "DMRPP.DisableDirectIO"

#define DMRPP_WAIT_FOR_FUTURE_MS 1

#define DMRPP_DEFAULT_CONTIGUOUS_CONCURRENT_THRESHOLD  (2*1024*1024)
#define DMRPP_CONTIGUOUS_CONCURRENT_THRESHOLD_KEY "DMRPP.ContiguousConcurrencyThreshold"

#define DMRPP_USE_SUPER_CHUNKS 1
#define DMRPP_ENABLE_THREAD_TIMERS 0

#define DMRPP_FIXED_LENGTH_STRING_ARRAY_ELEMENT "dmrpp:FixedLengthStringArray"
#define DMRPP_FIXED_LENGTH_STRING_LENGTH_ATTR "string_length"
#define DMRPP_FIXED_LENGTH_STRING_PAD_ATTR "pad"

#define DMRPP_VLSA_ELEMENT "dmrpp:vlsa"
#define DMRPP_VLSA_VALUE_ELEMENT "v"
#define DMRPP_VLSA_VALUE_SIZE_ATTR "s"
#define DMRPP_VLSA_VALUE_COUNT_ATTR "c"


#endif // E_DmrppNames_H



