////////////////////////////////////////////////////////////////////////////////
/// \mainpage
/// 
/// \section Introduction
/// This is the HDF5 OPeNDAP handler which extracts DAS/DDS/DODS information from
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
///
///
/// Copyright (C) 2007	HDF Group, Inc.
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
#include "cgi_util.h"
#include "DAS.h"
#include "DDS.h"
#include "ConstraintEvaluator.h"
#include "HDF5TypeFactory.h"
#include "DODSFilter.h"
#include "InternalErr.h"
#include "h5das.h"
#include "h5dds.h"
#include "H5Git.h" // <hyokyung 2007.02.23. 15:16:53>
#include "H5EOS.h" // <hyokyung 2007.03.23. 15:37:00>
