/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1996, California Institute of Technology.  
// ALL RIGHTS RESERVED.   U.S. Government Sponsorship acknowledged. 
//
// Please read the full copyright notice in the file COPYRIGH 
// in this directory.
//
// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: HDFStructure.cc,v $ - HDFStr class implementation
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <vector>

// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

#include "Error.h"
#include "escaping.h"
#include "HDFStructure.h"
#include "Sequence.h"
#include "dhdferr.h"

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
  int err = 0;
  int status = read_tagref(dataset, -1, -1, err);
  if (err)
    throw Error(unknown_error, "Could not read from dataset.");
  return status;
}

bool HDFStructure::read_tagref(const string& dataset, int32 tag, int32 ref, int &err) { 
  if (read_p())
    return true;

  // get the HDF dataset name, Vgroup name
  string hdf_file = dataset;
  string hdf_name = this->name();

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

// $Log: HDFStructure.cc,v $
// Revision 1.12  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.9.4.6  2002/04/12 00:07:04  jimg
// I removed old code that was wrapped in #if 0 ... #endif guards.
//
// Revision 1.9.4.5  2002/04/12 00:03:14  jimg
// Fixed casts that appear throughout the code. I changed most/all of the
// casts to the new-style syntax. I also removed casts that we're not needed.
//
// Revision 1.9.4.4  2002/04/10 18:38:10  jimg
// I modified the server so that it knows about, and uses, all the DODS
// numeric datatypes. Previously the server cast 32 bit floats to 64 bits and
// cast most integer data to 32 bits. Now if an HDF file contains these
// datatypes (32 bit floats, 16 bit ints, et c.) the server returns data
// using those types (which DODS has supported for a while...).
//
// Revision 1.9.4.3  2002/03/14 19:15:07  jimg
// Fixed use of int err in read() so that it's always initialized to zero.
// This is a fix for bug 135.
//
