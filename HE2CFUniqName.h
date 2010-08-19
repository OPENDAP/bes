//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) 2010 The HDF Group
// Author: Shu Zhang <szhang23@hdfgroup.org>
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
#ifndef HE2CFUniqName_H
#define HE2CFUniqName_H
#include <map>
#include <string>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
/// A class for generating uniq string when name clash occurs.
/// 
/// This will append the uniq number to the input string.
///
/// If suffix string is defined, it will be preended before the number.
////It doesn't have to check the length of string since we assume that short
///  name option will filter long ones with uniq id. You must call
//// HE2CFShortName::get_uniq_string() first and then call get_short_string()
/// of this class.
///
/// @author Shu Zhang <szhang23@hdfgroup.org>
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// @see HE2CFShortName
///
///////////////////////////////////////////////////////////////////////////////
class HE2CFUniqName
{
private:
    bool limit;                 // has limit on counter.
    int counter;                // It needs to have its own counter.
    string suffix;              // Separate from short name suffix.

    
    
public:
    /// Constructor
    HE2CFUniqName();
    virtual ~HE2CFUniqName();

    /// Member function to append numbers to avoid name clashes
    ///
    /// It appends an optional suffix and a unique number after the \a s
    /// string.
    string get_uniq_string(string s);
    
    /// Reset the counter to 0.
    ///
    /// It sets the counter to 0.
    void set_counter();

    /// Member function to set the suffix string
    ///
    /// It set values for private members short_name_size and short_name
    void set_uniq_name(string _suffix, bool _limit);


};
#endif


