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
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

// STL includes
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

using std::cerr ;
using std::ends ;
using std::endl ;

// HDF and HDFClass includes
// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

// DODS/HDF includes
#include "escaping.h"
#include "HDFInt32.h"
#include "HDFInt16.h"
#include "HDFUInt32.h"
#include "HDFUInt16.h"
#include "HDFFloat64.h"
#include "HDFFloat32.h"
#include "HDFByte.h"
#include "HDFStr.h"
#include "HDFArray.h"
#include "HDFGrid.h"
#include "HDFSequence.h"
#include "HDFStructure.h"
#include "hdfutil.h"
#include "dhdferr.h"
#include "hdf-maps.h"
#include "debug.h"

// Undefine the following to send signed bytes using unsigned bytes. 1/13/98
// jhrg.
#define SIGNED_BYTE_TO_INT32 1

BaseType *NewDAPVar(const string& varname, int32 hdf_type);
void LoadStructureFromField(HDFStructure *stru, hdf_field& f, int row);

// STL predicate comparing equality of hdf_field objects based on their names
class fieldeq {
public:
    fieldeq(const string& s) { _val = s; }
    bool operator() (const hdf_field& f) const {
	return (f.name == _val);
    }
private:
    string _val;
};

// Create a DAP HDFSequence from an hdf_vdata.
HDFSequence *NewSequenceFromVdata(const hdf_vdata& vd) {
    // check to make sure hdf_vdata object is set up properly
    if (vd.name.length() == 0)		// Vdata must have a name
	return 0;
    if (!vd  ||  vd.fields.size() == 0)	// 
	return 0;

    // construct HDFSequence
    HDFSequence *seq = new HDFSequence(vd.name);
    if (seq == 0)
	return 0;

    // step through each field and create a variable in the DAP Sequence
    for (int i=0; i<(int)vd.fields.size(); ++i) {
	if (!vd.fields[i]  ||  vd.fields[i].vals.size() < 1  ||
	    vd.fields[i].name.length() == 0) {
	    delete seq;		// problem with the field
	    return 0;
	}
	HDFStructure *st = new HDFStructure(vd.fields[i].name);
	if (st == 0) {
	    delete seq;
	    return 0;
	}
	// for each subfield add the subfield to st
	if (vd.fields[i].vals[0].number_type() == DFNT_CHAR8 ||
	    vd.fields[i].vals[0].number_type() == DFNT_UCHAR8) {

	    // collapse char subfields into one string
	    string subname = vd.fields[i].name + "__0";
	    BaseType *bt = new HDFStr(subname);
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
#if 0
		ostringstream strm(subname,hdfclass::MAXSTR);
		strm << vd.fields[i].name << "__" << j
#endif
		BaseType *bt = 
		    NewDAPVar(subname,
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


// Create a DAP HDFStructure from an hdf_vgroup.
HDFStructure *NewStructureFromVgroup(const hdf_vgroup& vg, vg_map& vgmap,
			     sds_map& sdmap, vd_map& vdmap, gr_map& grmap) {
    // check to make sure hdf_vgroup object is set up properly
    if (vg.name.length() == 0)		// Vgroup must have a name
	return 0;
    if (!vg )	                        // Vgroup must have some tagrefs
	return 0;
    
    // construct HDFStructure
    HDFStructure *str = new HDFStructure(vg.name);
    if (str == 0)
	return 0;
    bool nonempty = false;
    // step through each tagref and copy its contents to DAP
    for (int i=0; i<(int)vg.tags.size(); ++i) {
        int32 tag = vg.tags[i];
	int32 ref = vg.refs[i];
	BaseType *bt = 0;
	switch(tag) {
	case DFTAG_VH:
	  bt = NewSequenceFromVdata(vdmap[ref].vdata);
	  break;
	case DFTAG_NDG:
	  if(sdmap[ref].sds.has_scale()) {
	    bt = NewGridFromSDS(sdmap[ref].sds);
	  } else {
	    bt = NewArrayFromSDS(sdmap[ref].sds);
	  }
	  break;
	case DFTAG_VG:
	  // GR's are also stored as Vgroups
	  if(grmap.find(ref) != grmap.end())
	    bt = NewArrayFromGR(grmap[ref].gri);
	  else
	    bt = NewStructureFromVgroup(vgmap[ref].vgroup, vgmap, sdmap, vdmap,
					grmap);
	  break;
	default:
	  cerr << "Error: Unknown vgroup child: " << tag << endl;
	  break;
	}
	if(bt) {
	  str->add_var(bt);	// *st now manages *bt
	  nonempty = true;
	}
    }
    if(nonempty) {
      return str;
    } else {
      delete str;
      return 0;
    }
}

// Create a DAP HDFArray out of the primary array in an hdf_sds
HDFArray *NewArrayFromSDS(const hdf_sds& sds) {
    if (sds.name.length() == 0)		// SDS must have a name
	return 0;
    if (sds.dims.size() == 0)	// SDS must have rank > 0
	return 0;

    // construct HDFArray, assign data type
    HDFArray *ar = new HDFArray(sds.name);
    if (ar == 0)
	return 0;
    BaseType *bt = NewDAPVar(sds.name, sds.data.number_type());
    if (bt == 0) {		// something is not right with SDS number type?
	delete ar;
	return 0;
    }
    ar->add_var(bt);		// set type of Array; ar now manages bt

    // add dimension info to HDFArray
    for (int i=0; i<(int)sds.dims.size(); ++i)
	ar->append_dim(sds.dims[i].count, sds.dims[i].name);

    return ar;
}

// Create a DAP HDFArray out of a general raster
HDFArray *NewArrayFromGR(const hdf_gri& gr) {
    if (gr.name.length() == 0)		// GR must have a name
	return 0;

    // construct HDFArray, assign data type
    HDFArray *ar = new HDFArray(gr.name);
    if (ar == 0)
	return 0;
    BaseType *bt = NewDAPVar(gr.name, gr.image.number_type());
    if (bt == 0) {		// something is not right with GR number type?
	delete ar;
	return 0;
    }
    ar->add_var(bt);		// set type of Array; ar now manages bt

    // add dimension info to HDFArray
    if (gr.num_comp > 1)
	ar->append_dim(gr.num_comp, gr.name + "__comps");
    ar->append_dim(gr.dims[1], gr.name + "__Y");
    ar->append_dim(gr.dims[0], gr.name + "__X");
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
    HDFGrid *gr = new HDFGrid(sds.name);
    if (gr == 0) {
	delete ar;
	return 0;
    }
    gr->add_var(ar, array);	// note: gr now manages ar

    // create dimension scale HDFArrays (i.e., maps) and add them to the HDFGrid
    HDFArray *dmar=0;
    BaseType *dsbt = 0;
    string mapname;
    for (int i=0; i<(int)sds.dims.size(); ++i) {
	if (sds.dims[i].name.length() == 0) { // the dim must be named
	    delete gr; 
	    return 0;
	}
	mapname = sds.dims[i].name;
	if ( (dsbt = NewDAPVar(mapname,
			       sds.dims[i].scale.number_type())) == 0) {
	    delete gr;		// note: ~HDFGrid() cleans up the attached ar
	    return 0;
	}
	if ( (dmar = new HDFArray(mapname)) == 0) {
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
BaseType *
NewDAPVar(const string& varname, int32 hdf_type) 
{
    switch(hdf_type) {
      case DFNT_FLOAT32:
	return new HDFFloat32(varname);

      case DFNT_FLOAT64:
	return new HDFFloat64(varname);

      case DFNT_INT16:
	return new HDFInt16(varname);

#ifdef SIGNED_BYTE_TO_INT32
      case DFNT_INT8:
#endif
      case DFNT_INT32:
	return new HDFInt32(varname);

      case DFNT_UINT16:
	return new HDFUInt16(varname);

      case DFNT_UINT32:
	return new HDFUInt32(varname);

	// INT8 and UINT8 *should* be grouped under Int32 and UInt32, but
	// that breaks too many programs. jhrg 12/30/97
#ifndef SIGNED_BYTE_TO_INT32
      case DFNT_INT8:
#endif
      case DFNT_UINT8:
      case DFNT_UCHAR8:
      case DFNT_CHAR8:
	return new HDFByte(varname);

      default:
	return 0;
    }
}

// Return the DAP type name that corresponds to an HDF data type
string DAPTypeName(int32 hdf_type) {
    switch(hdf_type) {
      case DFNT_FLOAT32:
	return string("Float32");

      case DFNT_FLOAT64:
	return string("Float64");

      case DFNT_INT16:
	return string("Int16");

#ifdef SIGNED_BYTE_TO_INT32
      case DFNT_INT8:
#endif
      case DFNT_INT32:
	return string("Int32");

      case DFNT_UINT16:
	return string("UInt16");

      case DFNT_UINT32:
	return string("UInt32");

	// See the note above about INT8 and UINT8. jhrg 12/30/97.
#ifndef SIGNED_BYTE_TO_INT32
      case DFNT_INT8:
#endif
      case DFNT_UINT8:
	return string("Byte");

      case DFNT_CHAR8:
      case DFNT_UCHAR8:
	// note: DFNT_CHAR8 is Byte in DDS but String in DAS
	return string("String");  

      default:
	return string("");
    }
}

// load an HDFArray from an SDS
void LoadArrayFromSDS(HDFArray *ar, const hdf_sds& sds) {
#ifdef SIGNED_BYTE_TO_INT32
    switch (sds.data.number_type()) {
      case DFNT_INT8: {
	  char *data = static_cast<char *>(ExportDataForDODS(sds.data));
	  ar->val2buf(data);
	  delete []data;
	  break;
      }
      default:
	ar->val2buf(const_cast<char *>(sds.data.data()));
    }
#else
    ar->val2buf(const_cast<char *>(sds.data.data()));
#endif
    return;
}

// load an HDFArray from a GR image
void LoadArrayFromGR(HDFArray *ar, const hdf_gri& gr) {
#ifdef SIGNED_BYTE_TO_INT32
    switch (gr.image.number_type()) {
      case DFNT_INT8: {
	  char *data = static_cast<char *>(ExportDataForDODS(gr.image));
	  ar->val2buf(data);
	  delete []data;
	  break;
      }

      default:
	ar->val2buf(const_cast<char *>(gr.image.data()));
    }
#else
    ar->val2buf(const_cast<char *>(gr.image.data()));
#endif
    return;
}

// load an HDFGrid from an SDS
// I modified Todd's code so that only the parts of a Grid that are marked as
// to be sent will be read. 1/29/2002 jhrg
void LoadGridFromSDS(HDFGrid *gr, const hdf_sds& sds) {

    // load data into primary array
    HDFArray &primary_array = dynamic_cast<HDFArray &>(*gr->array_var());// ***
    if (primary_array.send_p()) {
	LoadArrayFromSDS(&primary_array, sds);
	primary_array.set_read_p(true);
    }

    // load data into maps
    if (primary_array.dimensions() != sds.dims.size())
	THROW(dhdferr_consist);	// # of dims of SDS and HDFGrid should agree!

    Grid::Map_iter p = gr->map_begin();
#if 0
    Pix p = gr->first_map_var();
#endif
    for (unsigned int i = 0; i < sds.dims.size() && p != gr->map_end(); ++i, ++p) {
	if ((*p)->send_p()) {
#ifdef SIGNED_BYTE_TO_INT32
	    switch (sds.dims[i].scale.number_type()) {
	      case DFNT_INT8: {
		  char *data = static_cast<char *>
		      (ExportDataForDODS(sds.dims[i].scale));
		  (*p)->val2buf(data);
		  delete []data;
		  break;
	      }
	      default:
		(*p)->val2buf(const_cast<char *>(sds.dims[i].scale.data()));
	    }
#else
	    (*p)->val2buf(const_cast<char *>(sds.dims[i].scale.data()));
#endif
	    (*p)->set_read_p(true);
	}
    }
    return;
}

// load an HDFSequence from a row of an hdf_vdata
void 
LoadSequenceFromVdata(HDFSequence *seq, hdf_vdata& vd, int row) 
{

    for (Sequence::Vars_iter p = seq->var_begin(); p != seq->var_end(); ++p) {
#if 0
    for (Pix p = seq->first_var(); p; seq->next_var(p)) {
#endif
	HDFStructure &stru = dynamic_cast<HDFStructure &>(*(*p));

	// find corresponding field in vd
	vector<hdf_field>::iterator vf = 
	    find_if(vd.fields.begin(), vd.fields.end(), fieldeq(stru.name()));
	if (vf == vd.fields.end())
	    THROW(dhdferr_consist);

	// for each field component of field, extract the proper data element
	// for the current row being requested and load into the Structure
	// variable
	LoadStructureFromField(&stru, *vf, row);
	stru.set_read_p(true);
    }
}

// Load an HDFStructure with the components of a row of an hdf_field.  If the
// field is made of char8 components, collapse these into one String component
void 
LoadStructureFromField(HDFStructure *stru, hdf_field& f, int row) 
{

    if (row < 0 || f.vals.size() <= 0  ||  row > (int)f.vals[0].size())
	THROW(dhdferr_conv);

    BaseType *firstp = *(stru->var_begin());
#if 0
    BaseType *firstp = stru->var(stru->first_var());
#endif
    if (firstp->type() == dods_str_c) {
	// If the Structure contains a String, then that is all it will 
	// contain.  In that case, concatenate the different char8 
	// components of the field and load the DODS String with the value.
	string str = "";
	for (unsigned int i = 0; i < f.vals.size(); ++i) {
	    DBG(cerr << i << ": " << f.vals[i].elt_char8(row) << endl);
	    str += f.vals[i].elt_char8(row);
	}

	firstp->val2buf(static_cast<void *>(&str)); // data);
	firstp->set_read_p(true);
    }
    else {
	// for each component of the field, load the corresponding component
	// of the DODS Structure.
	int i = 0;
	for (Structure::Vars_iter q = stru->var_begin(); q != stru->var_end(); ++q, ++i) {
#if 0
	for (Pix q=stru->first_var(); q; stru->next_var(q), ++i) {
#endif
	    // AccessDataForDODS does the same basic thing that
	    // ExportDataForDODS(hdf_genvec &, int) does except that the
	    // Access function does not allocate memeory; it provides access
	    // using the data held in the hdf_genvec without copying it. See
	    // hdfutil.cc. 4/10/2002 jhrg
	    (*q)->val2buf(static_cast<char *>
				  (ExportDataForDODS(f.vals[i], row)));
	    (*q)->set_read_p(true);
	}

    }
    return;
}

// Load an HDFStructure with the contents of a vgroup.
void 
LoadStructureFromVgroup(HDFStructure *str, const hdf_vgroup& vg,
			const string& hdf_file) 
{
    int i = 0;
    int err = 0;
    for (Structure::Vars_iter q = str->var_begin(); q != str->var_end(); ++q, ++i) {
#if 0
    for (Pix q = str->first_var(); err == 0 && q; str->next_var(q), ++i) {
#endif
	BaseType *p = (*q);
	if (p->send_p() && p->name() == vg.vnames[i] ) {
#ifdef __SUNPRO_CC
	    // As of v4.1, SunPro C++ is too braindamaged to support
	    // dynamic_cast<>, so we have to add read_tagref() to BaseType.h
	    // in the core instead of using a mixin class (multiple
	    // inheritance).
	    (ReadTagRef*)(p)->read_tagref(hdf_file, vg.tags[i], vg.refs[i], 
					  err);
#else
	    (dynamic_cast<ReadTagRef*>(p))->read_tagref(hdf_file, vg.tags[i],
							vg.refs[i], err);
#endif
	}
    }
}

// $Log: hc2dap.cc,v $
// Revision 1.18  2004/02/04 16:52:56  jimg
// Removed Pix methods.
//
// Revision 1.17  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.15.4.9  2002/12/18 23:32:50  pwest
// gcc3.2 compile corrections, mainly regarding the using statement. Also,
// missing semicolon in .y file
//
// Revision 1.15.4.8  2002/04/12 00:07:04  jimg
// I removed old code that was wrapped in #if 0 ... #endif guards.
//
// Revision 1.15.4.7  2002/04/11 23:55:50  jimg
// Removed the call to AccessDataForDODS; it is broken.
//
// Revision 1.15.4.6  2002/04/11 18:31:33  jimg
// Removed HDFTypeName functions. These were not used by the server.
//
// Revision 1.15.4.5  2002/04/11 03:14:11  jimg
// Massive changes to the code that actually loads data into the DODS variables.
// Previously the ExportDataForDODS functions (there's two) were used. These
// functions used the hdf_genvec methods to access values stored in the
// hdf_genvec instance and copy them to newly allocated memory. In general this
// is a waste since the BaseType::val2buf() methods (which each of the children
// of BaseType overload) also copy their values (this might change in the
// future, but those classes will never delete memory allocated outside of the
// DAP library). Eliminating this extra copy should improve performance with
// large datasets.
//
// Revision 1.15.4.4  2002/04/10 18:38:10  jimg
// I modified the server so that it knows about, and uses, all the DODS
// numeric datatypes. Previously the server cast 32 bit floats to 64 bits and
// cast most integer data to 32 bits. Now if an HDF file contains these
// datatypes (32 bit floats, 16 bit ints, et c.) the server returns data
// using those types (which DODS has supported for a while...).
//
// Revision 1.15.4.3  2002/01/29 20:19:02  jimg
// I modified LoadGridFromSDS() so that it only reads/loads the parts of a
// Grid that the client requested. The function now checks to see if send_p()
// is set for the primary array and each map, only loading those for which
// it is set.
//
// Revision 1.15.4.2  2002/01/28 23:38:18  dan
// Modified LoadStructureFromVgroup to support modifications
// to the ancillary DDS file, such that variables in the original
// DDS can be removed from the ancillary DDS.  Previously, the
// behavior of the server was to assume that the DDS mapped
// exactly to the sequential layout of the tags/refs in the hdf
// file itself.  When not the ancillary DDS is simply a cached version
// of the original DDS then this is true, however any modification
// to the DDS will break this assumption.  The new behavior does
// not allow variables to be moved, only removed, this is an
// important distinction.
//
// Revision 1.16  2001/08/27 17:21:34  jimg
// Merged with version 3.2.2
//
// Revision 1.15.4.1  2001/07/28 00:25:15  jimg
// I removed the code which escapes names. This function is now handled
// for all the servers by the dap.
//
// Revision 1.15  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.14  2000/03/31 16:56:06  jimg
// Merged with release 3.1.4
//
// Revision 1.13.8.2  2000/03/20 23:26:07  jimg
// Removed debugging output
//
// Revision 1.13.8.1  2000/03/20 22:26:52  jimg
// Switched to the id2dods, etc. escaping function in the dap.
//
// Revision 1.13  1999/05/06 03:23:36  jimg
// Merged changes from no-gnu branch
//
// Revision 1.12.6.1  1999/05/06 00:27:24  jimg
// Jakes String --> string changes
//
// Revision 1.8  1998/04/14 18:42:05  jimg
// Temporary fix for LoadStructureFromVgroup. Added err. There is a more
// comprehensive fix from Jake Hamby that will added later.
//
// Revision 1.7  1998/04/06 16:08:20  jimg
// Patch from Jake Hamby; change from switch to Mixin class for read_ref()
//
// Revision 1.6  1998/04/03 18:34:27  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.5  1998/02/05 20:14:31  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.4  1997/12/30 23:59:06  jimg
// Changed the functions that map datatypes so that 8-bit numbers are sent as
// Byte variables rather than Int32 or UInt32. This is a work-around for
// problems that client-side software has in dealing with 8-bit numbers that
// get transmitted in 32-bit fields.
//
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
