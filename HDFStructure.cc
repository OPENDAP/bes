
#include "config_hdf.h"

#include <vector>

#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

#include "Error.h"
#include "escaping.h"
#include "HDFStructure.h"
#include "Sequence.h"
#include "dhdferr.h"
#include "dodsutil.h"

HDFStructure::HDFStructure(const string &n) : Structure(n) {}
HDFStructure::~HDFStructure() {}
BaseType *HDFStructure::ptr_duplicate() { return new HDFStructure(*this); }
void LoadStructureFromVgroup(HDFStructure *str, const hdf_vgroup& vgroup,
			     const string& hdf_file);

void HDFStructure::set_read_p(bool state) {
  // override Structure::set_read_p() to not set children as read yet
  BaseType::set_read_p(state);
}

bool HDFStructure::read(const string& dataset) {
  int err;
  int status = read_tagref(dataset, -1, -1, err);
  if (err)
    throw Error(unknown_error, "Could not read from dataset.");
  return status;
}

bool HDFStructure::read_tagref(const string& dataset, int32 tag, int32 ref, int &err) { 
  if (read_p())
    return true;

  // get the HDF dataset name, Vgroup name
  string hdf_file = dods2id(dataset);
  string hdf_name = dods2id(this->name());

  bool foundvgroup = false;
  hdf_vgroup vgroup;

#ifdef NO_EXCEPTIONS
  if (VgroupExists(hdf_file.c_str(), hdf_name.c_str())) {
#else
  try {
#endif
    hdfistream_vgroup vgin(hdf_file.c_str());
    if(ref != -1)
      vgin.seek_ref(ref);
    else
      vgin.seek(hdf_name.c_str());
    vgin >> vgroup;
    vgin.close();
    foundvgroup = true;
  }
#ifndef NO_EXCEPTIONS
  catch(...) {}
#endif

  set_read_p(true);
  
  if (foundvgroup) {
    LoadStructureFromVgroup(this, vgroup, hdf_file);
    return true;
  }
  else {
    err = 1;
    return false;
  }
}

Structure *NewStructure(const string &n) { return new HDFStructure(n); }
