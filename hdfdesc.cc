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
//
// $RCSfile: hdfdesc.cc,v $ - routines to read, build, and cache the DDS and DAS
// 
// $Log: hdfdesc.cc,v $
// Revision 1.4  1997/10/09 22:19:40  jimg
// Resolved conflicts in merge of 2.14c to trunk.
//
// Revision 1.3  1997/03/10 22:45:55  jimg
// Update for 2.12
//
// Revision 1.5  1996/10/07 17:26:23  ike
// Added Dims2Attrs() to convert hdf_dim info into hdf_attr.
// SDS_descriptions() adds sds dimension attributes to the das.
//
// Revision 1.4  1996/09/24  22:35:40  todd
// Added copyright statement.
//
// Revision 1.3  1996/09/24  19:44:49  todd
// Many changes to support Vdatas, GR's.  Bug fixes.
//
// Revision 1.2  1996/05/06  17:52:59  todd
// Updated BROKEN_PARSER code.
//
// Revision 1.1  1996/05/02  18:18:45  todd
// Initial revision
//
//////////////////////////////////////////////////////////////////////////////

// STL and/or libg++ includes
#ifdef __GNUG__
#include <String.h>
#else
#include <bstring.h>
typedef string String;
#endif // ifdef __GNUG__
#include <fstream.h>
#include <strstream.h>

// HDF and HDFClass includes
#include <mfhdf.h>
#include <hcstream.h>
#include <hdfclass.h>
#include <hcerr.h>

// DODS includes
#include "DDS.h"
#include "DAS.h"

// DODS/HDF includes
#include "dhdferr.h"
#include "HDFArray.h"
#include "HDFSequence.h"
#include "HDFGrid.h"
#include "dodsutil.h"
#include "hdf-dods.h"

#ifdef __GNUG__			// force template instantiation due to g++ bug
template class vector<hdf_attr>;
template class vector<hdf_dim>;
template class vector<String>;
template class vector<hdf_sds>;
template class vector<hdf_vdata>;
template class vector<hdf_genvec>;
template class vector<hdf_field>;
template class vector<hdf_dim>;
template class vector<hdf_gri>;
template class vector<hdf_palette>;
#endif

template<class T>
String num2string(T n) {
    static char buf[hdfdods::MAXSTR];

    ostrstream(buf,hdfdods::MAXSTR) << n << ends;

    return (String)buf;
}


extern HDFGrid *NewGridFromSDS(const hdf_sds& sds);
extern HDFArray *NewArrayFromSDS(const hdf_sds& sds);
extern HDFArray *NewArrayFromGR(const hdf_gri& gr);
extern HDFSequence *NewSequenceFromVdata(const hdf_vdata& vd);
extern BaseType *NewDAPVar(int32 hdf_type);
extern String DAPTypeName(int32 hdf_type);

void AddHDFAttr(DAS& das, const String& varname, const vector<hdf_attr>& hav);
void AddHDFAttr(DAS& das, const String& varname, const vector<String>& anv);

static void update_descriptions(const String& filename);
static void build_descriptions(DDS& dds, DAS& das, const String& filename);
static void SDS_descriptions(DDS& dds, DAS& das, const String& filename);
static void Vdata_descriptions(DDS& dds, const String& filename);
static void GR_descriptions(DDS& dds, DAS& das, const String& filename);
static void FileAnnot_descriptions(DAS& das, const String& filename);
static vector<hdf_attr> Pals2Attrs(const vector<hdf_palette>palv);
static vector<hdf_attr> Dims2Attrs(const hdf_dim dim);

// Read DDS from cache
void read_dds(DDS& dds, const String& filename) {

    update_descriptions(filename);

    String ddsfile = filename + ".cdds";
    dds.parse(ddsfile);
    return;
}

// Read DAS from cache
void read_das(DAS& das, const String& filename) {

    update_descriptions(filename);

    String dasfile = filename + ".cdas";
    das.parse(dasfile);
    return;
}


// check dates of datafile and cached DDS, DAS; update cached files if necessary
static void update_descriptions(const String& filename) {

    // if cached version of DDS or DAS is nonexistent or out of date, 
    // then regenerate DDS, DAS (cached DDS, DAS are assumed to be in the 
    // same directory as the data file)
    Stat datafile(filename);
    Stat ddsfile((filename + ".cdds"));
    Stat dasfile((filename + ".cdas"));

    // flag error if could find filename
    if (!datafile)
	THROW(dhdferr_fexist);

#ifdef NO_CACHING
    {
#else
    if (!ddsfile || !dasfile || (datafile.mtime() > ddsfile.mtime()) ||
	datafile.mtime() > dasfile.mtime()) {
#endif
	DDS dds;
	dds.set_dataset_name(id2dods(basename(CONST_CAST(String,filename))));
	DAS das;
	
	// generate DDS, DAS
	build_descriptions(dds, das, filename);
	if (!dds.check_semantics()) // DDS didn't get built right
	    THROW(dhdferr_ddssem);
//	if (!das.check_semantics()) // DAS didn't get built right
//	    THROW(dhdferr_dassem);

	// output DDS, DAS to cache
	ofstream ddsout(ddsfile.filename());
	if (!ddsout)
	    THROW(dhdferr_ddsout);
	dds.print(ddsout);
	ddsout.close();
	ofstream dasout(dasfile.filename());
	if (!dasout)
	    THROW(dhdferr_dasout);
	das.print(dasout);
	dasout.close();
    }
    return;
}

// Scan the HDF file and build the DDS and DAS
static void build_descriptions(DDS& dds, DAS& das, const String& filename) {

    // Build descriptions of SDS items
    SDS_descriptions(dds, das, filename);

    // Build descriptions of file annotations
    FileAnnot_descriptions(das, filename);

    // Build descriptions of Vdatas
    Vdata_descriptions(dds, filename);

    // Build descriptions of General Rasters
    GR_descriptions(dds, das, filename);

    return;
}

// Read SDS's out of filename, build descriptions and put them into dds, das.
static void SDS_descriptions(DDS& dds, DAS& das, const String& filename) {

    hdfistream_sds sdsin(filename);
    sdsin.setmeta(true);
  
    // Read SDS file attributes
    vector<hdf_attr> fileattrs;
    sdsin >> fileattrs;

    // Read SDS's 
    sdsin.rewind();
    vector<hdf_sds>sdslist;
    sdsin >> sdslist;

    sdsin.close();

    // Build DDS
    BaseType *pbt = 0;
    for (int i=0; i<(int)sdslist.size(); ++i) {
	if (sdslist[i].has_scale()) // make a grid
	    pbt = NewGridFromSDS(sdslist[i]);
	else
	    pbt = NewArrayFromSDS(sdslist[i]);
	if (pbt != 0)
	    dds.add_var(pbt);
    }

    // Build DAS
    AddHDFAttr(das, String("HDF_GLOBAL"), fileattrs);// add SDS file attributes
    // add each SDS's attrs
    vector<hdf_attr> dattrs;
    for (int j=0; j<(int)sdslist.size(); ++j) {
      AddHDFAttr(das, id2dods(sdslist[j].name), sdslist[j].attrs);
      for (int k=0; k<(int)sdslist[j].dims.size(); ++k) {
	dattrs = Dims2Attrs(sdslist[j].dims[k]);
	AddHDFAttr(das,id2dods(sdslist[j].name)+"_dim_"+num2string(k),dattrs);
      }
//    ADDHDFAttr(das, id2dods(sdslist[j].name), sdslist[j].annots);
    }
    return;
}

// Read Vdata's out of filename, build descriptions and put them into dds.
static void Vdata_descriptions(DDS& dds, const String& filename) {

    hdfistream_vdata vdin(filename);
    vdin.setmeta(true);

    // Read Vdata's 
    vector<hdf_vdata>vdlist;
    vdin >> vdlist;

    vdin.close();

    // Build DDS
    BaseType *pbt = 0;
    for (int i=0; i<(int)vdlist.size(); ++i) {
	pbt = NewSequenceFromVdata(vdlist[i]);
	if (pbt != 0)
	    dds.add_var(pbt);
    }

    return;
}

static void GR_descriptions(DDS& dds, DAS& das, const String& filename) {

    hdfistream_gri grin(filename);
    grin.setmeta(true);

    // Read GR file attributes
    vector<hdf_attr> fileattrs;
    grin >> fileattrs;

    // Read general rasters
    grin.rewind();
    vector<hdf_gri>grlist;
    grin >> grlist;

    grin.close();

    // Build DDS
    BaseType *pbt = 0;
    for (int i=0; i<(int)grlist.size(); ++i) {
	pbt = NewArrayFromGR(grlist[i]);
	if (pbt != 0)
	    dds.add_var(pbt);
    }

    // Build DAS
    AddHDFAttr(das, String("HDF_GLOBAL"), fileattrs);// add GR file attributes

    // add each GR's attrs
    vector<hdf_attr> pattrs;
    for (int j=0; j<(int)grlist.size(); ++j) {

	// add GR attributes 
	AddHDFAttr(das, id2dods(grlist[j].name), grlist[j].attrs);

	// add palettes as attributes
	pattrs = Pals2Attrs(grlist[j].palettes);
	AddHDFAttr(das, id2dods(grlist[j].name), pattrs);

    }

    return;
}

// Read file annotations out of filename, put in attribute structure
static void FileAnnot_descriptions(DAS& das, const String& filename) {

    hdfistream_annot annotin(filename);
    vector<String> fileannots;
    
    annotin >> fileannots;
    AddHDFAttr(das, String("HDF_GLOBAL"), fileannots);

    annotin.close();
    return;
}

// add a vector of hdf_attr to a DAS
void AddHDFAttr(DAS& das, const String& varname, const vector<hdf_attr>& hav) {
    if (hav.size() == 0)	// nothing to add
	return;

    // get pointer to the AttrTable for the variable varname (create one if 
    // necessary)
    AttrTable *atp = das.get_table(varname);
    if (atp == 0) {
	atp = new AttrTable;
	if (atp == 0)
	    THROW(hcerr_nomemory);
	atp = das.add_table(varname, atp);
    }

    // add the attributes to the DAS
    vector<String>attv;	// vector of attribute strings
    String attrtype;	// name of type of attribute
    for (int i=0; i<(int)hav.size(); ++i) { // for each attribute

	attrtype = DAPTypeName(hav[i].values.number_type());
	// get a vector of strings representing the values of the attribute
	attv = vector<String>(); // clear attv
	hav[i].values.print(attv); 
	
	// add the attribute and its values to the DAS
	for (int j=0; j<(int)attv.size(); ++j) {
	    if (attrtype == "String") 
		attv[j] = '"' + escattr(attv[j]) + '"';
	    if (atp->append_attr(id2dods(hav[i].name), attrtype, attv[j]) == 0)
		THROW(dhdferr_addattr);
	}
    }	
    
    return;
}

// add a vector of annotations to a DAS.  They are stored as attributes.  They
// are encoded as String values of an attribute named "HDF_ANNOT".
void AddHDFAttr(DAS& das, const String& varname, const vector<String>& anv) {
    if (anv.size() == 0)	// nothing to add
	return;

    // get pointer to the AttrTable for the variable varname (create one if 
    // necessary)
    AttrTable *atp = das.get_table(varname);
    if (atp == 0) {
	atp = new AttrTable;
	if (atp == 0)
	    THROW(hcerr_nomemory);
	atp = das.add_table(varname, atp);
    }

    // add the annotations to the DAS
    String anntype = DAPTypeName(DFNT_CHAR);
    String an;
    for (int i=0; i<(int)anv.size(); ++i) { // for each annotation
#ifdef BROKEN_PARSER		// this is to make the DAS parser happy
	    an = "\"" + anv[i] + "\""; // quote strings and remove embedded newlines
	    gsub(an,"\n","");
#else
	    an = anv[i];
#endif
	if (atp->append_attr(String("HDF_ANNOT"), anntype, an) == 0)
	    THROW(dhdferr_addattr);
    }	
    
    return;
}

// Add a vector of palettes as attributes to a GR.  Each palette is added as
// two attributes: the first contains the palette data; the second contains
// the number of components in the palette.
static vector<hdf_attr> Pals2Attrs(const vector<hdf_palette>palv) { 
    vector<hdf_attr> pattrs;

    if (palv.size() != 0) {
	// for each palette create an attribute with the palette inside, and an
	// attribute containing the number of components
	hdf_attr pattr;
	String palname;
	for (int i=0; i<(int)palv.size(); ++i) {
	    palname = "hdf_palette_" + num2string(i);
	    pattr.name = palname;
	    pattr.values = palv[i].table;
	    pattrs.push_back(pattr);
	    pattr.name = palname + "_ncomps";
	    pattr.values = hdf_genvec(DFNT_INT32, (void *)&palv[i].ncomp, 1);
	    pattrs.push_back(pattr);
	    if (palv[i].name.length() != 0) {
		pattr.name = palname + "_name";
		pattr.values = hdf_genvec(DFNT_CHAR, 
			  (void *)palv[i].name.chars(), palv[i].name.length());
		pattrs.push_back(pattr);
	    }
	}
    }
    return pattrs;
}

// Convert the meta information in a hdf_dim into a vector of
// hdf_attr.
static vector<hdf_attr> Dims2Attrs(const hdf_dim dim) {
  vector<hdf_attr> dattrs;
  hdf_attr dattr;
  if (dim.name.length() != 0) {
    dattr.name = "name";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.name.chars(),
			      dim.name.length());
    dattrs.push_back(dattr);
  }
  if (dim.label.length() != 0) {
    dattr.name = "long_name";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.label.chars(),
			      dim.label.length());
    dattrs.push_back(dattr);
  }
  if (dim.unit.length() != 0) {
    dattr.name = "units";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.unit.chars(),
			      dim.unit.length());    
    dattrs.push_back(dattr);
  }
  if (dim.format.length() != 0) {
    dattr.name = "format";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.format.chars(),
			      dim.format.length());    
    dattrs.push_back(dattr);
  }
  return dattrs;
}
