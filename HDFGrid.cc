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
// $RCSfile: HDFGrid.cc,v $ - HDFGrid class implementation
//
// $Log: HDFGrid.cc,v $
// Revision 1.1  1996/10/31 18:43:33  jimg
// Added.
//
// Revision 1.4  1996/09/24 20:53:26  todd
// Added copyright and header.
//
//
/////////////////////////////////////////////////////////////////////////////

#include <Pix.h>
#include <vector.h>
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>
#include "config_dap.h"
#include "HDFGrid.h"
#include "HDFArray.h"
#include "dhdferr.h"
#include "hdfutil.h"

HDFGrid::HDFGrid(const String &n = (char *)0) : Grid(n) {}
HDFGrid::~HDFGrid() {}
BaseType *HDFGrid::ptr_duplicate() { return new HDFGrid(*this); }

HDFArray *CastBaseTypeToArray(BaseType *p);
void LoadGridFromSDS(HDFGrid *gr, const hdf_sds& sds);

// Read in a Grid from an SDS in an HDF file.
bool HDFGrid::read(const String& dataset, int& err) {
    err = -1;			// OK initially

    String hdf_file = dods2id(dataset);
    String hdf_name = dods2id(this->name());

    if (read_p())
	return true;
    hdf_sds sds;

    // get slab constraint from primary array
    vector<int> start, edge, stride;
    HDFArray *primary_array = CastBaseTypeToArray(this->array_var());
    bool isslab = primary_array->GetSlabConstraint(start, edge, stride);

    // read in SDS
#ifndef NO_EXCEPTIONS
    try {
#endif
	hdfistream_sds sdsin(hdf_file.chars());
	sdsin.seek(hdf_name.chars());
	if (isslab)
	    sdsin.setslab(start, edge, stride, false);
	sdsin >> sds;
	sdsin.close();
	if (!sds) {
	    err = 0;
	    return false;
	}
	
	// load data into primary array
	LoadGridFromSDS(this, sds);
#ifndef NO_EXCEPTIONS
    }
    catch (...) {
	err = 0;
	return false;
    }
#endif
    
    return true;
}

Grid *NewGrid(const String &n) { return new HDFGrid(n); }

