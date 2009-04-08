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

// Glue routines declared in hdfeos.lex
void hdfeos_switch_to_buffer(void *new_buffer);
void hdfeos_delete_buffer(void * buffer);
void *hdfeos_string(const char *yy_str);

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
    }

    return;
}

// Read DAS from cache
void read_das(DAS & das, const string & cachedir, const string & filename)
{
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
    }

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
                cerr << "unknown tag: " << tag << " ref: " << ref << endl;
            }
        }
    }

    // Build DDS for all toplevel vgroups
    BaseType *pbt = 0;
    for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) {
        if (!v->second.toplevel)
            continue;           // skip over non-toplevel vgroups
        pbt =
            NewStructureFromVgroup(v->second.vgroup, vgmap, sdmap, vdmap,
                                   grmap, filename);
        if (pbt != 0) {
            dds.add_var(pbt);
	    delete pbt;
        }
    }

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
            } else {
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
