//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) 2010 The HDF Group
///
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//////////////////////////////////////////////////////////////////////////////
#ifndef HE2CFShortName_H
#define HE2CFShortName_H

#include <string>
#include "HE2CFValidChar.h"
#include "HE2CFUniqName.h"

using namespace std;

/// A class for handling the short name option.
///
/// This class generates short name if user specifies short name rule in the input file.
///
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
class HE2CFShortName : public HE2CFValidChar, public HE2CFUniqName
{
private:
    int size;
    int cut_size;
    bool flag;

public:
    /// Constructor
    HE2CFShortName();
    virtual ~HE2CFShortName();
    
    /// Member function to append numbers to avoid name clashes
    ///
    /// It appends a suffix and number after the string
    string get_short_string(string s, bool* flag);
    
    /// Member function to set the private members short_name_size and short_name
    ///
    /// It set values for private members short_name_size and short_name
    void set_short_name(bool flag, int length, string suffix_rule);

    /// Member function to enable short name option.
    ///
    /// It turns on short name option on-demand. This is useful for CERES.
    void set_short_name_on();

    /// Member function to disable short name option.
    ///
    /// It turns off short name option on-demand. This is useful for CERES.
    void set_short_name_off();    
};
#endif


