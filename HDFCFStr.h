// This file is part of the hdf4_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDFCFStr.h
/// \brief This class provides a way to map HDF4 1-D character array  to DAP Str for the CF option
///
/// In the future, this may be merged with the default option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _HDFCFSTR_H
#define _HDFCFSTR_H

// STL includes
#include <string>

// DODS includes
#include <dods-limits.h>
#include <Str.h>
#include "mfhdf.h"
#include "hdf.h"

using namespace libdap;

class HDFCFStr:public Str {
  public:
    HDFCFStr(const int h4fd, int32 field_ref,const std::string &filename,const std::string &varname, const std::string &varnewname, bool is_vdata);
    virtual ~ HDFCFStr();
    virtual BaseType *ptr_duplicate();
    virtual bool read();
  private:
    std::string filename;
    std::string varname;
    int32 h4fd;
    int32 field_ref;
    bool is_vdata;
   
};

#endif                          // _HDFCFSTR_H

