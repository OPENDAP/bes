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
// Revision 1.13  1999/05/06 03:23:36  jimg
// Merged changes from no-gnu branch
//
// Revision 1.12.6.1  1999/05/06 00:27:24  jimg
// Jakes String --> string changes
//
// Revision 1.7  1998/04/03 18:34:28  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.6  1998/02/05 20:14:32  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.5  1997/12/30 23:55:42  jimg
// The Dataset name is no longer filtered by id2dods(). Now `.'s in the
// filename look like dots not `%2e'.
//
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

// STL includes
#include <string>
#include <fstream>
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
#include "hdf-maps.h"
#include "parser.h"

#ifdef __GNUG__			// force template instantiation due to g++ bug
template class vector<hdf_attr>;
template class vector<hdf_dim>;
template class vector<string>;
template class vector<hdf_sds>;
template class vector<hdf_vdata>;
template class vector<hdf_genvec>;
template class vector<hdf_field>;
template class vector<hdf_dim>;
template class vector<hdf_gri>;
template class vector<hdf_palette>;
#endif

template<class T>
string num2string(T n) {
    static char buf[hdfdods::MAXSTR];

    ostrstream(buf,hdfdods::MAXSTR) << n << ends;

    return (string)buf;
}

struct yy_buffer_state;
yy_buffer_state *hdfeos_scan_string(const char *str);
extern int hdfeosparse(void *arg); // defined in hdfeos.tab.c

void AddHDFAttr(DAS& das, const string& varname, const vector<hdf_attr>& hav);
void AddHDFAttr(DAS& das, const string& varname, const vector<string>& anv);

static void update_descriptions(const string& cachedir, const string& filename);
static void build_descriptions(DDS& dds, DAS& das, const string& filename);
static void SDS_descriptions(sds_map& map, DAS& das, const string& filename);
static void Vdata_descriptions(vd_map& map, DAS& das, const string& filename);
static void Vgroup_descriptions(DDS& dds, DAS& das, const string& filename,
				sds_map& sdmap, vd_map& vdmap, gr_map& grmap);
static void GR_descriptions(gr_map& map, DAS& das, const string& filename);
static void FileAnnot_descriptions(DAS& das, const string& filename);
static vector<hdf_attr> Pals2Attrs(const vector<hdf_palette>palv);
static vector<hdf_attr> Dims2Attrs(const hdf_dim dim);

// Generate cache filename.
string cache_name(const string& cachedir, const string& filename) {
  // default behavior: if no cache dir, store cache file with HDF
  if(cachedir=="")
    return filename;

  string newname = filename; // create a writable copy
  // skip over common path component (usually something like "/usr/local/www")
  uint32 start = 0, dirstart = 0;
  while(newname[start] == cachedir[dirstart]) {
    start++;
    dirstart++;
  }
  // now backup to the last path separator
  while(newname[start-1] != '/')
    start--;

  // turn the remaining path separators into "."
  uint32 slash = start;
  while((slash = newname.find_first_of('/', slash)) != newname.npos) {
    newname[slash] = '.';
  }
  string retval = cachedir + "/" + newname.substr(start);
  return retval; // return the new string
}

// Read DDS from cache
void read_dds(DDS& dds, const string& cachedir, const string& filename) {

    update_descriptions(cachedir, filename);

    string ddsfile = cache_name(cachedir, filename) + ".cdds";
    dds.parse(ddsfile);
    return;
}

// Read DAS from cache
void read_das(DAS& das, const string& cachedir, const string& filename) {

    update_descriptions(cachedir, filename);

    string dasfile = cache_name(cachedir, filename) + ".cdas";
    das.parse(dasfile);
    return;
}


// check dates of datafile and cached DDS, DAS; update cached files if necessary
static void update_descriptions(const string& cachedir, const string& filename) {

    // if cached version of DDS or DAS is nonexistent or out of date, 
    // then regenerate DDS, DAS (cached DDS, DAS are assumed to be in the 
    // same directory as the data file)
    Stat datafile(filename);
    Stat ddsfile((cache_name(cachedir, filename) + ".cdds"));
    Stat dasfile((cache_name(cachedir, filename) + ".cdas"));

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
	dds.set_dataset_name(basename(CONST_CAST(string,filename)));
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
static void build_descriptions(DDS& dds, DAS& das, const string& filename) {
    sds_map sdsmap;
    vd_map vdatamap;
    gr_map grmap;

    // Build descriptions of SDS items
    SDS_descriptions(sdsmap, das, filename);

    // Build descriptions of file annotations
    FileAnnot_descriptions(das, filename);

    // Build descriptions of Vdatas
    Vdata_descriptions(vdatamap, das, filename);

    // Build descriptions of General Rasters
    GR_descriptions(grmap, das, filename);

    // Build descriptions of Vgroups and add SDS/Vdata/GR in the correct order
    Vgroup_descriptions(dds, das, filename, sdsmap, vdatamap, grmap);

    return;
}

// Read SDS's out of filename, build descriptions and put them into dds, das.
static void SDS_descriptions(sds_map& map, DAS& das, const string& filename) {

    hdfistream_sds sdsin(filename);
    sdsin.setmeta(true);
  
    // Read SDS file attributes
    vector<hdf_attr> fileattrs;
    sdsin >> fileattrs;

    // Read SDS's 
    sdsin.rewind();
    while (!sdsin.eos()) {
      sds_info sdi;  // add the next sds_info to map
      sdsin >> sdi.sds;
      sdi.in_vgroup = false; // assume we're not part of a vgroup
      map[sdi.sds.ref] = sdi; // assign to map by ref
    }

    sdsin.close();

    // Build DAS
    AddHDFAttr(das, string("HDF_GLOBAL"), fileattrs);// add SDS file attributes
    // add each SDS's attrs
    vector<hdf_attr> dattrs;
    for (SDSI s=map.begin(); s!=map.end(); ++s) {
      const hdf_sds *sds = &s->second.sds;
      AddHDFAttr(das, id2dods(sds->name), sds->attrs);
      for (int k=0; k<(int)sds->dims.size(); ++k) {
	dattrs = Dims2Attrs(sds->dims[k]);
	AddHDFAttr(das,id2dods(sds->name)+"_dim_"+num2string(k),dattrs);
      }
//    ADDHDFAttr(das, id2dods(sdslist[j].name), sdslist[j].annots);
    }
    return;
}

// Read Vdata's out of filename, build descriptions and put them into dds.
static void Vdata_descriptions(vd_map& map, DAS& das, const string& filename) {

    hdfistream_vdata vdin(filename);
    vdin.setmeta(true);

    // Read Vdata's 
    while (!vdin.eos()) {
      vd_info vdi;  // add the next vd_info to map
      vdin >> vdi.vdata;
      vdi.in_vgroup = false; // assume we're not part of a vgroup
      map[vdi.vdata.ref] = vdi; // assign to map by ref
    }
    vdin.close();

    // Build DAS
    vector<hdf_attr> dattrs;
    for (VDI s=map.begin(); s!=map.end(); ++s) {
      const hdf_vdata *vd = &s->second.vdata;
      AddHDFAttr(das, id2dods(vd->name), vd->attrs);
    }

    return;
}

// Read Vgroup's out of filename, build descriptions and put them into dds.
static void Vgroup_descriptions(DDS& dds, DAS& das, const string& filename,
				sds_map& sdmap, vd_map& vdmap, gr_map& grmap) {
    hdfistream_vgroup vgin(filename);

    // Read Vgroup's 
    vg_map vgmap;
    while (!vgin.eos()) {
      vg_info vgi;    // add the next vg_info to map
      vgin >> vgi.vgroup; // read vgroup itself
      vgi.toplevel = true;  // assume toplevel until we prove otherwise
      vgmap[vgi.vgroup.ref] = vgi; // assign to map by vgroup ref
    }
    vgin.close();
    // for each Vgroup
{    for(VGI v=vgmap.begin(); v!=vgmap.end(); ++v) {
      const hdf_vgroup *vg = &v->second.vgroup;
      
      // Add Vgroup attributes
      AddHDFAttr(das, id2dods(vg->name), vg->attrs);

      // now, assign children
      for(uint32 i=0; i<vg->tags.size(); i++) {
	int32 tag = vg->tags[i];
	int32 ref = vg->refs[i];
	switch(tag) {
	case DFTAG_VG:
	  // Could be a GRI or a Vgroup
	  if(grmap.find(ref) != grmap.end())
	    grmap[ref].in_vgroup = true;
	  else
	    vgmap[ref].toplevel = false;
	  break;
	case DFTAG_VH:
	  vdmap[ref].in_vgroup = true;
	  break;
	case DFTAG_NDG:
	  sdmap[ref].in_vgroup = true;
	  break;
	default:
	  cerr << "unknown tag: " << tag << " ref: " << ref << endl;
	}
      }
    }
}
    // Build DDS for all toplevel vgroups
    BaseType *pbt = 0;
{    for(VGI v=vgmap.begin(); v!=vgmap.end(); ++v) {
      if(!v->second.toplevel)
	continue;   // skip over non-toplevel vgroups
      pbt = NewStructureFromVgroup(v->second.vgroup, vgmap, sdmap, vdmap, grmap);
      if (pbt != 0) {
	dds.add_var(pbt);
      }
    }
}
    // add lone SDS's
    for(SDSI s=sdmap.begin(); s!=sdmap.end(); ++s) {
      if(s->second.in_vgroup)
	continue;  // skip over SDS's in vgroups
      if (s->second.sds.has_scale()) // make a grid
	pbt = NewGridFromSDS(s->second.sds);
      else
	pbt = NewArrayFromSDS(s->second.sds);
      if (pbt != 0)
	dds.add_var(pbt);
    }

    // add lone Vdata's
    for(VDI v=vdmap.begin(); v!=vdmap.end(); ++v) {
      if(v->second.in_vgroup)
	continue;  // skip over Vdata in vgroups
      pbt = NewSequenceFromVdata(v->second.vdata);
      if (pbt != 0)
	dds.add_var(pbt);
    }

    // add lone GR's
    for(GRI g=grmap.begin(); g!=grmap.end(); ++g) {
      if(g->second.in_vgroup)
	continue;  // skip over GRs in vgroups
      pbt = NewArrayFromGR(g->second.gri);
      if (pbt != 0)
	dds.add_var(pbt);
    }
}

static void GR_descriptions(gr_map& map, DAS& das, const string& filename) {

    hdfistream_gri grin(filename);
    grin.setmeta(true);

    // Read GR file attributes
    vector<hdf_attr> fileattrs;
    grin >> fileattrs;

    // Read general rasters
    grin.rewind();
    while (!grin.eos()) {
      gr_info gri;  // add the next gr_info to map
      grin >> gri.gri;
      gri.in_vgroup = false; // assume we're not part of a vgroup
      map[gri.gri.ref] = gri; // assign to map by ref
    }

    grin.close();

    // Build DAS
    AddHDFAttr(das, string("HDF_GLOBAL"), fileattrs);// add GR file attributes

    // add each GR's attrs
    vector<hdf_attr> pattrs;
    for (GRI g=map.begin(); g!=map.end(); ++g) {
        const hdf_gri *gri = &g->second.gri;
	// add GR attributes 
	AddHDFAttr(das, id2dods(gri->name), gri->attrs);

	// add palettes as attributes
	pattrs = Pals2Attrs(gri->palettes);
	AddHDFAttr(das, id2dods(gri->name), pattrs);

    }

    return;
}

// Read file annotations out of filename, put in attribute structure
static void FileAnnot_descriptions(DAS& das, const string& filename) {

    hdfistream_annot annotin(filename);
    vector<string> fileannots;
    
    annotin >> fileannots;
    AddHDFAttr(das, string("HDF_GLOBAL"), fileannots);

    annotin.close();
    return;
}

// add a vector of hdf_attr to a DAS
void AddHDFAttr(DAS& das, const string& varname, const vector<hdf_attr>& hav) {
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
    vector<string>attv;	// vector of attribute strings
    string attrtype;	// name of type of attribute
    for (int i=0; i<(int)hav.size(); ++i) { // for each attribute

	attrtype = DAPTypeName(hav[i].values.number_type());
	// get a vector of strings representing the values of the attribute
	attv = vector<string>(); // clear attv
	hav[i].values.print(attv); 
	
	// add the attribute and its values to the DAS
	for (int j=0; j<(int)attv.size(); ++j) {
	    // handle HDF-EOS metadata with separate parser
	    string container_name = hav[i].name;
	    if (container_name.find("StructMetadata") == 0
		|| container_name.find("CoreMetadata") == 0
		|| container_name.find("ProductMetadata") == 0
		|| container_name.find("ArchiveMetadata") == 0
		|| container_name.find("coremetadata") == 0
		|| container_name.find("productmetadata") == 0) {
	      uint32 dotzero = container_name.find('.');
	      if(dotzero != container_name.npos)
		container_name.erase(dotzero); // erase .0
	      AttrTable *at = das.get_table(container_name);
	      if (!at)
		at = das.add_table(container_name, new AttrTable);

	      hdfeos_scan_string(attv[j].c_str());  // tell lexer to scan attribute string
	      
	      parser_arg arg(at);
	      if (hdfeosparse((void *)&arg) != 0)
		cerr << "HDF-EOS parse error!\n";
	    } else {
	      if (attrtype == "String") 
		attv[j] = "\"" + escattr(attv[j]) + '"';
	      if (atp->append_attr(id2dods(hav[i].name), attrtype, attv[j]) == 0)
		THROW(dhdferr_addattr);
	    }
	}
    }	
    
    return;
}

// add a vector of annotations to a DAS.  They are stored as attributes.  They
// are encoded as string values of an attribute named "HDF_ANNOT".
void AddHDFAttr(DAS& das, const string& varname, const vector<string>& anv) {
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
    string an;
    for (int i=0; i<(int)anv.size(); ++i) { // for each annotation
	    an = "\"" + escattr(anv[i]) + "\""; // quote strings
	if (atp->append_attr(string("HDF_ANNOT"), "String", an) == 0)
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
	string palname;
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
			  (void *)palv[i].name.c_str(), palv[i].name.length());
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
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.name.c_str(),
			      dim.name.length());
    dattrs.push_back(dattr);
  }
  if (dim.label.length() != 0) {
    dattr.name = "long_name";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.label.c_str(),
			      dim.label.length());
    dattrs.push_back(dattr);
  }
  if (dim.unit.length() != 0) {
    dattr.name = "units";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.unit.c_str(),
			      dim.unit.length());    
    dattrs.push_back(dattr);
  }
  if (dim.format.length() != 0) {
    dattr.name = "format";
    dattr.values = hdf_genvec(DFNT_CHAR,(void *)dim.format.c_str(),
			      dim.format.length());    
    dattrs.push_back(dattr);
  }
  return dattrs;
}
