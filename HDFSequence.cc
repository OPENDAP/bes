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

HDFSequence::HDFSequence(const String &n = (char *)0) : Sequence(n) {
    set_level(0);
}
HDFSequence::~HDFSequence() {}
BaseType *HDFSequence::ptr_duplicate() { return new HDFSequence(*this); }  
HDFStructure *CastBaseTypeToStructure(BaseType *p);

Sequence *NewSequence(const String &n) { return new HDFSequence(n); }

void LoadSequenceFromVdata(HDFSequence *seq, hdf_vdata& vd, int row);

bool HDFSequence::read(const String& dataset, int& err) { 

    static int row = 0;		// current row
    static hdf_vdata vd;	// holds Vdata

    String hdf_file = dods2id(dataset);
    String hdf_name = dods2id(this->name());

    // check to see if vd is empty; if so, read in Vdata
    if (vd.name.length() == 0) {
	hdfistream_vdata vin(hdf_file.chars());
	vin.seek(hdf_name.chars());
	vin >> vd;
	vin.close();
	if (!vd) {		// something is wrong
	    err = 1;		// indicate error 
	    return false;
	}
    }

    // is this an empty Vdata or have we read past last row?
    if (vd.fields.size() <= 0  ||  vd.fields[0].vals.size() <= 0  || 
	row >= vd.fields[0].vals[0].size()) {
	err = 1;
	return false;
    }

    LoadSequenceFromVdata(this, vd, row++);

    set_read_p(true); 
    err = 0;			// everything is OK
    return true;

}
