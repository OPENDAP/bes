#ifndef _HDF5Structure_h
#define _HDF5Structure_h 1

#ifdef _GNUG_
#pragma interface
#endif

#include <string>
#include <H5Ipublic.h>
#include "Structure.h"
#include "H5Git.h"
// Copied and Extended from HDFStructure.cc in hdf4_handler
// <hyokyung 2007.03. 2. 13:53:06>

/// A HDF5Structure class.
/// This class converts HDF5 compound type into DAP structure.
///
/// @author James Gallagher
/// @author Hyo-Kyung Lee
///
/// @see Structure 
class HDF5Structure: public Structure {
private:
  hid_t dset_id;
  hid_t ty_id;
  int array_index;
  int array_size; // size constrained by constraint expression
  int array_entire_size; // entire size in case of array of structure
  
public:
  // friend string print_type(hid_t datatype);   
  HDF5Structure(const string &n = "");
  virtual ~HDF5Structure();
  
  HDF5Structure &operator=(const HDF5Structure &rhs);
  virtual BaseType *ptr_duplicate();
  virtual bool read(const string &dataset);
  // virtual bool read_tagref(const string &dataset, int32 tag, int32 ref, int &error);
  // virtual void set_read_p(bool state);
  hid_t get_did();
  hid_t get_tid();
  int get_array_index();
  int get_array_size();
  int get_entire_array_size();
  
  void set_did(hid_t dset);
  void set_tid(hid_t type);
  void set_array_index(int i);
  void set_array_size(int i);
  void set_entire_array_size(int i);
  
  friend string return_type(hid_t datatype);   
};

#endif
