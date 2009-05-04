// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

/////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
//
//////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

// STL includes
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <functional>

using namespace std;

// Include this on linux to suppress an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
// HDF and HDFClass includes
#include <mfhdf.h>

#include "hcstream.h"
#include "hdfclass.h"
#include "hcerr.h"

// DODS includes
#include "DDS.h"
#include "DAS.h"
#include "escaping.h"
#include "debug.h"

// DODS/HDF includes
#include "dhdferr.h"
#include "HDFArray.h"
#include "HDFSequence.h"
#include "HDFTypeFactory.h"
#include "HDFGrid.h"
#include "dodsutil.h"
#include "hdf-dods.h"
#include "hdf-maps.h"
#include "parser.h"
#ifdef CF
#include "HDFEOS.h"		// <hyokyung 2009.04.10. 16:26:32>
#endif
// Glue routines declared in hdfeos.lex
void hdfeos_switch_to_buffer(void *new_buffer);
void hdfeos_delete_buffer(void * buffer);
void *hdfeos_string(const char *yy_str);
#ifdef USE_HDFEOS2
#include "HDFEOSGrid.h"
#include "HDFEOSArray.h"
#include "HDFEOSArray2D.h"
#include "HDFEOS2Array.h"
#include "HDFByte.h"
#include "HDFInt16.h"
#include "HDFUInt16.h"
#include "HDFInt32.h"
#include "HDFUInt32.h"
#include "HDFFloat32.h"
#include "HDFFloat64.h"
#endif

#if defined(CF) || defined(USE_HDFEOS2) 
HDFEOS eos;			// <hyokyung 2009.04.10. 16:26:34>
#endif

template < class T > string num2string(T n)
{
    ostringstream oss;
    oss << n;
    return oss.str();
}

struct yy_buffer_state;
yy_buffer_state *hdfeos_scan_string(const char *str);
extern int hdfeosparse(void *arg);      // defined in hdfeos.tab.c

void AddHDFAttr(DAS & das, const string & varname,
                const vector < hdf_attr > &hav);
void AddHDFAttr(DAS & das, const string & varname,
                const vector < string > &anv);

static void update_descriptions(const string & cachedir,
                                const string & filename);
static void build_descriptions(DDS & dds, DAS & das,
                               const string & filename);
static void SDS_descriptions(sds_map & map, DAS & das,
                             const string & filename);
static void Vdata_descriptions(vd_map & map, DAS & das,
                               const string & filename);
static void Vgroup_descriptions(DDS & dds, DAS & das,
                                const string & filename, sds_map & sdmap,
                                vd_map & vdmap, gr_map & grmap);
static void GR_descriptions(gr_map & map, DAS & das,
                            const string & filename);
static void FileAnnot_descriptions(DAS & das, const string & filename);
static vector < hdf_attr > Pals2Attrs(const vector < hdf_palette > palv);
static vector < hdf_attr > Dims2Attrs(const hdf_dim dim);

#ifdef CF
static void add_dimension_attributes(DAS &das);
#endif

#if defined(CF) || defined(USE_HDFEOS)
static void write_dimension_attributes_swath(DAS &das);
#endif

#ifdef USE_HDFEOS2
static void write_dimension_attributes_grid(DAS &das);
static void write_dimension_attributes_grid_non_ortho(DAS &das);
static void write_hdfeos2_grid(DDS &dds);
static void write_hdfeos2_grid_other_projection(DDS &dds);
static void write_hdfeos2_swath(DDS &dds);
#endif

// Generate cache filename.
string cache_name(const string & cachedir, const string & filename)
{
    // default behavior: if no cache dir, store cache file with HDF
    if (cachedir == "")
        return filename;

    string newname = filename;  // create a writable copy
    // skip over common path component (usually something like "/usr/local/www")
    string::size_type start = 0, dirstart = 0;
    while (newname[start] == cachedir[dirstart]) {
        start++;
        dirstart++;
    }
    // now backup to the last path separator
    while (start != 0 && newname[start - 1] != '/')
        start--;

    // turn the remaining path separators into "#"
    string::size_type slash = start;
    while ((slash = newname.find_first_of('/', slash)) != newname.npos) {
        newname[slash] = '#';
    }
    string retval = cachedir + "/" + newname.substr(start);
    return retval;              // return the new string
}

// Huh? What are these for... This code was designed to cache the DAS and DDS
// objects using the file system, which works well unless there's no way to
// write to the file system. In some cases, web daemons cannot do that. Since
// the code is written with the assumption that the DAS and DDS will be built
// and then cached, there's no easy way to build them separately (why would
// you need to? ... well there are lots of reasons). So these global static
// pointers are used to hold the last set of das and dds objects for the given
// filename. When cachedir in the read_das/dds() functions is empty, use these
// if possible.

// June 11, 2008 - pcw - These static vars and the static function
// save_state is used only to save off the last read das and dds.
// If the cache directory is empty, then we can't cache. The first thing that
// used to happen is we see if the last das/dds read is the one that we are
// reading in now. If it is, then use it, don't build a new one. Two things
// here:
// 1. In the CGI version, only one file is read in and the CGI server ends, so
// saving the last dds is pointless.
// 2. In the BES version, the last file read in could have been a part of a
// larger request with multiple files. So if the next request comes in with a
// new set of files, but that file is the first one, then it will use that dds,
// which we don't want to do.
// So ... we don't want to have save_state or the gs_dds or gs_filename static
// variables (or gs_das)

#if 0
static DDS *gs_dds = 0;
static DAS *gs_das = 0;
static string *gs_filename = 0;

static void
save_state(const string & filename, const DDS & dds, const DAS & das)
{
    if (!gs_filename)
        gs_filename = new string(filename);
    else
        *gs_filename = filename;

    // FIXME: the assignment below won't work if we have multiple containers
    if (!gs_dds)
        gs_dds = new DDS(dds);
    else
        *gs_dds = dds;

    // FIXME: the assignment below won't work if we have multiple containers
    if (!gs_das)
        gs_das = new DAS(das);
    else
        *gs_das = das;
}
#endif

// Read DDS from cache
void read_dds(DDS & dds, const string & cachedir, const string & filename)
{
#if  !defined(CF) || !defined(SHORT_NAME)
    if (!cachedir.empty()) {
        update_descriptions(cachedir, filename);

        string ddsfile = cache_name(cachedir, filename) + ".cdds";

	// create the type factory needed to read in hdf data
	HDFTypeFactory factory( filename ) ;
	dds.set_factory( &factory ) ;

        dds.parse(ddsfile);

	// set the factory back to null, we don't need the factory any more
	dds.set_factory( NULL  ) ;
    } else {
#if 0
	// see above comment from pcw
        if (gs_filename && filename == *gs_filename && gs_dds) {
            dds = *gs_dds;
        } else {
#endif
#endif                  
	// generate DDS, DAS

	// Throw away... w/o caching this code is very wasteful
	DAS das;

	dds.set_dataset_name(basename(filename));
	build_descriptions(dds, das, filename);

	if (!dds.check_semantics()) {       // DDS didn't get built right
	    dds.print(cerr);
	    THROW(dhdferr_ddssem);
	}

	//save_state(filename, dds, das);
#if 0
        }
#endif
#if !defined(CF) || !defined(SHORT_NAME)
    }
#endif
    return;
}

// Read DAS from cache
void read_das(DAS & das, const string & cachedir, const string & filename)
{
#ifndef SHORT_NAME  
    if (!cachedir.empty()) {
        update_descriptions(cachedir, filename);

        string dasfile = cache_name(cachedir, filename) + ".cdas";
        das.parse(dasfile);
    } else {
#if 0
	// see above comment from pcw
        if (gs_filename && filename == *gs_filename && gs_das) {
            das = *gs_das;
        } else {
#endif
#endif
	// generate DDS, DAS
	DDS dds(NULL);
	dds.set_dataset_name(basename(filename));
	build_descriptions(dds, das, filename);

	if (!dds.check_semantics()) {       // DDS didn't get built right
	    dds.print(cout);
	    THROW(dhdferr_ddssem);
	}

	//save_state(filename, dds, das);
#if 0
        }
#endif
#ifndef SHORT_NAME	
    }
#endif
    return;
}


// check dates of datafile and cached DDS, DAS; update cached files if necessary
static void update_descriptions(const string & cachedir,
                                const string & filename)
{
    // if cached version of DDS or DAS is nonexistent or out of date,
    // then regenerate DDS, DAS.
    Stat datafile(filename);
    Stat ddsfile((cache_name(cachedir, filename) + ".cdds"));
    Stat dasfile((cache_name(cachedir, filename) + ".cdas"));

    // flag error if could find filename
    if (!datafile)
        THROW(dhdferr_fexist);

    if (!ddsfile || !dasfile || (datafile.mtime() > ddsfile.mtime()) ||
        datafile.mtime() > dasfile.mtime()) {

        DDS dds(NULL);
        dds.set_dataset_name(basename(filename));
        DAS das;

        // generate DDS, DAS
        build_descriptions(dds, das, filename);
        if (!dds.check_semantics())     // DDS didn't get built right
            THROW(dhdferr_ddssem);


/* I dropped this file based bit:

        // output DDS, DAS to cache
        FILE *ddsout = fopen(ddsfile.filename(), "w");
        if (!ddsout)
            THROW(dhdferr_ddsout);
        dds.print(ddsout);
        fclose(ddsout);

and replaced it with this: */

        // output DDS to cache
        ofstream ddsout;
        ddsout.open (ddsfile.filename());
        dds.print(ddsout);
        ddsout.close();



/* I dropped this file based bit:

        FILE *dasout = fopen(dasfile.filename(), "w");
        if (!dasout)
            THROW(dhdferr_dasout);
        das.print(dasout);
        fclose(dasout);

and replaced it with this: */


        // output  DAS to cache
        ofstream dasout;
        dasout.open (dasfile.filename());
        das.print(dasout);
        dasout.close();


/* end - ndp */




    }
    return;
}

// Scan the HDF file and build the DDS and DAS
static void build_descriptions(DDS & dds, DAS & das,
                               const string & filename)
{
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
#ifdef USE_HDFEOS2
    // Build NC_GLOBAL part <hyokyung 2008.11.14. 08:18:50>
    if(eos.is_valid()){
      if(eos.is_grid()){
    	if(eos.is_orthogonal()){
	  write_dimension_attributes_grid(das);
	}
	else{
	  write_dimension_attributes_grid_non_ortho(das);
	} // if orthogonal
      } // if grid
      else if(eos.is_swath()) {
	   write_dimension_attributes_swath(das);
      }
      return;
    } // if valid
#endif
#ifdef CF
  // Build NC_GLOBAL part <hyokyung 2008.11.14. 08:18:50>
  if(eos.is_shared_dimension_set()){
    DBG(cerr << "CF generated NC_GLOBAL" << endl);
    if(!eos.is_swath())
      add_dimension_attributes(das);
    else{
      write_dimension_attributes_swath(das);
    }
  }
#endif
  
    return;
}

// These two Functor classes are used to look for EOS attributes with certain
// base names (is_named) and to accumulate values in in different hdf_attr
// objects with the same base names (accum_attr). These are used by
// merge_split_eos_attributes() to do just that. Some HDF EOS attributes are
// longer than HDF 4's 32,000 character limit. Those attributes are split up
// in the HDF 4 files and named `StructMetadata.0', `StructMetadata.1', et
// cetera. This code merges those attributes so that they can be processed
// correctly by the hdf eos attribute parser (see AddHDFAttr() further down
// in this file). 10/29/2001 jhrg

struct accum_attr
:public binary_function < hdf_genvec &, hdf_attr, hdf_genvec & > {

    string d_named;

     accum_attr(const string & named):d_named(named) {
    } hdf_genvec & operator() (hdf_genvec & accum, const hdf_attr & attr) {
        // Assume that all fields with the same base name should be combined,
        // and assume that they are in order.
        DBG(cerr << "attr.name: " << attr.name << endl);
        if (attr.name.find(d_named) != string::npos) {
            accum.append(attr.values.number_type(), attr.values.data(),
                         attr.values.size());
            return accum;
        } else {
            return accum;
        }
    }
};

struct is_named:public unary_function < hdf_attr, bool > {
    string d_named;

     is_named(const string & named):d_named(named) {
    } bool operator() (const hdf_attr & attr) {
        return (attr.name.find(d_named) != string::npos);
    }
};

static void
merge_split_eos_attributes(vector < hdf_attr > &attr_vec,
                           const string & attr_name)
{
    // Only do this if there's more than one part.
    if (count_if(attr_vec.begin(), attr_vec.end(), is_named(attr_name))
        > 1) {
        // Merge all split up parts named `attr_name.' Assume they are in
        // order in `attr_vec.'
        hdf_genvec attributes;
        attributes = accumulate(attr_vec.begin(), attr_vec.end(),
                                attributes, accum_attr(attr_name));

        // When things go south, check out the hdf_genvec...
        DBG2(vector < string > s_m;
             attributes.print(s_m);
             cerr << "Accum struct MD: (" << s_m.size() << ") "
             << s_m[0] << endl);

        // Remove all the parts that have been merged
        attr_vec.erase(remove_if(attr_vec.begin(), attr_vec.end(),
                                 is_named(attr_name)), attr_vec.end());

        // Make a new hdf_attr and assign it the newly merged attributes...
        hdf_attr merged_attr;
        merged_attr.name = attr_name;
        merged_attr.values = attributes;

        // And add it to the vector of attributes.
        attr_vec.push_back(merged_attr);
    }
}

// Read SDS's out of filename, build descriptions and put them into dds, das.
static void SDS_descriptions(sds_map & map, DAS & das,
                             const string & filename)
{

    hdfistream_sds sdsin(filename);
    sdsin.setmeta(true);

    // Read SDS file attributes attr_iter i = ;

    vector < hdf_attr > fileattrs;
    sdsin >> fileattrs;

    // Read SDS's
    sdsin.rewind();
    while (!sdsin.eos()) {
        sds_info sdi;           // add the next sds_info to map
        sdsin >> sdi.sds;
        sdi.in_vgroup = false;  // assume we're not part of a vgroup
        map[sdi.sds.ref] = sdi; // assign to map by ref
    }

    sdsin.close();

    // This is the call to combine SDS attributes that have been split up
    // into N 32,000 character strings. 10/24/2001 jhrg
    merge_split_eos_attributes(fileattrs, "StructMetadata");
    merge_split_eos_attributes(fileattrs, "CoreMetadata");
    merge_split_eos_attributes(fileattrs, "ProductMetadata");
    merge_split_eos_attributes(fileattrs, "ArchiveMetadata");
    merge_split_eos_attributes(fileattrs, "coremetadata");
    merge_split_eos_attributes(fileattrs, "productmetadata");

    // Build DAS, add SDS file attributes
    AddHDFAttr(das, string("HDF_GLOBAL"), fileattrs);
    // add each SDS's attrs
    vector < hdf_attr > dattrs;
    for (SDSI s = map.begin(); s != map.end(); ++s) {
        const hdf_sds *sds = &s->second.sds;
        AddHDFAttr(das, sds->name, sds->attrs);
        for (int k = 0; k < (int) sds->dims.size(); ++k) {
            dattrs = Dims2Attrs(sds->dims[k]);
            AddHDFAttr(das, sds->name + "_dim_" + num2string(k), dattrs);
        }
    }
    return;
}

// Read Vdata's out of filename, build descriptions and put them into dds.
static void Vdata_descriptions(vd_map & map, DAS & das,
                               const string & filename)
{
#ifdef USE_HDFEOS2
    eos.reset();
    if(eos.open((char*) filename.c_str()) < 0){
      cerr << "Not a valid EOS " << endl;
    }
    else{
     eos.print();
      //eos.eos2->_deleteme();
    }
#endif	

    hdfistream_vdata vdin(filename);
    vdin.setmeta(true);

    // Read Vdata's
    while (!vdin.eos()) {
        vd_info vdi;            // add the next vd_info to map
        vdin >> vdi.vdata;
        vdi.in_vgroup = false;  // assume we're not part of a vgroup
        map[vdi.vdata.ref] = vdi;       // assign to map by ref
    }
    vdin.close();

    // Build DAS
    vector < hdf_attr > dattrs;
    for (VDI s = map.begin(); s != map.end(); ++s) {
        const hdf_vdata *vd = &s->second.vdata;
        AddHDFAttr(das, vd->name, vd->attrs);
    }

    return;
}

// Read Vgroup's out of filename, build descriptions and put them into dds.
static void Vgroup_descriptions(DDS & dds, DAS & das,
                                const string & filename, sds_map & sdmap,
                                vd_map & vdmap, gr_map & grmap)
{
  hdfistream_vgroup vgin(filename);

  // Read Vgroup's
  vg_map vgmap;
  while (!vgin.eos()) {
    vg_info vgi;            // add the next vg_info to map
    vgin >> vgi.vgroup;     // read vgroup itself
    vgi.toplevel = true;    // assume toplevel until we prove otherwise
    vgmap[vgi.vgroup.ref] = vgi;    // assign to map by vgroup ref
  }
  vgin.close();
  // for each Vgroup
  for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) {
    const hdf_vgroup *vg = &v->second.vgroup;

    // Add Vgroup attributes
    AddHDFAttr(das, vg->name, vg->attrs);

    // now, assign children
    for (uint32 i = 0; i < vg->tags.size(); i++) {
      int32 tag = vg->tags[i];
      int32 ref = vg->refs[i];
      switch (tag) {
      case DFTAG_VG:
	// Could be a GRI or a Vgroup
	if (grmap.find(ref) != grmap.end())
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
#ifndef SHORT_NAME	
	cerr << "unknown tag: " << tag << " ref: " << ref << endl;
#endif	
	break;
      }	// switch (tag) 
    } //     for (uint32 i = 0; i < vg->tags.size(); i++) 
  } //   for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) 
#ifdef USE_HDFEOS2
  if(eos.is_valid()){
    // Check if the file is Grid or Swath.
    if(eos.is_grid()){
      if(eos.is_orthogonal())
	write_hdfeos2_grid(dds);
      else
	write_hdfeos2_grid_other_projection(dds);
    }
    if(eos.is_swath()){
      write_hdfeos2_swath(dds);
    }
  }
  else{
    // eos.eos2->_deleteme();
#endif
      // Build DDS for all toplevel vgroups
      BaseType *pbt = 0;
      for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) {
	if (!v->second.toplevel)
	  continue;           // skip over non-toplevel vgroups
#ifndef CF		
	pbt =
	  NewStructureFromVgroup(v->second.vgroup, vgmap, sdmap, vdmap,
				 grmap, filename);
	if (pbt != 0) {
	  dds.add_var(pbt);
	  delete pbt;
	}	
#endif
#ifdef CF
	NewStructureFromVgroupEOS(v->second.vgroup, vgmap, sdmap, vdmap,
				  grmap, filename, dds);
#endif	
      } // for (VGI v = vgmap.begin(); v != vgmap.end(); ++v)

      // add lone SDS's
      for (SDSI s = sdmap.begin(); s != sdmap.end(); ++s) {
        if (s->second.in_vgroup)
	  continue;           // skip over SDS's in vgroups
        if (s->second.sds.has_scale())  // make a grid
	  pbt = NewGridFromSDS(s->second.sds, filename);
        else
	  pbt = NewArrayFromSDS(s->second.sds, filename);
        if (pbt != 0) {
	  dds.add_var(pbt);
	  delete pbt;
	}
      }

      // add lone Vdata's
      for (VDI v = vdmap.begin(); v != vdmap.end(); ++v) {
        if (v->second.in_vgroup)
	  continue;           // skip over Vdata in vgroups
        pbt = NewSequenceFromVdata(v->second.vdata, filename);
        if (pbt != 0) {
	  dds.add_var(pbt);
	  delete pbt;
	}
      }

      // add lone GR's
      for (GRI g = grmap.begin(); g != grmap.end(); ++g) {
        if (g->second.in_vgroup)
	  continue;           // skip over GRs in vgroups
        pbt = NewArrayFromGR(g->second.gri, filename);
        if (pbt != 0) {
	  dds.add_var(pbt);
	  delete pbt ;
	}
      }
#ifdef USE_HDFEOS2
  }
#endif
}

static void GR_descriptions(gr_map & map, DAS & das,
                            const string & filename)
{

    hdfistream_gri grin(filename);
    grin.setmeta(true);

    // Read GR file attributes
    vector < hdf_attr > fileattrs;
    grin >> fileattrs;

    // Read general rasters
    grin.rewind();
    while (!grin.eos()) {
        gr_info gri;            // add the next gr_info to map
        grin >> gri.gri;
        gri.in_vgroup = false;  // assume we're not part of a vgroup
        map[gri.gri.ref] = gri; // assign to map by ref
    }

    grin.close();

    // Build DAS
    AddHDFAttr(das, string("HDF_GLOBAL"), fileattrs);   // add GR file attributes

    // add each GR's attrs
    vector < hdf_attr > pattrs;
    for (GRI g = map.begin(); g != map.end(); ++g) {
        const hdf_gri *gri = &g->second.gri;
        // add GR attributes
        AddHDFAttr(das, gri->name, gri->attrs);

        // add palettes as attributes
        pattrs = Pals2Attrs(gri->palettes);
        AddHDFAttr(das, gri->name, pattrs);

    }

    return;
}

// Read file annotations out of filename, put in attribute structure
static void FileAnnot_descriptions(DAS & das, const string & filename)
{

    hdfistream_annot annotin(filename);
    vector < string > fileannots;

    annotin >> fileannots;
    AddHDFAttr(das, string("HDF_GLOBAL"), fileannots);

    annotin.close();
    return;
}

// add a vector of hdf_attr to a DAS
void AddHDFAttr(DAS & das, const string & varname,
                const vector < hdf_attr > &hav)
{
    if (hav.size() == 0)        // nothing to add
        return;

    // get pointer to the AttrTable for the variable varname (create one if
    // necessary)
    AttrTable *atp = das.get_table(varname);
    if (atp == 0) {
        atp = new AttrTable;
#if 0
        // jhrg 4/1/2009
        if (atp == 0)
            THROW(hcerr_nomemory);
#endif
        atp = das.add_table(varname, atp);
    }
    // add the attributes to the DAS
    vector < string > attv;     // vector of attribute strings
    string attrtype;            // name of type of attribute
    for (int i = 0; i < (int) hav.size(); ++i) {        // for each attribute

        attrtype = DAPTypeName(hav[i].values.number_type());
        // get a vector of strings representing the values of the attribute
        attv = vector < string > ();    // clear attv
        hav[i].values.print(attv);

        // add the attribute and its values to the DAS
        for (int j = 0; j < (int) attv.size(); ++j) {
            // handle HDF-EOS metadata with separate parser
            string container_name = hav[i].name;
            if (container_name.find("StructMetadata") == 0
                || container_name.find("CoreMetadata") == 0
                || container_name.find("ProductMetadata") == 0
                || container_name.find("ArchiveMetadata") == 0
                || container_name.find("coremetadata") == 0
                || container_name.find("productmetadata") == 0) {
                string::size_type dotzero = container_name.find('.');
                if (dotzero != container_name.npos)
                    container_name.erase(dotzero);      // erase .0
#ifndef CF // <hyokyung 2008.12. 1. 11:20:24>						
                AttrTable *at = das.get_table(container_name);
                if (!at)
                    at = das.add_table(container_name, new AttrTable);

                // tell lexer to scan attribute string
                void *buf = hdfeos_string(attv[j].c_str());

                parser_arg arg(at);
                if (hdfeosparse(static_cast < void *>(&arg)) != 0
                    || arg.status() == false)
                    cerr << "HDF-EOS parse error!\n";
                hdfeos_delete_buffer(buf);
#endif
#ifdef CF
	// Parse one more time <hyokyung 2008.11.10. 15:38:36>
	if (container_name.find("StructMetadata") == 0){
	  eos.reset();
	  DBG(cerr << "=AddHDFAttr() container_name=" << container_name  << endl);
	  bool result = eos.parse_struct_metadata(attv[j].c_str());
#ifndef USE_HDFEOS2  // Without this, it causes a conflict <hyokyung 2009.04.24. 13:31:53>
	  if(result)
	    eos.set_dimension_array(); 
#endif	  
	  DBG(eos.print());
	}		
#endif
        }
             else {
                if (attrtype == "String")
                    attv[j] = "\"" + escattr(attv[j]) + "\"";
                if (atp->append_attr(hav[i].name, attrtype, attv[j]) == 0)
                    THROW(dhdferr_addattr);
            }
        }
    }

    return;
}

// add a vector of annotations to a DAS.  They are stored as attributes.  They
// are encoded as string values of an attribute named "HDF_ANNOT".
void AddHDFAttr(DAS & das, const string & varname,
                const vector < string > &anv)
{
    if (anv.size() == 0)        // nothing to add
        return;

    // get pointer to the AttrTable for the variable varname (create one if
    // necessary)
    AttrTable *atp = das.get_table(varname);
    if (atp == 0) {
        atp = new AttrTable;
#if 0
        // jhrg 4/1/2009
        if (atp == 0)
            THROW(hcerr_nomemory);
#endif
        atp = das.add_table(varname, atp);
    }
    // add the annotations to the DAS
    string an;
    for (int i = 0; i < (int) anv.size(); ++i) {        // for each annotation
        an = "\"" + escattr(anv[i]) + "\"";     // quote strings
        if (atp->append_attr(string("HDF_ANNOT"), "String", an) == 0)
            THROW(dhdferr_addattr);
    }

    return;
}

// Add a vector of palettes as attributes to a GR.  Each palette is added as
// two attributes: the first contains the palette data; the second contains
// the number of components in the palette.
static vector < hdf_attr > Pals2Attrs(const vector < hdf_palette > palv)
{
    vector < hdf_attr > pattrs;

    if (palv.size() != 0) {
        // for each palette create an attribute with the palette inside, and an
        // attribute containing the number of components
        hdf_attr pattr;
        string palname;
        for (int i = 0; i < (int) palv.size(); ++i) {
            palname = "hdf_palette_" + num2string(i);
            pattr.name = palname;
            pattr.values = palv[i].table;
            pattrs.push_back(pattr);
            pattr.name = palname + "_ncomps";
            pattr.values = hdf_genvec(DFNT_INT32,
                                      const_cast <
                                      int32 * >(&palv[i].ncomp), 1);
            pattrs.push_back(pattr);
            if (palv[i].name.length() != 0) {
                pattr.name = palname + "_name";
                pattr.values = hdf_genvec(DFNT_CHAR,
                                          const_cast <
                                          char *>(palv[i].name.c_str()),
                                          palv[i].name.length());
                pattrs.push_back(pattr);
            }
        }
    }
    return pattrs;
}

// Convert the meta information in a hdf_dim into a vector of
// hdf_attr.
static vector < hdf_attr > Dims2Attrs(const hdf_dim dim)
{
    vector < hdf_attr > dattrs;
    hdf_attr dattr;
    if (dim.name.length() != 0) {
        dattr.name = "name";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.name.c_str()),
                       dim.name.length());
        dattrs.push_back(dattr);
    }
    if (dim.label.length() != 0) {
        dattr.name = "long_name";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.label.c_str()),
                       dim.label.length());
        dattrs.push_back(dattr);
    }
    if (dim.unit.length() != 0) {
        dattr.name = "units";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.unit.c_str()),
                       dim.unit.length());
        dattrs.push_back(dattr);
    }
    if (dim.format.length() != 0) {
        dattr.name = "format";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.format.c_str()),
                       dim.format.length());
        dattrs.push_back(dattr);
    }
    return dattrs;
}

#ifdef CF
/// An abstract respresntation of DAP String type.
static const char STRING[] = "String";
/// An abstract respresntation of DAP Byte type.
static const char BYTE[] = "Byte";
/// An abstract respresntation of DAP Int32 type.
static const char INT32[] = "Int32";
/// An abstract respresntation of DAP Int16 type.
static const char INT16[] = "Int16";
/// An abstract respresntation of DAP Float64 type.
static const char FLOAT64[] = "Float64";
/// An abstract respresntation of DAP Float32 type.
static const char FLOAT32[] = "Float32";
/// An abstract respresntation of DAP Uint16 type.
static const char UINT16[] = "UInt16";
/// An abstract respresntation of DAP UInt32 type.
static const char UINT32[] = "UInt32";
/// For umappable HDF5 integer data types.
static const char INT_ELSE[] = "Int_else";
/// For unmappable HDF5 float data types.
static const char FLOAT_ELSE[] = "Float_else";
/// An abstract respresntation of DAP Structure type.
static const char COMPOUND[] = "Structure";
/// An abstract respresntation of DAP Array type.
static const char ARRAY[] = "Array";   
/// An abstract respresntation of DAP Url type.
static const char URL[] = "Url";


// <hyokyung 2008.11.13. 14:59:16>
static void add_dimension_attributes(DAS & das)
{

  AttrTable *at;

  at = das.add_table("NC_GLOBAL", new AttrTable);
  at->append_attr("title", STRING, "\"NASA EOS Aura Grid\"");
  at->append_attr("Conventions", STRING, "\"COARDS, GrADS\"");
  at->append_attr("dataType", STRING, "\"Grid\"");
  at->append_attr("history", STRING,
		  "\"Tue Jan 1 00:00:00 CST 2008 : imported by GrADS Data Server 1.3\"");

  at = das.add_table("lon", new AttrTable);
  at->append_attr("grads_dim", STRING, "\"x\"");
  at->append_attr("grads_mapping", STRING, "\"linear\"");
  at->append_attr("grads_size", STRING, "\"360\"");
  at->append_attr("units", STRING, "\"degrees_east\"");
  at->append_attr("long_name", STRING, "\"longitude\"");
  at->append_attr("minimum", FLOAT32, "-180.0");
  at->append_attr("maximum", FLOAT32, "180.0");
  at->append_attr("resolution", FLOAT32, "1.00");

  at = das.add_table("lat", new AttrTable);
  at->append_attr("grads_dim", STRING, "\"y\"");
  at->append_attr("grads_mapping", STRING, "\"linear\"");
  at->append_attr("grads_size", STRING, "\"180\"");
  at->append_attr("units", STRING, "\"degrees_north\"");
  at->append_attr("long_name", STRING, "\"latitude\"");
  at->append_attr("minimum", FLOAT32, "-90.0");
  at->append_attr("maximum", FLOAT32, "90.0");
  at->append_attr("resolution", FLOAT32, "1.00");

  at = das.add_table("lev", new AttrTable);
  at->append_attr("grads_dim", STRING, "\"z\"");
  at->append_attr("grads_mapping", STRING, "\"linear\"");
  at->append_attr("grads_size", STRING, "\"15\"");
  at->append_attr("units", STRING, "\"level\"");
  at->append_attr("long_name", STRING,
		  "\"level converted from nCandidate\"");


}
#endif

#ifdef USE_HDFEOS2
#ifndef CF
/// An abstract respresntation of DAP String type.
static const char STRING[] = "String";
/// An abstract respresntation of DAP Byte type.
static const char BYTE[] = "Byte";
/// An abstract respresntation of DAP Int32 type.
static const char INT32[] = "Int32";
/// An abstract respresntation of DAP Int16 type.
static const char INT16[] = "Int16";
/// An abstract respresntation of DAP Float64 type.
static const char FLOAT64[] = "Float64";
/// An abstract respresntation of DAP Float32 type.
static const char FLOAT32[] = "Float32";
/// An abstract respresntation of DAP Uint16 type.
static const char UINT16[] = "UInt16";
/// An abstract respresntation of DAP UInt32 type.
static const char UINT32[] = "UInt32";
/// For umappable HDF5 integer data types.
static const char INT_ELSE[] = "Int_else";
/// For unmappable HDF5 float data types.
static const char FLOAT_ELSE[] = "Float_else";
/// An abstract respresntation of DAP Structure type.
static const char COMPOUND[] = "Structure";
/// An abstract respresntation of DAP Array type.
static const char ARRAY[] = "Array";   
/// An abstract respresntation of DAP Url type.
static const char URL[] = "Url";
#endif

static void write_dimension_attributes_grid(DAS & das)
{

    AttrTable *at;

    at = das.add_table("NC_GLOBAL", new AttrTable);
    at->append_attr("title", STRING, "\"NASA EOS Grid\"");
    at->append_attr("Conventions", STRING, "\"COARDS, GrADS\"");
    at->append_attr("dataType", STRING, "\"Grid\"");


    at = das.add_table("lon", new AttrTable);
    at->append_attr("grads_dim", STRING, "\"x\"");
    at->append_attr("grads_mapping", STRING, "\"linear\"");
    {
      std::ostringstream o;
      o  << "\"" << eos.get_xdim_size() << "\"";
      at->append_attr("grads_size", STRING, o.str().c_str());
    }
    at->append_attr("units", STRING, "\"degrees_east\"");
    at->append_attr("long_name", STRING, "\"longitude\"");
    {
      std::ostringstream o;
      o << (eos.point_left / 1000000.0);      
      at->append_attr("minimum", FLOAT32, o.str().c_str());
    }
    {
      std::ostringstream o;
      o << (eos.point_right / 1000000.0);
      at->append_attr("maximum", FLOAT32, o.str().c_str());
    }    
    {
      std::ostringstream o;
      o << (eos.gradient_x / 1000000.0);
      at->append_attr("resolution", FLOAT32, o.str().c_str());
    }

    at = das.add_table("lat", new AttrTable);
    at->append_attr("grads_dim", STRING, "\"y\"");
    at->append_attr("grads_mapping", STRING, "\"linear\"");
    {
      std::ostringstream o;
      o  << "\"" << eos.get_ydim_size() << "\"";
      at->append_attr("grads_size", STRING, o.str().c_str());
    }
    at->append_attr("units", STRING, "\"degrees_north\"");
    at->append_attr("long_name", STRING, "\"latitude\"");
    {
      std::ostringstream o;
      o << (eos.point_lower / 1000000.0);      
      at->append_attr("minimum", FLOAT32, o.str().c_str());
    }
    
    {
      std::ostringstream o;
      o << (eos.point_upper / 1000000.0);            
      at->append_attr("maximum", FLOAT32, o.str().c_str());
    }
    
    {
      std::ostringstream o;
      o << (eos.gradient_y / 1000000.0);
      at->append_attr("resolution", FLOAT32, o.str().c_str());      
    }    
}



static void write_dimension_attributes_grid_non_ortho(DAS & das)
{
  AttrTable *at;

  at = das.add_table("NC_GLOBAL", new AttrTable);
  at->append_attr("title", STRING, "\"NASA EOS Aura Grid - Non-Ortho Projection\"");
  at->append_attr("Conventions", STRING, "\"CF-1.0\"");
  
  at = das.add_table("lon", new AttrTable);
  at->append_attr("units", STRING, "\"degrees_east\"");
  at->append_attr("long_name", STRING, "\"longitude\"");
  
  at = das.add_table("lat", new AttrTable);
  at->append_attr("units", STRING, "\"degrees_north\"");
  at->append_attr("long_name", STRING, "\"latitude\"");
  at->append_attr("coordinates", STRING, "\"lon lat\"");
  
}

static BaseType*  get_base_type(int type, string name)
{
  BaseType* bt = NULL;
    switch(type){
    case dods_byte_c:
      {	
	bt = new HDFByte(name,name);
	break;
      }
    case dods_int16_c:
      {
	bt = new HDFInt16(name,name);
	break;
      }
    case dods_uint16_c:
      {
	bt = new HDFUInt16(name,name);
	break;
      }
    case dods_int32_c:
      {
	bt = new HDFInt32(name,name);
	break;
      }
    case dods_uint32_c:
      {
	bt = new HDFUInt32(name,name);
	break;
      }      
    case dods_float32_c:
      {
	bt = new HDFFloat32(name,name);
	break;
      }
    case dods_float64_c:
      {
	bt = new HDFFloat64(name,name);
	break;
      }
    default:
      break;
	  
    }
    return bt;
}


static void write_hdfeos2_grid(DDS &dds)
{
    
  // Add all Grid variables.
  if (!eos.is_shared_dimension_set()) {
    
    int j;
    BaseType *bt = 0;
    Array *ar = 0;    
    vector < string > dimension_names;
    eos.get_all_dimensions(dimension_names);

    for(j=0; j < dimension_names.size(); j++){
      
      int shared_dim_size = eos.get_dimension_size(dimension_names.at(j));    
      string str_cf_name = eos.get_CF_name((char*)dimension_names.at(j).c_str());
      bt = new HDFFloat32(str_cf_name, str_cf_name);
      ar = new HDFEOSArray(str_cf_name, str_cf_name, bt);

      ar->add_var(bt);
      delete bt; bt = 0;
      ar->append_dim(shared_dim_size, str_cf_name);
      dds.add_var(ar);
      delete ar; ar = 0;

    } // for
    
    if(j > 0)      // Set the flag for "shared dimension" true.
      eos.set_shared_dimension();
  }


  BaseType *gr = 0;
  for (int i = 0; i < (int) eos.full_data_paths.size(); i++) {
    vector <string> tokens;
    
    // Split based on the type.
    BaseType* bt = get_base_type(eos.get_data_type(i), eos.full_data_paths.at(i));
    if(bt == 0){
      cerr
	<< "write_hdfeos2_grid(): Creating a DAP type class failed."
	<< " Name=" << eos.full_data_paths.at(i)	  
	<< " Type=" << eos.get_data_type(i)
	<<  endl;
      return;
    }
    // Build array first.
    HDFEOS2Array *ar = new HDFEOS2Array(eos.full_data_paths.at(i),bt);
    delete bt;
    eos.get_dimensions(eos.full_data_paths.at(i), tokens);
    ar->set_numdim(tokens.size());
    gr = new HDFEOSGrid(eos.full_data_paths.at(i),eos.full_data_paths.at(i));
    if(gr == 0){
      cerr << "Creating a HDFEOSGrid class failed." << endl;
      return;
    }
    else {
      for (int dim_index = 0; dim_index < tokens.size(); dim_index++) {
	DBG(cerr << "=read_objects_base_type():Dim name " <<
	    tokens.at(dim_index) << endl);

	string str_dim_name = tokens.at(dim_index);
	int dim_size = eos.get_dimension_size(str_dim_name);
	str_dim_name = eos.get_CF_name((char*)str_dim_name.c_str());
	BaseType *bt = 0;
	Array *ar2 = 0;
	try {
	  bt = new HDFFloat32(str_dim_name, str_dim_name);
	  ar2 = new HDFArray(str_dim_name, str_dim_name, bt);
	  delete bt; bt = 0;

	  ar2->append_dim(dim_size, str_dim_name);
	  ar->append_dim(dim_size, str_dim_name);

	  gr->add_var(ar2, maps);
	  delete ar2; ar2 = 0;
	    
	}
	catch (...) {
	  if( bt ) delete bt;
	  if( ar2 ) delete ar2;
	  throw;
	}
      }
	  
      gr->add_var(ar, array);
      dds.add_var(gr);
    }
  }
}

static void write_hdfeos2_grid_other_projection(DDS &dds)
{
  // Non geographic projection handling is similar to Grid case
  // but do not generate DAP "Grid" output.
  // Use the plain array for datasets.
  // Add 2-D shared dimension la/lon array.
  DBG(cerr << ">write_hdfeos2_grid_other_projection()" << endl);
  // Add all variables.
  if (!eos.is_shared_dimension_set()) {
    int j;
    BaseType *bt = 0;
    Array *ar = 0;    
    vector < string > dimension_names;
    eos.get_all_dimensions(dimension_names);

    int xdim_size = eos.get_xdim_size();
    int ydim_size = eos.get_ydim_size();
    
    for(j=0; j < dimension_names.size(); j++){
      int shared_dim_size = eos.get_dimension_size(dimension_names.at(j));    
      string str_cf_name = eos.get_CF_name((char*)dimension_names.at(j).c_str());
      bt = new HDFFloat32(str_cf_name,str_cf_name);
      ar = new HDFEOSArray2D(str_cf_name, bt); // <hyokyung 2009.02.18. 16:10:17>

      ar->add_var(bt);
      delete bt; bt = 0;
      if(str_cf_name == "lat"){
	if(eos.is_ydimmajor()){
	  DBG(cerr << "=write_hdfeos2_grid_other_projection():YDim major is detected." << endl);
	  ar->append_dim(ydim_size, str_cf_name);
	  ar->append_dim(xdim_size, "lon");
	}
	else{
	  ar->append_dim(xdim_size, "lon");
	  ar->append_dim(ydim_size, str_cf_name);

	}
      }
      else if(str_cf_name == "lon"){
	if(eos.is_ydimmajor()){
	  ar->append_dim(ydim_size, "lat");	  
	  ar->append_dim(xdim_size, str_cf_name);
	}
	else{
	  ar->append_dim(xdim_size, str_cf_name);
	  ar->append_dim(ydim_size, "lat");	  
	}
      }
      else{
	ar->append_dim(shared_dim_size, str_cf_name);
      }
      dds.add_var(ar);
      delete ar; ar = 0;
      // Set the flag for "shared dimension" true.
    }
    if(j > 0)
      eos.set_shared_dimension();
  }

  
  for (int i = 0; i < (int) eos.full_data_paths.size(); i++) {
    DBG(cerr
	<< " Name=" << eos.full_data_paths.at(i)	  
	<< " Type=" << eos.get_data_type(i)
	<<  endl);

    const char* cf_name = eos.get_CF_name((char*)eos.full_data_paths.at(i).c_str());
    string cf_varname(cf_name);
    DBG(cerr << "CF name=" << cf_varname << endl);    
    vector <string> tokens;
    
    // Build the type first.
    // BaseType *bt = 0;
    // Split based on the type.
    BaseType* bt = get_base_type(eos.get_data_type(i), cf_varname);
    if(bt == 0){
      cerr
	<< "Creating a DAP type class failed."
	<< " Name=" << cf_varname	  
	<< " Type=" << eos.get_data_type(i)
	<<  endl;
      return;
    }
    // Build array first.
    HDFEOS2Array *ar = new HDFEOS2Array(cf_varname,bt);

    delete bt;

    // Add dimensions
    eos.get_dimensions(eos.full_data_paths.at(i), tokens);
    ar->set_numdim(tokens.size());        
    for (int dim_index = 0; dim_index < tokens.size(); dim_index++) {
      DBG(cerr << "=read_objects_base_type():Dim name " <<
	  tokens.at(dim_index) << endl);

      string str_dim_name = tokens.at(dim_index);
      int dim_size = eos.get_dimension_size(str_dim_name);
      str_dim_name = eos.get_CF_name((char*)str_dim_name.c_str());
      ar->append_dim(dim_size, str_dim_name);
    }

    dds.add_var(ar);
    
  } 
    
}

static void write_hdfeos2_swath(DDS &dds)
{
  // Swath handling is similar to Grid case but do not generate DAP "Grid" output.
  // Use the plain array.

  for (int i = 0; i < (int) eos.full_data_paths.size(); i++) {
    DBG(cerr
	<< " Name=" << eos.full_data_paths.at(i)	  
	<< " Type=" << eos.get_data_type(i)
	<<  endl);

    const char* cf_name = eos.get_CF_name((char*)eos.full_data_paths.at(i).c_str());
    string cf_varname(cf_name);
    DBG(cerr << "CF name=" << cf_varname << endl);    
    vector <string> tokens;
    
    // Split based on the type.
    BaseType *bt = get_base_type(eos.get_data_type(i), cf_varname);
    if(bt == 0){
      cerr
	<< "Creating a DAP type class failed."
	<< " Name=" << cf_varname	  
	<< " Type=" << eos.get_data_type(i)
	<<  endl;
      return;
    }
    // Build array first.
    HDFEOS2Array *ar = new HDFEOS2Array(cf_varname,bt);

    delete bt;

    // Add dimensions
    eos.get_dimensions(eos.full_data_paths.at(i), tokens);
    ar->set_numdim(tokens.size());        
    for (int dim_index = 0; dim_index < tokens.size(); dim_index++) {
      DBG(cerr << "=read_objects_base_type():Dim name " <<
	  tokens.at(dim_index) << endl);

      string str_dim_name = tokens.at(dim_index);
      int dim_size = eos.get_dimension_size(str_dim_name);
      str_dim_name = eos.get_CF_name((char*)str_dim_name.c_str());
      ar->append_dim(dim_size, str_dim_name);
    }

    dds.add_var(ar);
    
  } 
    
}


#endif

#if defined(CF) || defined(USE_HDFEOS)
static void write_dimension_attributes_swath(DAS & das)
{

  AttrTable *at;
  
  // Let's try IDV without NC_GLOBAL to see it's required.  <hyokyung 2009.02.11. 12:08:52>
  at = das.add_table("NC_GLOBAL", new AttrTable);
  at->append_attr("title", STRING, "\"NASA EOS Swath\"");
  at->append_attr("Conventions", STRING, "\"CF-1.0\"");

  at = das.add_table("lon", new AttrTable);
  at->append_attr("units", STRING, "\"degrees_east\"");
  at->append_attr("long_name", STRING, "\"longitude\"");

  at = das.add_table("lat", new AttrTable);
  at->append_attr("units", STRING, "\"degrees_north\"");
  at->append_attr("long_name", STRING, "\"latitude\"");
  at->append_attr("coordinates", STRING, "\"lon lat\"");
  // For all swaths, insert the coordinates attribute if lat, lon dimension names match.
  
  at = das.get_table("Time");
  if(at != NULL){
    at->append_attr("units", STRING, "\"degrees_north\"");
    at->append_attr("long_name", STRING, "\"floating-point TAI qelapsed seconds since Jan 1, 193\"");
    at->append_attr("coordinates", STRING, "\"lon lat\"");
  }

}
#endif
