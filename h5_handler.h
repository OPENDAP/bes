
// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \mainpage
/// 
/// \section Introduction
/// This is the OPeNDAP HDF5 server which extracts DAS/DDS/DODS information from
/// a hdf5 data file. 
///
/// \section Usage
///
/// dap_h5_handler -o \<response\> -u \<url\> [options ...] [data set]
///
/// options:
///       -  -o \<response\>: DAS, DDS, DataDDS, DDX, BLOB or Version (Required)
///       -  -u \<url\>: The complete URL minus the CE (required for DDX)
///       -  -c: Compress the response using the deflate algorithm.
///       -  -e \<expr\>: When returning a DataDDS, use \<expr\> as the constraint.
///       -  -v \<version\>: Use \<version\> as the version number
///       -  -d \<dir\>: Look for ancillary file in \<dir\> (deprecated).
///       -  -f \<file\>: Look for ancillary data in \<file\> (deprecated).
///       -  -r \<dir\>: Use \<dir\> as a cache directory
///       -  -l \<time\>: Conditional request; if data source is unchanged since
///                    \<time\>, return an HTTP 304 response.
///       -  -t \<seconds\>: Timeout the handler after \<seconds\>.
///
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \file h5_handler.h
/// \brief The main header of HDF5 OPeNDAP handler
////////////////////////////////////////////////////////////////////////////////
#include "config_hdf5.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <string>

#include "debug.h"
#include "DAS.h"
#include "DDS.h"
#include "ConstraintEvaluator.h"
#include "DODSFilter.h"
#include "InternalErr.h"
#include "mime_util.h"
#include "h5das.h"
#include "h5dds.h"
#include "H5Git.h"              
#include "H5EOS.h"             

using namespace libdap;
