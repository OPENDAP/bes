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
// $Log: HDFSequence.cc,v $
// Revision 1.8  1998/09/17 17:57:49  jimg
// Resolved conflicts from previous merge
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
/////////////////////////////////////////////////////////////////////////////

#include <map.h>
#include <String.h>
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>
#include "HDFSequence.h"
#include "HDFStructure.h"
#include "dhdferr.h"
#include "dodsutil.h"

HDFSequence::HDFSequence(const String &n = (char *)0) : Sequence(n), row(0) {
    set_level(0);
}
HDFSequence::~HDFSequence() {}
BaseType *HDFSequence::ptr_duplicate() { return new HDFSequence(*this); }  
HDFStructure *CastBaseTypeToStructure(BaseType *p);

Sequence *NewSequence(const String &n) { return new HDFSequence(n); }

void LoadSequenceFromVdata(HDFSequence *seq, hdf_vdata& vd, int row);

bool HDFSequence::read(const String& dataset, int& err) {
  return read_tagref(dataset, -1, -1, err);
}

bool HDFSequence::read_tagref(const String& dataset, int32 tag, int32 ref, int& err) { 

    String hdf_file = dods2id(dataset);
    String hdf_name = dods2id(this->name());

    // check to see if vd is empty; if so, read in Vdata
    if (vd.name.length() == 0) {
	hdfistream_vdata vin(hdf_file.chars());
	if(ref != -1)
	  vin.seek_ref(ref);
	else
	  vin.seek(hdf_name.chars());
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
        err = 1;       // error
	return false;
    }

    LoadSequenceFromVdata(this, vd, row++);

    set_read_p(true); 
    err = 0;			// everything is OK
    return true;

}
