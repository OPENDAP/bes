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
/// \file HDFEOS2CFStr.h
/// \brief This class provides a way to map HDFEOS2  1-D character array  to DAP Str for the CF option
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#ifndef _HDFEOS2CFSTR_H
#define _HDFEOS2CFSTR_H

// STL includes
#include <string>

// DODS includes
#include <dods-limits.h>
#include <Str.h>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"

using namespace libdap;

class HDFEOS2CFStr:public Str {
  public:
    HDFEOS2CFStr(const int gsfd, 
                 const std::string &filename,
                 const std::string &objname,
                 const std::string &varname, 
                 const std::string &varnewname,
                 int grid_or_swath);

    virtual ~ HDFEOS2CFStr();
    virtual BaseType *ptr_duplicate();

    virtual bool read();

  private:
    int32 gsfd;
    std::string filename;
    std::string objname;
    std::string varname;
    int grid_or_swath;
   
};

#endif                          // _HDFEOS2CFSTR_H
#endif
