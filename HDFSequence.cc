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
// $RCSfile: HDFSequence.cc,v $ - HDFSequence class implementation
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <map>
#include <string>
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>
#include "HDFSequence.h"
#include "HDFStructure.h"
#include "dhdferr.h"
#include "escaping.h"
#include "dodsutil.h"

#include "Error.h"

HDFSequence::HDFSequence(const string &n) : Sequence(n), row(0) {
    set_level(0);
}
HDFSequence::~HDFSequence() {}
BaseType *HDFSequence::ptr_duplicate() { return new HDFSequence(*this); }  
HDFStructure *CastBaseTypeToStructure(BaseType *p);

Sequence *NewSequence(const string &n) { return new HDFSequence(n); }

void LoadSequenceFromVdata(HDFSequence *seq, hdf_vdata& vd, int row);

bool HDFSequence::read(const string& dataset) {
  int err;
  int status = read_tagref(dataset, -1, -1, err);
  if (err)
    throw Error(unknown_error, "Could not read from dataset.");
  return status;
}

bool HDFSequence::read_tagref(const string& dataset, int32 tag, int32 ref, int& err) { 

    string hdf_file = dataset;
    string hdf_name = this->name();

    // check to see if vd is empty; if so, read in Vdata
    if (vd.name.length() == 0) {
	hdfistream_vdata vin(hdf_file.c_str());
	if(ref != -1)
	  vin.seek_ref(ref);
	else
	  vin.seek(hdf_name.c_str());
	vin >> vd;
	vin.close();
	if (!vd) {		// something is wrong
	    err = 1;		// indicate error 
	    return false;
	}
    }

    // Return false when no more data are left to be read. Note that error is
    // also false (i.e., no error occurred). 02/06/98 jhrg
    if (row >= vd.fields[0].vals[0].size()) {
	set_read_p(true); 
	err = 0;		// everything is OK
	return false;		// Indicate EOF
    }

    // is this an empty Vdata.
    // I'm not sure that it should be an error to read from an empty vdata.
    // It maybe that valid files have empty vdatas when they are first
    // created. 02/06/98 jhrg
    if (vd.fields.size() <= 0  ||  vd.fields[0].vals.size() <= 0) {
	err = 1;
	return false;
    }

    LoadSequenceFromVdata(this, vd, row++);

    set_read_p(true); 
    err = 0;			// everything is OK
    return true;

}

// $Log: HDFSequence.cc,v $
// Revision 1.12  2001/08/27 17:21:34  jimg
// Merged with version 3.2.2
//
// Revision 1.11.4.1  2001/07/28 00:25:15  jimg
// I removed the code which escapes names. This function is now handled
// for all the servers by the dap.
//
// Revision 1.11  2000/10/09 19:46:19  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.10  2000/03/31 16:56:06  jimg
// Merged with release 3.1.4
//
// Revision 1.9.8.1  2000/03/20 22:26:52  jimg
// Switched to the id2dods, etc. escaping function in the dap.
//
// Revision 1.9  1999/05/06 03:23:35  jimg
// Merged changes from no-gnu branch
//
// Revision 1.8.6.1  1999/05/06 00:27:22  jimg
// Jakes String --> string changes
//
// Revision 1.7  1998/04/06 16:08:19  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.6  1998/04/03 18:34:23  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.5  1998/02/19 19:56:07  jimg
// Fixed an error where attempting to read past the last row of a vdata caused
// an error. It now returns false with error set to false (indicating no error).
// Note that when the read() member function returns false and error is false
// that indicates that the end of the input has been found.
//
// Revision 1.4  1997/10/09 22:19:39  jimg
// Resolved conflicts in merge of 2.14c to trunk.
//
// Revision 1.3  1997/03/10 22:45:34  jimg
// Update for 2.12
//
// Revision 1.4  1996/09/24 20:57:34  todd
// Added copyright and header.
//
//
