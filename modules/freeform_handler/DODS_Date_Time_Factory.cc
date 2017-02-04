
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

// Implementation of the DODS_Date_Time_Factory class

#include "config_ff.h"

static char rcsid[] not_used ="$Id$";

#include "Error.h"
#include "DODS_Date_Time_Factory.h"

// Build DODS_Date_Factory and DODS_Time_Factory objects using the DAS
// information. 

DODS_Date_Time_Factory::DODS_Date_Time_Factory(DDS &dds) : _ddf(dds), _dtf(dds)
{
}

DODS_Date_Time
DODS_Date_Time_Factory::get()
{
    DODS_Date d = _ddf.get();
    DODS_Time t = _dtf.get();

    return DODS_Date_Time(d, t);
}

// $Log: DODS_Date_Time_Factory.cc,v $
// Revision 1.4  2000/10/11 19:37:55  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.3  2000/08/31 22:16:53  jimg
// Merged with 3.1.7
//
// Revision 1.2.8.1  2000/08/03 20:18:57  jimg
// Removed config_dap.h and replaced it with config_ff.h (in *.cc files;
// neither should be included in a header file).
// Changed code that calculated leap year information so that it uses the
// functions in date_proc.c/h.
//
// Revision 1.2  1999/05/04 02:55:35  jimg
// Merge with no-gnu
//
// Revision 1.1.10.1  1999/05/01 04:50:20  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.1  1999/01/22 20:44:35  jimg
// Added
//
