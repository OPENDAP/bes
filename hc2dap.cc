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
// $RCSfile: hc2dap.cc,v $ - routines to convert between HDFClass and DAP 
//                           data structures
// 
// $Log: hc2dap.cc,v $
// Revision 1.3  1997/03/10 22:45:50  jimg
// Update for 2.12
//
// Revision 1.5  1996/11/20  22:28:23  todd
// Modified to support UInt32 type.
//
// Revision 1.4  1996/10/14 18:18:06  todd
// Added compile option DONT_HAVE_UINT to allow compilation until DODS has
// unsigned integer types.
//
// Revision 1.3  1996/09/24 22:34:55  todd
// Added copyright statement.
//
// Revision 1.2  1996/09/24  19:44:13  todd
// Many changes to support Vdatas, GR's.  Bug fixes.
//
// Revision 1.1  1996/05/02  18:18:18  todd
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

#include <strstream.h>
// STL/libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif
#include <vector.h>
#include <algo.h>

// HDF and HDFClass includes
#include <mfhdf.h>
#include <hdfclass.h>

// DODS/HDF includes
#include "HDFInt32.h"
#include "HDFFloat64.h"
#include "HDFByte.h"
#include "HDFStr.h"
#include "HDFArray.h"
#include "HDFGrid.h"
#include "HDFSequence.h"
#include "HDFStructure.h"
#include "hdfutil.h"
#include "dodsutil.h"
#include "dhdferr.h"

#include "HDFUInt32.h"

BaseType *NewDAPVar(const String& varname, int32 hdf_type);
HDFArray *CastBaseTypeToArray(BaseType *p);
HDFStructure *CastBaseTypeToStructure(BaseType *p);
void LoadStructureFromField(HDFStructure *stru, const hdf_field& f, int row);

// STL predicate comparing equality of hdf_field objects based on their names
class fieldeq {
public:
    fieldeq(const String& s) { _val = s; }
    bool operator() (const hdf_field& f) const {
	return (f.name == _val);
    }
private:
    String _val;
};

// Create a DAP HDFSequence from an hdf_vdata.
HDFSequence *NewSequenceFromVdata(const hdf_vdata& vd) {
    // check to make sure hdf_vdata object is set up properly
    if (vd.name.length() == 0)		// Vdata must have a name
	return 0;
    if (!vd  ||  vd.fields.size() == 0)	// 
	return 0;

    // construct HDFSequence
    HDFSequence *seq = new HDFSequence(id2dods(vd.name));
    if (seq == 0)
	return 0;

    // step through each field and create a variable in the DAP Sequence
    for (int i=0; i<(int)vd.fields.size(); ++i) {
	if (!vd.fields[i]  ||  vd.fields[i].vals.size() < 1  ||
	    vd.fields[i].name.length() == 0) {
	    delete seq;		// problem with the field
	    return 0;
	}
	HDFStructure *st = new HDFStructure(id2dods(vd.fields[i].name));
	if (st == 0) {
	    delete seq;
	    return 0;
	}
	// for each subfield add the subfield to st
	if (vd.fields[i].vals[0].number_type() == DFNT_CHAR8) {

	    // collapse char subfields into one string
	    String subname = vd.fields[i].name + "__0";
	    BaseType *bt = NewDAPVar(id2dods(subname), DFNT_CHAR8);
	    if (bt == 0) {
		delete st;
		delete seq;
		return 0;
	    }
	    st->add_var(bt);	// *st now manages *bt
	}
	else {

	    // create a DODS variable for each subfield
	    char subname[hdfclass::MAXSTR];
	    for (int j=0; j<(int)vd.fields[i].vals.size(); ++j ) {
		ostrstream(subname,hdfclass::MAXSTR) << 
		    vd.fields[i].name << "__" << j << ends;
		BaseType *bt = 
		    NewDAPVar(id2dods(subname), 
			      vd.fields[i].vals[j].number_type());
		if (bt == 0) {
		    delete st;
		    delete seq;
		    return 0;
		}
		st->add_var(bt);	// *st now manages *bt
	    }
	}
	seq->add_var(st);	// *seq now manages *st
    }

    return seq;
}


// Create a DAP HDFArray out of the primary array in an hdf_sds
HDFArray *NewArrayFromSDS(const hdf_sds& sds) {
    if (sds.name.length() == 0)		// SDS must have a name
	return 0;
    if (sds.dims.size() == 0)	// SDS must have rank > 0
	return 0;

    // construct HDFArray, assign data type
    HDFArray *ar = new HDFArray(id2dods(sds.name));
    if (ar == 0)
	return 0;
    BaseType *bt = NewDAPVar(id2dods(sds.name), sds.data.number_type());
    if (bt == 0) {		// something is not right with SDS number type?
	delete ar;
	return 0;
    }
    ar->add_var(bt);		// set type of Array; ar now manages bt

    // add dimension info to HDFArray
    for (int i=0; i<(int)sds.dims.size(); ++i)
	ar->append_dim(sds.dims[i].count, id2dods(sds.dims[i].name));

    return ar;
}

// Create a DAP HDFArray out of a general raster
HDFArray *NewArrayFromGR(const hdf_gri& gr) {
    if (gr.name.length() == 0)		// GR must have a name
	return 0;

    // construct HDFArray, assign data type
    HDFArray *ar = new HDFArray(id2dods(gr.name));
    if (ar == 0)
	return 0;
    BaseType *bt = NewDAPVar(id2dods(gr.name), gr.image.number_type());
    if (bt == 0) {		// something is not right with GR number type?
	delete ar;
	return 0;
    }
    ar->add_var(bt);		// set type of Array; ar now manages bt

    // add dimension info to HDFArray
    if (gr.num_comp > 1)
	ar->append_dim(gr.num_comp, id2dods(gr.name + "__comps"));
    ar->append_dim(gr.dims[0], id2dods(gr.name + "__X"));
    ar->append_dim(gr.dims[1], id2dods(gr.name + "__Y"));
    return ar;
}

// Create a DAP HDFGrid out of the primary array and dim scale in an hdf_sds
HDFGrid *NewGridFromSDS(const hdf_sds& sds) {
    if (!sds.has_scale())	// we need a dim scale to make a Grid
	return 0;

    // Create the HDFGrid and the primary array.  Add the primary array to 
    // the HDFGrid.
    HDFArray *ar = NewArrayFromSDS(sds);
    if (ar == 0)
	return 0;
    HDFGrid *gr = new HDFGrid(id2dods(sds.name));
    if (gr == 0) {
	delete ar;
	return 0;
    }
    gr->add_var(ar, array);	// note: gr now manages ar

    // create dimension scale HDFArrays (i.e., maps) and add them to the HDFGrid
    HDFArray *dmar=0;
    BaseType *dsbt = 0;
    String mapname;
    for (int i=0; i<(int)sds.dims.size(); ++i) {
	if (sds.dims[i].name.length() == 0) { // the dim must be named
	    delete gr; 
	    return 0;
	}
	mapname = sds.dims[i].name;
	if ( (dsbt = NewDAPVar(id2dods(mapname),
			       sds.dims[i].scale.number_type())) == 0) {
	    delete gr;		// note: ~HDFGrid() cleans up the attached ar
	    return 0;
	}
	if ( (dmar = new HDFArray(id2dods(mapname))) == 0) {
	    delete gr;
	    delete dsbt;
	    return 0;
	}
	dmar->add_var(dsbt);	// set type of dim map; dmar now manages dsbt
	dmar->append_dim(sds.dims[i].count); // set dimension size
	gr->add_var(dmar, maps); // add dimension map to grid; 
				 // gr now manages dmar
    }

    return gr;
}

// Return a ptr to DAP atomic data object corresponding to an HDF Type, or
// return 0 if the HDF Type is invalid or not supported.
BaseType *NewDAPVar(const String& varname, int32 hdf_type) {
    BaseType *bt;

    switch(hdf_type) {
    case DFNT_FLOAT32:
    case DFNT_FLOAT64:
	bt = new HDFFloat64(id2dods(varname));
	break;
    case DFNT_INT8:
    case DFNT_INT16:
    case DFNT_INT32:
	bt = new HDFInt32(id2dods(varname));
	break;
    case DFNT_UINT8:
    case DFNT_UINT16:
    case DFNT_UINT32:
	bt = new HDFUInt32(id2dods(varname));
	break;
    case DFNT_UCHAR8:
	bt = new HDFByte(id2dods(varname));
	break;
    case DFNT_CHAR8:
	bt = new HDFStr(id2dods(varname));
	break;
    default:
	bt = 0;			// unsupported or invalid type
    }
    return bt;
}

// Return the DAP type name that corresponds to an HDF data type
String DAPTypeName(int32 hdf_type) {
    String rv;
    switch(hdf_type) {
    case DFNT_FLOAT32:
    case DFNT_FLOAT64:
	rv = "Float64";
	break;
    case DFNT_INT8:
    case DFNT_INT16:
    case DFNT_INT32:
	rv = "Int32";
	break;
    case DFNT_UINT8:
    case DFNT_UINT16:
    case DFNT_UINT32:
	rv = "UInt32";
	break;
    case DFNT_UCHAR8:
	rv = "Byte";
	break;
    case DFNT_CHAR8:
	rv = "String";
	break;
    default:
	rv = String();
    }
    return rv;
}

// return the HDF data type corresponding to a DODS type name
int32 HDFTypeName(const String& dods_type) {
    if (dods_type == "Float64")
	return DFNT_FLOAT64;
    else if (dods_type == "Float32")
	return DFNT_FLOAT32;
    else if (dods_type == "Int32")
	return DFNT_INT32;
    else if (dods_type == "Byte")
	return DFNT_UCHAR8;
    else if (dods_type == "String")
	return DFNT_CHAR8;
    else if (dods_type == "UInt32")
	return DFNT_UINT32;
    else 
	return -1;
}

// return the HDF data type corresponding to a DODS data type variable
int32 HDFTypeName(BaseType *t) {
    return HDFTypeName(t->type_name());
}

// load an HDFArray from an SDS
void LoadArrayFromSDS(HDFArray *ar, const hdf_sds& sds) {
    void *data = ExportDataForDODS(sds.data);
    ar->val2buf(data);
    delete []data;
    return;
}

// load an HDFArray from a GR image
void LoadArrayFromGR(HDFArray *ar, const hdf_gri& gr) {
    void *data = ExportDataForDODS(gr.image);
    ar->val2buf(data);
    delete []data;
    return;
}

// load an HDFGrid from an SDS
void LoadGridFromSDS(HDFGrid *gr, const hdf_sds& sds) {

    // load data into primary array
    HDFArray *primary_array = CastBaseTypeToArray(gr->array_var());
    LoadArrayFromSDS(primary_array, sds);
    primary_array->set_read_p(true);

    // load data into maps
    if (primary_array->dimensions() != sds.dims.size())
	THROW(dhdferr_consist);	// # of dims of SDS and HDFGrid should agree!
    Pix p = gr->first_map_var();
    void *data = 0;
    for (int i=0; i<(int)sds.dims.size() && p!=0; ++i,gr->next_map_var(p)) {
	data = ExportDataForDODS(sds.dims[i].scale);
	gr->map_var(p)->val2buf(data);
	delete []data; data = 0;
	gr->map_var(p)->set_read_p(true);
    }
    return;
}

// load an HDFSequence from a row of an hdf_vdata
void LoadSequenceFromVdata(HDFSequence *seq, hdf_vdata& vd, int row) {

    HDFStructure *stru = 0;
    for (Pix p=seq->first_var(); p!=0; seq->next_var(p)) {
	
	stru = CastBaseTypeToStructure(seq->var(p));
        String fieldname = dods2id(stru->name());

	// find corresponding field in vd
	vector<hdf_field>::iterator vf = 
	    find_if(vd.fields.begin(), vd.fields.end(), fieldeq(fieldname));
	if (vf == vd.fields.end())
	    THROW(dhdferr_consist);

	// for each field component of field, extract the proper data element
	// for the current row being requested and load into the Structure
	// variable

	// Check to make sure the number of field components of the Structure
	// and the hdf_field object match
//	if (stru->nvars() != vf->vals.size()
//	    THROW(dhdferr_consist);

	LoadStructureFromField(stru, *vf, row);
	stru->set_read_p(true);
    }
}

// Load an HDFStructure with the components of a row of an hdf_field.  If the
// field is made of char8 components, collapse these into one String component
void LoadStructureFromField(HDFStructure *stru, const hdf_field& f, int row) {

    if (row < 0 || f.vals.size() <= 0  ||  row > (int)f.vals[0].size())
	THROW(dhdferr_conv);

    BaseType *firstp = stru->var(stru->first_var());
    if (firstp->type_name() == "String") {

	// If the Structure contains a String, then that is all it will 
	// contain.  In that case, concatenate the different char8 
	// components of the field and load the DODS String with the value.
	String str;
	for (int i=0; i<(int)f.vals.size(); ++i)
	    str += f.vals[i].elt_char8(row);
	void *data = (void *)&str;
	firstp->val2buf(data);
	firstp->set_read_p(true);

    }
    else {

	// for each component of the field, load the corresponding component
	// of the DODS Structure.
	int i=0;
	void *data=0;
	for (Pix q=stru->first_var(); q!=0; stru->next_var(q),++i) {
	    data = ExportDataForDODS(f.vals[i], row); // allocating one item
	    stru->var(q)->val2buf(data);
	    stru->var(q)->set_read_p(true);
	    delete data;	// deleting one item
	}

    }
    return;
}

