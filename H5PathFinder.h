
#include <map>
#include <string>

using namespace std;

/// This class that remembers all paths in a HDF5.
/// 
/// The purpose of this class is to find a cycle in reference and break the tie.
///
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// Copyright (c) 2007 HDF Group
///
/// All rights reserved.
class H5PathFinder {
  
private:
  map<string, string>     id_to_name_map;

  
public:
  H5PathFinder();
  virtual ~H5PathFinder();
  
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
