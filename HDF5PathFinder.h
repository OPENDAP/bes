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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

#include <map>
#include <string>

using namespace std;

/// \file HDF5PathFinder.h
/// 
/// \brief This class is to find and break a cycle in the HDF5 group. 
/// It is used to handle the rara case  for the default option.
///
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// All rights reserved.
class HDF5PathFinder {

  private:
    map < string, string > id_to_name_map;


  public:
    HDF5PathFinder();
    virtual ~ HDF5PathFinder();

    /// Adds \a name and \a id object number into an internal map
    /// 
    /// \param id  HDF5 object number
    /// \param name HDF5 object name
    /// \see h5das.cc
    /// \return true if addition is successful
    /// \return false otherwise
    bool add(string id, const string name);

    /// Check if \a id object is already visited by looking up in the map.
    ///
    /// \param id  HDF5 object number
    /// \see h5das.cc
    /// \return true if \a id object is already visited 
    /// \return false otherwise
    bool visited(string id);

    /// Get the object name of \a id object in the map.
    ///
    /// \param id  HDF5 object number
    /// \see h5das.cc
    /// \return object name string
    string get_name(string id);

};
