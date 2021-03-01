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
#define PARSER "dmrpp:parser"
#define CREDS  "dmrpp:creds"
#define DMRPP_CURL  "dmrpp:curl"

#define DMRPP_USE_TRANSFER_THREADS_KEY "DMRPP.UseParallelTransfers"
#define DMRPP_MAX_TRANSFER_THREADS_KEY "DMRPP.MaxParallelTransfers"

#define DMRPP_USE_COMPUTE_THREADS_KEY "DMRPP.UseComputeThreads"
#define DMRPP_MAX_COMPUTE_THREADS_KEY "DMRPP.MaxComputeThreads"

#define DMRPP_WAIT_FOR_FUTURE_MS 1

#define DMRPP_DEFAULT_CONTIGUOUS_CONCURRENT_THRESHOLD  (2*1024*1024)
#define DMRPP_CONTIGUOUS_CONCURRENT_THRESHOLD_KEY "DMRPP.ContiguousConcurrencyThreshold"

#define DMRPP_USE_SUPER_CHUNKS 1
#define DMRPP_ENABLE_THREAD_TIMERS 0


#endif // E_DmrppNames_H



