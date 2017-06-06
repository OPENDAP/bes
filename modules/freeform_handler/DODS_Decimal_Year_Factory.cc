
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1998
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// Implementation of the DODS_Decimal_Year_Factory class


#include "config_ff.h"

static char rcsid[] not_used ="$Id$";

#include "Error.h"
#include "DODS_Decimal_Year_Factory.h"

// Build DODS_Date_Factory and DODS_Time_Factory objects using the DAS
// information. 

DODS_Decimal_Year_Factory::DODS_Decimal_Year_Factory(DDS &dds) :
  _ddf(dds), _dtf(dds)
{
}

DODS_Decimal_Year
DODS_Decimal_Year_Factory::get()
{
    DODS_Date d = _ddf.get();
    DODS_Time t = _dtf.get();

    return DODS_Decimal_Year(d, t);
}



