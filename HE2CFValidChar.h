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
#ifndef HE2CFValidChar_H
#define HE2CFValidChar_H
#include <string>

using namespace std;

/// A class for sanitizing characters that are not compliant to CF convention.
///
/// This class contains functions that parse NASA EOS StructMetadata
/// and prepares the necessary (grid) data for OPeNDAP.
///
/// @author Shu Zhang <szhang23@hdfgroup.org>
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
class HE2CFValidChar
{

private:
    char prefix;
    char valid;
    
public:
    /// The default constructor
    HE2CFValidChar();
    virtual ~HE2CFValidChar();
    
    /// Member function to replace special chars with underscore.
    ///
    /// It replaces non-alphanumeric characters in \a s  with underscore.
    string get_valid_string(string s);
    void set_valid_char(char _prefix, char _valid);

};

#endif
