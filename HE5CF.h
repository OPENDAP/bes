// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

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
#ifndef _HE5CF_H
#define _HE5CF_H

#include "config_hdf5.h"

#include "HE5ShortName.h"
#include "HE5CFSwath.h"
#include "HE5CFGrid.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <InternalErr.h>
#include <debug.h>

using namespace std;
using namespace libdap;
/// A class for generating CF convention compliant output.
///
/// This class contains functions that generate CF-convention compliant output.
/// By default, hdf5 handler cannot generate the DAP ouput that OPeNDAP
/// visualization clients can display due to the discrepancy between the 
/// HDF-EOS5 model and the model based on CF-convention. Most visualization
/// clients require an output that follows CF-convention in order to display
/// data directly on a map. 
/// 
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2009 The HDF Group
class HE5CF: public HE5ShortName, public HE5CFSwath, public HE5CFGrid {
  
private:
  
public:
    /// A flag for checking whether shared dimension variables are generated
    /// or not.
    bool _shared_dimension;        

    /// A look-up map for translating HDF-EOS5 convention to CF-1.x convention
    map < string, string > eos_to_cf_map;

  
    HE5CF();
    virtual ~HE5CF();
    

    /// Returns a character pointer to the string that matches CF-convention.
    const char* get_CF_name(char *eos_name);

    /// Returns whether shared dimension variables are generated or not.
    bool        get_shared_dimension();
    

    /// Clears all internal map variables and flags in the subclasses.
    void        set();

    /// Sets the shared_dimension flag true.
    void        set_shared_dimension();

   /// Get valid CF names(the name should follow CF conventions)
   string get_valid_CF_name(string s);


  
};

#endif
