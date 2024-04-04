///////////////////////////////////////////////////////////////////////////////
/// \file hdfdesc.cc
/// \brief DAP attributes and structure description generation code.
///
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// The code includes the support of HDF-EOS2 and NASA HDF4 files that follow CF.
// Copyright (c) The HDF Group.
// Author: Kent Yang <myang6@hdfgroup.org>
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
//
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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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

#include "config.h"
#include "config_hdf.h"

#include <cstdio>
#include <cassert>
#include <cmath>
#include <libgen.h>

// STL includes
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <functional>
#include <unordered_set>
#include <set>


// Include this on linux to suppress an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <iomanip>              
#include <cerrno>


// HDF and HDFClass includes
#include <mfhdf.h>

// DODS includes
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>

#include <libdap/DDS.h>
#include <libdap/DAS.h>
#include <libdap/escaping.h>
#include <libdap/parser.h>
#include <libdap/InternalErr.h>
#include <libdap/debug.h>

#include <BESDebug.h>
#include <BESLog.h>

#include "HDF4RequestHandler.h"
// DODS/HDF includes for the default option only
#include "hcstream.h"
#include "hdfclass.h"
#include "hcerr.h"
#include "dhdferr.h"
#include "HDFArray.h"
#include "HDFSequence.h"
#include "HDFTypeFactory.h"
#include "HDFGrid.h"
#include "dodsutil.h"
#include "hdf-maps.h"

// DAP2 doesn't have signed char type, the signed char will be converted to int32 with this macro.
#define SIGNED_BYTE_TO_INT32 1

// HDF datatype headers for both the default and the CF options
#include "HDFByte.h"
#include "HDFInt8.h"
#include "HDFInt16.h"
#include "HDFUInt16.h"
#include "HDFInt32.h"
#include "HDFUInt32.h"
#include "HDFFloat32.h"
#include "HDFFloat64.h"
#include "HDFStr.h"

// Routines that handle SDS and Vdata attributes for the HDF-EOS2 objects in a hybrid HDF-EOS2 file for the CF option
#include "HE2CF.h"

// HDF4 for the CF option(EOS2 will treat as pure HDF4 objects if the HDF-EOS2 library is not configured in)
#include "HDFSP.h"
#include "HDFSPArray_RealField.h"
#include "HDFSPArrayGeoField.h"
#include "HDFSPArrayMissField.h"
#include "HDFSPArrayAddCVField.h"    
#include "HDFSPArray_VDField.h"
#include "HDFCFStrField.h"
#include "HDFCFStr.h"
#include "HDFCFUtil.h"

// HDF-EOS2 (including the hybrid) will be handled as HDF-EOS2 objects if the HDF-EOS2 library is configured in
#ifdef USE_HDFEOS2_LIB
#include "HDFEOS2.h"
#include "HDFEOS2Array_RealField.h"
#include "HDFEOS2ArrayGridGeoField.h"
#include "HDFEOS2ArraySwathGeoField.h"
#include "HDFEOS2ArrayMissField.h"
#include "HDFEOS2ArraySwathDimMapField.h"
#include "HDFEOS2ArraySwathGeoMultiDimMapField.h"
#include "HDFEOS2ArraySwathGeoDimMapExtraField.h"
#include "HDFEOS2CFStr.h"
#include "HDFEOS2CFStrField.h"
#include "HDFEOS2HandleType.h"
#endif

using namespace std;
using namespace libdap;

// Added 5/7/09; This bug (#1163) was fixed in July 2008 except for this
// handler. jhrg
#define ATTR_STRING_QUOTE_FIX

template < class T > string num2string(T n)
{
    ostringstream oss;
    oss << n;
    return oss.str();
}

// Glue routines declared in hdfeos.lex
void  hdfeos_switch_to_buffer(void *new_buffer);
void  hdfeos_delete_buffer(void * buffer);
void *hdfeos_string(const char *yy_str);

struct yy_buffer_state;
yy_buffer_state *hdfeos_scan_string(const char *str);
extern int hdfeosparse(libdap::parser_arg *arg);      // defined in hdfeos.tab.c

typedef struct eos2_grid {
    bool oned_latlon;
    int32 ydim;
    int32 xdim;
    string grid_name;

}eos2_grid_t;

// Functions for the default option
void AddHDFAttr(DAS & das, const string & varname,
                const vector < hdf_attr > &hav);
void AddHDFAttr(DAS & das, const string & varname,
                const vector < string > &anv);

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

// Start the declaration of HDF4 to DMR direct mapping functions
// 1. dmr and root level object handlings
void read_dmr(DMR *dmr, const string &filename);
void read_sd_attrs(D4Group *root_grp, int32 fileid, int32 sdfd);
void handle_sds_dims(D4Group *root_grp, int32 fileid, int32 sdfd);
void read_lone_sds(D4Group *root_grp, int32 file_id, int32 sdfd, const string &filename);
void read_lone_vdata(D4Group *root_grp, int32 file_id, int32 sdfd, const string &filename);
void read_dmr_vlone_groups(D4Group *root_grp, int32 file_id, int32 sdfd, const string &filename);

// 2. vgroup handlings
void convert_vgroup_objects(int32 vgroup_id,int32 file_id, int32 sdfd, D4Group* d4g, const string &vgroupname,const string & filename, bool is_eos2_grid);
void convert_vgroup_attrs(int32 vgroup_id,D4Group* d4g, const string &vgroupname);
void map_vgroup_attr(D4Group *d4g, const string &dap4_attrname,int32 attr_type, int32 attr_count, vector<char> & attr_value);
bool reserved_vgroups(const vector<char> &vgroup_class);

// 3. SDS handlings
#if 0
void vgroup_convert_sds_objects(int32 vgroup_id,int32 file_id,int32 sdfd,D4Group* d4g,const string& filename);
#endif
void convert_sds(int32 file_id, int32 sdfd,int32 vgroup_id, int32 obj_ref,  D4Group* d4g,const string &filename, bool is_eos2_grid);
void obtain_all_sds_refs(int32 file_id, int32 sdfd, unordered_set<int32>& sds_ref);
void exclude_all_sds_refs_in_vgroups(int32 file_id, int32 sdfd, unordered_set<int32>&sds_ref);
void exclude_sds_refs_in_vgroup(int32 file_id, int32 sdfd, int32 vgroup_id, unordered_set<int32>&sds_ref);
void map_sds_var_dap4_attrs(HDFArray *ar, int32 sds_id, int32 obj_ref, int32 n_sds_attrs);
void map_sds_vdata_attr(BaseType *d4b, const string &attr_name,int32 attr_type, int32 attr_count, vector<char> & attr_value);

// 4. Vdata handlings
void convert_vdata(int32 fileid, int32 sdfd, int32 vgroup_id,int32 obj_ref ,D4Group* d4g,const string& filename);
void map_vdata_to_dap4_structure_array(int32 vdata_id, int32 num_elms, int32 nflds, int32 obj_ref, D4Group *d4g, const string &filename);
void map_vdata_to_dap4_atomic_array(int32 vdata_id, int32 num_elms, int32 obj_ref, D4Group *d4g, const string &filename);
void map_vdata_to_dap4_attrs(HDFArray *ar, int32 vdata_id, int32 obj_ref);

// 5. HDF-EOS2 handlings
int is_group_eos2_grid(const string& vgroup_name, vector<eos2_grid_t>& eos2_grid_lls);
void add_eos2_latlon(D4Group *d4_grp, const eos2_grid_t &eos2_grid, const string&filename);

// 6. Helper functions
void add_obj_ref_attr(BaseType * d4b, bool is_sds, int32 obj_ref);
BaseType * gen_dap_var(int32 h4_type, const string & h4_str, const string & filename);
D4AttributeType h4type_to_dap4_attrtype(int32 h4_type);
string print_dap4_attr(int32 type, int loc, void *vals);
void close_vgroup_fileids(int32 fileid, int32 sdfd, int32 vgroup_id);
void add_var_dap4_attr(BaseType *d4_var,const string& attr_name, D4AttributeType attr_type, const string& attr_value);

// End of HDF4 to DMR direct mapping functions.


void read_das(DAS & das, const string & filename);
void read_dds(DDS & dds, const string & filename);

// For the CF option
// read_dds for HDF4 files. Some NASA non-eos2 HDF4 products are handled specifially to follow the CF conventions.
bool read_dds_hdfsp(DDS & dds, const string & filename,int32 sdfd, int32 fileid,const HDFSP::File*h4file);
bool read_das_hdfsp(DAS & das, const string & filename,int32 sdfd, int32 fileid,HDFSP::File**h4filepptr);

// read_dds for special NASA HDF-EOS2 hybrid(non-EOS2) objects
bool read_dds_hdfhybrid(DDS & dds, const string & filename,int32 sdfd, int32 fileid,const HDFSP::File*h4file);
bool read_das_hdfhybrid(DAS & das, const string & filename,int32 sdfd, int32 fileid,HDFSP::File**h4filepptr);

// Functions to read special 1-d HDF-EOS2 grid. This grid can be built up quickly.
bool read_dds_special_1d_grid(DDS &dds, const HDFSP::File *spf, const string & filename,int32 sdfd,bool can_cache);
bool read_das_special_eos2(DAS &das,const string & filename,int32 sdid, int32 fileid,bool ecs_metadata,HDFSP::File**h4filepptr);
bool read_das_special_eos2_core(DAS &das, const HDFSP::File *spf, const string & filename,bool ecs_metadata);

void read_das_sds(DAS & das, const string & filename,int32 sdfd, bool ecs_metadata,HDFSP::File**h4fileptr);
void read_dds_sds(DDS &dds, const string & filename,int32 sdfd, HDFSP::File*h4file,bool dds_set_cache);

void change_das_mod08_scale_offset(DAS & das, const HDFSP::File *spf);

// Functions to handle SDS fields for the CF option.
void read_dds_spfields(DDS &dds,const string& filename,const int sdfd,const HDFSP::SDField *spsds, SPType sptype); 

// Functions to handle Vdata fields for the CF option.
void read_dds_spvdfields(DDS &dds,const string& filename, const int fileid,int32 vdref, int32 numrec,HDFSP::VDField *spvd); 

// Check if this is a special HDF-EOS2 file that can be handled by HDF4 directly. Using HDF4 only can improve performance.
int check_special_eosfile(const string&filename,string&grid_name,int32 sdfd);

// The following blocks only handle HDF-EOS2 objects based on HDF-EOS2 libraries.
#ifdef USE_HDFEOS2_LIB

bool obtain_eos2_gd_ll_info(const string & fname, const string &grid_name, int32 &ydim, int32 &xdim, bool & oned_ll);
bool check_eos2_grids(const string &filename, int32 sdfd,vector<eos2_grid_t>& eos2_grid_lls);

// Parse HDF-EOS2's ECS metadata(coremetadata etc.)
void parse_ecs_metadata(DAS &das,const string & metaname, const string &metadata); 

// read_dds for HDF-EOS2
// We find some special HDF-EOS2(MOD08_M3) products that provides coordinate variables
// without using the dimension scales. We will handle this in a special way.
// So change the return value of read_dds_hdfeos2 to represent different cases
// 0: general non-EOS2 pure HDF4
// 1: HDF-EOS2 hybrid 
// 2: MOD08_M3
//  HDF-EOS2 but no need to use HDF-EOS2 lib: no real dimension scales but have CVs for every dimension, treat differently
// 3: AIRS version 6 
//  HDF-EOS2 but no need to use HDF-EOS2 lib: 
//  have dimension scales but don’t have CVs for every dimension, also need to condense dimensions, treat differently
// 4: Expected AIRS level 3 or level 2 
//  HDF-EOS2 but no need to use HDF-EOS2 lib: Have dimension scales for all dimensions
// 5: MERRA 
//  Special handling for MERRA file 
int read_dds_hdfeos2(DDS & dds, const string & filename,int32 sdfd, int32 gridfd, int32 swathfd,const HDFSP::File*h4file,HDFEOS2::File*eosfile);

// reas das for HDF-EOS2
int read_das_hdfeos2(DAS & das, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,bool ecs_metadata,HDFSP::File**h4filepptr,HDFEOS2::File**eosfilepptr);

// read_dds for one grid or swath
void read_dds_hdfeos2_grid_swath(DDS &dds, const string&filename, HDFEOS2::Dataset *dataset, int grid_or_swath,
                                 bool ownll, SOType sotype,bool multi_dmap,int32 sdfd, int32 gridfd,int32 swathfd)
{

    BESDEBUG("h4","Coming to read_dds_hdfeos2_grid_swath "<<endl);

    // grid_or_swath - 0: grid, 1: swath
    if (grid_or_swath < 0 ||  grid_or_swath > 1)
        throw InternalErr(__FILE__, __LINE__, "The current type should be either grid or swath");

    /// I. This section retrieve Swath dimension map info.

    // Declare dim. map entry. The defination of dimmap_entry can be found at HDFCFUtil.h. 
    vector<struct dimmap_entry> dimmaps;

    //The extra dim map file name(lat/lon of swath with dim. map can be found in a separate HDF file.
    string modis_geofilename="";
    bool geofile_has_dimmap = false;
    
    // 1. Obtain dimension map info and stores the info. to dimmaps. 
    // 2. Check if a MODIS swath geo-location HDF-EOS2 file exists for the dimension map case of MODIS Swath
    if (grid_or_swath == 1) 
        HDFCFUtil::obtain_dimmap_info(filename,dataset,dimmaps,modis_geofilename,geofile_has_dimmap);

    /// End of section I.


    /// Section II. Add swath geo-location fields to the data fields for Swath.
    const vector<HDFEOS2::Field*>& fields = dataset->getDataFields(); 
    vector<HDFEOS2::Field*> all_fields = fields;
    vector<HDFEOS2::Field*>::const_iterator it_f;
 
    if (1 == grid_or_swath) {
        auto sw = static_cast<HDFEOS2::SwathDataset *>(dataset);
        const vector<HDFEOS2::Field*>geofields = sw->getGeoFields();
        for (it_f = geofields.begin(); it_f != geofields.end(); it_f++) 
            all_fields.push_back(*it_f);
    }
    /// End of Section II.

    /// Section III. Generate DDS and pass DDS info. to Data.
    for(it_f = all_fields.begin(); it_f != all_fields.end(); it_f++)
    {	
        BESDEBUG("h4","New field Name " <<(*it_f)->getNewName()<<endl);

        BaseType *bt=nullptr;

        // Whether the field is real field,lat/lon field or missing Z-dimension field
        int fieldtype = (*it_f)->getFieldType();

        // Check if the datatype needs to be changed. This is for MODIS data that needs to apply scale and offset.
        // ctype_field_namelist is assigned in the function read_das_hdfeos2. 
        bool changedtype = false;
        for (auto const &cf:ctype_field_namelist){
            if (cf == (*it_f)->getNewName()){
                changedtype = true;
                break;
            } 
        }
                   
        switch((*it_f)->getType())
        {

#define HANDLE_CASE2(tid, type) \
    case tid: \
        if(true == changedtype && fieldtype==0) \
            bt = new (HDFFloat32) ((*it_f)->getNewName(), (dataset)->getName()); \
        else \
            bt = new (type)((*it_f)->getNewName(), (dataset)->getName()); \
        break;

#define HANDLE_CASE(tid, type)\
    case tid: \
        bt = new (type)((*it_f)->getNewName(), (dataset)->getName()); \
        break;
            HANDLE_CASE(DFNT_FLOAT32, HDFFloat32)
            HANDLE_CASE(DFNT_FLOAT64, HDFFloat64)
            HANDLE_CASE(DFNT_CHAR8,HDFStr)
#ifndef SIGNED_BYTE_TO_INT32
            HANDLE_CASE2(DFNT_INT8, HDFByte)
#else
            HANDLE_CASE2(DFNT_INT8,HDFInt32)
#endif
            HANDLE_CASE2(DFNT_UINT8, HDFByte)
            HANDLE_CASE2(DFNT_INT16, HDFInt16)
            HANDLE_CASE2(DFNT_UINT16,HDFUInt16)
            HANDLE_CASE2(DFNT_INT32, HDFInt32)
            HANDLE_CASE2(DFNT_UINT32, HDFUInt32)
            HANDLE_CASE2(DFNT_UCHAR8, HDFByte)
            default:
                throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
#undef HANDLE_CASE2
        }

        if(bt)
        {
            const vector<HDFEOS2::Dimension*>& dims= (*it_f)->getCorrectedDimensions();
            vector<HDFEOS2::Dimension*>::const_iterator it_d;

            // Char array maps to DAP string.
            if(DFNT_CHAR == (*it_f)->getType()) {

                if((*it_f)->getRank() >1) {

                    HDFEOS2CFStrField * ar = nullptr;

                    try {

                        ar = new HDFEOS2CFStrField(
                                                   (*it_f)->getRank() -1,
                                                   (grid_or_swath ==0)?gridfd:swathfd,
                                                   filename,
                                                   dataset->getName(),
                                                   (*it_f)->getName(),
                                                   grid_or_swath,
                                                   (*it_f)->getNewName(),
                                                   bt);
                    }
                    catch(...) {
                        delete bt;
                        throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStr instance.");
                    }
                    for(it_d = dims.begin(); it_d != dims.begin()+dims.size()-1; it_d++){
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                    }
                
                    dds.add_var(ar);
                    delete bt;
                    if(ar != nullptr) 
                       delete ar;

                }

                else {
                    HDFEOS2CFStr * sca_str = nullptr;
                    try {

                        sca_str = new HDFEOS2CFStr(
                                                   (grid_or_swath ==0)?gridfd:swathfd,
                                                   filename,
                                                   dataset->getName(),
                                                   (*it_f)->getName(),
                                                   (*it_f)->getNewName(),
                                                   grid_or_swath);
                    }
                    catch(...) {
                        delete bt;
                        throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStr instance.");
                    }
                    dds.add_var(sca_str);
                    delete bt;
                    delete sca_str; 
                }
 
            }

            // For general variables and non-lat/lon existing coordinate variables
            else if(fieldtype == 0 || fieldtype == 3 || fieldtype == 5) {

                // grid 
                if(grid_or_swath==0){
                    HDFEOS2Array_RealField *ar = nullptr;
                    ar = new HDFEOS2Array_RealField(
                        (*it_f)->getRank(),
                        filename,false,sdfd,gridfd,
                        dataset->getName(), "", (*it_f)->getName(),
                        sotype,
                        (*it_f)->getNewName(), bt);
                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                    dds.add_var(ar);
                    delete bt;
                    delete ar;
                }
                // swath
                else if(grid_or_swath==1){

                    string tempfieldname = (*it_f)->getName();

                    // This swath uses the dimension map, but not the multi-dim. map we can handle.
                    if((*it_f)->UseDimMap() && false == multi_dmap) {

                        // We also find that a separate geolocation file exists
                        if (!modis_geofilename.empty()) {

                            // This field can be found in the geo-location file. The field name may be corrected.
                            if (true == HDFCFUtil::is_modis_dimmap_nonll_field(tempfieldname)) {

                                if(false == geofile_has_dimmap) {

                                    // Here we have to use HDFEOS2Array_RealField since the field may
                                    // need to apply scale and offset equation.
                                    // MODIS geolocation swath name is always MODIS_Swath_Type_GEO.
                                    // We can improve the handling of this by not hard-coding the swath name
                                    // in the future. KY 2012-08-16
                                    HDFEOS2Array_RealField *ar = nullptr;
                                    ar = new HDFEOS2Array_RealField(
                                                                    (*it_f)->getRank(),
                                                                    modis_geofilename,
                                                                    true,
                                                                    sdfd,
                                                                    swathfd,
                                                                    "", 
                                                                    "MODIS_Swath_Type_GEO", 
                                                                    tempfieldname,
                                                                    sotype,
                                                                    (*it_f)->getNewName(), 
                                                                    bt);

                                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                                    dds.add_var(ar);
                                    delete bt;
                                    delete ar;
                                }
                                else {// Use dimension maps in the dimension map file

                                    HDFEOS2ArraySwathDimMapField * ar = nullptr;

                                    // SET dimmaps to empty. 
                                    // This is the key since we are using the geolocation file for the new information.
                                    // The dimension map info. will be obtained when the data is reading. KY 2013-03-13

                                    dimmaps.clear();
                                    ar = new HDFEOS2ArraySwathDimMapField(
                                                                          (*it_f)->getRank(), 
                                                                          modis_geofilename, 
                                                                          true,
                                                                          sdfd,
                                                                          swathfd,
                                                                          "", 
                                                                          "MODIS_Swath_Type_GEO", 
                                                                          tempfieldname, 
                                                                          dimmaps,
                                                                          sotype,
                                                                          (*it_f)->getNewName(),
                                                                          bt);
                                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                                    dds.add_var(ar);
                                    delete bt;
                                    delete ar;
                                }
                            }
                            else { // This field cannot be found in the dimension map file.

                                HDFEOS2ArraySwathDimMapField * ar = nullptr;
 
                                // Even if the dimension map file exists, it only applies to some
                                // specific data fields, if this field doesn't belong to these fields,
                                // we should still apply the dimension map rule to these fields.

                                ar = new HDFEOS2ArraySwathDimMapField(
                                                                      (*it_f)->getRank(),
                                                                      filename,
                                                                      false,
                                                                      sdfd,
                                                                      swathfd,
                                                                      "",
                                                                      dataset->getName(),
                                                                      tempfieldname,
                                                                      dimmaps,
                                                                      sotype,
                                                                      (*it_f)->getNewName(),
                                                                      bt);

                                for (it_d = dims.begin(); it_d != dims.end(); it_d++)
                                    ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                                dds.add_var(ar);
                                delete bt;
                                delete ar;
                            }
                        }
                        else {// No dimension map file

                            HDFEOS2ArraySwathDimMapField * ar = nullptr;
                            ar = new HDFEOS2ArraySwathDimMapField(
                                                                  (*it_f)->getRank(), 
                                                                  filename, 
                                                                  false,
                                                                  sdfd,
                                                                  swathfd,
                                                                  "", 
                                                                  dataset->getName(), 
                                                                  tempfieldname, 
                                                                  dimmaps,
                                                                  sotype,
                                                                  (*it_f)->getNewName(),
                                                                  bt);
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);
                            delete bt;
                            delete ar;
                        }
                    }
                    else { // No dimension map

                        HDFEOS2Array_RealField * ar = nullptr;
                        ar = new HDFEOS2Array_RealField(
                                                        (*it_f)->getRank(),
                                                        filename,
                                                        false,
                                                        sdfd,
                                                        swathfd,
                                                        "", 
                                                        dataset->getName(), 
                                                        tempfieldname,
                                                        sotype,
                                                        (*it_f)->getNewName(), 
                                                        bt);
                        for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                            ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                        dds.add_var(ar);
                        delete bt;
                        delete ar;
                    }
                }
                else {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The current type should be either grid or swath");
                }
            }

            // For latitude and longitude
            else if(fieldtype == 1 || fieldtype == 2) {

                // For grid
                if (grid_or_swath==0) {

                    HDFEOS2ArrayGridGeoField *ar = nullptr;
                    bool ydimmajor = (*it_f)->getYDimMajor();
                    bool condenseddim = (*it_f)->getCondensedDim();
                    bool speciallon = (*it_f)->getSpecialLon();
                    int  specialformat = (*it_f)->getSpecialLLFormat();

                    ar = new HDFEOS2ArrayGridGeoField(
                                                     (*it_f)->getRank(),
                                                     fieldtype,
                                                     ownll,
                                                     ydimmajor,
                                                     condenseddim,
                                                     speciallon,
                                                     specialformat,
                                                     /*fieldcache,*/
                                                     filename,
                                                     gridfd,
                                                     dataset->getName(),
                                                     (*it_f)->getName(),
                                                     (*it_f)->getNewName(), 
                                                     bt);
			    
                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                    dds.add_var(ar);
                    delete bt;
                    delete ar;
                }

                // We encounter a very special MODIS case (MOD/MYD ATML2 files),
                // Latitude and longitude fields are located under data fields.
                // So include this case. KY 2010-7-12
                // We also encounter another special case(MOD11_L2.A2012248.2355.041.2012249083024.hdf),
                // the latitude/longitude with dimension maps is under the "data fields".
                // So we have to consider this. KY 2012-09-24
                       
                else if(grid_or_swath ==1) {

                    if (true == multi_dmap) {
                        if ((*it_f)->getRank() !=2) 
                            throw InternalErr(__FILE__, __LINE__, "For the multi-dimmap case, the field rank must be 2.");
                        int dim0size = (dims[0])->getSize();
                        int dim1size = (dims[1])->getSize();
                        int dim0offset = (*it_f)->getLLDim0Offset();
                        int dim1offset = (*it_f)->getLLDim1Offset();
                        int dim0inc = (*it_f)->getLLDim0Inc();
                        int dim1inc = (*it_f)->getLLDim1Inc();
                        string fieldname;
                        if(fieldtype == 1)
                            fieldname = "Latitude";
                        else
                            fieldname = "Longitude";
#if 0                        
cerr<<"hdfdesc: newfieldname is "<<(*it_f)->getNewName() <<endl;
cerr<<"hdfdesc: dim0size "<<dim0size <<endl;
cerr<<"hdfdesc: dim1size "<<dim1size <<endl;
cerr<<"hdfdesc: dim0offset "<<dim0offset <<endl;
cerr<<"hdfdesc: dim1offset "<<dim1offset <<endl;
cerr<<"hdfdesc: dim0inc "<<dim0inc <<endl;
cerr<<"hdfdesc: dim1inc "<<dim1inc <<endl;
#endif

                        HDFEOS2ArraySwathGeoMultiDimMapField * ar = nullptr;

                        ar = new HDFEOS2ArraySwathGeoMultiDimMapField(
                                                        (*it_f)->getRank(),
                                                        filename,
                                                        swathfd,
                                                        dataset->getName(),
                                                        fieldname, 
                                                        dim0size,
                                                        dim0offset,
                                                        dim0inc,
                                                        dim1size,
                                                        dim1offset,
                                                        dim1inc,
                                                        (*it_f)->getNewName(), 
                                                        bt);

                        for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                            ar->append_dim((*it_d)->getSize(), (*it_d)->getName());

                        dds.add_var(ar);
                        delete bt;
                        delete ar;
                    }
                    else {

                        // Use Swath dimension map
                        if((*it_f)->UseDimMap()) {

                            // Have an extra HDF-EOS file for latitude and longtiude
                            if(!modis_geofilename.empty()) {

                                if (false == geofile_has_dimmap) {
                                    HDFEOS2ArraySwathGeoDimMapExtraField *ar = nullptr;
                                    ar = new HDFEOS2ArraySwathGeoDimMapExtraField(
                                                                              (*it_f)->getRank(),
                                                                              modis_geofilename,
                                                                              (*it_f)->getName(),
                                                                              (*it_f)->getNewName(),
                                                                              bt);
                                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                                    dds.add_var(ar);
                                    delete bt;
                                    delete ar;
                                }
                                else {
                                
                                    HDFEOS2ArraySwathDimMapField * ar = nullptr;

                                    // SET dimmaps to empty. 
                                    // This is essential since we are using the geolocation file for the new information. 
                                    // The dimension map info. will be obtained when the data is read. KY 2013-03-13
                                    dimmaps.clear();
                                    ar = new HDFEOS2ArraySwathDimMapField(
                                                                      (*it_f)->getRank(), 
                                                                      modis_geofilename, 
                                                                      true,
                                                                      sdfd,
                                                                      swathfd,
                                                                      "", 
                                                                      "MODIS_Swath_Type_GEO", 
                                                                      (*it_f)->getName(), 
                                                                      dimmaps,
                                                                      sotype,
                                                                      (*it_f)->getNewName(),
                                                                      bt);
                                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());

                                    dds.add_var(ar);
                                    delete bt;
                                    delete ar;
                                }
                            }
                            // Will interpolate by the handler
                            else {

                                HDFEOS2ArraySwathDimMapField * ar = nullptr;
                                ar = new HDFEOS2ArraySwathDimMapField(
                                                                      (*it_f)->getRank(), 
                                                                      filename, 
                                                                      false,
                                                                      sdfd,
                                                                      swathfd,
                                                                      "", 
                                                                      dataset->getName(), 
                                                                      (*it_f)->getName(), 
                                                                      dimmaps,
                                                                      sotype,
                                                                      (*it_f)->getNewName(),
                                                                      bt);
                                for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                    ar->append_dim((*it_d)->getSize(), (*it_d)->getName());

                                dds.add_var(ar);
                                delete bt;
                                delete ar;
                            }
                        }
                        else {// No Dimension map
 
                            HDFEOS2ArraySwathGeoField * ar = nullptr;
                            ar = new HDFEOS2ArraySwathGeoField(
                                                           (*it_f)->getRank(),
                                                           filename,
                                                           swathfd,
                                                           dataset->getName(), 
                                                           (*it_f)->getName(),
                                                           (*it_f)->getNewName(), 
                                                           bt);

                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);
                            delete bt;
                            delete ar;
                        }
                    }
                }
                else {
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The current type should be either grid or swath");
                }
 
            }
                    
            //Missing Z dimensional field
            else if(fieldtype == 4) { 

                if((*it_f)->getRank()!=1){
                    delete bt;
                    throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                }

                int nelem = ((*it_f)->getCorrectedDimensions()[0])->getSize();
                HDFEOS2ArrayMissGeoField *ar = nullptr;
                ar = new HDFEOS2ArrayMissGeoField(
                                                  (*it_f)->getRank(),
                                                  nelem,
                                                  (*it_f)->getNewName(),
                                                  bt);

                for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                    ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
 
                dds.add_var(ar);
                delete bt;
                delete ar;
            }
            else {
                delete bt;
                throw InternalErr(__FILE__, __LINE__, "Encounter unsupported datatype or The field type should be between 0 and 5. ");
            }

        }
    }

}

// Build DDS for HDF-EOS2 only.
int read_dds_hdfeos2(DDS & dds, const string & filename,int32 sdfd, int32 gridfd, int32 swathfd,const HDFSP::File*spf,HDFEOS2::File*f) 
{

    BESDEBUG("h4","Coming to read_dds_hdfeos2 "<<endl);

    // Set DDS dataset.
    dds.set_dataset_name(basename(filename));

    // There are some HDF-EOS2 files(MERRA) that should be treated
    // exactly like HDF4 SDS files. We don't need to use HDF-EOS2 APIs to 
    // retrieve any information. In fact, treating them as HDF-EOS2 files
    // will cause confusions and we may get wrong information.
    // A quick fix is to check if the file name contains MERRA. KY 2011-3-4 
    // Find MERRA data, return 5, then just use HDF4 SDS code.
    if ((basename(filename).size() >=5) && ((basename(filename)).compare(0,5,"MERRA")==0)) 
        return 5; 

    if (true == HDF4RequestHandler::get_enable_special_eos()) {

        string grid_name;
        int ret_val = check_special_eosfile(filename,grid_name,sdfd); 

        // These are AIRS-like products that use HDF4 SDS dimension scale perfectly.
        // We originally thought that the AIRS version 6 products fall into this category, so we added this case.
        // However, the current AIRS version 6 products still miss some dimension scales. So currently we don't 
        // find any products that support this case. Leave it for future use. KY 2015-06-03
        if (4== ret_val) 
            return ret_val;
    

        // Case 2 or 3 are MOD08M3 or AIRS version 6
        if (2 == ret_val || 3 == ret_val) {

            try {
                read_dds_special_1d_grid(dds,spf,filename,sdfd,false);
            } catch (...)
            {   
                throw;
            }   
                return ret_val;
        }

    }

    // Special HDF-EOS2 file, doesn't use HDF-EOS2 file structure. so 
    // the file pointer passed from DAS is Null. return 0.
    if (f == nullptr)
        return 0;
        
    //Some grids have one shared lat/lon pair. For this case,"onelatlon" is true.
    // Other grids have their individual grids. We have to handle them differently.
    // ownll is the flag to distinguish "one lat/lon pair" and multiple lat/lon pairs.
    const vector<HDFEOS2::GridDataset *>& grids = f->getGrids();
    bool ownll = false;
    bool onelatlon = f->getOneLatLon();

    // Set scale and offset type to the DEFAULT one.
    SOType sotype = SOType::DEFAULT_CF_EQU;

    // Iterate all the grids of this file and map them to DAP DDS.
    for (const auto &gd:grids){

        // Check if this grid provides its own lat/lon.
        ownll = onelatlon?onelatlon:gd->getLatLonFlag();

        // Obtain Scale and offset type. This is for MODIS products who use non-CF scale/offset rules.
        sotype = gd->getScaleType();
        try {
            read_dds_hdfeos2_grid_swath(
                dds, filename, static_cast<HDFEOS2::Dataset*>(gd), 0,ownll,sotype,false,sdfd,gridfd,swathfd);
            // Add 1-D CF grid projection required coordinate variables. 
            // Currently only supports sinusoidal projection.
            HDFCFUtil::add_cf_grid_cvs(dds,gd);
        }
        catch(...) {
            throw;
        }
    }

    // Obtain the multi dimmap flag.
    bool multi_dmap = f->getMultiDimMaps();


    // Iterate all the swaths of this file and map them to DAP DDS.
    const vector<HDFEOS2::SwathDataset *>& swaths= f->getSwaths();
    for (const auto &swath:swaths) {

        // Obtain Scale and offset type. This is for MODIS products who use non-CF scale/offset rules.
        sotype = swath->getScaleType();
        try {
            //No global lat/lon for multiple swaths
            read_dds_hdfeos2_grid_swath(
                dds, filename, static_cast<HDFEOS2::Dataset*>(swath), 1,false,sotype,multi_dmap,sdfd,gridfd,swathfd);
        }
        catch(...) {
            throw;
        }
    }

    // Clear the field name list of which the datatype is changed. KY 2012-8-1
    // ctype_field_namelist is a global vector(see HDFEOS2HandleType.h for more description)
    // Since the handler program is a continuously running service, this values of this global vector may
    // change from one file to another. So clearing this vector each time when mapping DDS is over.
    ctype_field_namelist.clear();

    return 1;
}


// The wrapper of building DDS of non-EOS fields and attributes in a hybrid HDF-EOS2 file.
//bool read_dds_hdfhybrid(DDS & dds, const string & filename,int32 sdfd, int32 fileid,int32 gridfd,int32 swathfd)
bool read_dds_hdfhybrid(DDS & dds, const string & filename,int32 sdfd, int32 fileid,const HDFSP::File*f)

{

    BESDEBUG("h4","Coming to read_dds_hdfhybrid "<<endl);

    // Set DDS dataset.
    dds.set_dataset_name(basename(filename));

    // Obtain non-EOS SDS fields.
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    // Read SDS 
    for(const auto &sdfield:spsds){
        try {
            read_dds_spfields(dds,filename,sdfd,sdfield,f->getSPType());
        }
        catch(...) {
            throw;
        }
    }

    // Read Vdata fields.
    // To speed up the performance for CERES data, we turn off some CERES vdata fields.

    // Many MODIS and MISR products use Vdata intensively. To make the output CF compliant, we map 
    // each vdata field to a DAP array. However, this may cause the generation of many DAP fields. So
    // we use the BES keys for users to turn on/off as they choose. By default, the key is turned on. KY 2012-6-26
    
    if (true == HDF4RequestHandler::get_enable_hybrid_vdata()) {

        for (const auto &vd:f->getVDATAs()) {
            if (false == vd->getTreatAsAttrFlag()){
                for (const auto &vdf:vd->getFields()) {
                    try {
                        read_dds_spvdfields(dds,filename,fileid, vd->getObjRef(),vdf->getNumRec(),vdf); 
                    }
                    catch(...) {
                        throw;
                    }
                }
            }
        }
    }
        
    return true;
}


// Build DAS for non-EOS objects in a hybrid HDF-EOS2 file.
bool read_das_hdfhybrid(DAS & das, const string & filename,int32 sdfd, int32 fileid,HDFSP::File**fpptr)
{

    BESDEBUG("h4","Coming to read_das_hdfhybrid "<<endl); 
    // Declare a non-EOS file pointer
    HDFSP::File *f = nullptr;
    try {
        // Read non-EOS objects in a hybrid HDF-EOS2 file.
        f = HDFSP::File::Read_Hybrid(filename.c_str(), sdfd,fileid);
    } 
    catch (HDFSP::Exception &e)
    {
        if (f!=nullptr)
            delete f;
        throw InternalErr(e.what());
    }

    // Remember the file pointer 
    *fpptr = f;

    // First Added non-HDFEOS2 SDS attributes.
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    for (const auto &spfield:spsds) {
        
        // Use CF field name as the DAS table name.
        AttrTable *at = das.get_table(spfield->getNewName());
        if (!at)
            at = das.add_table(spfield->getNewName(), new AttrTable);

        // Some fields have "long_name" attributes,so we have to use this attribute rather than creating our own "long_name"
        bool long_name_flag = false;

        for (const auto &attr:spfield->getAttributes()) {       

            if(attr->getName() == "long_name") {
                long_name_flag = true;
                break;
            }
        }
        
        if(false == long_name_flag) 
            at->append_attr("long_name", "String", spfield->getName());
        
        // Map all attributes to DAP DAS.
        for (const auto& attr:spfield->getAttributes()) {

            // Handle string first.
            if(attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){

                // Questionable use of string. KY 2014-02-12
                string tempstring2(attr->getValue().begin(),attr->getValue().end());
                auto tempfinalstr= string(tempstring2.c_str());

                // We want to escape the possible special characters except the fullpath attribute. 
                // The fullpath is only added for some CERES and MERRA data. People use fullpath to keep their
                // original names even their original name includes special characters.  KY 2014-02-12
                // The function to escape the special characters is moved to the libdap. So the handler just needs to pass the attribute values.
                // KY 2023-10-5
#if 0
                at->append_attr(attr->getNewName(), "String" , (attr->getNewName()=="fullpath")?tempfinalstr:HDFCFUtil::escattr(tempfinalstr));
#endif
                at->append_attr(attr->getNewName(), "String" , tempfinalstr);
            }
            else {
                for (int loc=0; loc < attr->getCount() ; loc++) {
                    string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                    at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                }
            }
        }

        // Check if having _FillValue. If having _FillValue, compare the datatype of _FillValue
        // with the variable datatype. Correct the fillvalue datatype if necessary. 
        if(at != nullptr) {
            int32 var_type = spfield->getType();
            try {
                HDFCFUtil::correct_fvalue_type(at,var_type);
            }
            catch(...) {
                throw;
            }
        }

        // If H4.EnableCheckScaleOffsetType BES key is true,  
        //    if yes, check if having scale_factor and add_offset attributes; 
        //       if yes, check if scale_factor and add_offset attribute types are the same; 
        //          if no, make add_offset's datatype be the same as the datatype of scale_factor. 
        // (CF requires the type of scale_factor and add_offset the same). 
        if (true == HDF4RequestHandler::get_enable_check_scale_offset_type() && at !=nullptr)  
            HDFCFUtil::correct_scale_offset_type(at); 

    }

    // Handle vdata attributes.
    try {
        HDFCFUtil::handle_vdata_attrs_with_desc_key(f,das);
    }
    catch(...) {
        throw;
    }

    return true;

}

/// This is a wrapper to map HDF-EOS2,hybrid and HDF4 to DDS by using(configuring with)
/// the HDF-EOS2 lib.
void read_dds_use_eos2lib(DDS & dds, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,
                          HDFSP::File*h4file,HDFEOS2::File*eosfile)
{

    BESDEBUG("h4","Coming to read_dds_use_eos2lib" <<endl);

    int ret_value = read_dds_hdfeos2(dds,filename,sdfd,gridfd,swathfd,h4file,eosfile);

    BESDEBUG("h4","ret_value of read_dds_hdfeos2 is "<<ret_value<<endl);

    // read_dds_hdfeos2 return value description:
    // 0: general non-EOS2 pure HDF4
    // 1: HDF-EOS2 hybrid 
    // 2: MOD08_M3
    //  HDF-EOS2 but no need to use HDF-EOS2 lib: no real dimension scales but have CVs for every dimension, treat differently
    // 3: AIRS version 6  
    //  HDF-EOS2 but no need to use HDF-EOS2 lib: 
    //  have dimension scales but don’t have CVs for every dimension, also need to condense dimensions, treat differently
    // 4. Ideal(Expected) AIRS version 6(No real products yet)
    // HDF-EOS2 but no need to use HDF-EOS2 lib: Have dimension scales for all dimensions
    // 5. MERRA 
    // Special handling for MERRA file 


    // Treat MERRA and non-HDFEOS2 HDF4 products as pure HDF4 objects
    // For Ideal AIRS version 6 products, we temporarily handle them in a generic HDF4 way.
    if (0 == ret_value  || 5 == ret_value  || 4 == ret_value ) {
        if (true == read_dds_hdfsp(dds, filename,sdfd,fileid,h4file))
            return;
    }
    // Special handling
    else if ( 1 == ret_value ) {

         //  Map non-EOS2 objects to DDS
        if (true ==read_dds_hdfhybrid(dds,filename,sdfd,fileid,h4file))
            return;
    }
    else {// ret_value = 2 and 3 are handled already in the read_dds_hdfeos2 calls. Just return.
        return;
    }

// leave this code block for performance comparison.
#if 0
    // first map HDF-EOS2 objects to DDS
    if(true == read_dds_hdfeos2(dds, filename)){

        //  Map non-EOS2 objects to DDS
        if(true == read_dds_hdfhybrid(dds,filename))
            return;
    }

    // Map HDF4 objects in pure HDF4 files to DDS
    if(read_dds_hdfsp(dds, filename)){
        return;
    }
#endif

    // Call the default mapping of HDF4 to DDS. It should never reach here.
    // We add this line to ensure the HDF4 objects mapped to DDS even if the above routines return false.
    read_dds(dds, filename);
   
}

// Map other HDF global attributes, this routine must be called after all ECS metadata are handled.
void write_non_ecsmetadata_attrs(HE2CF& cf) {

    cf.set_non_ecsmetadata_attrs();

}

// Map HDF-EOS2's ECS attributes to DAS. ECS attributes include coremetadata, structmetadata etc.
void write_ecsmetadata(DAS& das, HE2CF& cf, const string& _meta)
{

    // There is a maximum length for each ECS metadata if one uses ECS toolkit to add the metadata.
    // For some products of which the metadata size is huge, one metadata may be stored in several
    // ECS attributes such as coremetadata.0, coremetadata.1 etc. 
    // When mapping the ECS metadata, we need to merge such metadata attributes into one attribute(coremetadata)
    // so that end users can easily understand this metadata.
    // ECS toolkit recommends data producers to use the format shown in the following coremetadata example:
    // coremetadata.0, coremetadata.1, etc.
    // Most NASA HDF-EOS2 products follow this naming convention.
    // However, the toolkit also allows data producers to freely name its metadata.
    // So we also find the following slightly different format:
    // (1) No suffix: coremetadata
    // (2) only have one such ECS attribute: coremetadata.0
    // (3) have several ECS attributes with two dots after the name: coremetadata.0, coremetadata.0.1 etc.
    // (4) Have non-number suffix: productmetadata.s, productmetadata.t etc.
    // We handle the above case in the function set_metadata defined in HE2CF.cc. KY 2013-07-06

    bool suffix_is_number = true;
    vector<string> meta_nonum_names;
    vector<string> meta_nonum_data;

    string meta = cf.get_metadata(_meta,suffix_is_number,meta_nonum_names, meta_nonum_data);

    if(""==meta && true == suffix_is_number){
        return;                 // No _meta metadata exists.
    }
    
    BESDEBUG("h4",meta << endl);        

    if (false == suffix_is_number) {
        // For the case when the suffix is like productmetadata.s, productmetadata.t, 
        // we will not merge the metadata since we are not sure about the order.
        // We just parse each attribute individually.
        for (unsigned int i = 0; i <meta_nonum_names.size(); i++) 
            parse_ecs_metadata(das,meta_nonum_names[i],meta_nonum_data[i]);
    }
    else 
        parse_ecs_metadata(das,_meta,meta);

}

void parse_ecs_metadata(DAS &das,const string & metaname, const string &metadata) {

    AttrTable *at = das.get_table(metaname);
    if (!at)
        at = das.add_table(metaname, new AttrTable);

    // tell lexer to scan attribute string
    void *buf = hdfeos_string(metadata.c_str());
    parser_arg arg(at);

    if (hdfeosparse(&arg) != 0) {
        hdfeos_delete_buffer(buf);
        throw Error("HDF-EOS parse error while processing a " + metadata + " HDFEOS attribute.");
    }

    if (arg.status() == false) {
        (*BESLog::TheLog())<<  "HDF-EOS parse error while processing a "
                           << metadata  << " HDFEOS attribute. (2) " << endl;
#if 0
// for debugging
                           << arg.error()->get_error_message() << endl;
#endif
    }

    hdfeos_delete_buffer(buf);
}

// Build DAS for HDFEOS2 files.
int read_das_hdfeos2(DAS & das, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,
                     bool ecs_metadata,HDFSP::File**spfpptr,HDFEOS2::File **fpptr)
{

    BESDEBUG("h4","Coming to read_das_hdfeos2 " << endl);

    // There are some HDF-EOS2 files(MERRA) that should be treated
    // exactly like HDF4 SDS files. We don't need to use HDF-EOS2 APIs to 
    // retrieve any information. In fact, treating them as HDF-EOS2 files
    // will cause confusions and retrieve wrong information, though may not be essential.
    // So far, we've only found that the MERRA product has this problem.
    // A quick fix is to check if the file name contains MERRA. KY 2011-3-4 
    // Actually, AIRS version 6 and MODO8M3 also fall into this category,
    // they are also specially handled, check read_das_special_eos2_core. KY 2015-06-04

    // Find MERRA data, return 5.
    if((basename(filename).size() >=5) && ((basename(filename)).compare(0,5,"MERRA")==0)) {
        return 5;
    }

    // We will check if the handler wants to turn on the special EOS key checking
    if (true == HDF4RequestHandler::get_enable_special_eos()) {

        string grid_name;
        int ret_val = check_special_eosfile(filename,grid_name,sdfd); 

        // Expected AIRS level 2 or 3
        if (4== ret_val) 
            return ret_val;
    
        bool airs_l2_l3_v6 = false;
        bool special_1d_grid = false;

        // AIRS level 2,3 version 6 or MOD08_M3-like products
        if (2 == ret_val || 3 == ret_val) {

            HDFSP::File *spf = nullptr; 
            try {
                spf  = HDFSP::File::Read(filename.c_str(),sdfd,fileid);
            }
            catch (HDFSP::Exception &e)
            {
                if (spf != nullptr)
                    delete spf;
                throw InternalErr(e.what());
            }

            try {
                if (2 == ret_val) {

                    // More check and build the relations if this is a special MOD08_M3-like file
                    if (spf->Check_update_special(grid_name)== true){

                        special_1d_grid = true;

                        // Building the normal HDF4 DAS here.
                        read_das_special_eos2_core(das,spf,filename,ecs_metadata);

                        // Need to handle MOD08M3 product
                        if (grid_name =="mod08") {
                           change_das_mod08_scale_offset(das,spf);
                        }
                    }
                }
                else {

                    airs_l2_l3_v6 = true;
                    spf->Handle_AIRS_L23();
                    read_das_special_eos2_core(das,spf,filename,ecs_metadata);
                }

            } 
            catch (...)
            {   
                delete spf;
                throw;
            }   

            // If this is MOD08M3 or AIRS version 6,we just need to return the file pointer.
            if (true == special_1d_grid || true == airs_l2_l3_v6) {
                *spfpptr = spf;
                return ret_val;
            }
       
        }
    }
 
    HDFEOS2::File *f = nullptr;
    
    try {
        // Read all the information of EOS objects from an HDF-EOS2 file
        f= HDFEOS2::File::Read(filename.c_str(),gridfd,swathfd);
    } 
    catch (HDFEOS2::Exception &e){
       
        if (f != nullptr)
            delete f;

        // If this file is not an HDF-EOS2 file, return 0.
        if (!e.getFileType()){
            return 0;
        }
        else
        {
            throw InternalErr(e.what());
        }
    }

   try {
        // Generate CF coordinate variables(including auxiliary coordinate variables) and dimensions
        // All the names follow CF.
        f->Prepare(filename.c_str());
    }

    catch (HDFEOS2:: Exception &e) {
        if (f!=nullptr) 
            delete f;
        throw InternalErr(e.what());
    }

    *fpptr = f;

    // HE2CF cf is used to handle hybrid SDS and SD attributes.
    HE2CF cf;
    
    try {
        cf.open(filename,sdfd,fileid);
    }
    catch(...) {
        throw;
    }
    cf.set_DAS(&das);

    SOType sotype = SOType::DEFAULT_CF_EQU;

    // A flag not to generate structMetadata for the MOD13C2 file.
    // MOD13C2's structMetadata has wrong values. It couldn't pass the parser. 
    // So we want to turn it off. KY 2010-8-10
    bool tempstrflag = false;

#if 0
    // AMSR_E may stop using "SCALE_FACTOR", so the following "if block" is empty. Still leave it here for future reference. KY 2022-06-16
    // Product name(AMSR_E) that needs to change attribute from "SCALE FACTOR" to scale_factor etc. to follow the CF conventions
    if (f->getSwaths().empty() == false) {
        string temp_fname = basename(filename);
        string temp_prod_prefix = "AMSR_E";
        if ((temp_fname.size() > temp_prod_prefix.size()) && 
            (0 == (temp_fname.compare(0,temp_prod_prefix.size(),temp_prod_prefix)))) {
        }

    }
#endif

    // Obtain information to identify MEaSURES VIP. This product needs to be handled properly.
    bool gridname_change_valid_range = false;
    if (1 == f->getGrids().size()) {
        string gridname = f->getGrids()[0]->getName();
        if ("VIP_CMG_GRID" == gridname) 
            gridname_change_valid_range = true;
    }

    // Obtain information to identify MODIS_SWATH_Type_L1B product. This product's scale and offset need to be handled properly. 
    bool is_modis_l1b = false;

    // Since this is a swath product, we check swath only. 
    for (int i = 0; i<(int) f->getSwaths().size(); i++) {
        const HDFEOS2::SwathDataset* swath = f->getSwaths()[i];
        string sname = swath->getName();
        if ("MODIS_SWATH_Type_L1B" == sname){
            is_modis_l1b = true;
            break;
        }
    }

    try {

        // MAP grids to DAS.
        for (int i = 0; i < (int) f->getGrids().size(); i++) {

            const HDFEOS2::GridDataset*  grid = f->getGrids()[i];
            string gname = grid->getName();
            sotype = grid->getScaleType();
      
            const vector<HDFEOS2::Field*>gfields = grid->getDataFields();

            for (const auto &gf:gfields) {

                bool change_fvtype = false;

                // original field name
                string fname = gf->getName();

                //  new field name that follows CF
                string newfname = gf->getNewName();

                BESDEBUG("h4","Original field name: " <<  fname << endl);
                BESDEBUG("h4","Corrected field name: " <<  newfname << endl);

                // whether coordinate variable or data variables
                int fieldtype = gf->getFieldType(); 

                // 0 means that the data field is NOT a coordinate variable.
                if (fieldtype == 0){

                    // If you don't find any _FillValue through generic APIs.
                    if (gf->haveAddedFillValue()) {
                        BESDEBUG("h4","Has an added fill value." << endl);
                        float addedfillvalue = 
                            gf->getAddedFillValue();
                        int type = 
                            gf->getType();
                        BESDEBUG("h4","Added fill value = "<<addedfillvalue);
                        cf.write_attribute_FillValue(newfname, 
                                                     type, addedfillvalue);
                    }
                    string coordinate = gf->getCoordinate();
                    BESDEBUG("h4","Coordinate attribute: " << coordinate <<endl);
                    if (coordinate != "") 
                        cf.write_attribute_coordinates(newfname, coordinate);
                }

                // This will override _FillValue if it's defined on the field.
                cf.write_attribute(gname, fname, newfname, 
                                   (int)(f->getGrids().size()), fieldtype);  

                // For fieldtype values:
                // 0 is general fields
                // 1 is latitude.
                // 2 is longtitude.    
                // 3 is the existing 3rd-dimension coordinate variable
                // 4 is the dimension that misses the coordinate variable,use natural number
                // 5 is time
                if (fieldtype > 0){

                    // MOD13C2 is treated specially. 
                    if (fieldtype == 1 && (gf->getSpecialLLFormat())==3)
                        tempstrflag = true;

                    // Don't change the units if the 3-rd dimension field exists.(fieldtype =3)
                    // KY 2013-02-15
                    if (fieldtype !=3) {
                        string tempunits = gf->getUnits();
                        BESDEBUG("h4",
                                 "fieldtype " << fieldtype 
                                 << " units" << tempunits 
                                 << endl);
                        cf.write_attribute_units(newfname, tempunits);
                    }
                }

	        //Rename attributes of MODIS products.
                AttrTable *at = das.get_table(newfname);

                // No need for the case that follows the CF scale and offset .
                if (sotype!=SOType::DEFAULT_CF_EQU && at!=nullptr)
                {
                    bool has_Key_attr = false;
                    AttrTable::Attr_iter it = at->attr_begin();
                    while (it!=at->attr_end())
                    {
                        if (at->get_name(it)=="Key")
                        {
                            has_Key_attr = true;
                            break;
                        }
                        it++;
                    }

                    if ((false == is_modis_l1b) && (false == gridname_change_valid_range)&&(false == has_Key_attr) && 
                        (true == HDF4RequestHandler::get_disable_scaleoffset_comp())) 
                        HDFCFUtil::handle_modis_special_attrs_disable_scale_comp(at,basename(filename), true, newfname,sotype);
                    else {

                        // Check if the datatype of this field needs to be changed.
                        bool changedtype = HDFCFUtil::change_data_type(das,sotype,newfname);

                        // Build up the field name list if the datatype of the field needs to be changed.
                        if (true == changedtype) 
                            ctype_field_namelist.push_back(newfname);

                        HDFCFUtil::handle_modis_special_attrs(at,basename(filename),true, newfname,sotype,gridname_change_valid_range,changedtype,change_fvtype);
          
                    }
                }

                // Handle AMSR-E attributes.
                HDFCFUtil::handle_amsr_attrs(at);

                // Check if having _FillValue. If having _FillValue, compare the datatype of _FillValue
                // with the variable datatype. Correct the fillvalue datatype if necessary. 
                if ((false == change_fvtype) && at != nullptr) {
                    int32 var_type = gf->getType();
                    HDFCFUtil::correct_fvalue_type(at,var_type);
                }

                // if h4.enablecheckscaleoffsettype bes key is true, 
                //    if yes, check if having scale_factor and add_offset attributes;
                //       if yes, check if scale_factor and add_offset attribute types are the same;
                //          if no, make add_offset's datatype be the same as the datatype of scale_factor.
                // (cf requires the type of scale_factor and add_offset the same).
                if (true == HDF4RequestHandler::get_enable_check_scale_offset_type() && at!=nullptr) 
                    HDFCFUtil::correct_scale_offset_type(at);

            }

            // Add possible 1-D CV CF attributes to identify projection info. for CF.
            // Currently only the Sinusoidal projection is supported.
            HDFCFUtil::add_cf_grid_cv_attrs(das,grid);

        }
    }
    catch(...) {
        //delete f;
        throw;
    }

    try {
        // MAP Swath attributes to DAS.
        for (int i = 0; i < (int) f->getSwaths().size(); i++) {

            const HDFEOS2::SwathDataset*  swath = f->getSwaths()[i];

            // Swath includes two parts: "Geolocation Fields" and "Data Fields".
            // The all_fields vector includes both.
            const vector<HDFEOS2::Field*> geofields = swath->getGeoFields();
            vector<HDFEOS2::Field*> all_fields = geofields;

            const vector<HDFEOS2::Field*> datafields = swath->getDataFields();
            for (const auto &df:datafields)
                all_fields.push_back(df);

            auto total_geofields = (int)(geofields.size());

            string gname = swath->getName();
            BESDEBUG("h4","Swath name: " << gname << endl);

            sotype = swath->getScaleType();

            // field_counter is only used to separate the geo field from the data field.
            int field_counter = 0;

            for (const auto &af:all_fields)
            {
                bool change_fvtype = false;
                string fname = af->getName();
                string newfname = af->getNewName();
                BESDEBUG("h4","Original Field name: " <<  fname << endl);
                BESDEBUG("h4","Corrected Field name: " <<  newfname << endl);

                int fieldtype = af->getFieldType();
                if (fieldtype == 0){
                    string coordinate = af->getCoordinate();
                    BESDEBUG("h4","Coordinate attribute: " << coordinate <<endl);
                    if (coordinate != "")
                        cf.write_attribute_coordinates(newfname, coordinate);
                }

                // 1 is latitude.
                // 2 is longitude.
                // Don't change "units" if a non-latlon coordinate variable  exists.
                if(fieldtype >0 && fieldtype !=3){
                    string tempunits = af->getUnits();
                    BESDEBUG("h4",
                        "fieldtype " << fieldtype 
                        << " units" << tempunits << endl);
                    cf.write_attribute_units(newfname, tempunits);
                
                }
                BESDEBUG("h4","Field Name: " <<  fname << endl);

                // coordinate "fillvalue" attribute
                // This operation should only apply to data fields.
                if (field_counter >=total_geofields) {
                    if(af->haveAddedFillValue()){
                        float addedfillvalue = 
                            af->getAddedFillValue();
                        int type = 
                            af->getType();
                        BESDEBUG("h4","Added fill value = "<<addedfillvalue);
                        cf.write_attribute_FillValue(newfname, type, addedfillvalue);
                    }
                }
                cf.write_attribute(gname, fname, newfname, 
                                   (int)(f->getSwaths().size()), fieldtype); 

	        AttrTable *at = das.get_table(newfname);

                // No need for CF scale and offset equation.
                if (sotype!=SOType::DEFAULT_CF_EQU && at!=nullptr)
                {

                    bool has_Key_attr = false;
                    AttrTable::Attr_iter it = at->attr_begin();
                    while (it!=at->attr_end())
                    {
                        if (at->get_name(it)=="Key")
                        {
                            has_Key_attr = true;
                            break;
                        }
                        it++;
                    }

                    if ((false == is_modis_l1b) && (false == gridname_change_valid_range) &&(false == has_Key_attr) && 
                       (true == HDF4RequestHandler::get_disable_scaleoffset_comp())) 
                        HDFCFUtil::handle_modis_special_attrs_disable_scale_comp(at,basename(filename),false,newfname,sotype);
                    else {

                        // Check if the datatype of this field needs to be changed.
                        bool changedtype = HDFCFUtil::change_data_type(das,sotype,newfname);

                        // Build up the field name list if the datatype of the field needs to be changed.
                        if (true == changedtype) 

                            ctype_field_namelist.push_back(newfname);
               
                        // Handle MODIS special attributes such as valid_range, scale_factor and add_offset etc.
                        // Need to catch the exception since this function calls handle_modis_vip_special_attrs that may 
                        // throw an exception.
                        HDFCFUtil::handle_modis_special_attrs(at,basename(filename), false,newfname,sotype,gridname_change_valid_range,changedtype,change_fvtype);
                    }
                }

                // Handle AMSR-E attributes
                if (at!=nullptr) 
                    HDFCFUtil::handle_amsr_attrs(at);

                // Check if having _FillValue. If having _FillValue, compare the datatype of _FillValue
                // with the variable datatype. Correct the fillvalue datatype if necessary. 
                if ((false == change_fvtype) && at != nullptr) {
                    int32 var_type = af->getType();
                    HDFCFUtil::correct_fvalue_type(at,var_type);
                }

                // If H4.EnableCheckScaleOffsetType BES key is true,  
                //    if yes, check if having scale_factor and add_offset attributes; 
                //       if yes, check if scale_factor and add_offset attribute types are the same; 
                //          if no, make add_offset's datatype be the same as the datatype of scale_factor. 
                // (CF requires the type of scale_factor and add_offset the same). 
                if (true == HDF4RequestHandler::get_enable_check_scale_offset_type() && at !=nullptr)  
                    HDFCFUtil::correct_scale_offset_type(at); 

                field_counter++;
            }
        }
    }
    catch(...) {
        throw;
    } 
    

    try {

      if(ecs_metadata == true) {

        // Handle ECS metadata. The following metadata are what we found so far.
        write_ecsmetadata(das,cf, "CoreMetadata");

        write_ecsmetadata(das,cf, "coremetadata");

        write_ecsmetadata(das,cf,"ArchiveMetadata");

        write_ecsmetadata(das,cf,"archivemetadata");

        write_ecsmetadata(das,cf,"ProductMetadata");

        write_ecsmetadata(das,cf,"productmetadata");
      }

        // This cause a problem for a MOD13C2 file, So turn it off temporarily. KY 2010-6-29
        if (false == tempstrflag) {

            if (false == HDF4RequestHandler::get_disable_structmeta() ) {
                write_ecsmetadata(das, cf, "StructMetadata");
            }
        }

        // Write other HDF global attributes, this routine must be called after all ECS metadata are handled.
        write_non_ecsmetadata_attrs(cf);

        cf.close();
    }
    catch(...) {
        throw;
    }

    try {
        
        // Check if swath or grid object (like vgroup) attributes should be mapped to DAP2. If yes, start mapping.
        if (true == HDF4RequestHandler::get_enable_swath_grid_attr()) {

            // MAP grid attributes  to DAS.
            for (int i = 0; i < (int) f->getGrids().size(); i++) {

                HDFEOS2::GridDataset*  grid = f->getGrids()[i];

                string gname = HDFCFUtil::get_CF_string(grid->getName());

                AttrTable*at = nullptr;

                // Create a "grid" DAS table if this grid has attributes.
                if (grid->getAttributes().empty() == false){ 
                    at = das.get_table(gname);
                    if (!at)
                        at = das.add_table(gname, new AttrTable);
                }
                if (at!= nullptr) {

                    //  Process grid attributes
                    const vector<HDFEOS2::Attribute *> grid_attrs = grid->getAttributes();
                    for (const auto &attr:grid_attrs) {

                        int attr_type = attr->getType();

                        // We treat string differently. DFNT_UCHAR and DFNT_CHAR are treated as strings.
                        if(attr_type==DFNT_UCHAR || attr_type == DFNT_CHAR){
                            string tempstring2(attr->getValue().begin(),attr->getValue().end());
                            auto tempfinalstr= string(tempstring2.c_str());
                            at->append_attr(attr->getNewName(), "String" , tempfinalstr);
                        }
            

                        else {
                            for (int loc=0; loc < attr->getCount() ; loc++) {
                                string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                                at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                            }
                        }
                    }
                }
            }

            //
            // MAP swath attributes to DAS.
            for (int i = 0; i < (int) f->getSwaths().size(); i++) {

                const HDFEOS2::SwathDataset*  swath = f->getSwaths()[i];
                string sname = swath->getName();
                AttrTable*at = nullptr;

                // Create a "swath" DAS table if this swath has attributes.
                if(swath->getAttributes().empty() == false) {
                    at = das.get_table(sname);
                    if (!at) 
                        at = das.add_table(sname, new AttrTable);
                }

                if(at != nullptr) {
                    const vector<HDFEOS2::Attribute *> swath_attrs = swath->getAttributes();
                    for (const auto &attr:swath_attrs) {

                        int attr_type = attr->getType();

                        // We treat string differently. DFNT_UCHAR and DFNT_CHAR are treated as strings.
                        if(attr_type==DFNT_UCHAR || attr_type == DFNT_CHAR){
                            string tempstring2(attr->getValue().begin(),attr->getValue().end());
                            auto tempfinalstr= string(tempstring2.c_str());
                            at->append_attr(attr->getNewName(), "String" , tempfinalstr);
                        }
                        else {
                            for (int loc=0; loc < attr->getCount() ; loc++) {
                                string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                                at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                            }
          
                        }
                    }
                }
            }
        }// end of mapping swath and grid object attributes to DAP2
    }
    catch(...) {
        throw;
    }

    return 1;
}    

//The wrapper of building HDF-EOS2 and special HDF4 files. 
void read_das_use_eos2lib(DAS & das, const string & filename,
                          int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,bool ecs_metadata,
                          HDFSP::File**h4filepptr,HDFEOS2::File**eosfilepptr)
{

    BESDEBUG("h4","Coming to read_das_use_eos2lib" << endl);

    int ret_value = read_das_hdfeos2(das,filename,sdfd,fileid, gridfd, swathfd,ecs_metadata,h4filepptr,eosfilepptr);

    BESDEBUG("h4","ret_value of read_das_hdfeos2 is "<<ret_value <<endl);

    // read_das_hdfeos2 return value description:
    // 0: general non-EOS2 pure HDF4
    // 1: HDF-EOS2 hybrid 
    // 2: MOD08_M3
    //  HDF-EOS2 but no need to use HDF-EOS2 lib: no real dimension scales but have CVs for every dimension, treat differently
    // 3: AIRS version 6 level 3 and level 2 
    //  HDF-EOS2 but no need to use HDF-EOS2 lib: 
    //  have dimension scales but don’t have CVs for every dimension, also need to condense dimensions, treat differently
    // 4. Expected AIRS version 6 level 3 and level 2
    // HDF-EOS2 but no need to use HDF-EOS2 lib: Have dimension scales for all dimensions
    // 5. MERRA 
    // Special handling for MERRA products.

    // Treat as pure HDF4 objects
    if (ret_value == 4) {
        if(true == read_das_special_eos2(das, filename,sdfd,fileid,ecs_metadata,h4filepptr))
            return;
    }
    // Special handling, already handled
    else if (ret_value == 2 || ret_value == 3) {
        return;
    }
    else if (ret_value == 1) {

        //  Map non-EOS2 objects to DDS
        if(true == read_das_hdfhybrid(das,filename,sdfd,fileid,h4filepptr))
            return;
    }
    else {// ret_value is 0(pure HDF4) or 5(Merra)
        if(true == read_das_hdfsp(das, filename,sdfd, fileid,h4filepptr))
            return;
    }


// Leave the original code that don't pass the file pointers.
#if 0
    // First map HDF-EOS2 attributes to DAS
    if(true == read_das_hdfeos2(das, filename)){
        
        // Map non-EOS2 attributes to DAS
        if (true == read_das_hdfhybrid(das,filename))
            return;
    }

    // Map HDF4 attributes in pure HDF4 files to DAS
    if(true == read_das_hdfsp(das, filename)){
        return;
    }
#endif

    // Call the default mapping of HDF4 to DAS. It should never reach here.
    // We add this line to ensure the HDF4 attributes mapped to DAS even if the above routines return false.
    read_das(das, filename);
}


#endif // end of #ifdef USE_HDFEOS2_LIB #endif block

// The wrapper of building DDS function.
bool read_dds_hdfsp(DDS & dds, const string & filename,int32 sdfd, int32 fileid,const HDFSP::File*f)
{

    BESDEBUG("h4","Coming to read_dds_sp "<<endl);
    dds.set_dataset_name(basename(filename));

    // Obtain SDS fields
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    // Read SDS 
    for (const auto& spf:spsds){

        // Although the following line's logic needs to improve, it is right.
        // When Has_Dim_NoScale_Field is false, it only happens to the OTHERHDF case. 
        // For the OTHERHDF case, we will not map the dimension_no_dim_scale (empty) field. This is equivalent to 
        if (false == f->Has_Dim_NoScale_Field() || (0 == spf->getFieldType()) || (true == spf->IsDimScale())){
            try {
                read_dds_spfields(dds,filename,sdfd,spf,f->getSPType());
            }
            catch(...) {
                throw;
            }
        }
    }

    // Read Vdata fields.
    // To speed up the performance for handling CERES data, we turn off some CERES vdata fields, this should be resumed in the future version with BESKeys.

    bool output_vdata_flag = true;  
    if (false == HDF4RequestHandler::get_enable_ceres_vdata() && 
        (CER_AVG == f->getSPType() ||
         CER_ES4 == f->getSPType() ||
         CER_SRB == f->getSPType() ||
         CER_ZAVG == f->getSPType()))
        output_vdata_flag = false;

    if(true == output_vdata_flag) {
        for (const auto &vd:f->getVDATAs()) {
            if(!vd->getTreatAsAttrFlag()){
                for(const auto &vdf:vd->getFields()) {
                    try {
                        read_dds_spvdfields(dds,filename,fileid,vd->getObjRef(),vdf->getNumRec(),vdf); 
                    }
                    catch(...) {
                        throw;
                    }
                }
            }
        }
    }
        
    return true;
}

// Follow CF to build DAS for non-HDFEOS2 HDF4 products. This routine also applies
// to all HDF4 products when HDF-EOS2 library is not configured in.
bool read_das_hdfsp(DAS & das, const string & filename, int32 sdfd, int32 fileid,HDFSP::File**fpptr) 
{

    BESDEBUG("h4","Coming to read_das_sp "<<endl);

    // Define a file pointer
    HDFSP::File *f = nullptr;
    try {
        // Obtain all the necesary information from HDF4 files.
        f = HDFSP::File::Read(filename.c_str(), sdfd,fileid);
    } 
    catch (HDFSP::Exception &e)
    {
        if (f != nullptr)
            delete f;
        throw InternalErr(e.what());
    }
        
    try {
        // Generate CF coordinate variables(including auxiliary coordinate variables) and dimensions
        // All the names follow CF.
        f->Prepare();
    } 
    catch (HDFSP::Exception &e) {
        delete f;
        throw InternalErr(e.what());
    }

    *fpptr = f;

    // Check if mapping vgroup attribute key is turned on, if yes, mapping vgroup attributes.

    if (true == HDF4RequestHandler::get_enable_vgroup_attr()) {

        // Obtain vgroup attributes if having vgroup attributes.
        vector<HDFSP::AttrContainer *>vg_container = f->getVgattrs();
        for (const auto &vgattr_c:f->getVgattrs()) {
            AttrTable *vgattr_at = das.get_table(vgattr_c->getName());
            if (!vgattr_at)
                vgattr_at = das.add_table(vgattr_c->getName(), new AttrTable);

            for (const auto &attr:vgattr_c->getAttributes()) {

                // Handle string first.
                if(attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){
                    string tempstring2(attr->getValue().begin(),attr->getValue().end());
                    string tempfinalstr= string(tempstring2.c_str());
                    vgattr_at->append_attr(attr->getNewName(), "String" , tempfinalstr);
                }
                else {
                    for (int loc=0; loc < attr->getCount() ; loc++) {

                        string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                        vgattr_at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                    }
                }
            }
        }
    }// end of mapping vgroup attributes.

    // Initialize ECS metadata
    string core_metadata = "";
    string archive_metadata = "";
    string struct_metadata = "";
    
    // Obtain SD pointer, this is used to retrieve the file attributes associated with the SD interface
    const HDFSP::SD* spsd = f->getSD();
 
    // Except for TRMM, we don't find ECS metadata in other non-EOS products. For the option to treat EOS2 as pure HDF4, we
    // kind of relax the support of merging metadata as we do for the EOS2 case(read_das_hdfeos2). We will see if we have the user
    // request to make them consistent in the future. KY 2013-07-08
    for (const auto &sp_attr:spsd->getAttributes()) {
 
        // Here we try to combine ECS metadata into a string.
        if((sp_attr->getName().compare(0, 12, "CoreMetadata" )== 0) ||
            (sp_attr->getName().compare(0, 12, "coremetadata" )== 0)){

            // We assume that CoreMetadata.0, CoreMetadata.1, ..., CoreMetadata.n attribures
            // are processed in the right order during HDFSP::Attribute vector iteration.
            // Otherwise, this won't work.
            string tempstring(sp_attr->getValue().begin(),sp_attr->getValue().end());
          
            // Temporarily turn off CERES data since there are so many fields in CERES. It will choke clients KY 2010-7-9
            if(f->getSPType() != CER_AVG && 
               f->getSPType() != CER_ES4 && 
               f->getSPType() !=CER_SRB  && 
               f->getSPType() != CER_ZAVG) 
                core_metadata.append(tempstring);
        }
        else if((sp_attr->getName().compare(0, 15, "ArchiveMetadata" )== 0) ||
                (sp_attr->getName().compare(0, 16, "ArchivedMetadata")==0)  ||
                (sp_attr->getName().compare(0, 15, "archivemetadata" )== 0)){

            string tempstring(sp_attr->getValue().begin(),sp_attr->getValue().end());
            // Currently some TRMM "swath" archivemetadata includes special characters that cannot be handled by OPeNDAP
            // So turn it off.
            // Turn off CERES  data since it may choke JAVA clients KY 2010-7-9
            if(f->getSPType() != TRMML2_V6 && f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG)
                archive_metadata.append(tempstring);
        }
        else if((sp_attr->getName().compare(0, 14, "StructMetadata" )== 0) ||
                (sp_attr->getName().compare(0, 14, "structmetadata" )== 0)){

            if (false == HDF4RequestHandler::get_disable_structmeta()) {

                string tempstring(sp_attr->getValue().begin(),sp_attr->getValue().end());

                // Turn off TRMM "swath" verison 6 level 2 productsCERES  data since it may choke JAVA clients KY 2010-7-9
                if(f->getSPType() != TRMML2_V6 && 
                   f->getSPType() != CER_AVG && 
                   f->getSPType() != CER_ES4 && 
                   f->getSPType() !=CER_SRB && 
                   f->getSPType() != CER_ZAVG)
                    struct_metadata.append(tempstring);

            }
        }        
        else {
            //  Process gloabal attributes
            AttrTable *at = das.get_table("HDF_GLOBAL");
            if (!at)
                at = das.add_table("HDF_GLOBAL", new AttrTable);

            // We treat string differently. DFNT_UCHAR and DFNT_CHAR are treated as strings.
            if (sp_attr->getType()==DFNT_UCHAR || sp_attr->getType() == DFNT_CHAR){

                string tempstring2(sp_attr->getValue().begin(),sp_attr->getValue().end());
                auto tempfinalstr= string(tempstring2.c_str());
                at->append_attr(sp_attr->getNewName(), "String" , tempfinalstr);
            }

            else {
                for (int loc=0; loc < sp_attr->getCount() ; loc++) {
                    string print_rep = HDFCFUtil::print_attr(sp_attr->getType(), loc, (void*) &(sp_attr->getValue()[0]));
                    at->append_attr(sp_attr->getNewName(), HDFCFUtil::print_type(sp_attr->getType()), print_rep);
                }
            }
        }
    }
    
    // The following code may be condensed in the future. KY 2012-09-19
    // Coremetadata, structmetadata and archive metadata need special parsers.

    // Write coremetadata.
    if(core_metadata.size() > 0){
        AttrTable *at = das.get_table("CoreMetadata");
        if (!at)
            at = das.add_table("CoreMetadata", new AttrTable);
        // tell lexer to scan attribute string
        void *buf = hdfeos_string(core_metadata.c_str());
        parser_arg arg(at);

        if (hdfeosparse(&arg) != 0) {
            hdfeos_delete_buffer(buf);
            throw Error("Parse error while processing a CoreMetadata attribute.");
        }

        // Errors returned from here are ignored.
        if (arg.status() == false) {
            ERROR_LOG("Parse error while processing a CoreMetadata attribute. (2) " << endl);
#if 0
        //        << arg.error()->get_error_message() << endl;
#endif
        }

        hdfeos_delete_buffer(buf);
    }
            
    // Write archive metadata.
    if(archive_metadata.size() > 0){
        AttrTable *at = das.get_table("ArchiveMetadata");
        if (!at)
            at = das.add_table("ArchiveMetadata", new AttrTable);
        // tell lexer to scan attribute string
        void *buf = hdfeos_string(archive_metadata.c_str());
        parser_arg arg(at);
        if (hdfeosparse(&arg) != 0){
            hdfeos_delete_buffer(buf);
            throw Error("Parse error while processing an ArchiveMetadata attribute.");
        }

        // Errors returned from here are ignored.
        if (arg.status() == false) {
            ERROR_LOG("Parse error while processing an ArchiveMetadata attribute. (2) " << endl);
#if 0
 //               << arg.error()->get_error_message() << endl;
#endif
        }

        hdfeos_delete_buffer(buf);
    }

    // Write struct metadata.
    if(struct_metadata.size() > 0){
        AttrTable *at = das.get_table("StructMetadata");
        if (!at)
            at = das.add_table("StructMetadata", new AttrTable);
        // tell lexer to scan attribute string
        void *buf = hdfeos_string(struct_metadata.c_str());
        parser_arg arg(at);
        if (hdfeosparse(&arg) != 0){
            hdfeos_delete_buffer(buf);
            throw Error("Parse error while processing a StructMetadata attribute.");
        }

        if (arg.status() == false) {
            ERROR_LOG("Parse error while processing a StructMetadata attribute.  (2)" << endl);
        }

        // Errors returned from here are ignored.
#if 0
        if (arg.status() == false) {
            (*BESLog::TheLog())<< "Parse error while processing a StructMetadata attribute. (2)" << endl
                << arg.error()->get_error_message() << endl;
        }
#endif

        hdfeos_delete_buffer(buf);
    }

     // The following code checks the special handling of scale and offset of the OBPG products. 
    //Store value of "Scaling" attribute.
    string scaling;

    //Store value of "Slope" attribute.
    float slope = 0.; 
    bool global_slope_flag = false;
    float intercept = 0.;
    bool global_intercept_flag = false;

    // Check OBPG attributes. Specifically, check if slope and intercept can be obtained from the file level. 
    // If having global slope and intercept,  obtain OBPG scaling, slope and intercept values.
    HDFCFUtil::check_obpg_global_attrs(f,scaling,slope,global_slope_flag,intercept,global_intercept_flag);
    
    // Handle individual fields
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
    vector<HDFSP::SDField *>::const_iterator it_g;
    for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

        // The following two if-statements are double secure checks. It will
        // make sure no-dimension-scale dimension variables and the associated coordinate variables(if any) are ignored.
        // Ignore ALL coordinate variables if this is "OTHERHDF" case and some dimensions 
        // don't have dimension scale data.
        if ( true == f->Has_Dim_NoScale_Field() && 
             ((*it_g)->getFieldType() !=0)&& 
             ((*it_g)->IsDimScale() == false))
            continue;
     
        // Ignore the empty(no data) dimension variable.
        if (OTHERHDF == f->getSPType() && true == (*it_g)->IsDimNoScale()) 
            continue;
    
        AttrTable *at = das.get_table((*it_g)->getNewName());
        if (!at)
            at = das.add_table((*it_g)->getNewName(), new AttrTable);

        // Some fields have "long_name" attributes,so we have to use this attribute rather than creating our own
        bool long_name_flag = false;

        for (const auto& attr:(*it_g)->getAttributes()) {
            if(attr->getName() == "long_name") {
                long_name_flag = true;
                break;
            }
        }

        if(false == long_name_flag) {
            if (f->getSPType() == TRMML2_V7)  {
                if((*it_g)->getFieldType() == 1) 
                    at->append_attr("standard_name","String","latitude");
                else if ((*it_g)->getFieldType() == 2) {
                    at->append_attr("standard_name","String","longitude");

                }

            }
            else if (f->getSPType() == TRMML3S_V7 || f->getSPType() == TRMML3M_V7) {
                if((*it_g)->getFieldType() == 1) {
                    at->append_attr("long_name","String","latitude");
                    at->append_attr("standard_name","String","latitude");

                }
                else if ((*it_g)->getFieldType() == 2) {
                    at->append_attr("long_name","String","longitude");
                    at->append_attr("standard_name","String","longitude");
                }

            }
            else 
                at->append_attr("long_name", "String", (*it_g)->getName());
        }

        // For some OBPG files that only provide slope and intercept at the file level, 
        // we need to add the global slope and intercept to all fields and change their names to scale_factor and add_offset.
        // For OBPG files that provide slope and intercept at the field level, we need to rename those attribute names to scale_factor and add_offset.
        HDFCFUtil::add_obpg_special_attrs(f,das,*it_g,scaling,slope,global_slope_flag,intercept,global_intercept_flag);

        // MAP individual SDS field to DAP DAS
        for (const auto &attr:(*it_g)->getAttributes()) {

            // Handle string first.
            if(attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){
                string tempstring2(attr->getValue().begin(),attr->getValue().end());
                string tempfinalstr= string(tempstring2.c_str());
                at->append_attr(attr->getNewName(), "String" ,tempfinalstr);
            }
            else {
                for (int loc=0; loc < attr->getCount() ; loc++) {
                    string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                    at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                }
            }
        }

        // MAP dimension info. to DAS(Currently this should only affect the OTHERHDF case when no dimension scale for some dimensions)
        // KY 2012-09-19
        // For the type DFNT_CHAR, one dimensional char array is mapped to a scalar DAP string, 
        //                         N dimensional char array is mapped to N-1 dimensional DAP string,
        // So the number of dimension info stored in the attribute container should be reduced by 1.
        // KY 2014-04-11
 
        bool has_dim_info = true;
        vector<HDFSP::AttrContainer *>::const_iterator it_end = (*it_g)->getDimInfo().end();
        if ((*it_g)->getType() == DFNT_CHAR) {
            if ((*it_g)->getRank() >1 && (*it_g)->getDimInfo().size() >1)            
                it_end = (*it_g)->getDimInfo().begin()+(*it_g)->getDimInfo().size() -1;
            else 
                has_dim_info = false;
        }
        
        if( true == has_dim_info) {

            // Note: the following loop cannot be changed to range-loop because the last element is not the end of the iterator.
            for(vector<HDFSP::AttrContainer *>::const_iterator i=(*it_g)->getDimInfo().begin();i!=it_end;i++) {

                // Here a little surgory to add the field path(including) name before dim0, dim1, etc.
                string attr_container_name = (*it_g)->getNewName() + (*i)->getName();
                AttrTable *dim_at = das.get_table(attr_container_name);
                if (!dim_at)
                    dim_at = das.add_table(attr_container_name, new AttrTable);

                for (const auto &attr:(*i)->getAttributes()) {

                    // Handle string first.
                    if (attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){
                        string tempstring2(attr->getValue().begin(),attr->getValue().end());
                        string tempfinalstr= string(tempstring2.c_str());
                        dim_at->append_attr(attr->getNewName(), "String" , tempfinalstr);
                    }
                    else {
                        for (int loc=0; loc < attr->getCount() ; loc++) {

                            string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                            dim_at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                        }
                    }
                }
            }
        }

        // Handle special CF attributes such as units, valid_range and coordinates 
        // Overwrite units if fieldtype is latitude.
        if ((*it_g)->getFieldType() == 1){

            at->del_attr("units");      // Override any existing units attribute.
            at->append_attr("units", "String",(*it_g)->getUnits());
            if (f->getSPType() == CER_ES4) // Drop the valid_range attribute since the value will be interpreted wrongly by CF tools
                at->del_attr("valid_range");
                
        }
        // Overwrite units if fieldtype is longitude
        if ((*it_g)->getFieldType() == 2){
            at->del_attr("units");      // Override any existing units attribute.
            at->append_attr("units", "String",(*it_g)->getUnits());
            if (f->getSPType() == CER_ES4) // Drop the valid_range attribute since the value will be interpreted wrongly by CF tools
                at->del_attr("valid_range");

        }

        // The following if-statement may not be necessary since fieldtype=4 is the missing CV.
        // This missing CV is added by the handler and the units is always level.
        if ((*it_g)->getFieldType() == 4){
            at->del_attr("units");      // Override any existing units attribute.
            at->append_attr("units", "String",(*it_g)->getUnits());
        }

        // Overwrite coordinates if fieldtype is neither lat nor lon.
        if ((*it_g)->getFieldType() == 0){
            at->del_attr("coordinates");      // Override any existing units attribute.

            // If no "dimension scale" dimension exists, delete the "coordinates" attributes
            if (false == f->Has_Dim_NoScale_Field()) {
                string coordinate = (*it_g)->getCoordinate();
                if (coordinate !="") 
                    at->append_attr("coordinates", "String", coordinate);
            }
        }
    }


    // For OTHERHDF products, add units for latitude and longitude; also change unit to units.
    HDFCFUtil::handle_otherhdf_special_attrs(f,das);

    // For NASA products, add missing CF attributes if possible
    HDFCFUtil::add_missing_cf_attrs(f,das);

    // Check if having _FillValue. If having _FillValue, compare the datatype of _FillValue
    // with the variable datatype. Correct the fillvalue datatype if necessary. 
    for (it_g = spsds.begin(); it_g != spsds.end(); it_g++){

        AttrTable *at = das.get_table((*it_g)->getNewName());
        if (at != nullptr) {
            int32 var_type = (*it_g)->getType();
            try {
                HDFCFUtil::correct_fvalue_type(at,var_type);
            }
            catch(...) {
                throw;
            }
        }

        // If H4.EnableCheckScaleOffsetType BES key is true,  
        //    if yes, check if having scale_factor and add_offset attributes; 
        //       if yes, check if scale_factor and add_offset attribute types are the same; 
        //          if no, make add_offset's datatype be the same as the datatype of scale_factor. 
        // (CF requires the type of scale_factor and add_offset the same). 
        if (true == HDF4RequestHandler::get_enable_check_scale_offset_type() && at !=nullptr)  
            HDFCFUtil::correct_scale_offset_type(at); 
    }

    // Optimization for users to tune the DAS output.
    HDFCFUtil::handle_merra_ceres_attrs_with_bes_keys(f,das,filename);

    // Check the EnableVdataDescAttr key. If this key is turned on, the handler-added attribute VDdescname and
    // the attributes of vdata and vdata fields will be outputed to DAS. Otherwise, these attributes will
    // not output to DAS. The key will be turned off by default to shorten the DAP output. KY 2012-09-18
    try {
        HDFCFUtil::handle_vdata_attrs_with_desc_key(f,das);
    }
    catch(...) {
        throw;
    }
        
    return true;
}

// This routine is for case 4 of the cases returned by read_das_hdfeos2.
// Creating this routine is for performance reasons. Structmetadata is
// turned off because the information has been retrieved and presented 
// by DDS and DAS.
// Currently we don't have a user case for this routine and also
// this code is not used. We still keep it for the future usage.
// KY 2014-01-29

bool read_das_special_eos2(DAS &das,const string& filename,int32 sdfd,int32 fileid,bool ecs_metadata,HDFSP::File**fpptr) {

    BESDEBUG("h4","Coming to read_das_special_eos2 " << endl);

    // Define a file pointer
    HDFSP::File *f = nullptr;

    try {
        // Obtain all the necesary information from HDF4 files.
        f = HDFSP::File::Read(filename.c_str(), sdfd,fileid);
    } 
    catch (HDFSP::Exception &e)
    {
        if (f!= nullptr)
            delete f;
        throw InternalErr(e.what());
    }
        
    try {
        // Generate CF coordinate variables(including auxiliary coordinate variables) and dimensions
        // All the names follow CF.
        f->Prepare();
    } 
    catch (HDFSP::Exception &e) {
        delete f;
        throw InternalErr(e.what());
    }

    *fpptr = f;

    try {
        read_das_special_eos2_core(das, f, filename,ecs_metadata);
    }
    catch(...) {
        throw;
    }

    // The return value is a dummy value, not used.
    return true;
}

// This routine is for special EOS2 that can be tuned to build up DAS and DDS quickly.
// We also turn off the generation of StructMetadata for the performance reason.
bool read_das_special_eos2_core(DAS &das,const HDFSP::File* f,const string& filename,bool ecs_metadata) {

    BESDEBUG("h4","Coming to read_das_special_eos2_core "<<endl); 

    // Initialize ECS metadata
    string core_metadata = "";
    string archive_metadata = "";
    string struct_metadata = "";
    
    // Obtain SD pointer, this is used to retrieve the file attributes associated with the SD interface
    const HDFSP::SD* spsd = f->getSD();
  
    //Ignore StructMetadata to improve performance 
    for (const auto &attr:spsd->getAttributes()) {
 
        // Here we try to combine ECS metadata into a string.
        if ((attr->getName().compare(0, 12, "CoreMetadata" )== 0) ||
            (attr->getName().compare(0, 12, "coremetadata" )== 0)){

            if (ecs_metadata == true) {

                // We assume that CoreMetadata.0, CoreMetadata.1, ..., CoreMetadata.n attribures
                // are processed in the right order during HDFSP::Attribute vector iteration.
                // Otherwise, this won't work.
                string tempstring(attr->getValue().begin(),attr->getValue().end());
                core_metadata.append(tempstring);
            }
        }
        else if((attr->getName().compare(0, 15, "ArchiveMetadata" )== 0) ||
                (attr->getName().compare(0, 16, "ArchivedMetadata")==0)  ||
                (attr->getName().compare(0, 15, "archivemetadata" )== 0)){
            if (ecs_metadata == true) {
                string tempstring(attr->getValue().begin(),attr->getValue().end());
                archive_metadata.append(tempstring);
            }
        }
        else if((attr->getName().compare(0, 14, "StructMetadata" )== 0) ||
                (attr->getName().compare(0, 14, "structmetadata" )== 0))
              ; // Ignore StructMetadata for performance
        else {
            //  Process gloabal attributes
            AttrTable *at = das.get_table("HDF_GLOBAL");
            if (!at)
                at = das.add_table("HDF_GLOBAL", new AttrTable);

            // We treat string differently. DFNT_UCHAR and DFNT_CHAR are treated as strings.
            if(attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){
                string tempstring2(attr->getValue().begin(),attr->getValue().end());
                auto tempfinalstr= string(tempstring2.c_str());
                at->append_attr(attr->getNewName(), "String" , tempfinalstr);
            }

            else {
                for (int loc=0; loc < attr->getCount() ; loc++) {
                    string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                    at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                }
            }
        }
    }
    
    // The following code may be condensed in the future. KY 2012-09-19
    // Coremetadata, structmetadata and archive metadata need special parsers.
    if (ecs_metadata == true) {

        // Write coremetadata.
        if (core_metadata.size() > 0){
            AttrTable *at = das.get_table("CoreMetadata");
            if (!at)
                at = das.add_table("CoreMetadata", new AttrTable);
            // tell lexer to scan attribute string
            void *buf = hdfeos_string(core_metadata.c_str());
            parser_arg arg(at);
    
            if (hdfeosparse(&arg) != 0) {
                hdfeos_delete_buffer(buf);
                throw Error("Parse error while processing a CoreMetadata attribute.");
            }
    
            // Errors returned from here are ignored.
            if (arg.status() == false) {
                ERROR_LOG("Parse error while processing a CoreMetadata attribute. (2)" << endl);
#if 0
                //for debugging
                << arg.error()->get_error_message() << endl;
#endif
            }
    
            hdfeos_delete_buffer(buf);
    
        }
                
        // Write archive metadata.
        if (archive_metadata.size() > 0){
            AttrTable *at = das.get_table("ArchiveMetadata");
            if (!at)
                at = das.add_table("ArchiveMetadata", new AttrTable);
            // tell lexer to scan attribute string
            void *buf = hdfeos_string(archive_metadata.c_str());
            parser_arg arg(at);
            if (hdfeosparse(&arg) != 0) {
                hdfeos_delete_buffer(buf);
                throw Error("Parse error while processing an ArchiveMetadata attribute.");
            }
    
            // Errors returned from here are ignored.
            if (arg.status() == false) 
                ERROR_LOG("Parse error while processing an ArchiveMetadata attribute. (2)" << endl);
    
            hdfeos_delete_buffer(buf);
        }
    }

    // Handle individual fields
    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    for (const auto &sdf:spsds){

        if(sdf->getFieldType() != 0){

            AttrTable *at = das.get_table(sdf->getNewName());
            if (!at)
                at = das.add_table(sdf->getNewName(), new AttrTable);

            string tempunits = sdf->getUnits();
            if(at->simple_find("units")== at->attr_end() && tempunits!="")
                at->append_attr("units", "String" ,tempunits);
            if(sdf->getFieldType() == 1){
                if(at->simple_find("long_name")== at->attr_end())
                    at->append_attr("long_name","String","Latitude");
            }
            else if(sdf->getFieldType() == 2) {
                if(at->simple_find("long_name")== at->attr_end())
                    at->append_attr("long_name","String","Longitude");
            }
        }
        else {// We will check if having the coordinates attribute.

            AttrTable *at = das.get_table(sdf->getNewName());
            if (!at)
                at = das.add_table(sdf->getNewName(), new AttrTable);
            string tempcoors = sdf->getCoordinate();

            // If we add the coordinates attribute, any existing coordinates attribute will be removed.
            if (tempcoors!=""){
                at->del_attr("coordinates");
                at->append_attr("coordinates","String",tempcoors);
            }
        }

        // Ignore variables that don't have attributes.
        if (sdf->getAttributes().empty())
            continue;
    
        AttrTable *at = das.get_table(sdf->getNewName());
        if (!at)
            at = das.add_table(sdf->getNewName(), new AttrTable);

        // MAP individual SDS field to DAP DAS
        for (const auto &attr:sdf->getAttributes()) {

            // Handle string first.
            if (attr->getType()==DFNT_UCHAR || attr->getType() == DFNT_CHAR){
                string tempstring2(attr->getValue().begin(),attr->getValue().end());
                auto tempfinalstr= string(tempstring2.c_str());
                at->append_attr(attr->getNewName(), "String" ,tempfinalstr);
            }
            else {
                for (int loc=0; loc < attr->getCount() ; loc++) {
                    string print_rep = HDFCFUtil::print_attr(attr->getType(), loc, (void*) &(attr->getValue()[0]));
                    at->append_attr(attr->getNewName(), HDFCFUtil::print_type(attr->getType()), print_rep);
                }
            }
        }
    }

    // Handle HDF-EOS2 object attributes. These are found in AIRS version 6.
    HDFCFUtil::map_eos2_objects_attrs(das,filename);

    return true;
}


// MOD/MYD08M3 follows the no-CF scale/offset rulea,we need to change the add_offset value when add_offset is 0.
void change_das_mod08_scale_offset(DAS &das, const HDFSP::File *f) {

    // Handle individual fields
    // Check HDFCFUtil::handle_modis_special_attrs_disable_scale_comp

    const vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();
    for (const auto &sdf:spsds) {
        if (sdf->getFieldType() == 0){
            AttrTable *at = das.get_table(sdf->getNewName());
            if (!at)
                at = das.add_table(sdf->getNewName(), new AttrTable);

            // Declare add_offset type in string format.
            string add_offset_type;

            // add_offset values
            string add_offset_value="0";
            double  orig_offset_value = 0;
            bool add_offset_modify = false;
            

            // Go through all attributes to find add_offset
            // If add_offset is 0 or add_offset is not found, we don't need
            // to modify the add_offset value.
            AttrTable::Attr_iter it = at->attr_begin();
            while (it!=at->attr_end())
            {
                if (at->get_name(it)=="add_offset")
                {
                    add_offset_value = (*at->get_attr_vector(it)->begin());
                    orig_offset_value = atof(add_offset_value.c_str());
                    add_offset_type = at->get_type(it);
                    if(add_offset_value == "0.0" || orig_offset_value == 0) 
                        add_offset_modify = false;
                    else 
                        add_offset_modify = true;
                    break;
                }
                it++;

            }
            
            // We need to modify the add_offset value if the add_offset exists.
            if (true == add_offset_modify) {

                // Declare scale_factor type in string format.
                string scale_factor_type;

                // Scale values
                string scale_factor_value="";
                double  orig_scale_value = 1;

                it = at->attr_begin();
                while (it!=at->attr_end()) 
                {
                    if(at->get_name(it)=="scale_factor")
                    {
                        scale_factor_value = (*at->get_attr_vector(it)->begin());
                        orig_scale_value = atof(scale_factor_value.c_str());
                        scale_factor_type = at->get_type(it);
                    }
                    it++;
                }

                if (scale_factor_value.size() !=0) {
                    double new_offset_value = -1 * orig_scale_value*orig_offset_value;
                    string print_rep = HDFCFUtil::print_attr(DFNT_FLOAT64,0,(void*)(&new_offset_value));
                    at->del_attr("add_offset");
                    at->append_attr("add_offset", HDFCFUtil::print_type(DFNT_FLOAT64), print_rep);
                }
            }
        }
    }
}

// Function to build special AIRS version 6 and MOD08_M3 DDS. Doing this way is for improving performance.
bool read_dds_special_1d_grid(DDS &dds,const HDFSP::File* spf,const string& filename, int32 sdid,bool check_cache) {

    BESDEBUG("h4","Coming to read_dds_special_1d_grid "<<endl);
    bool dds_cache = false;
    size_t total_bytes_dds_cache = 0;

    // Only support AIRS version 6 level 2 or level 3 KY 2015-06-07
    if (true == check_cache) {

        total_bytes_dds_cache = HDFCFUtil::obtain_dds_cache_size(spf);
        BESDEBUG("h4","Total DDS cache file size is "<< total_bytes_dds_cache<<endl);
        if(total_bytes_dds_cache !=0)
            dds_cache = true;
   
    }

    SPType sptype = OTHERHDF;
    const vector<HDFSP::SDField *>& spsds = spf->getSD()->getFields();

    // Read SDS 
    for (const auto &spsdsf:spsds) {

        BaseType *bt=nullptr;
        switch(spsdsf->getType()) {
#define HANDLE_CASE(tid, type)                                          \
            case tid:                                           \
        bt = new (type)(spsdsf->getNewName(),filename); \
        break;
        HANDLE_CASE(DFNT_FLOAT32, HDFFloat32)
        HANDLE_CASE(DFNT_FLOAT64, HDFFloat64)
        HANDLE_CASE(DFNT_CHAR, HDFStr)
#ifndef SIGNED_BYTE_TO_INT32
        HANDLE_CASE(DFNT_INT8, HDFByte)
#else
        HANDLE_CASE(DFNT_INT8,HDFInt32)
#endif
        HANDLE_CASE(DFNT_UINT8, HDFByte)
        HANDLE_CASE(DFNT_INT16, HDFInt16)
        HANDLE_CASE(DFNT_UINT16, HDFUInt16)
        HANDLE_CASE(DFNT_INT32, HDFInt32)
        HANDLE_CASE(DFNT_UINT32, HDFUInt32)
        HANDLE_CASE(DFNT_UCHAR8, HDFByte)
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
        }

        if(bt)
        {
              
            const vector<HDFSP::Dimension*>& dims= spsdsf->getDimensions();

            vector<HDFSP::Dimension*>::const_iterator it_d;

            // Char will be mapped to DAP string.
            if(DFNT_CHAR == spsdsf->getType()) {
                if(1 == spsdsf->getRank()) {
                    HDFCFStr * sca_str = nullptr;
                    try {
                        sca_str = new HDFCFStr(
                                               sdid,
                                               spsdsf->getFieldRef(),
                                               filename,
                                               spsdsf->getName(),
                                               spsdsf->getNewName(),
                                               false
                                              );
                    }
                    catch(...) {
                        delete bt;
                        throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStr instance.");
                    }
                    dds.add_var(sca_str);
                    delete bt;
                    delete sca_str;
                }

                else {
                    HDFCFStrField *ar = nullptr;
                    try {

                        ar = new HDFCFStrField(
                                               spsdsf->getRank() -1 ,
                                               filename,     
                                               false,
                                               sdid,
                                               spsdsf->getFieldRef(),
                                               0,
                                               spsdsf->getName(),
                                               spsdsf->getNewName(),
                                               bt);
 
                    }
                    catch(...) {
                        delete bt;
                        throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStrField instance.");
                    }

                    for(it_d = dims.begin(); it_d != dims.begin()+dims.size()-1; it_d++)
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                    dds.add_var(ar);
                    delete bt;
                    delete ar;
                }
 
            }

            else {// Other datatypes

                // Non missing fields
                if(spsdsf->getFieldType()!= 4) {
                    HDFSPArray_RealField *ar = nullptr;

                    try {

                        vector<int32>dimsizes;
                       
                        dimsizes.resize(spsdsf->getRank());
                        for(int i = 0; i <spsdsf->getRank();i++)
                            dimsizes[i] = (dims[i])->getSize(); 
                        ar = new HDFSPArray_RealField(
                                                      spsdsf->getRank(),
                                                      filename,
                                                      sdid,
                                                      spsdsf->getFieldRef(),
                                                      spsdsf->getType(),
                                                      sptype,
                                                      spsdsf->getName(),
                                                      dimsizes,
                                                      spsdsf->getNewName(),
                                                      bt);
                    }
                    catch(...) {
                        delete bt;
                        throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFSPArray_RealField instance.");
                    }
                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                    dds.add_var(ar);
                    delete bt;
                    delete ar;
                }
                else {
                    if(spsdsf->getRank()!=1){
                        delete bt;
                        throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                    }
                    int nelem = (spsdsf->getDimensions()[0])->getSize();
                        
                    HDFSPArrayMissGeoField *ar = nullptr;

                    try {
                        ar = new HDFSPArrayMissGeoField(
                                                        spsdsf->getRank(),
                                                        nelem,
                                                        spsdsf->getNewName(),
                                                        bt);
                    }
                    catch(...) {                
                        delete bt;
                        throw InternalErr(__FILE__,__LINE__,
                                        "Unable to allocate the HDFSPArrayMissGeoField instance.");
                    }   


                    for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                    dds.add_var(ar);
                    delete bt;
                    delete ar;
 
                }
            }
        }
    }

    // If we need to generate a DDS cache file,
    if (true == dds_cache) {

        // Check the file path
        string md_cache_dir;
        string key = "H4.Cache.metadata.path";
        bool found = false;
        TheBESKeys::TheKeys()->get_value(key,md_cache_dir,found);

        if (true == found) {

            // Create the DDS cache file name. 
            string base_file_name = basename(filename);
            string dds_filename = md_cache_dir + "/"+base_file_name +"_dds";  

            // DDS cache file is a binary file, this makes the file size smaller.
            FILE* dds_file =fopen(dds_filename.c_str(),"wb");
            if(nullptr == dds_file) {
                string msg = "Cannot create the cache file. " + dds_filename + get_errno();
                throw InternalErr(__FILE__,__LINE__,msg);
            }
            int fd = fileno(dds_file);
            struct flock *l= lock(F_WRLCK);
            if (fcntl(fd, F_SETLKW, l) == -1) {
                fclose(dds_file);
                string msg = "Cannot hold the write lock for dds cached file "+ dds_filename;
                throw InternalErr (__FILE__, __LINE__,msg);
            }
            // TRY CATCH to close fclose.
            try {
                HDFCFUtil::write_sp_sds_dds_cache(spf,dds_file,total_bytes_dds_cache,dds_filename);
            }
            catch(...) {
                if (fcntl(fd, F_SETLK, lock(F_UNLCK)) == -1) {
                    fclose(dds_file);
                    string msg = "Cannot release the write lock for dds cached file "+ dds_filename;
                    throw InternalErr (__FILE__, __LINE__,msg);
                }
 
                fclose(dds_file);
                throw InternalErr(__FILE__,__LINE__,"Fail to generate a dds cache file.");
            }
            if (fcntl(fd, F_SETLK, lock(F_UNLCK)) == -1) {
                fclose(dds_file);
                string msg = "Cannot release the write lock for dds cached file "+ dds_filename;
                throw InternalErr (__FILE__, __LINE__,msg);
            }
            fclose(dds_file);
            
        }

        else {
            throw InternalErr (__FILE__, __LINE__, 
                  "DDS/DAS metadata cache path cannot be found when 'H4.EnableMetaDataCacheFile' key is set to be true.");
        }
    }

    return true;

}

// Read SDS fields
void read_dds_spfields(DDS &dds,const string& filename,const int sdfd,const HDFSP::SDField *spsds, SPType sptype) {

    BESDEBUG("h4","Coming to read_dds_spfields "<<endl);

    // Ignore the dimension variable that is empty for non-special handling NASA HDF products
    if(OTHERHDF == sptype && (true == spsds->IsDimNoScale()))
        return;
    
    BaseType *bt=nullptr;
    switch(spsds->getType()) {

#define HANDLE_CASE(tid, type)                                          \
    case tid:                                           \
        bt = new (type)(spsds->getNewName(),filename); \
        break;
        HANDLE_CASE(DFNT_FLOAT32, HDFFloat32)
        HANDLE_CASE(DFNT_FLOAT64, HDFFloat64)
        HANDLE_CASE(DFNT_CHAR, HDFStr)
#ifndef SIGNED_BYTE_TO_INT32
        HANDLE_CASE(DFNT_INT8, HDFByte)
        //HANDLE_CASE(DFNT_CHAR, HDFByte);
#else
        HANDLE_CASE(DFNT_INT8,HDFInt32)
        //HANDLE_CASE(DFNT_CHAR, HDFInt32);
#endif
        HANDLE_CASE(DFNT_UINT8, HDFByte) 
        HANDLE_CASE(DFNT_INT16, HDFInt16)
        HANDLE_CASE(DFNT_UINT16, HDFUInt16)
        HANDLE_CASE(DFNT_INT32, HDFInt32)
        HANDLE_CASE(DFNT_UINT32, HDFUInt32)
        HANDLE_CASE(DFNT_UCHAR, HDFByte)
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }
    int fieldtype = spsds->getFieldType();// Whether the field is real field,lat/lon field or missing Z-dimension field 
             
    if(bt)
    {
              
        const vector<HDFSP::Dimension*>& dims= spsds->getCorrectedDimensions();
        vector<HDFSP::Dimension*>::const_iterator it_d;

        if(DFNT_CHAR == spsds->getType()) {

            if(1 == spsds->getRank()) {

                HDFCFStr * sca_str = nullptr;

                try {

                    sca_str = new HDFCFStr(
                                          sdfd,
                                          spsds->getFieldRef(),
                                          filename,
                                          spsds->getName(),
                                          spsds->getNewName(),
                                          false
                                          );
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStr instance.");
                }
                dds.add_var(sca_str);
                delete bt;
                delete sca_str;
            }
            else {
                HDFCFStrField *ar = nullptr;
                try {

                    ar = new HDFCFStrField(
                                           spsds->getRank() -1 ,
                                           filename,     
                                           false,
                                           sdfd,
                                           spsds->getFieldRef(),
                                           0,
                                           spsds->getName(),
                                           spsds->getNewName(),
                                           bt);
 
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStrField instance.");
                }

                for(it_d = dims.begin(); it_d != dims.begin()+dims.size()-1; it_d++)
                    ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                dds.add_var(ar);
                delete bt;
                delete ar;
            }
 
        }

        // For non-CV variables and the existing non-lat/lon CV variables
        else if(fieldtype == 0 || fieldtype == 3 ) {

            HDFSPArray_RealField *ar = nullptr;

            try {
                vector<int32>dimsizes;
                dimsizes.resize(spsds->getRank());
                for(int i = 0; i <spsds->getRank();i++)
                    dimsizes[i] = (dims[i])->getSize(); 
 
                ar = new HDFSPArray_RealField(
                                              spsds->getRank(),
                                              filename,
                                              sdfd,
                                              spsds->getFieldRef(),
                                              spsds->getType(),
                                              sptype,
                                              spsds->getName(),
                                              dimsizes,
                                              spsds->getNewName(),
                                              bt);
            }
            catch(...) {
                delete bt;
                throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFSPArray_RealField instance.");
            }

            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
            dds.add_var(ar);
            delete bt;
            delete ar;
        }

        // For latitude and longitude
        else if(fieldtype == 1 || fieldtype == 2) {

            if (sptype == MODISARNSS || sptype == TRMML2_V7) { 
                HDFSPArray_RealField *ar = nullptr;
                try {

                    vector<int32>dimsizes;
                       
                    dimsizes.resize(spsds->getRank());
                    for(int i = 0; i <spsds->getRank();i++)
                        dimsizes[i] = (dims[i])->getSize(); 
 
                    ar = new HDFSPArray_RealField(
                                                  spsds->getRank(),
                                                  filename,
                                                  sdfd,
                                                  spsds->getFieldRef(),
                                                  spsds->getType(),
                                                  sptype,
                                                  spsds->getName(),
                                                  dimsizes,
                                                  spsds->getNewName(),
                                                  bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,
                                "Unable to allocate the HDFSPArray_RealField instance.");
                }


                for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                    ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                dds.add_var(ar);
                delete bt;
                delete ar;

            }
            else {

                HDFSPArrayGeoField *ar = nullptr;

                try {
                    ar = new HDFSPArrayGeoField(
                                                spsds->getRank(),
                                                filename,
                                                sdfd,
                                                spsds->getFieldRef(),
                                                spsds->getType(),
                                                sptype,
                                                fieldtype,
                                                spsds->getName(),
                                                spsds->getNewName(), 
                                                bt);
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,
                                "Unable to allocate the HDFSPArray_RealField instance.");
                }

                for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                    ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                dds.add_var(ar);
                delete bt;
                delete ar;
            }
        }
                    
                    
        else if(fieldtype == 4) { //missing Z dimensional field(or coordinate variables with missing values)
            if(spsds->getRank()!=1){
                delete bt;
                throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
            }
            int nelem = (spsds->getDimensions()[0])->getSize();
                        
            HDFSPArrayMissGeoField *ar = nullptr;

            try {
                ar = new HDFSPArrayMissGeoField(
                                            spsds->getRank(),
                                            nelem,
                                            spsds->getNewName(),
                                            bt);
            }
            catch(...) {                
                delete bt;
                throw InternalErr(__FILE__,__LINE__,
                                "Unable to allocate the HDFSPArrayMissGeoField instance.");
            }   


            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
            dds.add_var(ar);
            delete bt;
            delete ar;
        }
        // fieldtype =5 originally keeps for time. Still keep it for a while.

        else if(fieldtype == 6) { //Coordinate variables added from the product specification

            if(spsds->getRank()!=1){
                delete bt;
                throw InternalErr(__FILE__, __LINE__, "The rank of added coordinate variable  must be 1");
            }
            int nelem = (spsds->getDimensions()[0])->getSize();

            HDFSPArrayAddCVField *ar = nullptr;
            try {
                ar = new HDFSPArrayAddCVField(
                                            spsds->getType(),
                                            sptype,
                                            spsds->getName(),
                                            nelem,
                                            spsds->getNewName(),
                                            bt);
            }
            catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,
                                "Unable to allocate the HDFSPArrayAddCVField instance.");
            }


            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
            dds.add_var(ar);
            delete bt;
            delete ar;
        }
        else {
            delete bt;
            throw InternalErr(__FILE__, __LINE__, "The field type should be one of  0,1,2,3,4 or 6.");
 
        }
    }

}

// Read Vdata fields.
void read_dds_spvdfields(DDS &dds,const string & filename, const int fileid,int32 objref,int32 numrec,HDFSP::VDField *spvd) {

    BESDEBUG("h4","Coming to read_dds_spvdfields "<<endl);

    // First map the HDF4 datatype to DAP2
    BaseType *bt=nullptr;
    switch(spvd->getType()) {
#define HANDLE_CASE(tid, type)                                          \
    case tid:                                           \
        bt = new (type)(spvd->getNewName(),filename); \
        break;
        HANDLE_CASE(DFNT_FLOAT32, HDFFloat32)
        HANDLE_CASE(DFNT_FLOAT64, HDFFloat64)
        HANDLE_CASE(DFNT_CHAR8,HDFStr)
#ifndef SIGNED_BYTE_TO_INT32
        HANDLE_CASE(DFNT_INT8, HDFByte)
#else
        HANDLE_CASE(DFNT_INT8,HDFInt32)
#endif
        HANDLE_CASE(DFNT_UINT8, HDFByte)
        HANDLE_CASE(DFNT_INT16, HDFInt16)
        HANDLE_CASE(DFNT_UINT16, HDFUInt16)
        HANDLE_CASE(DFNT_INT32, HDFInt32)
        HANDLE_CASE(DFNT_UINT32, HDFUInt32)
        HANDLE_CASE(DFNT_UCHAR8, HDFByte)
        //HANDLE_CASE(DFNT_CHAR8, HDFByte)
        //HANDLE_CASE(DFNT_CHAR8, HDFByte)
        default:
            throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
    }
             
    if(bt)
    {

        if(DFNT_CHAR == spvd->getType()) {

            // If the field order is >1, the vdata field will be 2-D array
            // with the number of elements along the fastest changing dimension
            // as the field order.
            int vdrank = ((spvd->getFieldOrder())>1)?2:1;
            if (1 == vdrank) {
 
                HDFCFStr * sca_str = nullptr;
                try {
                    sca_str = new HDFCFStr(
                                          fileid,
                                          objref,
                                          filename,
                                          spvd->getName(),
                                          spvd->getNewName(),
                                          true
                                          );
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStr instance.");
                }
                dds.add_var(sca_str);
                delete bt;
                delete sca_str;
            }

            else {

                HDFCFStrField *ar = nullptr;
                try {

                    ar = new HDFCFStrField(
                                           vdrank -1 ,
                                           filename,     
                                           true,
                                           fileid,
                                           objref,
                                           spvd->getFieldOrder(),
                                           spvd->getName(),
                                           spvd->getNewName(),
                                           bt);
 
                }
                catch(...) {
                    delete bt;
                    throw InternalErr(__FILE__,__LINE__,"Unable to allocate the HDFCFStrField instance.");
                }

                string dimname0 = "VDFDim0_"+spvd->getNewName();  
                ar->append_dim(numrec, dimname0);
                dds.add_var(ar);
                delete bt;
                delete ar;

            }
        }
        else {
            HDFSPArray_VDField *ar = nullptr;

            // If the field order is >1, the vdata field will be 2-D array
            // with the number of elements along the fastest changing dimension
            // as the field order.
            int vdrank = ((spvd->getFieldOrder())>1)?2:1;
            ar = new HDFSPArray_VDField(
                                        vdrank,
                                        filename,
                                        fileid,
                                        objref,
                                        spvd->getType(),
                                        spvd->getFieldOrder(),
                                        spvd->getName(),
                                        spvd->getNewName(),
                                        bt);

            string dimname1 = "VDFDim0_"+spvd->getNewName();

            string dimname2 = "VDFDim1_"+spvd->getNewName();
            if(spvd->getFieldOrder() >1) {
                ar->append_dim(numrec,dimname1);
                ar->append_dim(spvd->getFieldOrder(),dimname2);
            }
            else 
                ar->append_dim(numrec,dimname1);

            dds.add_var(ar);
            delete bt;
            delete ar;
        }
    }

}

// This routine will check if this is a special EOS2 file that we can improve the performance
// Currently AIRS level 2 and 3 version 6 and MOD08_M3-like products are what we can serve. KY 2014-01-29
int check_special_eosfile(const string & filename, string& grid_name,int32 sdfd) {

    int32 sds_id     = 0;
    int32 n_sds      = 0;
    int32 n_sd_attrs = 0;
    bool is_eos      = false;
    int ret_val      = 0;

    // Obtain number of SDS objects and number of SD(file) attributes
    if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL){
        throw InternalErr (__FILE__,__LINE__,"SDfileinfo failed ");
    }

    char attr_name[H4_MAX_NC_NAME];
    int32 attr_type     = -1;
    int32 attr_count    = -1;
    char structmdname[] = "StructMetadata.0";

    // Is this an HDF-EOS2 file? 
    for (int attr_index = 0; attr_index < n_sd_attrs;attr_index++) {
        if(SDattrinfo(sdfd,attr_index,attr_name,&attr_type,&attr_count) == FAIL) {
            throw InternalErr (__FILE__,__LINE__,"SDattrinfo failed ");
        }

        if(strcmp(attr_name,structmdname)==0)  {
            is_eos = true;
            break;
        }
    }
    
    if(true == is_eos) {

        int sds_index  = 0;
        int32 sds_rank   = 0;
        int32 dim_sizes[H4_MAX_VAR_DIMS];
        int32 sds_dtype = 0;
        int32  n_sds_attrs = 0;
        char sds_name[H4_MAX_NC_NAME];
        char xdim_name[] ="XDim";
        char ydim_name[] ="YDim";
   
        string temp_grid_name1;
        string temp_grid_name2;
        bool   xdim_is_cv_flag = false;
        bool   ydim_is_cv_flag = false;


        // The following for-loop checks if this is a MOD08_M3-like HDF-EOS2 product.
        for (sds_index = 0; sds_index < n_sds; sds_index++) {
        
            sds_id = SDselect (sdfd, sds_index);
            if (sds_id == FAIL) {
                throw InternalErr (__FILE__,__LINE__,"SDselect failed ");
            }

            // Obtain object name, rank, size, field type and number of SDS attributes
            int status = SDgetinfo (sds_id, sds_name, &sds_rank, dim_sizes,
                                              &sds_dtype, &n_sds_attrs);
            if (status == FAIL) {
                SDendaccess(sds_id);
                throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
            }

            if (1 == sds_rank) {

                // This variable "XDim" exists
                if(strcmp(sds_name,xdim_name) == 0) { 
                    int32 sds_dimid = SDgetdimid(sds_id,0);          
                    if(sds_dimid == FAIL) {
                        SDendaccess(sds_id);
                        throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
                    }
                    char dim_name[H4_MAX_NC_NAME];
                    int32 dim_size = 0;
                    int32 dim_type = 0;
                    int32 num_dim_attrs = 0;
                    if(SDdiminfo(sds_dimid,dim_name,&dim_size,&dim_type,&num_dim_attrs) == FAIL) {
                        SDendaccess(sds_id);
                        throw InternalErr(__FILE__,__LINE__,"SDdiminfo failed ");
                    }

                    // No dimension scale and XDim exists
                    if(0 == dim_type) {
                        string tempdimname(dim_name);
                        if(tempdimname.size() >=5) {
                            if(tempdimname.compare(0,5,"XDim:") == 0) {

                                // Obtain the grid name.
                                temp_grid_name1 = tempdimname.substr(5);
                                xdim_is_cv_flag = true;

                            }
                        }
                        else if("XDim" == tempdimname)
                            xdim_is_cv_flag = true;
                    }
                }

                // The variable "YDim" exists
                if(strcmp(sds_name,ydim_name) == 0) {

                    int32 sds_dimid = SDgetdimid(sds_id,0);          
                    if(sds_dimid == FAIL) {
                        SDendaccess (sds_id);
                        throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
                    }
                    char dim_name[H4_MAX_NC_NAME];
                    int32 dim_size = 0;
                    int32 dim_type = 0;
                    int32 num_dim_attrs = 0;
                    if(SDdiminfo(sds_dimid,dim_name,&dim_size,&dim_type,&num_dim_attrs) == FAIL) {
                        SDendaccess(sds_id);
                        throw InternalErr(__FILE__,__LINE__,"SDdiminfo failed ");
                    }

                    // For this case, the dimension should not have dimension scales.
                    if(0 == dim_type) {
                        string tempdimname(dim_name);
                        if(tempdimname.size() >=5) {
                            if(tempdimname.compare(0,5,"YDim:") == 0) {
                                // Obtain the grid name.
                                temp_grid_name2 = tempdimname.substr(5);
                                ydim_is_cv_flag = true;
                            }
                        }
                        else if ("YDim" == tempdimname)
                            ydim_is_cv_flag = true;
                    }
                }
            }

            SDendaccess(sds_id);
            if((true == xdim_is_cv_flag) && (true == ydim_is_cv_flag ))
              break;

        }

        // If one-grid and variable XDim/YDim exist and also they don't have dim. scales,we treat this as MOD08-M3-like products
        if ((temp_grid_name1 == temp_grid_name2) && (true == xdim_is_cv_flag) && (true == ydim_is_cv_flag)) {
            grid_name = temp_grid_name1;
            ret_val = 2;
        }

        // Check if this is a new AIRS level 2 and 3 product. Since new AIRS level 2 and 3 version 6 products still have dimensions that don't have
        // dimension scales and the old way to handle level 2 and 3 dimensions makes the performance suffer. We will see if we can improve
        // performance by handling the data with just the HDF4 interfaces.
        // At least the file name should have string AIRS.L3. or AIRS.L2..
        else if((basename(filename).size() >8) && (basename(filename).compare(0,4,"AIRS") == 0) 
                && ((basename(filename).find(".L3.")!=string::npos) || (basename(filename).find(".L2.")!=string::npos))){ 

            bool has_dimscale = false;

            // Go through the SDS object and check if this file has dimension scales.
            for (sds_index = 0; sds_index < n_sds; sds_index++) {
        
                sds_id = SDselect (sdfd, sds_index);
                if (sds_id == FAIL) {
                    throw InternalErr (__FILE__,__LINE__,"SDselect failed ");
                }

                // Obtain object name, rank, size, field type and number of SDS attributes
                int status = SDgetinfo (sds_id, sds_name, &sds_rank, dim_sizes,
                                              &sds_dtype, &n_sds_attrs);
                if (status == FAIL) {
                    SDendaccess(sds_id);
                    throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
                }

                for (int dim_index = 0; dim_index<sds_rank; dim_index++) {
            
                    int32 sds_dimid = SDgetdimid(sds_id,dim_index);          
                    if(sds_dimid == FAIL) {
                        SDendaccess(sds_id);
                        throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
                    }

                    char dim_name[H4_MAX_NC_NAME];
                    int32 dim_size = 0;
                    int32 dim_type = 0;
                    int32 num_dim_attrs = 0;
                    if(SDdiminfo(sds_dimid,dim_name,&dim_size,&dim_type,&num_dim_attrs) == FAIL) {
                        SDendaccess(sds_id);
                        throw InternalErr(__FILE__,__LINE__,"SDdiminfo failed ");
                    }

                    if(dim_type !=0) {
                        has_dimscale = true;
                        break;
                    }
                }
                SDendaccess(sds_id);
                if( true == has_dimscale) 
                    break;
            }

            // If having dimension scales, this is an AIRS level 2 or 3 version 6. Treat it differently. Otherwise, this is an old AIRS level 3 product.
            if (true == has_dimscale)
                ret_val = 3;
        }
        else 
            ret_val = 1;
#if 0
        else {// Check if this is an HDF-EOS2 file but not using HDF-EOS2 at all.
             // We turn off this for the time being because 
             // 1) We need to make sure this is a grid file not swath or point file. 
             // It will be time consuming to identify grids or swaths and hurts the performance for general case.    
             // 2) No real NASA files exist. We will handle them later.
             // KY 2014-01-29
             ;
            bool has_dimscale = true;
            bool is_grid      = false;

            // Go through the SDS object
            for (sds_index = 0; sds_index < n_sds; sds_index++) {
        
                sds_id = SDselect (sdid, sds_index);
                if (sds_id == FAIL) {
                    SDend(sdid);
                    throw InternalErr (__FILE__,__LINE__,"SDselect failed ");
                }

                // Obtain object name, rank, size, field type and number of SDS attributes
                int status = SDgetinfo (sds_id, sds_name, &sds_rank, dim_sizes,
                                                      &sds_dtype, &n_sds_attrs);
                if (status == FAIL) {
                    SDendaccess(sds_id);
                    SDend(sdid);
                    throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
                }


                for (int dim_index = 0; dim_index<sds_rank; dim_index++) {
            
                    int32 sds_dimid = SDgetdimid(sds_id,dim_index);          
                    if(sds_dimid == FAIL) {
                        SDendaccess(sds_id);
                        SDend(sdid);
                        throw InternalErr (__FILE__,__LINE__,"SDgetinfo failed ");
                    }
                    char dim_name[H4_MAX_NC_NAME];
                    int32 dim_size = 0;
                    int32 dim_type = 0;
                    int32 num_dim_attrs = 0;
                    if(SDdiminfo(sds_dimid,dim_name,&dim_size,&dim_type,&num_dim_attrs) == FAIL) {
                        SDendaccess(sds_id);
                        SDend(sdid);
                        throw InternalErr(__FILE__,__LINE__,"SDdiminfo failed ");
                    }

                    if(0 == dim_type) {
                        has_dimscale = false;
                    }

                }
                SDendaccess(sds_id);
            }
            if (true == has_dimscale)
                ret_val = 4;
        }
#endif
    }

    return ret_val;
}

// Generate DAS for the file that only use SDS APIs. Currently this routine only applies to AIRS version 6 
// that can take advantage of the handler's metadata cache feature.
void read_das_sds(DAS & das, const string & filename,int32 sdfd, bool ecs_metadata,HDFSP::File**h4fileptr) {

    HDFSP::File *spf = nullptr; 
    try {
        spf  = HDFSP::File::Read(filename.c_str(),sdfd,-1);
        spf->Handle_AIRS_L23();
        read_das_special_eos2_core(das,spf,filename,ecs_metadata);
    }
    catch (HDFSP::Exception &e)
    {
        if (spf != nullptr)
            delete spf;
        throw InternalErr(e.what());
    }

    *h4fileptr = spf;
    return;
}

// Generate DDS for the file that only use SDS APIs. Currently this routine only applies to AIRS version 6 
// that can take advantage of the handler's metadata cache feature.
void read_dds_sds(DDS &dds, const string & filename,int32 sdfd, HDFSP::File*h4file,bool dds_setcache) {

    // Set DDS dataset.
    dds.set_dataset_name(basename(filename));
    read_dds_special_1d_grid(dds,h4file,filename,sdfd,dds_setcache);
    return;

}
// Default option 
void read_dds(DDS & dds, const string & filename)
{
    // generate DDS, DAS
    DAS das;
    dds.set_dataset_name(basename(filename));
    build_descriptions(dds, das, filename);

    if (!dds.check_semantics()) {       // DDS didn't get built right
        THROW(dhdferr_ddssem);
    }
    return;
}

void read_das(DAS & das, const string & filename)
{
    // generate DDS, DAS
    DDS dds(nullptr);
    dds.set_dataset_name(basename(filename));

    build_descriptions(dds, das, filename);

    if (!dds.check_semantics()) {       // DDS didn't get built right
        dds.print(cout);
        THROW(dhdferr_ddssem);
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
    // If CF option is enabled, StructMetadata will be parsed here.
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

    explicit accum_attr(const string & named):d_named(named) {
    }

    hdf_genvec & operator() (hdf_genvec & accum, const hdf_attr & attr) {
        // Assume that all fields with the same base name should be combined,
        // and assume that they are in order.
        BESDEBUG("h4",  "attr.name: " << attr.name << endl);
        if (attr.name.find(d_named) != string::npos) {
#if 0
            string stuff;
            stuff.assign(attr.values.data(), attr.values.size());
            cerr << "Attribute chunk: " << attr.name << endl;
            cerr << stuff << endl;
#endif
            accum.append(attr.values.number_type(), attr.values.data(),
                attr.values.size());
            return accum;
        }
        else {
            return accum;
        }
    }
};

struct is_named:public unary_function < hdf_attr, bool > {
    string d_named;

    explicit is_named(const string & named):d_named(named) {
    }

    bool operator() (const hdf_attr & attr) {
        return (attr.name.find(d_named) != string::npos);
    }
};

static void
merge_split_eos_attributes(vector < hdf_attr > &attr_vec, 
                           const string & attr_name)
{
    // Only do this if there's more than one part.
    if (count_if(attr_vec.begin(), attr_vec.end(), is_named(attr_name)) > 1) {
        // Merge all split up parts named `attr_name.' Assume they are in
        // order in `attr_vec.'
        hdf_genvec attributes;
        attributes = accumulate(attr_vec.begin(), attr_vec.end(),
                                attributes, accum_attr(attr_name));

        // When things go south, check out the hdf_genvec...
        // BEDEBUG seems not providing a way to handle the following debugging info.
        // I can define a vector and call attributes.print(s_m), then use
        // BESDEBUG to output the debugging info. The downside is that whether BESDEBUG 
        // is called, a vector of s_m will always be generated and a chunk of memory is
        // always used. So don't change this for the time being. KY 2012-09-13
        DBG(vector < string > s_m;
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

    // TODO Remove these attributes (name and dimension)? jhrg 8/17/11
    // ***
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
                ERROR_LOG("unknown tag: " << tag << " ref: " << ref << endl);
                // TODO: Make this an exception? jhrg 8/19/11
                // Don't make an exception. Possibly you will meet other valid tags. Need to know if it
                // is worth to tackle this. KY 09/13/12
                // cerr << "unknown tag: " << tag << " ref: " << ref << endl;
                break;
            }// switch (tag) 
        } //     for (uint32 i = 0; i < vg->tags.size(); i++) 
    } //   for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) 
    // Build DDS for all toplevel vgroups
    BaseType *pbt = nullptr;
    for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) {
        if (!v->second.toplevel)
            continue;           // skip over non-toplevel vgroups
        pbt = NewStructureFromVgroup(v->second.vgroup,
                                     vgmap, sdmap, vdmap,
                                     grmap, filename);
        if (pbt != nullptr) {
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
        if (pbt != nullptr) {
            dds.add_var(pbt);
            delete pbt;
        }
    }

    // add lone Vdata's
    for (VDI v = vdmap.begin(); v != vdmap.end(); ++v) {
        if (v->second.in_vgroup)
            continue;           // skip over Vdata in vgroups
        pbt = NewSequenceFromVdata(v->second.vdata, filename);
        if (pbt != nullptr) {
            dds.add_var(pbt);
            delete pbt;
        }
    }
    // add lone GR's
    for (GRI g = grmap.begin(); g != grmap.end(); ++g) {
        if (g->second.in_vgroup)
            continue;           // skip over GRs in vgroups
        pbt = NewArrayFromGR(g->second.gri, filename);
        if (pbt != nullptr) {
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
    AddHDFAttr(das, string("HDF_GLOBAL"), fileattrs); // add GR file attributes

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
    string tempname = varname;
    AttrTable *atp = das.get_table(tempname);
    if (atp == nullptr) {
        atp = new AttrTable;
        atp = das.add_table(tempname, atp);
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

                // cerr << "About to print attributes to be parsed..." << endl;
                // TODO: remove when done!
                // cerr << "attv[" << j << "]" << endl << attv[j].c_str() << endl;

                parser_arg arg(at);
                // HDF-EOS attribute parsing is complex and some errors are
                // tolerated. Thus, if the parser proper returns an error,
                // that results in an exception that is fatal. However, if
                // the status returned by an otherwise successful parse shows
                // an error was encountered but successful parsing continued,
                // that's OK, but it should be logged.
                //
                // Also, HDF-EOS files should be read using the new HDF-EOS
                // features and not this older parser. jhrg 8/18/11
                //
                // TODO: How to log (as opposed to using BESDEBUG)?
                if (hdfeosparse(&arg) != 0){
                    hdfeos_delete_buffer(buf);
                    throw Error("HDF-EOS parse error while processing a " + container_name + " HDFEOS attribute.");
                }

                // We don't use the parse_error for this case since it generates memory leaking. KY 2014-02-25
                if (arg.status() == false) {
                    ERROR_LOG("HDF-EOS parse error while processing a "
                    << container_name << " HDFEOS attribute. (2)" << endl);
                    //<< arg.error()->get_error_message() << endl;
                }

                hdfeos_delete_buffer(buf);
            }
            else {
                // Special characters are handled in libdap4. So the following code block is commented out. KY 2022-11-16
#if 0
                if (attrtype == "String")
#ifdef ATTR_STRING_QUOTE_FIX
                    attv[j] = escattr(attv[j]);
                     
#else
                attv[j] = "\"" + escattr(attv[j]) + "\"";
#endif
#endif

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
    if (atp == nullptr) {
        atp = new AttrTable;
        atp = das.add_table(varname, atp);
    }
    // add the annotations to the DAS
    string an;
    for (int i = 0; i < (int) anv.size(); ++i) {        // for each annotation
        // Special characters are handled in libdap4. So the following code block is commented out. KY 2022-11-16
        an = anv[i];
#if 0
#ifdef ATTR_STRING_QUOTE_FIX
        an = escattr(anv[i]);     // quote strings
#else
        an = "\"" + escattr(anv[i]) + "\"";     // quote strings
#endif
#endif
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
            if (palv[i].name.size() != 0) {
                pattr.name = palname + "_name";
                pattr.values = hdf_genvec(DFNT_CHAR,
                                          const_cast <
                                          char *>(palv[i].name.c_str()),
                                          palv[i].name.size());
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
    if (dim.name.size() != 0) {
        dattr.name = "name";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.name.c_str()),
                       dim.name.size());
        dattrs.push_back(dattr);
    }
    if (dim.label.size() != 0) {
        dattr.name = "long_name";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.label.c_str()),
                       dim.label.size());
        dattrs.push_back(dattr);
    }
    if (dim.unit.size() != 0) {
        dattr.name = "units";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.unit.c_str()),
                       dim.unit.size());
        dattrs.push_back(dattr);
    }
    if (dim.format.size() != 0) {
        dattr.name = "format";
        dattr.values =
            hdf_genvec(DFNT_CHAR, const_cast < char *>(dim.format.c_str()),
                       dim.format.size());
        dattrs.push_back(dattr);
    }
    return dattrs;
}

// The following functions handle the direct mapping from HDF4 to DMR.
void read_dmr(DMR *dmr, const string &filename) {

    BESDEBUG("h4"," Begin read_dmr()"<<endl);
    // H open
    int32 fileid = Hopen(filename.c_str(), DFACC_READ,0);
    if (-1 == fileid) {
        string invalid_file_msg="HDF4 SDstart error for the file ";
        invalid_file_msg +=filename;
        invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
        throw InternalErr(__FILE__,__LINE__,invalid_file_msg);
    }

    int32 sdfd = SDstart(filename.c_str(), DFACC_READ);
    if( -1 == sdfd){
        Hclose(fileid);
        string invalid_file_msg="HDF4 SDstart error for the file ";
        invalid_file_msg +=filename;
        invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
        throw InternalErr(__FILE__,__LINE__,invalid_file_msg);
    }

    D4Group* root_grp = dmr->root();

    // It is possible that there are SDS and Vdata objects that don't belong to any vgroups.
    // We must handle lone SDS objects first because DAP4 requires a dimension with a name 
    // to be in the front of a dmr.
    // We also need to map SD file attributes to DAP4 root group attributes.
    // TODO: possible we need to handle HDF-EOS2 swath dimensions. It needs special care. 

    // SDS dimensions and lone SDS objects 
    handle_sds_dims(root_grp,fileid, sdfd);
    read_lone_sds(root_grp,fileid,sdfd,filename);

    // Lone vdata: We find one NASA file has lone vdata, so we need to handle this case.
    read_lone_vdata(root_grp,fileid,sdfd,filename);

    // SD file attributes under the root group.
    read_sd_attrs(root_grp,fileid, sdfd);

    // Now go to handle HDF4 vgroups and the objects attached to these vgroups.
    read_dmr_vlone_groups(root_grp, fileid, sdfd, filename);

    Hclose(fileid);
    SDend(sdfd);

    BESDEBUG("h4"," End read_dmr()"<<endl);
}

void handle_sds_dims(D4Group *root_grp, int32 fileid, int32 sdfd) {

    BESDEBUG("h4"," Begin handle_sds_dims() "<<endl);
    int32 n_sds      = 0;       
    int32 n_sd_attrs = 0;

    // Obtain number of SDS objects and number of SD(file) attributes
    if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL){
        close_vgroup_fileids(fileid,sdfd,-1);
        throw InternalErr (__FILE__,__LINE__,"SDfileinfo failed ");
    }

    D4Dimensions *dims = root_grp->dims();

    // Create an unordered_set for SDS dimension names.
    // Unlike SDS, an SDS dimension name is unique.
    unordered_set<string> sds_dimname_set;
    for (int i = 0; i <n_sds; i++) {

        int32 sds_id  = SDselect(sdfd,i);

        if (sds_id == FAIL) {
            close_vgroup_fileids(fileid,sdfd,-1);
            throw InternalErr(__FILE__, __LINE__, "Fail to obtain the SDS ID.");
        }

        // Obtain object name, rank, size, field type and number of attributes of an SDS
        int32  dim_sizes[H4_MAX_VAR_DIMS];
        int32  sds_rank = 0;
        int32  sds_type = 0;
        int32 n_sds_attrs = 0;
        if (FAIL == SDgetinfo (sds_id, nullptr, &sds_rank, dim_sizes, &sds_type, &n_sds_attrs)) {
            SDendaccess(sds_id);
            close_vgroup_fileids(fileid,sdfd,-1);
            throw InternalErr(__FILE__, __LINE__, "Fail to obtain SDS info.");
        }                        

        for (int dimindex = 0; dimindex <sds_rank; dimindex++) {

            int dimid = SDgetdimid (sds_id, dimindex);
            if (dimid == FAIL) {
                SDendaccess (sds_id);
                close_vgroup_fileids(fileid,sdfd,-1);
                throw InternalErr(__FILE__, __LINE__, "SDgetdimid failed.");
            }
            char dim_name[H4_MAX_NC_NAME];
            int32 dim_size = 0;
            int32 dim_type = 0;
    
            // Number of dimension attributes(This is almost never used)
            int32  num_dim_attrs = 0;
    
            intn status = SDdiminfo (dimid, dim_name, &dim_size, &dim_type,
                                &num_dim_attrs);
    
            if (status == FAIL) {
                SDendaccess (sds_id);
                close_vgroup_fileids(fileid,sdfd,-1);
                throw InternalErr(__FILE__, __LINE__, "SDdiminfo failed.");
            }
    
            if (dim_size == 0 && dim_sizes[dimindex] == 0) 
                continue;
            string dim_name_str (dim_name);

            // Make dimension names to follow the CF rule.
            dim_name_str = HDFCFUtil::get_CF_string(dim_name_str);
            
            auto it_set = sds_dimname_set.insert(dim_name_str);
            if (it_set.second) {
                auto d4_dim0_unique = make_unique<D4Dimension>(dim_name_str,dim_sizes[dimindex]);
                auto d4_dim0 = d4_dim0_unique.release();
                dims->add_dim_nocopy(d4_dim0);
            }
        }
        SDendaccess(sds_id);
    }

    BESDEBUG("h4"," End handle_sds_dims() "<<endl);
}

void read_lone_sds(D4Group *root_grp, int32 file_id,int32 sdfd, const string &filename) {

    BESDEBUG("h4"," Begin read_lone_sds() "<<endl);

    unordered_set<int32> lone_sds_refs;

    obtain_all_sds_refs(file_id,sdfd,lone_sds_refs);
    exclude_all_sds_refs_in_vgroups(file_id,sdfd,lone_sds_refs); 

    // Convert  unorder set to set. It is necessary to ensure the same order of objects on different platforms.
    set<int32> ordered_lone_sds_refs(lone_sds_refs.begin(),lone_sds_refs.end());

   // Map lone SDS objects and put them under  DAP4's root group.
   for (const auto &sds_ref:ordered_lone_sds_refs) {
        convert_sds(file_id, sdfd, -1,sds_ref, root_grp, filename,false);  
   }

   BESDEBUG("h4"," End read_lone_sds() "<<endl);
}

void obtain_all_sds_refs(int32 file_id, int32 sdfd, unordered_set<int32>& sds_ref) {

    BESDEBUG("h4"," Begin obtain_all_sds_refs() "<<endl);
    int32 n_sds      = 0;       
    int32 n_sd_attrs = 0;

    // Obtain number of SDS objects and number of SD(file) attributes
    if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL){
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr (__FILE__,__LINE__,"SDfileinfo failed ");
    }

    for (int i = 0; i <n_sds; i++) {

        int32 sds_id  = SDselect(sdfd,i);
        if (sds_id == FAIL) {
            close_vgroup_fileids(file_id,sdfd,-1);
            throw InternalErr (__FILE__,__LINE__,"SDselect failed ");
        }
        int32 obj_ref = SDidtoref(sds_id);
        if (obj_ref == FAIL) {
            SDendaccess(sds_id);
            close_vgroup_fileids(file_id,sdfd,-1);
            throw InternalErr (__FILE__,__LINE__,"SDidtoref failed ");
        }
        sds_ref.insert(obj_ref);
        SDendaccess(sds_id);
    }

    BESDEBUG("h4"," End obtain_all_sds_refs() "<<endl);
}

void exclude_all_sds_refs_in_vgroups(int32 file_id, int32 sdfd, unordered_set<int32>& sds_ref) {

    BESDEBUG("h4"," Begin exclude_all_sds_refs_in_vgroups() "<<endl);
    int      num_lonevg = 0; 
    vector<int> ref_array;
    int32  istat = 0;

    istat = Vstart(file_id);
    if (istat == FAIL) { 
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr(__FILE__, __LINE__, "unable to start hdf4 V interface.");
    }

    num_lonevg = Vlone(file_id,nullptr,0);

    if (num_lonevg == FAIL) {
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr(__FILE__, __LINE__, "error in obtaining lone vgroup number.");
    }

    /* obtain object reference array. */

    /* if no lone vgroup, quit from this function. */
    if (num_lonevg == 0) {
        Vend(file_id);
        return;
    }

    ref_array.resize(num_lonevg);

    num_lonevg = Vlone(file_id,ref_array.data(),num_lonevg);

    /* walk through every lone group in the file */

    for(int lone_vg_number = 0; lone_vg_number < num_lonevg;
            lone_vg_number++) {

        int32 vgroup_id = Vattach(file_id,ref_array[lone_vg_number],"r");
        if (vgroup_id ==FAIL) {
            close_vgroup_fileids(file_id,sdfd,-1);
            throw InternalErr(__FILE__, __LINE__, "error in attaching lone vgroup.");
        }

        uint16  vclassnamelen; /*Vgroup class name length */
        vector<char> vclass_name;
        if (Vgetclassnamelen(vgroup_id,&vclassnamelen) == FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name length.");
        }
 
        if (vclassnamelen!=0) {
            vclass_name.resize(vclassnamelen+1);
            if (Vgetclass(vgroup_id,vclass_name.data()) == FAIL) {
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name.");
            }
            if (reserved_vgroups(vclass_name)) {
                Vdetach(vgroup_id);
                continue;
            }
        }

        exclude_sds_refs_in_vgroup(file_id,sdfd,vgroup_id,sds_ref);
        Vdetach(vgroup_id);
    }

    Vend(file_id);

    BESDEBUG("h4","End exclude_all_sds_refs_in_vgroups() "<<endl);
}

void exclude_sds_refs_in_vgroup(int32 file_id, int32 sdfd, int32 vgroup_id, unordered_set<int32> &sds_ref) {

    int num_gobjects; /* number of global objects */
    int32 obj_tag; /* object tag */
    int32 obj_ref;/* object reference */

    num_gobjects = Vntagrefs(vgroup_id);
    if (num_gobjects == FAIL) {
        close_vgroup_fileids(file_id,sdfd,vgroup_id);
        throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name.");
    }

    for( int i = 0;i<num_gobjects;i++) { 
            
        if (Vgettagref(vgroup_id,i,&obj_tag,&obj_ref)==FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "fail to obtain object tag and ref. of Vgroup members");
        }
 
        if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG) {

            if (0 == sds_ref.erase(obj_ref)) {
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                string err_msg = "Unable to remove the SDS object from the sds reference list for the reference number " + to_string(obj_ref);
                throw InternalErr(__FILE__, __LINE__, err_msg);
            }

        }
        else if (Visvg(vgroup_id, obj_ref)) {

            int32 vgroup_cid = Vattach(file_id,obj_ref,"r");
            if (vgroup_cid == FAIL) {
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "fail to attach vgroup_cid.");
            }
               
            uint16  vclassnamelen; /*Vgroup class name length */
            vector<char> vclass_name;
            if (Vgetclassnamelen(vgroup_cid,&vclassnamelen) == FAIL) {
                Vdetach(vgroup_cid);
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name length.");
            }
     
            if (vclassnamelen!=0) {
                vclass_name.resize(vclassnamelen+1);
                if (Vgetclass(vgroup_cid,vclass_name.data()) == FAIL) {
                    Vdetach(vgroup_cid);
                    close_vgroup_fileids(file_id,sdfd,vgroup_id);
                    throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name.");
                }
                if (reserved_vgroups(vclass_name)) {
                    Vdetach(vgroup_cid);
                    continue;
                }
            }

            exclude_sds_refs_in_vgroup(file_id,sdfd,vgroup_cid,sds_ref);
            Vdetach(vgroup_cid);

        }
    }
}

void read_sd_attrs(D4Group *root_grp, int32 fileid, int32 sdfd) {

    int32 n_sds      = 0;       
    int32 n_sd_attrs = 0;

    // Obtain number of SDS objects and number of SD(file) attributes
    if (SDfileinfo (sdfd, &n_sds, &n_sd_attrs) == FAIL){
        Hclose(fileid);
        SDend(sdfd);
        throw InternalErr (__FILE__,__LINE__,"SDfileinfo failed ");
    }

    for (int attr_index = 0; attr_index < n_sd_attrs;attr_index++) {

        char attr_name[H4_MAX_NC_NAME];
        int32 attr_type     = -1;
        int32 attr_count    = -1;
 
        if (SDattrinfo(sdfd,attr_index,attr_name,&attr_type,&attr_count) == FAIL) {
            Hclose(fileid);
            SDend(sdfd);
            throw InternalErr (__FILE__,__LINE__,"SDattrinfo failed ");
        }

        vector<char> attr_value;
        attr_value.resize(attr_count * DFKNTsize(attr_type));
        if (SDreadattr (sdfd, attr_index, attr_value.data()) == -1)  {
            Hclose(fileid);
            SDend(sdfd);
            throw InternalErr(__FILE__,__LINE__, "SDreadattr failed ");
        }
            
        string dap4_attrname (attr_name);
        dap4_attrname = HDFCFUtil::get_CF_string(dap4_attrname);
        map_vgroup_attr(root_grp, dap4_attrname,attr_type,attr_count, attr_value);

    }

}


void read_lone_vdata(D4Group *root_grp, int32 file_id, int32 sdfd, const string &filename) {

    BESDEBUG("h4"," Begin read_lone_vdata()"<<endl);
    if (Vstart(file_id) == FAIL) {
        Hclose(file_id);
        SDend(sdfd);
        throw InternalErr(__FILE__, __LINE__, "Fail to start the V interface.");
    } 
    // Obtain number of lone vdata.
    int num_lone_vdata = VSlone (file_id, nullptr, 0);

    if (num_lone_vdata == FAIL) {
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain the number of lone vdatas.");
    }

    if (num_lone_vdata > 0) {

        vector<int32>ref_array;
        ref_array.resize(num_lone_vdata);

        if (VSlone (file_id, ref_array.data(), num_lone_vdata) == FAIL) {
            close_vgroup_fileids(file_id,sdfd,-1);
            throw InternalErr(__FILE__, __LINE__, "Cannot obtain lone vdata reference arrays.");
        }

        for (int i = 0; i < num_lone_vdata; i++) 
            convert_vdata(file_id,sdfd, -1, ref_array[i],root_grp,filename);
    }
    Vend(file_id);

    BESDEBUG("h4"," End read_lone_vdata()"<<endl);
}

void read_dmr_vlone_groups(D4Group *root_grp, int32 file_id, int32 sdfd, const string & filename) {

    BESDEBUG("h4","Start read_dmr_vlone_groups "<<endl);

    int      num_lonevg = 0; 
    vector<int> ref_array;
    int32  istat = 0;

    istat = Vstart(file_id);
    if (istat == FAIL) { 
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr(__FILE__, __LINE__, "Unable to start hdf4 V interface.");
    }

    num_lonevg = Vlone(file_id,nullptr,0);

    if (num_lonevg == FAIL) {
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr(__FILE__, __LINE__, "error in obtaining the total number of lone vgroups.");
    }

    // Obtain object reference array. 

    // If no lone vgroups, quit from this function. 
    if (num_lonevg == 0){
        Vend(file_id);
        return;
    }

    // Check if this is an HDF-EOS2 grid. 
    vector<eos2_grid_t> eos2_grid_lls;
 

#ifdef USE_HDFEOS2_LIB
    bool ret_value = check_eos2_grids(filename,sdfd,eos2_grid_lls);
    if (ret_value == false) {
        close_vgroup_fileids(file_id,sdfd,-1);
        throw InternalErr(__FILE__, __LINE__, "error in the check_eos2_grid(). ");
    }
//HDF-EOS2 DEBUG
#if 0
for(const auto& eg:eos2_grid_lls) {
    cout<<"grid name: "<<eg.grid_name <<endl;
if (eg.oned_latlon==true) 
    cout<<"1d latlon "<<endl;
else
    cout<<"2d latlon "<<endl;
    cout<<"xdim: "<< eg.xdim <<endl;
    cout<<"ydim: "<< eg.ydim <<endl;


}
#endif
#endif
    
    ref_array.resize(num_lonevg);

    num_lonevg = Vlone(file_id,ref_array.data(),num_lonevg);

    // Walk through every lone group in the file. 

    for(int lone_vg_number = 0; lone_vg_number < num_lonevg;
            lone_vg_number++) {

        int32 vgroup_id = Vattach(file_id,ref_array[lone_vg_number],"r");
        if (vgroup_id ==FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "error in attaching lone vgroup.");
        }

        uint16  vclassnamelen = 0; /*Vgroup class name length */
        vector<char> vclass_name;
        if (Vgetclassnamelen(vgroup_id,&vclassnamelen) == FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name length.");
        }
 
        if (vclassnamelen!=0) {
            vclass_name.resize(vclassnamelen+1);
            if (Vgetclass(vgroup_id,vclass_name.data()) == FAIL) {
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name.");
            }
            if (reserved_vgroups(vclass_name)) {
                Vdetach(vgroup_id);
                continue;
            }
        }

        uint16 vgroupnamelen = 0;
        if (Vgetnamelen(vgroup_id,&vgroupnamelen) == FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "cannot obtain vgroup name length.");
        }

        if (vgroupnamelen == 0) {
            Vdetach(vgroup_id);
            continue;
        }
        vector<char> vgroup_name;
        vgroup_name.resize(vgroupnamelen+1);

        if (Vgetname(vgroup_id,vgroup_name.data())==FAIL){
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup name.");
        }

        string vgroup_name_orig_str(vgroup_name.begin(),vgroup_name.end()-1);

        int eos2_grid_lls_index = -1;
        if (!eos2_grid_lls.empty())
            eos2_grid_lls_index = is_group_eos2_grid(vgroup_name_orig_str,eos2_grid_lls);
        
        string vgroup_name_str = HDFCFUtil::get_CF_string(vgroup_name_orig_str);
        auto tem_d4_cgroup_ptr = make_unique<D4Group>(vgroup_name_str);
        auto tem_d4_cgroup = tem_d4_cgroup_ptr.release();
        root_grp->add_group_nocopy(tem_d4_cgroup);
        
        // Add latitude and longitude fields,plus attributes
        bool is_eos2_grid = false;
        if (eos2_grid_lls_index >=0) {
           add_eos2_latlon(tem_d4_cgroup, eos2_grid_lls[eos2_grid_lls_index],filename);
           is_eos2_grid = true;
        }

        convert_vgroup_objects(vgroup_id,file_id,sdfd,tem_d4_cgroup,vgroup_name_orig_str,filename,is_eos2_grid);

        Vdetach(vgroup_id);
        
    }
    Vend(file_id);
    BESDEBUG("h4","Finish read_dmr_vlone_groups "<<endl);

}

bool reserved_vgroups(const vector<char>& vgroup_class) {

  if(strcmp(vgroup_class.data(),_HDF_ATTRIBUTE)==0) 
      return true;
  else if(strcmp(vgroup_class.data(),_HDF_VARIABLE) ==0) 
      return true;
  else if(strcmp(vgroup_class.data(),_HDF_DIMENSION)==0) 
      return true;
  else if(strcmp(vgroup_class.data(),_HDF_UDIMENSION)==0) 
      return true;
  else if(strcmp(vgroup_class.data(),_HDF_CDF)==0) 
      return true;
  else if(strcmp(vgroup_class.data(),GR_NAME)==0) 
      return true;
  else if(strcmp(vgroup_class.data(),RI_NAME)==0) 
      return true;
  return false;

}

#if 0
void vgroup_convert_sds_objects(int32 vgroup_id, int32 file_id, int32 sdfd, D4Group *d4g, const string &filename) {

    int num_gobjects = 0; 
    int32 obj_tag = 0; 
    int32 obj_ref = 0;

    num_gobjects = Vntagrefs(vgroup_id);
    if (num_gobjects == FAIL) {
        close_vgroup_fileids(file_id,sdfd,vgroup_id); 
        throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name.");
    }

    for( int i = 0;i<num_gobjects;i++) { 
            
        if (Vgettagref(vgroup_id,i,&obj_tag,&obj_ref)==FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id); 
            throw InternalErr(__FILE__, __LINE__, "fail to obtain object tag and ref. of Vgroup members");
        }
 
        if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG) {
            convert_sds(file_id, sdfd,vgroup_id, obj_ref, d4g,filename);
        }
    }

}
#endif

void convert_vgroup_objects(int32 vgroup_id,int32 file_id,int32 sdfd,D4Group *d4g,const string &vgroupname, const string& filename, bool is_eos2_grid) {

    BESDEBUG("h4"," Start convert_vgroup_objects"<<endl);

    int num_gobjects = 0; 
    int32 obj_tag = 0; 
    int32 obj_ref = 0;

    num_gobjects = Vntagrefs(vgroup_id);
    if (num_gobjects == FAIL) {
        close_vgroup_fileids(file_id,sdfd,vgroup_id);
        throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup class name.");
    }

    // Need to convert objects first.
    for ( int i = 0;i<num_gobjects;i++) { 
            
        if (Vgettagref(vgroup_id,i,&obj_tag,&obj_ref)==FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "fail to obtain object tag and ref. of Vgroup members");
        }
        
 
        if (obj_tag == DFTAG_NDG || obj_tag == DFTAG_SDG) {
            convert_sds(file_id,sdfd,vgroup_id,obj_ref,d4g,filename,is_eos2_grid);
        }
        else if(Visvs(vgroup_id,obj_ref)) {
            convert_vdata(file_id, sdfd, vgroup_id, obj_ref,d4g,filename);
        }
    }

    // Then attributes.
    convert_vgroup_attrs(vgroup_id,d4g, vgroupname);

    // Then the children groups.
    for( int i = 0;i<num_gobjects;i++) { 
            
        if (Vgettagref(vgroup_id,i,&obj_tag,&obj_ref)==FAIL) {
            close_vgroup_fileids(file_id,sdfd,vgroup_id);
            throw InternalErr(__FILE__, __LINE__, "fail to obtain object tag and ref. of Vgroup members");
        }
 
        if (Visvg(vgroup_id, obj_ref)) {

            int32 vgroup_cid = Vattach(file_id,obj_ref,"r");
            if (vgroup_cid == FAIL) {
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "fail to attach vgroup_cid.");
            }
               
            uint16  vclassnamelen = 0; 
            vector<char> vclass_name;
            if (Vgetclassnamelen(vgroup_cid,&vclassnamelen) == FAIL) {
                Vdetach(vgroup_cid);
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "Error in obtaining vgroup class name length.");
            }
     
            if (vclassnamelen!=0) {
                vclass_name.resize(vclassnamelen+1);
                if (Vgetclass(vgroup_cid,vclass_name.data()) == FAIL) {
                    Vdetach(vgroup_cid);
                    close_vgroup_fileids(file_id,sdfd,vgroup_id);
                    throw InternalErr(__FILE__, __LINE__, "Error in obtaining vgroup class name.");
                }
                if (reserved_vgroups(vclass_name)) {
                    Vdetach(vgroup_cid);
                    continue;
                }
            }

            uint16 vgroupnamelen = 0;
            if (Vgetnamelen(vgroup_cid,&vgroupnamelen) == FAIL) {
                Vdetach(vgroup_cid);
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "cannot obtain vgroup name length.");
            }
    
            vector<char> vgroup_name;
            vgroup_name.resize(vgroupnamelen+1);
    
            if (Vgetname(vgroup_cid,vgroup_name.data())==FAIL){
                Vdetach(vgroup_cid);
                close_vgroup_fileids(file_id,sdfd,vgroup_id);
                throw InternalErr(__FILE__, __LINE__, "error in obtaining vgroup name.");
            }

            // Need to remove the last '/0'. 
            string vgroup_name_orig_str(vgroup_name.begin(),vgroup_name.end()-1);

            string vgroup_name_str = HDFCFUtil::get_CF_string(vgroup_name_orig_str);
            auto d4c_g_ptr = make_unique<D4Group>(vgroup_name_str);
            auto d4c_g  = d4c_g_ptr.release();
            d4g->add_group_nocopy(d4c_g);

            convert_vgroup_objects(vgroup_cid,file_id,sdfd,d4c_g,vgroup_name_orig_str,filename, is_eos2_grid);

            Vdetach(vgroup_cid);
        }
    }
    BESDEBUG("h4"," End convert_vgroup_objects"<<endl);
}

void convert_vgroup_attrs(int32 vgroup_id,D4Group *d4g, const string &vgroupname) {
 
    // Need to use Vnattrs2 instead of Vnattrs to include all attributes. 
    intn n_attrs = Vnattrs2(vgroup_id);
    if (n_attrs == FAIL) {
        Vdetach(vgroup_id);
        throw InternalErr(__FILE__,__LINE__,"Vnattrs failed");
    }

    for(int attr_index = 0; attr_index <n_attrs; attr_index++) {
    
        char attr_name[H4_MAX_NC_NAME];
        int attr_value_size = 0;
        int32 attr_type = 0;
        int32 attr_count = 0;
        intn status_n = Vattrinfo2(vgroup_id, attr_index, attr_name, &attr_type,
                            &attr_count, &attr_value_size, nullptr, nullptr);
        if (status_n == FAIL) {
            Vdetach(vgroup_id);
            throw InternalErr(__FILE__,__LINE__,"Vattrinfo failed");
        }

        // Create the DAP4 attribute 
        string tempname (attr_name);

        // Here we need to exclude the HDF-EOS2 Grid or Swath specific internal attributes starting from _FV_
        // These attributes are represented as field attributes already, so we don't need to duplicate them.
        if(vgroupname=="Grid Attributes" || vgroupname=="Swath Attributes") {
            if(tempname.size()>4) {
                string tempname_f4chars = tempname.substr(0,4);
                if (tempname_f4chars=="_FV_")
                    continue;
            } 
        }
 
        vector<char> attr_value;
        attr_value.resize(attr_value_size);
        status_n = Vgetattr2(vgroup_id,attr_index,attr_value.data());
        if (status_n == FAIL) {
            Vdetach(vgroup_id);
            throw InternalErr(__FILE__,__LINE__,"Vgetattr failed");
        }

        string dap4_attrname = HDFCFUtil::get_CF_string(tempname);
        map_vgroup_attr(d4g,dap4_attrname,attr_type,attr_count,attr_value);

    }

}


void convert_vdata(int32 fileid, int32 sdfd, int32 vgroup_id,int32 obj_ref ,D4Group* d4g,const string& filename) {

    // Obtain vdata ID
    int32 vdata_id = VSattach (fileid, obj_ref, "r");
    if (vdata_id == FAIL) {
        close_vgroup_fileids(fileid,sdfd,vgroup_id);  
        throw InternalErr(__FILE__, __LINE__, "Cannot attach vdata.");
    }

    // Vdata class
    char vdata_class[VSNAMELENMAX];

    if (VSgetclass (vdata_id, vdata_class) == FAIL) {
        VSdetach (vdata_id);
        close_vgroup_fileids(fileid,sdfd,vgroup_id);  
        throw InternalErr(__FILE__, __LINE__, "VSgetclass failed.");
    }

    if (VSisattr(vdata_id) == FALSE && VSisinternal(vdata_class) == FALSE) {

        int32 vs_nflds = VFnfields(vdata_id);
        if (vs_nflds == FAIL) {
            VSdetach(vdata_id);
            close_vgroup_fileids(fileid,sdfd,vgroup_id);  
            throw InternalErr(__FILE__, __LINE__, "Cannot get the number of fields of a vdata.");
        }
        
        int32 num_elms = VSelts(vdata_id);
        if (num_elms == FAIL) {
            VSdetach(vdata_id);
            close_vgroup_fileids(fileid,sdfd,vgroup_id);  
            throw InternalErr(__FILE__, __LINE__, "Cannot get the number of records of a vdata.");
        }

        // We map one field vdata to an atomic array; multi-field vdata to an array of structure.
        try {
            if (vs_nflds >1) 
                map_vdata_to_dap4_structure_array(vdata_id, num_elms, vs_nflds, obj_ref, d4g, filename);
            else if (vs_nflds == 1)
                map_vdata_to_dap4_atomic_array(vdata_id, num_elms, obj_ref, d4g, filename);
        }
        catch(...) {
            VSdetach(vdata_id);
            close_vgroup_fileids(fileid,sdfd,vgroup_id);  
            if (vs_nflds >1)
                throw InternalErr(__FILE__, __LINE__, "Cannot map vdata to a dap structure array.");
            else 
                throw InternalErr(__FILE__, __LINE__, "Cannot map vdata to a dap atomic array.");
        }
    }
    VSdetach(vdata_id);
}


void map_vdata_to_dap4_atomic_array(int32 vdata_id, int32 num_elms, int32 obj_ref, D4Group *d4g, const string &filename) {

    // Only one field, so the index is 0.
    int32 vdata_field_order = VFfieldorder(vdata_id,0);
    if (vdata_field_order == FAIL) {
        throw InternalErr(__FILE__, __LINE__, "VFfieldorder failed");
    }

    char vdata_name[VSNAMELENMAX];       
    if (VSgetname(vdata_id,vdata_name) == FAIL) {
        throw InternalErr(__FILE__, __LINE__, "Cannot get vdata name.");
    }       
 
    string vdata_name_str(vdata_name);

    const char *fieldname = VFfieldname(vdata_id,0);
    if (fieldname == nullptr) {
        throw InternalErr(__FILE__, __LINE__, "Cannot get vdata field name.");
    }

    string fieldname_str(fieldname);
    string dap4_vdata_str;

    // For HDF-EOS2, one field vdata and the field share the same name. So we just keep one.
    if (vdata_name_str == fieldname_str) 
        dap4_vdata_str = fieldname_str;
    else
        dap4_vdata_str = HDFCFUtil::get_CF_string(vdata_name_str) +"_"+ HDFCFUtil::get_CF_string(fieldname_str);

    int32 fieldtype = VFfieldtype(vdata_id,0);
    
    BaseType *bt = gen_dap_var(fieldtype,dap4_vdata_str,filename);
    auto ar_unique = make_unique<HDFArray>(dap4_vdata_str,filename,bt);
    auto ar = ar_unique.release();    
    ar->append_dim_ll(num_elms);
    if (vdata_field_order != 1)
        ar->append_dim_ll(vdata_field_order);

    // map vdata attributes to dap4. We will ignore vdata field attributes. Haven't seen one in NASA files.
    map_vdata_to_dap4_attrs(ar,vdata_id,obj_ref);
    
    ar->set_is_dap4(true);
    d4g->add_var_nocopy(ar);

    delete bt;

}

void map_vdata_to_dap4_structure_array(int32 vdata_id, int32 num_elms, int32 nflds, int32 obj_ref, D4Group *d4g, const string &filename) {

    char vdata_name[VSNAMELENMAX];       
    if (VSgetname(vdata_id,vdata_name) == FAIL) {
        throw InternalErr(__FILE__, __LINE__, "Cannot get vdata name.");
    }       
    string vdata_name_str(vdata_name);
    vdata_name_str = HDFCFUtil::get_CF_string(vdata_name_str);

    auto structure_unique_ptr = make_unique<HDFStructure>(vdata_name_str,filename);
    auto structure_ptr = structure_unique_ptr.release();
   
    for (int j = 0; j <nflds; j++) {

        int32 vdata_field_order = VFfieldorder(vdata_id,j);
        if (vdata_field_order == FAIL) {
            throw InternalErr(__FILE__, __LINE__, "VFfieldorder failed");
        }
    
        const char *fieldname = VFfieldname(vdata_id,j);
        if (fieldname == nullptr) {
            throw InternalErr(__FILE__, __LINE__, "Cannot get vdata field name.");
        }
    
        string fieldname_str(fieldname);
        fieldname_str = HDFCFUtil::get_CF_string(fieldname_str);
    
        int32 fieldtype = VFfieldtype(vdata_id,j);
        
        if (vdata_field_order == 1) {
            BaseType *bt = gen_dap_var(fieldtype,fieldname_str,filename);
            structure_ptr->add_var(bt);
            delete bt; 
        }
        else {
            BaseType *bt = gen_dap_var(fieldtype,fieldname_str,filename);
            auto arf_unique = make_unique<HDFArray>(fieldname_str,filename,bt);
            auto arf = arf_unique.release();    
            arf->append_dim_ll(vdata_field_order);
            structure_ptr->add_var(arf);
            delete bt; 
            delete arf;
        }
    }

    auto ar_unique = make_unique<HDFArray>(vdata_name_str,filename,structure_ptr);
    auto ar = ar_unique.release();    
    ar->append_dim_ll(num_elms);

    // map vdata attributes to dap4. We will ignore vdata field attributes. Haven't seen one in NASA files.
    map_vdata_to_dap4_attrs(ar,vdata_id,obj_ref);
    ar->set_is_dap4(true);
    d4g->add_var(ar);

    delete structure_ptr;
    delete ar;
}

void map_vdata_to_dap4_attrs(HDFArray *ar, int32 vdata_id, int32 obj_ref) {
                
    // Number of attributes 
    int32 nattrs = 0;
                
    // API status indicator
    int32 status = 0;
                
    // Number of vdata attributes,we will not handle field attribute now.
    nattrs = VSfnattrs (vdata_id, _HDF_VDATA);
            
    if (nattrs > 0) {
            
        // Obtain number of vdata attributes
        for (int i = 0; i < nattrs; i++) {

            // Vdata attribute name
            char attr_name[H4_MAX_NC_NAME];
            int attrsize = 0;
            int32 attr_type = 0;
            int32 attr_count = 0;
 
            status = VSattrinfo (vdata_id, _HDF_VDATA, i, attr_name,
                                 &attr_type, &attr_count, &attrsize);
            if (status == FAIL) {
                throw InternalErr(__FILE__, __LINE__, "VSattrinfo failed.");
            }   
                
            // Checking and handling the special characters for the vdata attribute name.
            string tempname(attr_name);
            vector<char> attr_value;
            attr_value.resize(attrsize);

            if (VSgetattr (vdata_id, _HDF_VDATA, i, attr_value.data()) == FAIL) {
                throw InternalErr(__FILE__, __LINE__, "VSgetattr failed.");
            }
            string dap4_attrname = HDFCFUtil::get_CF_string(tempname);
            
            map_sds_vdata_attr(ar,dap4_attrname,attr_type,attr_count,attr_value);
        }       
    }

    // This is for dmrpp. The reference number is used to retrieve the data.
    add_obj_ref_attr(ar,false,obj_ref);

}


void convert_sds(int32 file_id, int32 sdfd, int32 vgroup_id, int32 obj_ref, D4Group *d4g, const string &filename, bool is_eos2_grid) {

    int32 sds_index = 0; 
    int32 sds_id = 0; 
    vector<char> sds_name;
    uint16  name_len = 0;
    int32 sds_rank = 0;
    int32 sds_type = 0;
    int32 dim_sizes[H4_MAX_VAR_DIMS];
    int32 n_sds_attrs = 0;

    sds_index = SDreftoindex(sdfd,obj_ref);
    if (sds_index == FAIL) {
        close_vgroup_fileids(file_id,sdfd,vgroup_id);  
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain the SDS index.");
    }

    sds_id = SDselect(sdfd,sds_index);
    if (sds_id == FAIL) {
        close_vgroup_fileids(file_id,sdfd,vgroup_id);  
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain the SDS ID.");
     }

    if (SDgetnamelen(sds_id, &name_len) == FAIL) {
        SDendaccess(sds_id);
        close_vgroup_fileids(file_id,sdfd,vgroup_id);  
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain the SDS name length.");
    }

    sds_name.resize(name_len+1);

    // Obtain object name, rank, size, field type and number of SDS attributes
    if (FAIL == SDgetinfo (sds_id, sds_name.data(), &sds_rank, dim_sizes, &sds_type, &n_sds_attrs)) {
        SDendaccess(sds_id);
        close_vgroup_fileids(file_id,sdfd,vgroup_id);  
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain the SDS ID.");
    }                        
    
    bool has_zero_dim_size = false;
    for (int j = 0; j <sds_rank; j++) {
        if(dim_sizes[j] == 0) {
            has_zero_dim_size = true;
            break;
        }
    }
    
    if (has_zero_dim_size) {
        SDendaccess(sds_id);
        return;
    }

    string sds_name_str(sds_name.begin(),sds_name.end()-1);

    // Handle special characters in the sds_name.
    sds_name_str = HDFCFUtil::get_CF_string(sds_name_str);
    BaseType *bt = gen_dap_var(sds_type,sds_name_str, filename);
    auto ar_unique = make_unique<HDFArray>(sds_name_str,filename,bt);
    auto ar = ar_unique.release();    

    // Obtain dimension info. 

    // Handle dimensions with original dimension names
    for (int dimindex = 0; dimindex < sds_rank; dimindex++) {

        int dimid = SDgetdimid (sds_id, dimindex);
        if (dimid == FAIL) {
            delete bt;
            delete ar;
            SDendaccess (sds_id);
            close_vgroup_fileids(file_id,sdfd,vgroup_id);  
            throw InternalErr(__FILE__, __LINE__, "SDgetdimid failed.");
        }
        char dim_name[H4_MAX_NC_NAME];
        int32 dim_size = 0;
        int32 dim_type = 0;

        // Number of dimension attributes(This is almost never used)
        int32  num_dim_attrs = 0;

        intn status = SDdiminfo (dimid, dim_name, &dim_size, &dim_type,
                            &num_dim_attrs);

        if (status == FAIL) {
            delete ar;
            delete bt;
            SDendaccess (sds_id);
            close_vgroup_fileids(file_id,sdfd,vgroup_id);  
            throw InternalErr(__FILE__, __LINE__, "SDdiminfo failed.");
        }

        string dim_name_str (dim_name);
        dim_name_str = HDFCFUtil::get_CF_string(dim_name_str);

        // We add a "/" in front of dim_name_str for the dimension name to make it FQN 
        // since current build_dmrpp requires this.
        dim_name_str = "/" + dim_name_str;
        ar->append_dim_ll(dim_sizes[dimindex], dim_name_str);

    }
    
    // map sds var attributes to dap4
    try {
        map_sds_var_dap4_attrs(ar,sds_id,obj_ref,n_sds_attrs);
    }
    catch(...){
            delete ar;
            delete bt;
            SDendaccess (sds_id);
            close_vgroup_fileids(file_id,sdfd,vgroup_id);  
            throw InternalErr(__FILE__, __LINE__, "Map SDS attributes to DAP4 failed.");
    }

    if (is_eos2_grid) {

        // If this SDS belongs to an HDF-EOS2 grid. We need to add the coordinates attributes to ensure the CF tools 
        // to correctly display the data. Not absolutely necessary but highly desireable and useful for end users. 
        // So add it.
        string d4_grp_path = d4g->FQN();
    
        // Obtain the grid name, it should be the first part of the d4_grp_path.
        string grid_name = first_part_of_full_path(d4_grp_path);
        string coordinates =grid_name+"/Longitude ";   
        coordinates +=grid_name+"/Latitude";
        add_var_dap4_attr(ar,"coordinates",attr_str_c,coordinates);
    }

    ar->set_is_dap4(true);
    d4g->add_var_nocopy(ar);

    delete bt;
    SDendaccess(sds_id);
}


void map_sds_var_dap4_attrs(HDFArray *ar, int32 sds_id, int32 obj_ref, int32 n_sds_attrs) {

    char attr_name[H4_MAX_NC_NAME];
    for (int attrindex = 0; attrindex < n_sds_attrs; attrindex++) {

        int32 sds_attr_type = 0;
        int32 attr_value_count = 0;
        if (FAIL==SDattrinfo (sds_id, attrindex, attr_name, &sds_attr_type, &attr_value_count))
                throw InternalErr(__FILE__,__LINE__, "SDattrinfo failed ");

        vector<char> attr_value;
        attr_value.resize(attr_value_count * DFKNTsize(sds_attr_type));
        if (SDreadattr (sds_id, attrindex, attr_value.data()) == -1) 
            throw InternalErr(__FILE__,__LINE__, "SDreadattr failed ");
            
        string dap4_attrname (attr_name);
        dap4_attrname = HDFCFUtil::get_CF_string(dap4_attrname);

        map_sds_vdata_attr(ar,dap4_attrname,sds_attr_type,attr_value_count,attr_value);
        
    }
    // Save the reference number since it will be used by the dmrpp module.
    add_obj_ref_attr(ar,true,obj_ref);
}

// Note: HDF4 doesn't support 64-bit integers.
BaseType * gen_dap_var(int32 h4_type, const string & h4_str, const string &filename) {

    BaseType * ret_bt = nullptr;
    switch (h4_type) {

    case DFNT_INT16:
    case DFNT_NINT16:
        ret_bt = new HDFInt16(h4_str,filename);
        break;

    case DFNT_NINT32:
    case DFNT_INT32:
        ret_bt = new HDFInt32(h4_str,filename);
        break;

    case DFNT_UINT16:
    case DFNT_NUINT16:
        ret_bt = new HDFUInt16(h4_str,filename);
        break;

    case DFNT_NUINT32:
    case DFNT_UINT32:
        ret_bt = new HDFUInt32(h4_str,filename);
        break;

    case DFNT_FLOAT32:
        ret_bt = new HDFFloat32(h4_str,filename);
        break;

    case DFNT_FLOAT64:
        ret_bt = new HDFFloat64(h4_str,filename);
        break;

    case DFNT_INT8:
    case DFNT_NINT8:
    case DFNT_CHAR8:
        ret_bt = new HDFInt8(h4_str,filename);
        break;

    case DFNT_UINT8:
    case DFNT_NUINT8:
    case DFNT_UCHAR8:
        ret_bt = new HDFByte(h4_str,filename);
        break;

    default:
        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");

    }
    return ret_bt;
}

// Note: we map the attributes with DFNT_CHAR(char) to DAP4 string since
//       this is the common way that users apply.
D4AttributeType h4type_to_dap4_attrtype(int32 h4_type) {

    D4AttributeType dap4_attr_type = attr_null_c;
    switch (h4_type) {

    case DFNT_CHAR:
        dap4_attr_type = attr_str_c;
        break;
    case DFNT_INT8:
    case DFNT_NINT8:
        dap4_attr_type = attr_int8_c;
        break;
    case DFNT_INT16:
    case DFNT_NINT16:
        dap4_attr_type = attr_int16_c;
        break;
    case DFNT_NINT32:
    case DFNT_INT32:
        dap4_attr_type = attr_int32_c;
        break;

    case DFNT_UINT16:
    case DFNT_NUINT16:
        dap4_attr_type = attr_uint16_c;
        break;

    case DFNT_NUINT32:
    case DFNT_UINT32:
        dap4_attr_type = attr_uint32_c;
        break;

    case DFNT_FLOAT32:
        dap4_attr_type = attr_float32_c;
        break;

    case DFNT_FLOAT64:
        dap4_attr_type = attr_float64_c;
        break;

    case DFNT_UINT8:
    case DFNT_NUINT8:
    case DFNT_UCHAR:
        dap4_attr_type = attr_byte_c;
        break;

    default:
        throw InternalErr(__FILE__,__LINE__,"unsupported data type.");

    }
    return dap4_attr_type;

}

string
print_dap4_attr(int32 type, int loc, void *vals)
{
    string ret_value;
    ostringstream rep;

    union {
        char *cp;
        unsigned char *ucp;
        short *sp;
        unsigned short *usp;
        int32 /*nclong*/ *lp;
        unsigned int *ui;
        float *fp;
        double *dp;
    } gp;

    switch (type) {

    // Mapping both DFNT_UINT8 and DFNT_INT8 to unsigned char 
    // may cause overflow. Documented at jira ticket HFRHANDLER-169.
    case DFNT_UINT8:
    case DFNT_UCHAR:
        {
            unsigned char uc;
            gp.ucp = (unsigned char *) vals;
            uc = *(gp.ucp+loc);
            rep << (int)uc;
            return rep.str();
        }

    case DFNT_INT8:
    case DFNT_NINT8:
        {
            char c;
            gp.cp = (char *) vals;
            c = *(gp.cp+loc);
            rep << (int)c;
            return rep.str();
        }

    case DFNT_CHAR:
        {
            string tmp_str = static_cast<const char*>(vals);
            return tmp_str;
        }

    case DFNT_INT16:
    case DFNT_NINT16:
        {
            gp.sp = (short *) vals;
            rep << *(gp.sp+loc);
            return rep.str();
        }

    case DFNT_UINT16:
    case DFNT_NUINT16:
        {
            gp.usp = (unsigned short *) vals;
            rep << *(gp.usp+loc);
            return rep.str();
        }

    case DFNT_INT32:
    case DFNT_NINT32:
        {
            gp.lp = (int32 *) vals;
            rep << *(gp.lp+loc);
            return rep.str();
        }

    case DFNT_UINT32:
    case DFNT_NUINT32:
        {
            gp.ui = (unsigned int *) vals;
            rep << *(gp.ui+loc);
            return rep.str();
        }

    case DFNT_FLOAT:
        {
            float attr_val = *(float*)vals;
            bool is_a_fin = isfinite(attr_val);
            gp.fp = (float *) vals;
            rep << showpoint;
            // setprecision seeme to cause the one-bit error when 
            // converting from float to string. Watch whether this
            // is an isue.
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            string tmp_rep_str = rep.str();
            if (tmp_rep_str.find('.') == string::npos
                && tmp_rep_str.find('e') == string::npos
                && tmp_rep_str.find('E') == string::npos) {
                if(true == is_a_fin)
                    rep << ".";
            }
            return rep.str();
        }

    case DFNT_DOUBLE:
        {

            double attr_val = *(double*)vals;
            bool is_a_fin = isfinite(attr_val);
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            string tmp_rep_str = rep.str();
            if (tmp_rep_str.find('.') == string::npos
                && tmp_rep_str.find('e') == string::npos
                && tmp_rep_str.find('E') == string::npos) {
                if(true == is_a_fin)
                    rep << ".";
            }
            return rep.str();
        }
    default:
        return ret_value;
    }

}

void map_vgroup_attr(D4Group *d4g, const string &dap4_attrname,int32 attr_type, int32 attr_count, vector<char> & attr_value) {

    D4AttributeType dap4_attr_type = h4type_to_dap4_attrtype(attr_type);

    // We encounter an unsupported DAP4 attribute type.
    if (attr_null_c == dap4_attr_type) 
        throw InternalErr(__FILE__, __LINE__, "unsupported DAP4 attribute type");

    // Create the DAP4 attribute 
    auto d4_attr_unique = make_unique<D4Attribute>(dap4_attrname, dap4_attr_type);
    D4Attribute *d4_attr = d4_attr_unique.release();

    // Map characters to string first.
    if (attr_type == DFNT_CHAR) {
        string t_dap4_attr_value(attr_value.begin(),attr_value.end());
        auto dap4_attr_value = string(t_dap4_attr_value.c_str());
        d4_attr->add_value(dap4_attr_value);
    }
    else {
        for (int loc=0; loc < attr_count ; loc++) {
            
            string print_rep = print_dap4_attr(attr_type, loc, (void*) (attr_value.data()));
            if (print_rep.empty())
                throw InternalErr(__FILE__,__LINE__, "Cannot obtain the attribute value for DAP4 ");
            else 
                d4_attr->add_value(print_rep);

        }
    }
    d4g->attributes()->add_attribute_nocopy(d4_attr);

}

void map_sds_vdata_attr(BaseType *d4b, const string &attr_name,int32 attr_type, int32 attr_count, vector<char> & attr_value) {

    D4AttributeType dap4_attr_type = h4type_to_dap4_attrtype(attr_type);

    // We encounter an unsupported DAP4 attribute type.
    if (attr_null_c == dap4_attr_type) 
        throw InternalErr(__FILE__, __LINE__, "unsupported DAP4 attribute type");

    // Create the DAP4 attribute 

    string dap4_attrname (attr_name);
    auto d4_attr_unique = make_unique<D4Attribute>(dap4_attrname, dap4_attr_type);
    D4Attribute *d4_attr = d4_attr_unique.release();

    if (attr_type == DFNT_CHAR) {
        string t_dap4_attr_value(attr_value.begin(),attr_value.end());
        auto dap4_attr_value = string(t_dap4_attr_value.c_str());
        d4_attr->add_value(dap4_attr_value);
    }
    else {
        for (int loc=0; loc < attr_count ; loc++) {
            
            string print_rep = print_dap4_attr(attr_type, loc, (void*) (attr_value.data()));
            if (print_rep.empty())
                throw InternalErr(__FILE__,__LINE__, "Cannot obtain the attribute value for DAP4 ");
            else 
                d4_attr->add_value(print_rep);

        }
    }
    d4b->attributes()->add_attribute_nocopy(d4_attr);

}

void  add_obj_ref_attr(BaseType * d4b, bool is_sds, int32 obj_ref) {

    // Add the object ref and tag information later. 
    string obj_ref_attr_name;
    if (is_sds)
         obj_ref_attr_name = "dmr_sds_ref";
    else
         obj_ref_attr_name = "dmr_vdata_ref";
 
    auto d4_obj_ref_attr_unique = make_unique<D4Attribute>(obj_ref_attr_name, attr_int32_c);
    D4Attribute *d4_obj_ref_attr = d4_obj_ref_attr_unique.release();
    string obj_ref_str = print_dap4_attr(DFNT_INT32, 0,(void*)&obj_ref);
    d4_obj_ref_attr->add_value(obj_ref_str);
  
    d4b->attributes()->add_attribute_nocopy(d4_obj_ref_attr);

}
void close_vgroup_fileids(int32 fileid, int32 sdfd, int32 vgroup_id) {
    if (vgroup_id != FAIL)
        Vdetach(vgroup_id);
    Vend(fileid);
    Hclose(fileid);
    SDend(sdfd);
}

// Direct CF to DAP4, helper function to DAP4 variable  attributes.
void add_var_dap4_attr(BaseType *var,const string& attr_name, D4AttributeType attr_type, const string& attr_value){

    auto d4_attr_unique = make_unique<D4Attribute>(attr_name,attr_type);
    auto d4_attr = d4_attr_unique.release();
    d4_attr->add_value(attr_value);
    var->attributes()->add_attribute_nocopy(d4_attr);
    
}     

int is_group_eos2_grid(const string& vgroup_name, vector<eos2_grid_t>& eos2_grid_lls) {
    
    int ret_value = -1;
    for (int i = 0; i <eos2_grid_lls.size();i++) {
        if (vgroup_name==eos2_grid_lls[i].grid_name){
            ret_value = i;
            break;
        }
    }
        
    return ret_value;
}

void add_eos2_latlon(D4Group *d4_grp, const eos2_grid_t &eos2_grid, const string &filename){

    string ydim_path = "YDim/"+eos2_grid.grid_name;
    ydim_path = HDFCFUtil::get_CF_string(ydim_path);
    ydim_path ="/"+ydim_path;
    string xdim_path = "XDim/"+eos2_grid.grid_name;
    xdim_path = HDFCFUtil::get_CF_string(xdim_path);
    xdim_path="/"+xdim_path;

    string lat_name ="Latitude";
    string lon_name ="Longitude";
    
    // Array
    auto ar_bt_lat_unique = make_unique<Float64>(lat_name);
    auto ar_bt_lat = ar_bt_lat_unique.get();
    auto ar_lat_unique = make_unique<HDFArray>(lat_name,filename,ar_bt_lat);
    auto ar_lat = ar_lat_unique.release();

    // Dimensions
    ar_lat->append_dim_ll(eos2_grid.ydim,ydim_path);
    if (!eos2_grid.oned_latlon)
        ar_lat->append_dim_ll(eos2_grid.xdim,xdim_path);

    // Attribute
    add_var_dap4_attr(ar_lat,"units",attr_str_c,"degrees_north");

    ar_lat->set_is_dap4(true);
    d4_grp->add_var_nocopy(ar_lat);

    auto ar_bt_lon_unique = make_unique<Float64>(lon_name);
    auto ar_bt_lon = ar_bt_lon_unique.get();
    auto ar_lon_unique = make_unique<HDFArray>(lon_name,filename,ar_bt_lon);
    auto ar_lon = ar_lon_unique.release();
    if (!eos2_grid.oned_latlon)
        ar_lon->append_dim_ll(eos2_grid.ydim,ydim_path);
    ar_lon->append_dim_ll(eos2_grid.xdim,xdim_path);

    add_var_dap4_attr(ar_lon,"units",attr_str_c,"degrees_east");

    ar_lon->set_is_dap4(true);
    d4_grp->add_var_nocopy(ar_lon);

}

#ifdef USE_HDFEOS2_LIB
bool check_eos2_grids(const string &filename, int32 sdfd,vector<eos2_grid_t>& eos2_grid_lls) {

    bool ret_value = true;
    // Here we need to check if this is an HDF-EOS2 grid file.
    string dummy_grid_name;
    bool is_eos2_file = false;
    if (1 == check_special_eosfile(filename,dummy_grid_name,sdfd))
        is_eos2_file = true;

    if (is_eos2_file) {

        vector<string> eos2_grid_names;
        ret_value = HDFEOS2::Utility::ReadNamelist(filename.c_str(),GDinqgrid,eos2_grid_names);
        if (ret_value == false)
            return ret_value;

        for (const auto& gname:eos2_grid_names) {
            int32 ydim = 0;
            int32 xdim = 0;
            bool one_d_ll = false;
            ret_value = obtain_eos2_gd_ll_info(filename,gname,ydim,xdim,one_d_ll);
            if (ret_value ==false)
                break;

            eos2_grid_t eg;
            eg.oned_latlon = one_d_ll;
            eg.ydim = ydim;
            eg.xdim = xdim;
            eg.grid_name = gname;
            eos2_grid_lls.push_back(eg);
            
        }
    }

    return ret_value;
}

bool obtain_eos2_gd_ll_info(const string & fname, const string & grid_name, int32 &ydim, int32 &xdim, bool &oned_ll) {

    int32 gfid = GDopen(const_cast<char*>(fname.c_str()),DFACC_READ);
    if (gfid <0) 
        return false;
    int32 gridid = GDattach(gfid, const_cast < char *>(grid_name.c_str ()));
    if (gridid <0) {
        GDclose(gfid);
        return false;
    }

    /* Declare projection code, zone, etc grid parameters. */
    int32 projcode = -1;
    int32 zone     = -1;
    int32 sphere   = -1;
    float64 params[16];
    float64 upleft[2];
    float64 lowright[2];

    if (GDprojinfo (gridid, &projcode, &zone, &sphere, params) == -1) {
        GDdetach(gridid);
        GDclose(gfid);
        return false;
    }   

    // Retrieve dimensions and X-Y coordinates of corners
    if (GDgridinfo(gridid, &xdim, &ydim, upleft,
                           lowright) == -1) {
        GDdetach(gridid);
        GDclose(gfid);
        return false;
    }
    
    if (GCTP_CEA == projcode || GCTP_GEO == projcode)
        oned_ll = true;

    GDdetach(gridid);
    GDclose(gfid);

    return true;
}
#endif
