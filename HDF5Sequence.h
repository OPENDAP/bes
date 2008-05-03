#ifndef _HDF5Sequence_h
#define _HDF5Sequence_h 1

#include "Sequence.h"
#include "common.h"
#include "H5Git.h"

using namespace libdap;

/// A HDF5Sequence class.
/// This class is not used in the current hdf5 handler and
/// is provided to support DAP Sequence data type if necessary.
///
/// @author James Gallagher
///
/// @see Sequence 
class HDF5Sequence:public Sequence {

  private:
    hid_t dset_id;
    hid_t ty_id;

  public:

     HDF5Sequence(const string & n = "");
     virtual ~ HDF5Sequence();

    virtual BaseType *ptr_duplicate();
    virtual bool read(const string & dataset);

    friend string return_type(hid_t datatype);

    hid_t get_did();
    hid_t get_tid();
    void set_did(hid_t dset);
    void set_tid(hid_t type);


};

#endif
