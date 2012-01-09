/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values.
//  Authors:   Lily Dong <donglm@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFDESC_H
#define HDFDESC_H

#include "misrproj.h"
#include "errormacros.h"

using namespace libdap;
using namespace std;

enum MODISType
{MODIS_TYPE1, MODIS_TYPE2, MODIS_TYPE3, OTHER_TYPE};

// MODIS product type.
extern int mtype;

#endif


