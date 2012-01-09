///////////////////////////////////////////////////////////////////////////////
/// \file hdfdesc.cc
/// \brief DAP attributes and structure description generation code.
///
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Copyright (c) 2008-2010 The HDF Group.
// Author: MuQun Yang <myang6@hdfgroup.org>
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
//#define DODS_DEBUG

#include <cstdio>
#include <cassert>
#include <libgen.h>

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

// DODS includes
#include <DDS.h>
#include <DAS.h>
#include <escaping.h>
#include <parser.h>
#include <InternalErr.h>
#include <debug.h>

#include <BESDebug.h>
#include <BESLog.h>

// DODS/HDF includes
#include "hcstream.h"
#include "hdfclass.h"
#include "hcerr.h"
#include "dhdferr.h"
#include "HDFArray.h"
#include "HDFSequence.h"
#include "HDFTypeFactory.h"
#include "HDFGrid.h"
#include "dodsutil.h"
#include "hdf-dods.h"
#include "hdf-maps.h"

#define SIGNED_BYTE_TO_INT32 1

// Glue routines declared in hdfeos.lex
void  hdfeos_switch_to_buffer(void *new_buffer);
void  hdfeos_delete_buffer(void * buffer);
void *hdfeos_string(const char *yy_str);

#ifdef USE_HDFEOS2_LIB

#include "HDFByte.h"
#include "HDFInt16.h"
#include "HDFUInt16.h"
#include "HDFInt32.h"
#include "HDFUInt32.h"
#include "HDFFloat32.h"
#include "HDFFloat64.h"
#include "HE2CFNcML.h"
#include "HE2CFShortName.h"
#include "HE2CF.h"
#include "HDFEOS2.h"
#include "HDFEOS2Array_RealField.h"
#include "HDFEOS2ArrayGridGeoField.h"
#include "HDFEOS2ArraySwathGeoField.h"
#include "HDFEOS2ArrayMissField.h"
#include "HDFEOS2ArraySwathDimMapField.h"
#include "HDFEOS2ArraySwathGeoDimMapField.h"
#include "HDFEOS2ArraySwathGeoDimMapExtraField.h"
#include "HDFEOS2Util.h"

#include "HDFSP.h"
#include "HDFSPArray_RealField.h"
#include "HDFSPArrayGeoField.h"
#include "HDFSPArrayMissField.h"
#include "HDFSPArray_VDField.h"

#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <iomanip>              // <hyokyung 2010.06.17. 16:18:57>
#endif


// Added 5/7/09; This bug (#1163) was fixed in July 2008 except for this
// handler. jhrg
#define ATTR_STRING_QUOTE_FIX

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


#ifdef USE_HDFEOS2_LIB
void read_das(DAS & das, const string & filename);
void read_dds(DDS & dds, const string & filename);
// read_dds for HDF-EOS2
bool read_dds_hdfeos2(DDS & dds, const string & filename,
                      HE2CFNcML* ncml,
                      HE2CFShortName* sn, HE2CFShortName* sn_dim,
                      HE2CFUniqName* un, HE2CFUniqName* un_dim);

// read_dds for special NASA HDF4 files
bool read_dds_hdfsp(DDS & dds, const string & filename,
                      HE2CFNcML* ncml,
                      HE2CFShortName* sn, HE2CFShortName* sn_dim,
                      HE2CFUniqName* un, HE2CFUniqName* un_dim);

bool read_das_hdfsp(DAS & das, const string & filename,
                      HE2CFNcML* ncml,
                      HE2CFShortName* sn, HE2CFShortName* sn_dim,
                      HE2CFUniqName* un, HE2CFUniqName* un_dim);

// read_dds for special NASA HDF-EOS2 hybrid(non-EOS2) objects
bool read_dds_hdfhybrid(DDS & dds, const string & filename,
                      HE2CFNcML* ncml,
                      HE2CFShortName* sn, HE2CFShortName* sn_dim,
                      HE2CFUniqName* un, HE2CFUniqName* un_dim);

bool read_das_hdfhybrid(DAS & das, const string & filename,
                      HE2CFNcML* ncml,
                      HE2CFShortName* sn, HE2CFShortName* sn_dim,
                      HE2CFUniqName* un, HE2CFUniqName* un_dim);

void read_dds_spfields(DDS &dds,const string& filename,HDFSP::SDField *spsds, SPType sptype); 
void read_dds_spvdfields(DDS &dds,const string& filename,int32 vdref, int32 numrec,HDFSP::VDField *spvd); 

DAS *_das;
bool change_data_type(string newfname) 
{
	AttrTable *at = _das->get_table(newfname);
        if(mtype!=OTHER_TYPE && at!=NULL)
        {
		AttrTable::Attr_iter it = at->attr_begin();
                string  scale_factor_value="", add_offset_value="", radiance_scales_value="", radiance_offsets_value="", reflectance_scales_value="", reflectance_offsets_value="";
                string scale_factor_type, add_offset_type;
		while (it!=at->attr_end())
                {
                        if(at->get_name(it)=="radiance_scales")
                                radiance_scales_value = (*at->get_attr_vector(it)->begin());
                        if(at->get_name(it)=="radiance_offsets")
                                radiance_offsets_value = (*at->get_attr_vector(it)->begin());
                        if(at->get_name(it)=="reflectance_scales")
                                reflectance_scales_value = (*at->get_attr_vector(it)->begin());
                        if(at->get_name(it)=="reflectance_offsets")
                                reflectance_offsets_value = (*at->get_attr_vector(it)->begin());
			if(at->get_name(it).find("scale_factor")!=string::npos)
                        {
                                scale_factor_value = (*at->get_attr_vector(it)->begin());
                                scale_factor_type = at->get_type(it);
                        }
                        if(at->get_name(it).find("add_offset")!=string::npos)
                        {
                                add_offset_value = (*at->get_attr_vector(it)->begin());
                                add_offset_type = at->get_type(it);
                        }
			it++;
		}
		if((radiance_scales_value.length()!=0 && radiance_offsets_value.length()!=0) || (reflectance_scales_value.length()!=0 && reflectance_offsets_value.length()!=0))
                        return true;
		
		if(scale_factor_value.length()!=0 && add_offset_value.length()!=0)
                {
                        if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) 
                                return true;
                }
	}

	return false;
}

// read_dds for one grid or swath
void read_dds_hdfeos2_grid_swath(DDS &dds, const string&filename,
                                 HDFEOS2::Dataset *dataset, int grid_or_swath,bool ownll)
// grid_or_swath - 0: grid, 1: swath
{
    if(grid_or_swath < 0 ||  grid_or_swath > 1)
     throw InternalErr(__FILE__, __LINE__, "The current type should be either grid or swath");

    const std::vector<HDFEOS2::Field*>& fields = (dataset)->getDataFields(); 
    std::vector<HDFEOS2::Field*>::const_iterator it_f;
    std::vector<struct dimmap_entry> dimmaps;

    // Obtain the extra dim map file name
    std::string dimmapfilename="";
    
    if(grid_or_swath == 1) {// swath

        HDFEOS2::SwathDataset *sw = static_cast<HDFEOS2::SwathDataset *>(dataset);
        const std::vector<HDFEOS2::SwathDataset::DimensionMap*>& origdimmaps = sw->getDimensionMaps();
        std::vector<HDFEOS2::SwathDataset::DimensionMap*>::const_iterator it_dmap;
        struct dimmap_entry tempdimmap;

        for(int i=0;i<origdimmaps.size();i++){
           tempdimmap.geodim = origdimmaps[i]->getGeoDimension();
           tempdimmap.datadim = origdimmaps[i]->getDataDimension();
           tempdimmap.offset = origdimmaps[i]->getOffset();
           tempdimmap.inc    = origdimmaps[i]->getIncrement();
           dimmaps.push_back(tempdimmap);
        }
        // Only when there is dimension map, we need to consider the additional MODIS geolocation files.

        if(origdimmaps.size() != 0) {
          std::string filenametemp = filename;
          char*tempcstr;
          tempcstr = new char [filename.size()+1];
          strcpy (tempcstr,filename.c_str());
          std::string basefilename = basename(tempcstr);
          std::string dirfilename = dirname(tempcstr);
          delete [] tempcstr;

          // This part is implemented specifically for supporting MODIS dimension map data.
          // MODIS Aqua Swath dimension map geolocation file always starts with MYD03
          // MODIS Terra Swath dimension map geolocation file always starts with MOD03
	  // We will check the first three characters to see if the dimension map geolocation file exists.
	  // An example MODIS swath filename is MOD05_L2.A2008120.0000.005.2008121182723.hdf
	  // An example MODIS geo-location file name is MOD03.A2008120.0000.005.2010003235220.hdf
	  // Notice that the "A2008120.0000" in the middle of the name is the "Acquistion Date" of the data
	  // So the geo-location file name should have exactly the same string. We will use this
	  // string to identify if a MODIS geo-location file exists or not.
          // Note the string size is 14.
	  // More information of naming convention, check http://modis-atmos.gsfc.nasa.gov/products_filename.html
	  // KY 2010-5-10


          // Obtain string "MYD" or "MOD"
          std::string fnameprefix = basefilename.substr(0,3);

          if(fnameprefix == "MYD" || fnameprefix =="MOD") {
   	       size_t fnamemidpos = basefilename.find(".A");
               if(fnamemidpos != string::npos) {
       	          std::string fnamemiddle = basefilename.substr(fnamemidpos,14);
                  if(fnamemiddle.size()==14) {
                    std::string geofnameprefix = fnameprefix+"03";
          	    // geofnamefp will be something like "MOD03.A2008120.0000"
                    std::string geofnamefp = geofnameprefix + fnamemiddle;
                    DIR *dirp;
                    struct dirent* dirs;
    
    	            dirp = opendir(dirfilename.c_str());
                    while (dirs = readdir(dirp)){
	               if(strncmp(dirs->d_name,geofnamefp.c_str(),geofnamefp.size())==0){
          	         dimmapfilename = dirfilename + "/"+ dirs->d_name;
                         closedir(dirp);
                         break;
                       }
                    }
                 }
               }
          }
       }
    }

    int32 projcode=-1;
    if(grid_or_swath==0) // grid
    {
	int32 gfid, gridid;

    	// Obtain the grid id
    	gfid = GDopen(const_cast < char *>(filename.c_str ()), DFACC_READ);
	if(gfid<0) ERROR("GDopen");

	// Attach the grid id; make the grid valid.
    	gridid = GDattach(gfid, const_cast < char *>(dataset->getName().c_str())); 
	if(gridid<0) ERROR("GDattach");	

	int32 zone, sphere;
        float64 params[NPROJ];
        intn r;

        r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
        if (r!=0) ERROR("GDprojinfo");

	GDdetach(gridid);
	GDclose(gfid);	
    }

    // First handling data fields
    for(it_f = fields.begin(); it_f != fields.end(); it_f++)
    {	
           DBG(cout <<"New field Name " <<(*it_f)->getNewName()<<endl);
           BaseType *bt=NULL;
	   int fieldtype = (*it_f)->getFieldType();// Whether the field is real field,lat/lon field or missing Z-dimension field 
           switch((*it_f)->getType())
		{

#define HANDLE_CASE2(tid, type) \
	case tid: \
	        if(change_data_type((*it_f)->getNewName()) && fieldtype==0) \
                        bt = new (HDFFloat32) ((*it_f)->getNewName(), (dataset)->getName()); \
		else \
                        bt = new (type)((*it_f)->getNewName(), (dataset)->getName()); \
        break;

#define HANDLE_CASE(tid, type)\
                    case tid: \
			if(projcode==GCTP_SOM) \
				bt = new (HDFFloat32) ((*it_f)->getNewName(), (dataset)->getName()); \
			else \
                        	bt = new (type)((*it_f)->getNewName(), (dataset)->getName()); \
			break;
                    HANDLE_CASE(DFNT_FLOAT32, HDFFloat32);
                    HANDLE_CASE(DFNT_FLOAT64, HDFFloat64);
#ifndef SIGNED_BYTE_TO_INT32
                    HANDLE_CASE2(DFNT_INT8, HDFByte);
#else
                    HANDLE_CASE2(DFNT_INT8,HDFInt32);
#endif
                    HANDLE_CASE2(DFNT_UINT8, HDFByte);
                    HANDLE_CASE2(DFNT_INT16, HDFInt16);
                    HANDLE_CASE2(DFNT_UINT16,HDFUInt16);
		    HANDLE_CASE2(DFNT_INT32, HDFInt32);
                    HANDLE_CASE2(DFNT_UINT32, HDFUInt32);
                    HANDLE_CASE2(DFNT_UCHAR8, HDFByte);
                    HANDLE_CASE2(DFNT_CHAR8, HDFByte);
                  default:
                    throw InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
#undef HANDLE_CASE2
		}

            //int fieldtype = (*it_f)->getFieldType();// Whether the field is real field,lat/lon field or missing Z-dimension field 
             
            if(bt)
		{
                    const std::vector<HDFEOS2::Dimension*>& dims= (*it_f)->getCorrectedDimensions();
                    std::vector<HDFEOS2::Dimension*>::const_iterator it_d;

                    if(fieldtype == 0 || fieldtype == 3 || fieldtype == 5) {

                        // grid or swath without using dimension map
			if(grid_or_swath==0){
                            HDFEOS2Array_RealField *ar = NULL;
                            ar = new HDFEOS2Array_RealField(
                                                            (*it_f)->getRank(),
                                                            filename,
                                                            (dataset)->getName(), "", (*it_f)->getName(),
                                                            (*it_f)->getNewName(), bt);
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                             ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);
                            delete ar;

                        }
			else if(grid_or_swath==1){

                            // current design: Z field should be under geofield.
//We find the third dimension is located under "data fields" group for some MODIS files; so release this for you. KY 2010-6-26
#if 0
                            if(fieldtype == 3) 
                              throw InternalErr(__FILE__, __LINE__,
                  			"Coordinate variables for Swath should be under geolocation group");

#endif
                            std::string tempfieldname;
                            // Because the field name gets changed for third-dimension grid 
                            // to fulfill the IDV/Panoply COARD request(the field name can not be the same as the dimension name)
                            // we have to obtain the original field name,saved as the tempfieldname.
                             
                            if((*it_f)->getSpecialCoard()) {
                               if(fieldtype != 3){
                                  throw InternalErr(__FILE__, __LINE__,
                                  "Coordinate variables for Swath should be under geolocation group");
                               }
                               tempfieldname = (*it_f)->getName_specialcoard();
                            }
                            else tempfieldname = (*it_f)->getName();
                            if((*it_f)->UseDimMap()) {
                                HDFEOS2ArraySwathDimMapField * ar = NULL;
    				ar = new HDFEOS2ArraySwathDimMapField(                           
                                                                     (*it_f)->getRank(),
                                                           	      filename,
                                                                     (dataset)->getName(), "", tempfieldname,
                                                                     dimmaps,(*it_f)->getNewName(),bt);
                                for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                   ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                                dds.add_var(ar);
                                delete ar;
                               
                            }
                            else {
                               HDFEOS2Array_RealField * ar = NULL;
                               ar = new HDFEOS2Array_RealField(
                                                            (*it_f)->getRank(),
                                                            filename,
                                                            "", (dataset)->getName(), tempfieldname,
                                                            (*it_f)->getNewName(), bt);
                               for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                   ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                               dds.add_var(ar);
                               delete ar;
                            }
                        }
			else {
                               delete bt;
		         	throw InternalErr(__FILE__, __LINE__, "The current type should be either grid or swath");
                        }
                           

                    }

                    if(fieldtype == 1 || fieldtype == 2) {
                        if(grid_or_swath==0) {

                            HDFEOS2ArrayGridGeoField *ar = NULL;
                            int fieldtype = (*it_f)->getFieldType();
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
                                                              filename,
                                                              (dataset)->getName(),(*it_f)->getName(),
                                                              (*it_f)->getNewName(), bt);
			    if(projcode==GCTP_SOM)
				ar->append_dim(180, "SOMBlockDim");
			    
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;
                        }
                        // We encounter a very special MODIS case (MOD/MYD ATML2 files),
                        // Latitude and longitude fields are located under data fields.
                        // So include this case. KY 2010-7-12
                        if(grid_or_swath ==1) {

                            HDFEOS2ArraySwathGeoField * ar = NULL;
                            ar = new HDFEOS2ArraySwathGeoField(
                                                               (*it_f)->getRank(),
                                                               filename,
                                                               (dataset)->getName(), (*it_f)->getName(),
                                                               (*it_f)->getNewName(), bt);

                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;

                        }
                    }
                    
                    if(fieldtype == 4) { //missing Z dimensional field
                        if(grid_or_swath == 0) {// only need to handle the grid case.
                            //assert((*it_f)->getRank()== 1);
                            if((*it_f)->getRank()!=1){
                               delete bt;
                               throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                            }
                            int nelem = ((*it_f)->getDimensions()[0])->getSize();
                        
                            HDFEOS2ArrayMissGeoField *ar = NULL;
                            ar = new HDFEOS2ArrayMissGeoField(
                                                              (*it_f)->getRank(),
                                                              nelem,
                                                              (*it_f)->getNewName(),bt);

                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;
                        }
                    }

		}
	}

    // For swath, we have to include fields under "geolocation fields"
    if(grid_or_swath == 1) {// swath

        HDFEOS2::SwathDataset *sw = static_cast<HDFEOS2::SwathDataset *>(dataset);
        const std::vector<HDFEOS2::Field*>& geofields = sw->getGeoFields();

        for(it_f = geofields.begin(); it_f != geofields.end(); it_f++)
            {
                DBG(cout <<"New field Name " <<(*it_f)->getNewName()<<endl);
                BaseType *bt=NULL;
                switch((*it_f)->getType())
                    {
#define HANDLE_CASE(tid, type)                                          \
                        case tid:                                       \
                            bt = new (type)((*it_f)->getNewName(), (dataset)->getName()); \
                            break;
                        HANDLE_CASE(DFNT_FLOAT32, HDFFloat32);
                        HANDLE_CASE(DFNT_FLOAT64, HDFFloat64);
#ifndef SIGNED_BYTE_TO_INT32
                        HANDLE_CASE(DFNT_INT8, HDFByte);
#else 
                        HANDLE_CASE(DFNT_INT8, HDFInt32);
#endif
                        HANDLE_CASE(DFNT_UINT8, HDFByte);
                        HANDLE_CASE(DFNT_INT16, HDFInt16);
                        HANDLE_CASE(DFNT_UINT16, HDFUInt16);
                        HANDLE_CASE(DFNT_INT32, HDFInt32);
                        HANDLE_CASE(DFNT_UINT32, HDFUInt32);
                        HANDLE_CASE(DFNT_UCHAR8, HDFByte);
                        HANDLE_CASE(DFNT_CHAR8, HDFByte);
                    default:
                        InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
                    }
                int fieldtype = (*it_f)->getFieldType();// Whether the field is real field,lat/lon field or missing Z-dimension field

                if(bt)
                    {
                        const std::vector<HDFEOS2::Dimension*>& dims= (*it_f)->getCorrectedDimensions();

                        std::vector<HDFEOS2::Dimension*>::const_iterator it_d;

                        // Real field or Z field
                        if(fieldtype == 0 || fieldtype == 3 || fieldtype == 5) {

                            std::string tempfieldname;
                            // Because the field name gets changed for third-dimension grid 
                            // to fulfill the IDV/Panoply COARD request(the field name can not be the same as the dimension name)
                            // we have to obtain the original field name,saved as the tempfieldname.
                             
                            if((*it_f)->getSpecialCoard()) {
                               if(fieldtype != 3){
                                  throw InternalErr(__FILE__, __LINE__,
                                  "Coordinate variables for Swath should be under geolocation group");
                               }
                               tempfieldname = (*it_f)->getName_specialcoard();
                            }
                            else tempfieldname = (*it_f)->getName();
 

                            HDFEOS2Array_RealField *ar = NULL;
                            ar = new HDFEOS2Array_RealField(
                                                            (*it_f)->getRank(),
                                                            filename,
                                                            "",(dataset)->getName(), tempfieldname,
                                                            (*it_f)->getNewName(), bt);
 
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;
                         

                       }
                    
                       if(fieldtype == 1 || fieldtype == 2) { // Latitude,Longitude

                          // Use Swath dimension map
                          if((*it_f)->UseDimMap()) {
                                // Have an extra HDF-EOS file for latitude and longtiude
                                if(!dimmapfilename.empty()) {
				     HDFEOS2ArraySwathGeoDimMapExtraField *ar = NULL;
                                     ar = new HDFEOS2ArraySwathGeoDimMapExtraField(
                                                                     (*it_f)->getRank(),
                                                                      dimmapfilename,
                                                                      (*it_f)->getName(),
                                                                     (*it_f)->getNewName(),bt);
                                      for(it_d = dims.begin(); it_d != dims.end(); it_d++)
	                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
        	                      dds.add_var(ar);
                	              delete ar;

                                }
                                // Will interpolate by the handler
                                else {
                                     HDFEOS2ArraySwathGeoDimMapField * ar = NULL;
                                     ar = new HDFEOS2ArraySwathGeoDimMapField(
                                                                     (*it_f)->getType(),
                                                                     (*it_f)->getRank(),
                                                                      filename,
                                                                     (dataset)->getName(), (*it_f)->getName(),
                                                                     dimmaps,(*it_f)->getNewName(),bt);
                                      for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                        ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                                      dds.add_var(ar);
                                      delete ar;

                                }
                               
                         }
                         else {// General swath(no dimension map)

                            HDFEOS2ArraySwathGeoField * ar = NULL;
                            ar = new HDFEOS2ArraySwathGeoField(
                                                               (*it_f)->getRank(),
                                                               filename,
                                                               (dataset)->getName(), (*it_f)->getName(),
                                                               (*it_f)->getNewName(), bt);

                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;

                        }
                       }


                       if(fieldtype == 4) {//missing Z dimensional field
                           if((*it_f)->getRank()!=1){
                               delete bt;
                               throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                           }
                           // Using getCorrectedDimensions() also covers the potential dimension map case.
                           int nelem = ((*it_f)->getCorrectedDimensions()[0])->getSize();
                        
                           HDFEOS2ArrayMissGeoField * ar = NULL;
                           ar = new HDFEOS2ArrayMissGeoField(
                                                             (*it_f)->getRank(),
                                                             nelem,
                                                             (*it_f)->getNewName(),bt);
                           for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                           dds.add_var(ar);

                           delete ar;

                      }
                    }
            }
    }
}

// Check if this file is an HDF-EOS2 file and if yes, build DDS only.
bool read_dds_hdfeos2(DDS & dds, const string & filename, 
                      HE2CFNcML* ncml,
	              HE2CFShortName* sn, HE2CFShortName* sn_dim,
        	      HE2CFUniqName* un, HE2CFUniqName* un_dim)
{

    dds.set_dataset_name(basename(filename));

    // There are some HDF-EOS2 files(MERRA) that should be treated
    // exactly like HDF4 SDS files. We don't need to use HDF-EOS2 APIs to 
    // retrieve any information. In fact, treating them as HDF-EOS2 files
   // will cause confusions and wrong information, although may not be essential.
   // So far, we've found only MERRA data that have this problem.
   // A quick fix is to check if the file name contains MERRA. KY 2011-3-4 
//   cerr<<"basename " <<basename(filename) <<endl;
     // Find MERRA data, return false, use HDF4 SDS code.
     if((basename(filename)).compare(0,5,"MERRA")==0) {
        return false; 
     }

    HDFEOS2::File *f;
    try {
        f = HDFEOS2::File::Read(filename.c_str());
    } catch (HDFEOS2::Exception &e)
	{
          if(!e.getFileType()){
            return false;
          }
          else {
            throw InternalErr(e.what());
	}
    }
        
   try {
       f->Prepare(filename.c_str(),sn,sn_dim,un,un_dim);
   } catch (HDFEOS2::Exception &e)
      {
        delete f;
        throw InternalErr(e.what());
   }
        
    std::vector<std::string> out;

    // Remove the path, only obtain the "file name"
    HDFEOS2::Utility::Split(filename.c_str(), (int)filename.length(), '/',
                            out);
    dds.set_dataset_name(*out.rbegin());

    //Some grids have one shared lat/lon pair. For this case,"onelatlon" is true.
    // Other grids have their individual grids. We have to handle them differently.
    // ownll is the flag to distinguish "one lat/lon pair" and multiple lat/lon pairs.
    const std::vector<HDFEOS2::GridDataset *>& grids = f->getGrids();
    bool ownll= false;
    bool onelatlon = f->getOneLatLon();

    std::vector<HDFEOS2::GridDataset *>::const_iterator it_g;
    for(it_g = grids.begin(); it_g != grids.end(); it_g++){
        ownll = onelatlon?onelatlon:(*it_g)->getLatLonFlag();
        read_dds_hdfeos2_grid_swath(
                                    dds, filename, static_cast<HDFEOS2::Dataset*>(*it_g), 0,ownll);
    }

    const std::vector<HDFEOS2::SwathDataset *>& swaths= f->getSwaths();
    std::vector<HDFEOS2::SwathDataset *>::const_iterator it_s;
    for(it_s = swaths.begin(); it_s != swaths.end(); it_s++)
        read_dds_hdfeos2_grid_swath(
                                    dds, filename, static_cast<HDFEOS2::Dataset*>(*it_s), 1,false);//No global lat/lon for multiple swaths


    delete f;
    return true;
}

// The wrapper of building DDS function.
bool read_dds_hdfsp(DDS & dds, const string & filename, 
                      HE2CFNcML* ncml,
	              HE2CFShortName* sn, HE2CFShortName* sn_dim,
        	      HE2CFUniqName* un, HE2CFUniqName* un_dim)
{

    dds.set_dataset_name(basename(filename));

    // Very strange behavior for the HDF4 library, I have to obtain the ID here.
    // If defined inside the Read function, the id will be 0 and the following output is 
    // unexpected. KY 2010-8-9
    int32 myfileid;
    myfileid = Hopen(const_cast<char *>(filename.c_str()), DFACC_READ,0);

    HDFSP::File *f;

    try {
        f = HDFSP::File::Read(filename.c_str(), myfileid);
    } catch (HDFSP::Exception &e)
	{
            throw InternalErr(e.what());
    }
    
        
    try {
        f->Prepare(sn,sn_dim,un,un_dim);
    } catch (HDFSP::Exception &e) {
        delete f;
        throw InternalErr(e.what());
    }
        
    std::vector<std::string> out;
    HDFEOS2::Utility::Split(filename.c_str(), (int)filename.length(), '/',
                            out);
    dds.set_dataset_name(*out.rbegin());

    const std::vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    // Read SDS 
    std::vector<HDFSP::SDField *>::const_iterator it_g;
    for(it_g = spsds.begin(); it_g != spsds.end(); it_g++)
       read_dds_spfields(dds,filename,(*it_g),f->getSPType());

    // Read Vdata fields.
    // This is just for speeding up the performance for CERES data, we turn off some CERES vdata fields
    if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) {
       for(std::vector<HDFSP::VDATA *>::const_iterator i=f->getVDATAs().begin();
          i!=f->getVDATAs().end();i++) {
          if(!(*i)->getTreatAsAttrFlag()){
             for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) 
                read_dds_spvdfields(dds,filename,(*i)->getObjRef(),(*j)->getNumRec(),(*j)); 
          }
       }
    }
        
    delete f;
    return true;
}

// The wrapper of building hybrid DDS function.
bool read_dds_hdfhybrid(DDS & dds, const string & filename, 
                      HE2CFNcML* ncml,
	              HE2CFShortName* sn, HE2CFShortName* sn_dim,
        	      HE2CFUniqName* un, HE2CFUniqName* un_dim)
{

    dds.set_dataset_name(basename(filename));

    // Very strange behavior for the HDF4 library, I have to obtain the ID here.
    // If defined inside the Read function, the id will be 0 and the following output is 
    // unexpected. KY 2010-8-9
    int32 myfileid;
    myfileid = Hopen(const_cast<char *>(filename.c_str()), DFACC_READ,0);
//cerr<<"myfileid at hdfdesc.cc "<<myfileid <<endl;

    HDFSP::File *f;

    try {
        f = HDFSP::File::Read_Hybrid(filename.c_str(), myfileid);
    } catch (HDFSP::Exception &e)
	{
            throw InternalErr(e.what());
    }
    
        
    std::vector<std::string> out;
    HDFEOS2::Utility::Split(filename.c_str(), (int)filename.length(), '/',
                            out);
    dds.set_dataset_name(*out.rbegin());

    const std::vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    // Read SDS 
    std::vector<HDFSP::SDField *>::const_iterator it_g;
    for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

//       cerr<<"sds new name "<<(*it_g)->getNewName() <<endl;
       read_dds_spfields(dds,filename,(*it_g),f->getSPType());
}

#if 0
    // Read Vdata fields.
    // This is just for speeding up the performance for CERES data, we turn off some CERES vdata fields
    if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) {
       for(std::vector<HDFSP::VDATA *>::const_iterator i=f->getVDATAs().begin();
          i!=f->getVDATAs().end();i++) {
          if(!(*i)->getTreatAsAttrFlag()){
             for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) 
                read_dds_spvdfields(dds,filename,(*i)->getObjRef(),(*j)->getNumRec(),(*j)); 
          }
       }
    }
#endif
        
    delete f;
    return true;
}


// Read SDS fields
void read_dds_spfields(DDS &dds,const string& filename,HDFSP::SDField *spsds, SPType sptype) {
//cerr <<"sptype " <<sptype <<endl;
     BaseType *bt=NULL;
//cerr<<"come here "<<endl;
//cerr<<"spsds->getType()"<<spsds->getType() <<endl;
     switch(spsds->getType())
		{
#define HANDLE_CASE(tid, type)                                          \
                    case tid:                                           \
                        bt = new (type)(spsds->getNewName(),filename); \
			break;
                    HANDLE_CASE(DFNT_FLOAT32, HDFFloat32);
                    HANDLE_CASE(DFNT_FLOAT64, HDFFloat64);
#ifndef SIGNED_BYTE_TO_INT32
                    HANDLE_CASE(DFNT_INT8, HDFByte);
#else
                    HANDLE_CASE(DFNT_INT8,HDFInt32);
#endif
                   HANDLE_CASE(DFNT_UINT8, HDFByte); 
                    HANDLE_CASE(DFNT_INT16, HDFInt16);
                    HANDLE_CASE(DFNT_UINT16, HDFUInt16);
                    HANDLE_CASE(DFNT_INT32, HDFInt32);
                    HANDLE_CASE(DFNT_UINT32, HDFUInt32);
                    HANDLE_CASE(DFNT_UCHAR8, HDFByte);
                    HANDLE_CASE(DFNT_CHAR8, HDFByte);
                  default:
                    InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
		}
            int fieldtype = spsds->getFieldType();// Whether the field is real field,lat/lon field or missing Z-dimension field 
             
//cerr<<"fieldtype "<<fieldtype <<endl;
            if(bt)
		{
              
                    const std::vector<HDFSP::Dimension*>& dims= spsds->getCorrectedDimensions();
                    std::vector<HDFSP::Dimension*>::const_iterator it_d;

                    if(fieldtype == 0 || fieldtype == 3 ) {

                            HDFSPArray_RealField *ar = NULL;
                            ar = new HDFSPArray_RealField(
                                                            spsds->getRank(),
                                                            filename,
                                                            spsds->getSDSref(),
                                                            spsds->getType(),
                                                            sptype,
                                                            spsds->getName(),
                                                             spsds->getNewName(),bt);
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                             ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);
                            delete ar;

                    }

                    if(fieldtype == 1 || fieldtype == 2) {
//cerr<<"File type "<<sptype <<endl;

                      if(sptype == MODISARNSS) { 
//cerr<<"coming to MODISARNSS" <<endl;

                             HDFSPArray_RealField *ar = NULL;
                            ar = new HDFSPArray_RealField(
                                                            spsds->getRank(),
                                                            filename,
                                                            spsds->getSDSref(),
                                                            spsds->getType(),
                                                            sptype,
                                                            spsds->getName(),
                                                             spsds->getNewName(),bt);
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                             ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);
                            delete ar;

                      }
                      else {

                            HDFSPArrayGeoField *ar = NULL;

                            ar = new HDFSPArrayGeoField(
                                                           spsds->getRank(),
                                                           filename,
                                                           spsds->getSDSref(),
                                                           spsds->getType(),
                                                           sptype,
                                                           fieldtype,
                                                           spsds->getName(),
                                                           spsds->getNewName(), bt);
                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;
                      }
                    }
                    
                    
                    if(fieldtype == 4) { //missing Z dimensional field
                            //assert((*it_f)->getRank()== 1);
                            if(spsds->getRank()!=1){
                               delete bt;
                               throw InternalErr(__FILE__, __LINE__, "The rank of missing Z dimension field must be 1");
                            }
                            int nelem = (spsds->getDimensions()[0])->getSize();
                        
                            HDFSPArrayMissGeoField *ar = NULL;
                            ar = new HDFSPArrayMissGeoField(
                                                              spsds->getRank(),
                                                              nelem,
                                                              spsds->getNewName(),bt);

                            for(it_d = dims.begin(); it_d != dims.end(); it_d++)
                                ar->append_dim((*it_d)->getSize(), (*it_d)->getName());
                            dds.add_var(ar);

                            delete ar;
                    }
                   

        	}

      }

// Read Vdata fields.
void read_dds_spvdfields(DDS &dds,const string& filename,int32 objref,int32 numrec,HDFSP::VDField *spvd) {

     BaseType *bt=NULL;
     switch(spvd->getType())
		{
#define HANDLE_CASE(tid, type)                                          \
                    case tid:                                           \
                        bt = new (type)(spvd->getNewName(),filename); \
			break;
                    HANDLE_CASE(DFNT_FLOAT32, HDFFloat32);
                    HANDLE_CASE(DFNT_FLOAT64, HDFFloat64);
#ifndef SIGNED_BYTE_TO_INT32
                    HANDLE_CASE(DFNT_INT8, HDFByte);
#else
                    HANDLE_CASE(DFNT_INT8,HDFInt32);
#endif
                    HANDLE_CASE(DFNT_UINT8, HDFByte);
                    HANDLE_CASE(DFNT_INT16, HDFInt16);
                    HANDLE_CASE(DFNT_UINT16, HDFUInt16);
                    HANDLE_CASE(DFNT_INT32, HDFInt32);
                    HANDLE_CASE(DFNT_UINT32, HDFUInt32);
                    HANDLE_CASE(DFNT_UCHAR8, HDFByte);
                    HANDLE_CASE(DFNT_CHAR8, HDFByte);
                  default:
                    InternalErr(__FILE__,__LINE__,"unsupported data type.");
#undef HANDLE_CASE
		}
             
            if(bt)
		{
                            HDFSPArray_VDField *ar = NULL;

                            // If the field order is >1, the vdata field will be 2-D array
                            // with the number of elements along the fastest changing dimension
                            // as the field order.
                            int vdrank = ((spvd->getFieldOrder())>1)?2:1;
                            ar = new HDFSPArray_VDField(
                                                            vdrank,
                                                            filename,
                                                            objref,
                                                            spvd->getType(),
                                                            spvd->getFieldOrder(),
                                                            spvd->getName(),
                                                            spvd->getNewName(),
                                                            bt);

                            //std::string dimname1 = "VDfDim0_"+spvd->getNewName();
                            std::string dimname1 = "D0_"+spvd->getNewName();

                            //std::string dimname2 = "VDfDim1_"+spvd->getNewName();
                            std::string dimname2 = "D1_"+spvd->getNewName();
                            if(spvd->getFieldOrder() >1) {
                              ar->append_dim(numrec,dimname1);
                              ar->append_dim(spvd->getFieldOrder(),dimname2);
                            }
                            else 
                              ar->append_dim(numrec,dimname1);

                            dds.add_var(ar);
                            delete ar;
                  }

      }


string
print_type(int32 type)
{

    // I expanded the list based on libdap/AttrTable.h.
    static const char UNKNOWN[]="Unknown";    
    static const char BYTE[]="Byte";
    static const char INT16[]="Int16";
    static const char UINT16[]="Uint16";        
    static const char INT32[]="Int32";
    static const char UINT32[]="Uint32";
    static const char FLOAT32[]="Float32";
    static const char FLOAT64[]="Float64";    
    static const char STRING[]="String";
    
    // I got different cases from hntdefs.h.
    switch (type) {
        
    case DFNT_CHAR:
	return STRING;
        
    case DFNT_UCHAR8:
	return STRING;        
        
    case DFNT_UINT8:
	return BYTE;
        
    case DFNT_INT8:
	return INT16;
        
    case DFNT_UINT16:
	return UINT16;
        
    case DFNT_INT16:
	return INT16;

    case DFNT_INT32:
	return INT32;

    case DFNT_UINT32:
	return UINT32;
        
    case DFNT_FLOAT:
	return FLOAT32;

    case DFNT_DOUBLE:
	return FLOAT64;
        
    default:
	return UNKNOWN;
    }
    
}

// Borrowed codes from ncdas.cc in netcdf_handle4 module.
string
print_attr(int32 type, int loc, void *vals)
//print_attr(int32 type, int loc, char *vals)
{
    ostringstream rep;
    
    union {
        char *cp;
        short *sp;
        int32 /*nclong */ *lp;
        float *fp;
        double *dp;
    } gp;

    switch (type) {
        
    case DFNT_UINT8:
    case DFNT_INT8:        
        {
            unsigned char uc;
            gp.cp = (char *) vals;

            uc = *(gp.cp+loc);
            rep << (int)uc;
            return rep.str();
        }

    case DFNT_CHAR:
        {
            return escattr(static_cast<const char*>(vals));
        }

    case DFNT_INT16:
    case DFNT_UINT16:        
        {
            gp.sp = (short *) vals;
            rep << *(gp.sp+loc);
            return rep.str();
        }

    case DFNT_INT32:
    case DFNT_UINT32:        
        {
            gp.lp = (int32 *) vals; 
            rep << *(gp.lp+loc);
            return rep.str();
        }

    case DFNT_FLOAT:
        {
            gp.fp = (float *) vals;
            rep << showpoint;
            rep << setprecision(10);
            rep << *(gp.fp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
        }
        
    case DFNT_DOUBLE:
        {
            gp.dp = (double *) vals;
            rep << std::showpoint;
            rep << std::setprecision(17);
            rep << *(gp.dp+loc);
            if (rep.str().find('.') == string::npos
                && rep.str().find('e') == string::npos)
                rep << ".";
            return rep.str();
            break;
        }
    default:
        return string("UNKNOWN");
    }
    
}

// Build DAS for special HDF4 files.

bool read_das_hdfsp(DAS & das, const string & filename, 
                      HE2CFNcML* ncml,
	              HE2CFShortName* sn, HE2CFShortName* sn_dim,
        	      HE2CFUniqName* un, HE2CFUniqName* un_dim)
{
    _das = &das;

    int32 myfileid;
    myfileid = Hopen(const_cast<char *>(filename.c_str()), DFACC_READ,0);

// cerr <<"inside hdfsp myfileid = "<<myfileid<<endl;
    //Hclose(myfileid);

    HDFSP::File *f;
    try {
        f = HDFSP::File::Read(filename.c_str(), myfileid);
    } 
    catch (HDFSP::Exception &e)
	{
            throw InternalErr(e.what());
	}
        
    try {
        f->Prepare(sn,sn_dim,un,un_dim);
    } 
    catch (HDFSP::Exception &e) {
        delete f;
        throw InternalErr(e.what());
    }
        
    string core_metadata = "";
    string archive_metadata = "";
    string struct_metadata = "";
    
    HDFSP::SD* spsd = f->getSD();

    //Store value of "Scaling" attribute.
    string scaling;
    //Store value of "Slope" attribute.
    float slope, intercept;
    
    for(std::vector<HDFSP::Attribute *>::const_iterator i=spsd->getAttributes().begin();i!=spsd->getAttributes().end();i++) {
//        cerr<<"Attribute Name SD "<<(*i)->getName() <<endl;
//        cerr<<"Attribute Count SD "<<(*i)->getCount() <<endl;
//        cerr<<"Attribute Type SD "<<(*i)->getType() <<endl; // <hyokyung 2010.06.17. 16:00:45>
        // To present attribute in corrected type, 
        // More work needs to be done convert, there is a bug in the code. 
//        string tempstring((*i)->getValue().begin(),(*i)->getValue().end());
        //cerr<<"Attribute value SD " << tempstring << endl;
        
	//We want to add two new attributes, "scale_factor" and "add_offset" to data fields if the scaling equation is linear. Their values will be copied directly from file attributes. This addition is for OBPG L3 only.
	if(f->getSPType()==OBPGL3)
	{
		if((*i)->getName()=="Scaling")
		{
			string tmpstring((*i)->getValue().begin(), (*i)->getValue().end());
			scaling = tmpstring;
		}
		if((*i)->getName()=="Slope")
		{
			
			switch((*i)->getType())
			{
#define GET_SLOPE(TYPE, CAST) \
        case DFNT_##TYPE: \
        { \
                CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
                slope = (float)tmpvalue; \
        } \
        break;

				GET_SLOPE(INT16,   int16);
				GET_SLOPE(INT32,   int32);
				GET_SLOPE(FLOAT32, float);
				GET_SLOPE(FLOAT64, double);
#undef GET_SLOPE
			} ;
			//slope = *(float*)&((*i)->getValue()[0]);
		}
		if((*i)->getName()=="Intercept")
                {	
			switch((*i)->getType())
                        {
#define GET_INTERCEPT(TYPE, CAST) \
        case DFNT_##TYPE: \
        { \
                CAST tmpvalue = *(CAST*)&((*i)->getValue()[0]); \
                intercept = (float)tmpvalue; \
        } \
        break;
				GET_INTERCEPT(INT16,   int16);
				GET_INTERCEPT(INT32,   int32);
                                GET_INTERCEPT(FLOAT32, float);
                                GET_INTERCEPT(FLOAT64, double);
#undef GET_INTERCEPT
                        } ;
                        //intercept = *(float*)&((*i)->getValue()[0]);
                }
	}
 
        if(((*i)->getName().compare(0, 12, "CoreMetadata" )== 0) ||
            ((*i)->getName().compare(0, 12, "coremetadata" )== 0)){

            // We assume that CoreMetadata.0, CoreMetadata.1, ..., CoreMetadata.n attribures
            // are processed in the right order duing HDFSP::Attribute vector iteration.
            // Otherwise, this won't work.
            string tempstring((*i)->getValue().begin(),(*i)->getValue().end());
          
             // Temporarily turn off CERES data since there are so many fields in CERES. It will choke clients KY 2010-7-9
             if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) 
                core_metadata.append(tempstring);
        }
        else if(((*i)->getName().compare(0, 15, "ArchiveMetadata" )== 0) ||
                 ((*i)->getName().compare(0, 16, "ArchivedMetadata")==0) ||
                ((*i)->getName().compare(0, 15, "archivemetadata" )== 0)){
            string tempstring((*i)->getValue().begin(),(*i)->getValue().end());
              // Currently some TRMM "swath" archivemetadata includes special characters that cannot be handled by OPeNDAP
              // So turn it off.
              // Turn off CERES  data since it may choke JAVA clients KY 2010-7-9
              if(f->getSPType() != TRMML2 && f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG)
                archive_metadata.append(tempstring);
        }
        else if(((*i)->getName().compare(0, 14, "StructMetadata" )== 0) ||
                ((*i)->getName().compare(0, 14, "structmetadata" )== 0)){
            string tempstring((*i)->getValue().begin(),(*i)->getValue().end());
              // Currently some TRMM "swath" archivemetadata includes special characters that cannot be handled by OPeNDAP
              // So turn it off
              // Turn off CERES  data since it may choke JAVA clients KY 2010-7-9
              if(f->getSPType() != TRMML2 && f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG)
                struct_metadata.append(tempstring);
        }        
        else {
            // <hyokyung 2010.06.17. 15:44:49>
            //  Process gloabal attributes
            AttrTable *at = das.get_table("HDF_GLOBAL");
            if (!at)
                at = das.add_table("HDF_GLOBAL", new AttrTable);

            // We treat string differently. DFNT_UCHAR and DFNT_CHAR are treated as strings.
            if((*i)->getType()==DFNT_UCHAR || (*i)->getType() == DFNT_CHAR){
                string tempstring2((*i)->getValue().begin(),(*i)->getValue().end());
                string tempfinalstr= string(tempstring2.c_str());
                at->append_attr((*i)->getName(), "String" , tempfinalstr);
            }

            else {
              for (unsigned int loc=0; loc < (*i)->getCount() ; loc++) {
                 string print_rep = print_attr((*i)->getType(), loc, (void*) &((*i)->getValue()[0]));
                 at->append_attr((*i)->getName(), print_type((*i)->getType()), print_rep);
              }
          
            }
        }
        
    }
    
    // Coremetadata, structmetadata and archive metadata need special parsers.
    // Write coremetadata.
    if(core_metadata.size() > 0){
        AttrTable *at = das.get_table("CoreMetadata");
        if (!at)
            at = das.add_table("CoreMetadata", new AttrTable);
        // tell lexer to scan attribute string
        void *buf = hdfeos_string(core_metadata.c_str());
        parser_arg arg(at);
#if 0
        // jhrg 8/19/11
        if (hdfeosparse(static_cast < void *>(&arg)) != 0
            || arg.status() == false){
            (*BESLog::TheLog()) << "HDF-EOS parse error " << core_metadata << endl;
        }
#endif
        if (hdfeosparse(static_cast < void *>(&arg)) != 0)
        	throw Error("HDF-EOS parse error while processing a CoreMetadata HDFEOS attribute.");

        // Errors returned from here are ignored.
        if (arg.status() == false) {
        	BESDEBUG("h4", "HDF-EOS parse error while processing a CoreMetadata HDFEOS attribute. (2)" << endl
        		<< arg.error()->get_error_message() << endl);
        }

        hdfeos_delete_buffer(buf);
    }
            
    // Write archive metadata.
    if(archive_metadata.size() > 0){
        AttrTable *at = das.get_table("ArhiveMetadata");
        if (!at)
            at = das.add_table("ArhiveMetadata", new AttrTable);
        // tell lexer to scan attribute string
        void *buf = hdfeos_string(archive_metadata.c_str());
        parser_arg arg(at);
#if 0
        // jhrg 8/19/11

        if (hdfeosparse(static_cast < void *>(&arg)) != 0
            || arg.status() == false){
            (*BESLog::TheLog()) << "HDF-EOS parse error " << archive_metadata << endl;
        }
#endif
        if (hdfeosparse(static_cast < void *>(&arg)) != 0)
        	throw Error("HDF-EOS parse error while processing a ArhiveMetadata HDFEOS attribute.");

        // Errors returned from here are ignored.
        if (arg.status() == false) {
        	BESDEBUG("h4", "HDF-EOS parse error while processing a ArhiveMetadata HDFEOS attribute. (2)" << endl
        		<< arg.error()->get_error_message() << endl);
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
#if 0
        // jhrg 8/19/11

        if (hdfeosparse(static_cast < void *>(&arg)) != 0
            || arg.status() == false){
            (*BESLog::TheLog()) << "HDF-EOS parse error "<< struct_metadata << endl;
        }
#endif
        if (hdfeosparse(static_cast < void *>(&arg)) != 0)
        	throw Error("HDF-EOS parse error while processing a StructMetadata HDFEOS attribute.");

        // Errors returned from here are ignored.
        if (arg.status() == false) {
        	BESDEBUG("h4", "HDF-EOS parse error while processing a StructMetadata HDFEOS attribute. (2)" << endl
        		<< arg.error()->get_error_message() << endl);
        }

        hdfeos_delete_buffer(buf);
    }
    
    const std::vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    std::vector<HDFSP::SDField *>::const_iterator it_g;
    for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){
        
        //<hyokyung 2010.06.17. 16:02:58>
        AttrTable *at = das.get_table((*it_g)->getNewName());
        if (!at)
            at = das.add_table((*it_g)->getNewName(), new AttrTable);

        // Some fields have "long_name" attributes,so we have to use this attribute rather than creating our own

        bool long_name_flag = false;
	//For OBPG L3 only.
	bool scale_factor_flag = false;
	bool add_offset_flag = false;

        for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {       

           if((*i)->getName() == "long_name") {
             long_name_flag = true;
             //break;
           }

	   if(f->getSPType()==OBPGL3 && (*i)->getName()=="scale_offset")
	     scale_factor_flag = true;		

	   if(f->getSPType()==OBPGL3 && (*i)->getName()=="add_offset")
	     add_offset_flag = true;
        }
        
        if(!long_name_flag) at->append_attr("long_name", "String", (*it_g)->getName());

	if(f->getSPType()==OBPGL3 && (*it_g)->getFieldType()==0)
	{
		if(scaling.find("linear")!=string::npos && !scale_factor_flag)
		{
			string print_rep = print_attr(DFNT_FLOAT32, 0, (void*)&(slope));
			at->append_attr("scale_factor", print_type(DFNT_FLOAT32), print_rep);
		}

		if(scaling.find("linear")!=string::npos && !add_offset_flag)
		{
			string print_rep = print_attr(DFNT_FLOAT32, 0, (void*)&(intercept));
                	at->append_attr("add_offset", print_type(DFNT_FLOAT32), print_rep);
		}
	}
        for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {

          // Handle string first.
          if((*i)->getType()==DFNT_UCHAR || (*i)->getType() == DFNT_CHAR){
                string tempstring2((*i)->getValue().begin(),(*i)->getValue().end());
                string tempfinalstr= string(tempstring2.c_str());
                at->append_attr((*i)->getName(), "String" , tempfinalstr);
          }
          else {
           for (unsigned int loc=0; loc < (*i)->getCount() ; loc++) {

                string print_rep = print_attr((*i)->getType(), loc, (void*) &((*i)->getValue()[0]));
                at->append_attr((*i)->getName(), print_type((*i)->getType()), print_rep);
            }
          }

       }
        
        // Overwrite units if fieldtype is latitude.
        if((*it_g)->getFieldType() == 1){
            at->del_attr("units");      // Override any existing units attribute.
            // at->append_attr("units", "String", "degrees_north");
            at->append_attr("units", "String",(*it_g)->getUnits());
        }
        // Overwrite units if fieldtype is longitude
        if((*it_g)->getFieldType() == 2){
            at->del_attr("units");      // Override any existing units attribute.
            // at->append_attr("units", "String", "degrees_east");
            at->append_attr("units", "String",(*it_g)->getUnits());
        }

        // Overwrite units if fieldtype is level
        if((*it_g)->getFieldType() == 4){
            at->del_attr("units");      // Override any existing units attribute.
            // at->append_attr("units", "String", "degrees_east");
            at->append_attr("units", "String",(*it_g)->getUnits());
        }

        // Overwrite coordinates if fieldtype is neither lat nor lon.
        if((*it_g)->getFieldType() == 0){
            at->del_attr("coordinates");      // Override any existing units attribute.
            at->append_attr("coordinates", "String", (*it_g)->getCoordinate());
        }
      }

       // For some HDF4 files that follow HDF4 dimension scales, P.O. DAAC's AVHRR files.
      // The "otherHDF" category can almost make AVHRR files work, except
     // that AVHRR uses the attribute name "unit" instead of "units" for latitude and longitude,
     // I have to correct the name to follow CF conventions(using "units"). I won't check
     // the latitude and longitude values since latitude and longitude values for some files(LISO files)   
     // are not in the standard range(0-360 for lon and 0-180 for lat). KY 2011-3-3
       if(f->getSPType() == OTHERHDF) {

         bool latflag = false;
         bool latunitsflag = false; //CF conventions use "units" instead of "unit"
         bool lonflag = false;
         bool lonunitsflag = false; // CF conventions use "units" instead of "unit"
         int llcheckoverflag = 0;

         // Here I try to condense the code within just two for loops
        // The outer loop: Loop through all SDS objects
        // The inner loop: Loop through all attributes of this SDS
        // Inside two inner loops(since "units" are common for other fields), 
           // inner loop 1: when an attribute's long name value is L(l)atitude,mark it.
          // inner loop 2: when an attribute's name is units, mark it.
       // Outside the inner loop, when latflag is true, and latunitsflag is false,
       // adding a new attribute called units and the value should be "degrees_north".
       // doing the same thing for longitude.

         for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){

             AttrTable *at = das.get_table((*it_g)->getNewName());
             if (!at)
               at = das.add_table((*it_g)->getNewName(), new AttrTable);

             for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
               if((*i)->getType()==DFNT_UCHAR || (*i)->getType() == DFNT_CHAR){

                if((*i)->getName() == "long_name") {
                  string tempstring2((*i)->getValue().begin(),(*i)->getValue().end());
                  string tempfinalstr= string(tempstring2.c_str());// This may remove some garbage characters
                  if(tempfinalstr=="latitude" || tempfinalstr == "Latitude") // Find long_name latitude
                     latflag = true;
                  if(tempfinalstr=="longitude" || tempfinalstr == "Longitude") // Find long_name latitude
                     lonflag = true;
                }
              }
             }

             if(latflag) {
               for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                if((*i)->getName() == "units") 
                  latunitsflag = true;
              }
             }

             if(lonflag) {
               for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {
                if((*i)->getName() == "units") 
                  lonunitsflag = true;
              }
             }
             if(latflag && !latunitsflag){ // No "units" for latitude, add "units"
               at->append_attr("units","String","degrees_north");
               latflag = false;
               latunitsflag = false;
               llcheckoverflag++;
             }

             if(lonflag && !lonunitsflag){ // No "units" for latitude, add "units"
               at->append_attr("units","String","degrees_east");
               lonflag = false;
               latunitsflag = false;
               llcheckoverflag++;
             }
             if(llcheckoverflag ==2) break;

           }

        }
        

    std::string VDdescname = "hdf4_vd_desc";
    std::string VDdescvalue = "This is an HDF4 Vdata.";
    std::string VDfieldprefix = "Vdata_field_";
    std::string VDattrprefix = "Vdata_attr_";
    std::string VDfieldattrprefix ="Vdata_field_attr_";

 if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) {
    for(std::vector<HDFSP::VDATA *>::const_iterator i=f->getVDATAs().begin();
       i!=f->getVDATAs().end();i++) {

      // Add special vdata attributes
//      at->append_attr(VDdescname, "String" , VDdescvalue);
      bool emptyvddasflag = true;
      if(!((*i)->getAttributes().empty())) emptyvddasflag = false;
      if(((*i)->getTreatAsAttrFlag()))
        emptyvddasflag = false;
      else {
        for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
          if(!((*j)->getAttributes().empty())) {
            emptyvddasflag = false;
            break;
          }
       }
      }

     if(emptyvddasflag) continue;
     AttrTable *at = das.get_table((*i)->getNewName());
     if(!at)
        at = das.add_table((*i)->getNewName(),new AttrTable);
       at->append_attr(VDdescname, "String" , VDdescvalue);

      for(std::vector<HDFSP::Attribute *>::const_iterator it_va = (*i)->getAttributes().begin();it_va!=(*i)->getAttributes().end();it_va++) {

        if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

           string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
           string tempfinalstr= string(tempstring2.c_str());
           at->append_attr(VDattrprefix+(*it_va)->getName(), "String" , tempfinalstr);
          }
        else {
           for (unsigned int loc=0; loc < (*it_va)->getCount() ; loc++) {
                string print_rep = print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
// cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDattrprefix+(*it_va)->getName(), print_type((*it_va)->getType()), print_rep);
            }
          }


//cerr<<"Vdata attribute name "<<(*it_va)->getName() <<endl;
      }
     if(!((*i)->getTreatAsAttrFlag())){ 


      //NOTE: for vdata field, we assume that no special characters are found 
      for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {

//cerr<<"num_rec "<< (*j)->getNumRec()<<endl;
//cerr<<"field order " << (*j)->getFieldOrder()<<endl;
//cerr<<"size of field type " << DFKNTsize((*j)->getType())<<endl;
//cerr<<"size of the vector "<<(*j)->getValue().size() <<endl;

         // This vdata field will NOT be treated as attributes, only save the field attribute as the attribute

       for(std::vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

//cerr<<"Vdata field attribute name "<<(*it_va)->getName() <<endl;

            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

              string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
              string tempfinalstr= string(tempstring2.c_str());
              at->append_attr(VDfieldattrprefix+(*j)->getNewName()+"_"+(*it_va)->getName(), "String" , tempfinalstr);
            }
            else {
              for (unsigned int loc=0; loc < (*it_va)->getCount() ; loc++) {
                string print_rep = print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldattrprefix+(*j)->getNewName()+"_"+(*it_va)->getName(), print_type((*it_va)->getType()), print_rep);
              }
            }

//cerr<<"Vdata attribute name "<<(*it_va)->getName() <<endl;
          }
     }
    }

    else {

      for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
           
 
         if((*j)->getFieldOrder() == 1) {
           if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
             string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
             string tempfinalstr= string(tempstring2.c_str());
             at->append_attr(VDfieldprefix+(*j)->getName(), "String" , tempfinalstr);
           }
           else {
             for (unsigned int loc=0; loc < (*j)->getNumRec(); loc++) {
                string print_rep = print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldprefix+(*j)->getName(), print_type((*j)->getType()), print_rep);
            }
           }
        }
        else {//When field order is greater than 1,we want to print each record in group with single quote,'0 1 2','3 4 5', etc.

           if((*j)->getValue().size() != (DFKNTsize((*j)->getType())*((*j)->getFieldOrder())*((*j)->getNumRec()))){
//cerr<<"num_rec "<< (*j)->getNumRec()<<endl;
//cerr<<"field order " << (*j)->getFieldOrder()<<endl;
//cerr<<"size of field type " << DFKNTsize((*j)->getType())<<endl;
//cerr<<"size of the vector "<<(*j)->getValue().size() <<endl;
               throw InternalErr(__FILE__,__LINE__,"the vdata field size doesn't match the vector value");
            }

          if((*j)->getNumRec()==1){
             if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
               string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
               string tempfinalstr= string(tempstring2.c_str());
               at->append_attr(VDfieldprefix+(*j)->getName(),"String",tempfinalstr);
             }
             else {
              for (unsigned int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                string print_rep = print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldprefix+(*j)->getName(), print_type((*j)->getType()), print_rep);
              }
             }

          }

         else {
          if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){

            for(unsigned int tempcount = 0; tempcount < (*j)->getNumRec()*DFKNTsize((*j)->getType());tempcount ++) {
               std::vector<char>::const_iterator tempit;
               tempit = (*j)->getValue().begin()+tempcount*((*j)->getFieldOrder());
               string tempstring2(tempit,tempit+(*j)->getFieldOrder());
               string tempfinalstr= string(tempstring2.c_str());
               string tempoutstring = "'"+tempfinalstr+"'";
               at->append_attr(VDfieldprefix+(*j)->getName(),"String",tempoutstring);
            }
          }

          else {
            for(unsigned int tempcount = 0; tempcount < (*j)->getNumRec();tempcount ++) {
               at->append_attr(VDfieldprefix+(*j)->getName(),print_type((*j)->getType()),"'");
               for (unsigned int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                string print_rep = print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[tempcount*((*j)->getFieldOrder())]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldprefix+(*j)->getName(), print_type((*j)->getType()), print_rep);
               }
               at->append_attr(VDfieldprefix+(*j)->getName(),print_type((*j)->getType()),"'");
            }
          }
        }
        }

//cerr<<"Vdata field name "<<(*j)->getName()<<endl;
         for(std::vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

//cerr<<"Vdata field attribute name "<<(*it_va)->getName() <<endl;

            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

              string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
              string tempfinalstr= string(tempstring2.c_str());
              at->append_attr(VDfieldattrprefix+(*it_va)->getName(), "String" , tempfinalstr);
            }
            else {
              for (unsigned int loc=0; loc < (*it_va)->getCount() ; loc++) {
                string print_rep = print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldattrprefix+(*it_va)->getName(), print_type((*it_va)->getType()), print_rep);
              }
            }
//cerr<<"Vdata attribute name "<<(*it_va)->getName() <<endl;
          }


         }
       }

      }
  } 
        
  delete f;
  return true;
}

// Build DAS for non-EOS objects of Hybrid HDF-EOS2 files.

bool read_das_hdfhybrid(DAS & das, const string & filename, 
                      HE2CFNcML* ncml,
	              HE2CFShortName* sn, HE2CFShortName* sn_dim,
        	      HE2CFUniqName* un, HE2CFUniqName* un_dim)
{
    _das = &das;

    int32 myfileid;
    myfileid = Hopen(const_cast<char *>(filename.c_str()), DFACC_READ,0);

// cerr <<"inside hdfsp myfileid = "<<myfileid<<endl;
    //Hclose(myfileid);

    HDFSP::File *f;
    try {
        f = HDFSP::File::Read_Hybrid(filename.c_str(), myfileid);
    } 
    catch (HDFSP::Exception &e)
	{
            throw InternalErr(e.what());
	}
        
    const std::vector<HDFSP::SDField *>& spsds = f->getSD()->getFields();

    std::vector<HDFSP::SDField *>::const_iterator it_g;
    for(it_g = spsds.begin(); it_g != spsds.end(); it_g++){
        
        //<hyokyung 2010.06.17. 16:02:58>
        AttrTable *at = das.get_table((*it_g)->getNewName());
        if (!at)
            at = das.add_table((*it_g)->getNewName(), new AttrTable);

        // Some fields have "long_name" attributes,so we have to use this attribute rather than creating our own

        bool long_name_flag = false;

        for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {       

           if((*i)->getName() == "long_name") {
             long_name_flag = true;
             break;
           }
        }
        
        if(!long_name_flag) at->append_attr("long_name", "String", (*it_g)->getName());
        
        for(std::vector<HDFSP::Attribute *>::const_iterator i=(*it_g)->getAttributes().begin();i!=(*it_g)->getAttributes().end();i++) {

          // Handle string first.
          if((*i)->getType()==DFNT_UCHAR || (*i)->getType() == DFNT_CHAR){
                string tempstring2((*i)->getValue().begin(),(*i)->getValue().end());
                string tempfinalstr= string(tempstring2.c_str());
                at->append_attr((*i)->getName(), "String" , tempfinalstr);
          }
          else {
           for (unsigned int loc=0; loc < (*i)->getCount() ; loc++) {

                string print_rep = print_attr((*i)->getType(), loc, (void*) &((*i)->getValue()[0]));
                at->append_attr((*i)->getName(), print_type((*i)->getType()), print_rep);
            }
          }

       }
        
/// For the current additional SDS objects, we don't have to consider their latitude,etc.
#if 0
        // Overwrite units if fieldtype is latitude.
        if((*it_g)->getFieldType() == 1){
            at->del_attr("units");      // Override any existing units attribute.
            // at->append_attr("units", "String", "degrees_north");
            at->append_attr("units", "String",(*it_g)->getUnits());
        }
        // Overwrite units if fieldtype is longitude
        if((*it_g)->getFieldType() == 2){
            at->del_attr("units");      // Override any existing units attribute.
            // at->append_attr("units", "String", "degrees_east");
            at->append_attr("units", "String",(*it_g)->getUnits());
        }

        // Overwrite units if fieldtype is level
        if((*it_g)->getFieldType() == 4){
            at->del_attr("units");      // Override any existing units attribute.
            // at->append_attr("units", "String", "degrees_east");
            at->append_attr("units", "String",(*it_g)->getUnits());
        }

        // Overwrite coordinates if fieldtype is neither lat nor lon.
        if((*it_g)->getFieldType() == 0){
            at->del_attr("coordinates");      // Override any existing units attribute.
            at->append_attr("coordinates", "String", (*it_g)->getCoordinate());
        }
#endif
        
    }
/// Don't handle vdata for the time being. KY 2011-2-14
#if 0
    std::string VDdescname = "hdf4_vd_desc";
    std::string VDdescvalue = "This is an HDF4 Vdata.";
    std::string VDfieldprefix = "Vdata_field_";
    std::string VDattrprefix = "Vdata_attr_";
    std::string VDfieldattrprefix ="Vdata_field_attr_";

 if(f->getSPType() != CER_AVG && f->getSPType() != CER_ES4 && f->getSPType() !=CER_SRB && f->getSPType() != CER_ZAVG) {
    for(std::vector<HDFSP::VDATA *>::const_iterator i=f->getVDATAs().begin();
       i!=f->getVDATAs().end();i++) {

      // Add special vdata attributes
//      at->append_attr(VDdescname, "String" , VDdescvalue);
      bool emptyvddasflag = true;
      if(!((*i)->getAttributes().empty())) emptyvddasflag = false;
      if(((*i)->getTreatAsAttrFlag()))
        emptyvddasflag = false;
      else {
        for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
          if(!((*j)->getAttributes().empty())) {
            emptyvddasflag = false;
            break;
          }
       }
      }

     if(emptyvddasflag) continue;
     AttrTable *at = das.get_table((*i)->getNewName());
     if(!at)
        at = das.add_table((*i)->getNewName(),new AttrTable);
       at->append_attr(VDdescname, "String" , VDdescvalue);

      for(std::vector<HDFSP::Attribute *>::const_iterator it_va = (*i)->getAttributes().begin();it_va!=(*i)->getAttributes().end();it_va++) {

        if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

           string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
           string tempfinalstr= string(tempstring2.c_str());
           at->append_attr(VDattrprefix+(*it_va)->getName(), "String" , tempfinalstr);
          }
        else {
           for (unsigned int loc=0; loc < (*it_va)->getCount() ; loc++) {
                string print_rep = print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
// cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDattrprefix+(*it_va)->getName(), print_type((*it_va)->getType()), print_rep);
            }
          }


//cerr<<"Vdata attribute name "<<(*it_va)->getName() <<endl;
      }
     if(!((*i)->getTreatAsAttrFlag())){ 


      //NOTE: for vdata field, we assume that no special characters are found 
      for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {

//cerr<<"num_rec "<< (*j)->getNumRec()<<endl;
//cerr<<"field order " << (*j)->getFieldOrder()<<endl;
//cerr<<"size of field type " << DFKNTsize((*j)->getType())<<endl;
//cerr<<"size of the vector "<<(*j)->getValue().size() <<endl;

         // This vdata field will NOT be treated as attributes, only save the field attribute as the attribute

       for(std::vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

//cerr<<"Vdata field attribute name "<<(*it_va)->getName() <<endl;

            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

              string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
              string tempfinalstr= string(tempstring2.c_str());
              at->append_attr(VDfieldattrprefix+(*j)->getNewName()+"_"+(*it_va)->getName(), "String" , tempfinalstr);
            }
            else {
              for (unsigned int loc=0; loc < (*it_va)->getCount() ; loc++) {
                string print_rep = print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldattrprefix+(*j)->getNewName()+"_"+(*it_va)->getName(), print_type((*it_va)->getType()), print_rep);
              }
            }

//cerr<<"Vdata attribute name "<<(*it_va)->getName() <<endl;
          }
     }
    }

    else {

      for(std::vector<HDFSP::VDField *>::const_iterator j=(*i)->getFields().begin();j!=(*i)->getFields().end();j++) {
           
 
         if((*j)->getFieldOrder() == 1) {
           if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
             string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
             string tempfinalstr= string(tempstring2.c_str());
             at->append_attr(VDfieldprefix+(*j)->getName(), "String" , tempfinalstr);
           }
           else {
             for (unsigned int loc=0; loc < (*j)->getNumRec(); loc++) {
                string print_rep = print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldprefix+(*j)->getName(), print_type((*j)->getType()), print_rep);
            }
           }
        }
        else {//When field order is greater than 1,we want to print each record in group with single quote,'0 1 2','3 4 5', etc.

           if((*j)->getValue().size() != (DFKNTsize((*j)->getType())*((*j)->getFieldOrder())*((*j)->getNumRec()))){
//cerr<<"num_rec "<< (*j)->getNumRec()<<endl;
//cerr<<"field order " << (*j)->getFieldOrder()<<endl;
//cerr<<"size of field type " << DFKNTsize((*j)->getType())<<endl;
//cerr<<"size of the vector "<<(*j)->getValue().size() <<endl;
               throw InternalErr(__FILE__,__LINE__,"the vdata field size doesn't match the vector value");
            }

          if((*j)->getNumRec()==1){
             if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){
               string tempstring2((*j)->getValue().begin(),(*j)->getValue().end());
               string tempfinalstr= string(tempstring2.c_str());
               at->append_attr(VDfieldprefix+(*j)->getName(),"String",tempfinalstr);
             }
             else {
              for (unsigned int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                string print_rep = print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldprefix+(*j)->getName(), print_type((*j)->getType()), print_rep);
              }
             }

          }

         else {
          if((*j)->getType()==DFNT_UCHAR || (*j)->getType() == DFNT_CHAR){

            for(unsigned int tempcount = 0; tempcount < (*j)->getNumRec()*DFKNTsize((*j)->getType());tempcount ++) {
               std::vector<char>::const_iterator tempit;
               tempit = (*j)->getValue().begin()+tempcount*((*j)->getFieldOrder());
               string tempstring2(tempit,tempit+(*j)->getFieldOrder());
               string tempfinalstr= string(tempstring2.c_str());
               string tempoutstring = "'"+tempfinalstr+"'";
               at->append_attr(VDfieldprefix+(*j)->getName(),"String",tempoutstring);
            }
          }

          else {
            for(unsigned int tempcount = 0; tempcount < (*j)->getNumRec();tempcount ++) {
               at->append_attr(VDfieldprefix+(*j)->getName(),print_type((*j)->getType()),"'");
               for (unsigned int loc=0; loc < (*j)->getFieldOrder(); loc++) {
                string print_rep = print_attr((*j)->getType(), loc, (void*) &((*j)->getValue()[tempcount*((*j)->getFieldOrder())]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldprefix+(*j)->getName(), print_type((*j)->getType()), print_rep);
               }
               at->append_attr(VDfieldprefix+(*j)->getName(),print_type((*j)->getType()),"'");
            }
          }
        }
        }

//cerr<<"Vdata field name "<<(*j)->getName()<<endl;
         for(std::vector<HDFSP::Attribute *>::const_iterator it_va = (*j)->getAttributes().begin();it_va!=(*j)->getAttributes().end();it_va++) {

//cerr<<"Vdata field attribute name "<<(*it_va)->getName() <<endl;

            if((*it_va)->getType()==DFNT_UCHAR || (*it_va)->getType() == DFNT_CHAR){

              string tempstring2((*it_va)->getValue().begin(),(*it_va)->getValue().end());
              string tempfinalstr= string(tempstring2.c_str());
              at->append_attr(VDfieldattrprefix+(*it_va)->getName(), "String" , tempfinalstr);
            }
            else {
              for (unsigned int loc=0; loc < (*it_va)->getCount() ; loc++) {
                string print_rep = print_attr((*it_va)->getType(), loc, (void*) &((*it_va)->getValue()[0]));
//cerr<<"print_rep "<<print_rep <<endl;
                at->append_attr(VDfieldattrprefix+(*it_va)->getName(), print_type((*it_va)->getType()), print_rep);
              }
            }
//cerr<<"Vdata attribute name "<<(*it_va)->getName() <<endl;
          }


         }
       }

      }
  } 
        
#endif
  delete f;
  return true;
}

void read_dds_use_eos2lib(DDS & dds, const string & filename,
                  HE2CFNcML* ncml,
                  HE2CFShortName* sn, HE2CFShortName* sn_dim,
                  HE2CFUniqName* un, HE2CFUniqName* un_dim)
{
   
    
    if(read_dds_hdfeos2(dds, filename, ncml, sn, sn_dim, un, un_dim)){

      if(read_dds_hdfhybrid(dds,filename,ncml,sn,sn_dim,un,un_dim))
         return;
    }
//cerr<<"After HDFEOS2 DDS "<<endl;

   if(read_dds_hdfsp(dds, filename,ncml,sn,sn_dim,un,un_dim)){
//    else if(read_dds_hdfsp(dds, filename,ncml,sn,sn_dim,un,un_dim)){
        return;
    }
    
    read_dds(dds, filename);
   
}
void set_counters(HE2CFShortName* sn, HE2CFShortName* sn_dim,
                  HE2CFUniqName* un, HE2CFUniqName* un_dim)
{
    sn->set_counter();
    sn_dim->set_counter();
    un->set_counter();
    un_dim->set_counter();

}
void write_metadata(DAS& das, HE2CF& cf, const string& _meta)
{

    string meta = cf.get_metadata(_meta);

    if(meta == ""){
        return;                 // No _meta metadata exists.
    }
    
    DBG(cout << meta << endl);        

    AttrTable *at = das.get_table(_meta);
    if (!at)
        at = das.add_table(_meta, new AttrTable);
    // tell lexer to scan attribute string
    void *buf = hdfeos_string(meta.c_str());
    parser_arg arg(at);
    if (hdfeosparse(static_cast < void *>(&arg)) != 0
        || arg.status() == false){
        cerr << "HDF-EOS parse error " << _meta << endl;
    }
    hdfeos_delete_buffer(buf);
    
}

// Read configuration input XML file.
void read_conf_xml(DAS & das, const string & filename,
                   HE2CFNcML* ncml,
                   HE2CFShortName* sn, HE2CFShortName* sn_dim,
                   HE2CFUniqName* un, HE2CFUniqName* un_dim)
{
    string path =  ncml->get_current_working_directory(filename);
    DBG(cout << "Path = " << path << endl);        
    ncml->set_filename(path+"conf.xml");
    if(ncml->read(das)){
        DBG(cout << "Convention is " << ncml->get_convention() << endl);
    }

    char p = ncml->get_prefix();
    char v = ncml->get_valid_char();
    int size = ncml->get_short_name_size();
    
    if(size > 0){
        bool flag = true;
        string suffix = ncml->get_suffix();
        sn->set_short_name(flag, size, suffix);
        // Later we need to define <suffix_dim> tag in input file.
        sn_dim->set_short_name(flag, size, "D");
        // Later, we need to define <suffix_uniq> tag in input file.        
        un->set_uniq_name("U", true);
        un_dim->set_uniq_name("D", true);
    }
    else{
        un->set_uniq_name("U", false);
        un_dim->set_uniq_name("D", false);
        
    }
    sn->set_valid_char(p,v);    
    sn_dim->set_valid_char(p,v);
}

/**
 * Calculate MODIS product type.
 * @param gname group name
 */
int mtype;
void calculate_type(string gname) 
{
	// Group features of MODIS products.
	string modis_type1[] = {"L1B", "GEO", "BRDF", "0.05Deg", "Reflectance", "MOD17A2", "North"};
	string modis_type2   = "LST";
	string modis_type3[] = {"VI", "1km_2D", "L2g_2d"};

	if(gname=="mod05" || gname=="mod06" || gname=="mod07" || gname=="mod08" || gname=="atml2")
	{
		mtype = MODIS_TYPE1;
		return;
	}

	if(gname.find("MOD")==0 || gname.find("mod")==0)
	{
		size_t pos = gname.rfind(modis_type2);
		if(pos==gname.length()-modis_type2.length())
		{
			mtype = MODIS_TYPE2;
			return;
		}

		for(int k=0; k<sizeof(modis_type1)/sizeof(*modis_type1); k++)
		{
			pos = gname.rfind(modis_type1[k]);
			if(pos==gname.length()-modis_type1[k].length())
			{
				mtype = MODIS_TYPE1;
				return;
			}
		}

		for(int k=0; k<sizeof(modis_type3)/sizeof(*modis_type3); k++)
		{
			pos = gname.rfind(modis_type3[k]);
			if(pos==gname.length()-modis_type3[k].length())
				mtype = MODIS_TYPE3;
		}
	}
}


// Build DAS for HDFEOS2 files.
bool read_das_hdfeos2(DAS & das, const string & filename,
                      HE2CFNcML* ncml,
                      HE2CFShortName* sn, HE2CFShortName* sn_dim,
                      HE2CFUniqName* un, HE2CFUniqName* un_dim)
{
     _das = &das;

     // There are some HDF-EOS2 files(MERRA) that should be treated
    // exactly like HDF4 SDS files. We don't need to use HDF-EOS2 APIs to 
    // retrieve any information. In fact, treating them as HDF-EOS2 files
   // will cause confusions and wrong information, although may not be essential.
   // So far, we've found only MERRA data that have this problem.
   // A quick fix is to check if the file name contains MERRA. KY 2011-3-4 
//   cerr<<"basename " <<basename(filename) <<endl;
     // Find MERRA data, return false, use HDF4 SDS code.
     if((basename(filename)).compare(0,5,"MERRA")==0) {
        return false;
     }


    HDFEOS2::File *f;
    
    try {

        f= HDFEOS2::File::Read(filename.c_str());
    } 
    catch (HDFEOS2::Exception &e){

	if (!e.getFileType()){
              return false;
           }
        else
           {
              throw InternalErr(e.what());
           }
    }

   try {
	f->Prepare(filename.c_str(),sn,sn_dim,un,un_dim);
   }

   catch (HDFEOS2:: Exception &e) {
         delete f;
         throw InternalErr(e.what());
   }

   HE2CF cf;
   cf.open(filename);
   cf.set_DAS(&das);

   // Calculate MODIS product type.
   mtype = OTHER_TYPE;
   for(int i=0; i<(int)f->getSwaths().size(); i++)
   {
	HDFEOS2::SwathDataset *swath = f->getSwaths()[i];
	calculate_type(swath->getName());
	if(mtype!=OTHER_TYPE)
		break;
   }
   for(int i=0; i<(int)f->getGrids().size(); i++)
   {
	HDFEOS2::GridDataset *grid = f->getGrids()[i];
	calculate_type(grid->getName());
	if(mtype!=OTHER_TYPE)
		break;
   }

    // A flag not to generate structMetadata for the MOD13C2 file.
    // MOD13C2's structMetadata has wrong values. It couldn't pass the parser. 
    // So we want to turn it off. KY 2010-8-10
    bool tempstrflag = false;

    // Loop through Grids and Swaths fields.
    for (int i = 0; i < (int) f->getGrids().size(); i++) {
        HDFEOS2::GridDataset*  grid = f->getGrids()[i];
        string gname = grid->getName();
        
        DBG(cout << "Grid Name: " <<  gname << endl);

        for (unsigned int j = 0; j < grid->getDataFields().size(); ++j) {
            // original name
            string fname = grid->getDataFields()[j]->getName();
            // sanitized name
            string newfname = grid->getDataFields()[j]->getNewName(); 
            DBG(cout << "Original field name: " <<  fname << endl);
            DBG(cout << "Corrected field name: " <<  newfname << endl);

            int dimlistsize = grid->getDataFields()[j]->getDimensions().size();
            DBG(cout << "the original dimlist size: "<< dimlistsize << endl);
            for(unsigned int k = 0; 
                k < grid->getDataFields()[j]->getDimensions().size();
                ++k) {
                string dimname = 
                    grid->getDataFields()[j]->getDimensions()[k]->getName();
                int dimsize = 
                    grid->getDataFields()[j]->getDimensions()[k]->getSize();
                DBG(cout << "Original dimension name: " <<  dimname << endl);
                DBG(cout << "Original dimension size: " <<  dimsize << endl);
            }

            dimlistsize = 
                grid->getDataFields()[j]->getCorrectedDimensions().size();
            DBG(cout << "the corrected dimlist size: "<< dimlistsize << endl);

            for(unsigned int k = 0; 
                k < grid->getDataFields()[j]->getCorrectedDimensions().size();
                ++k) {
                string dimname = 
                    grid
                    ->getDataFields()[j]
                    ->getCorrectedDimensions()[k]->getName();
                int dimsize = 
                    grid
                    ->getDataFields()[j]
                    ->getCorrectedDimensions()[k]->getSize();
                DBG(cout << "Corrected dimension name: " <<  dimname << endl);
                DBG(cout << "Corrected dimension size: " <<  dimsize << endl);
            }

            // whether coordinate variable or data variables
            int fieldtype = grid->getDataFields()[j]->getFieldType(); 
            // 0 means that the data field is NOT a coordinate variable.
            if (fieldtype == 0){
                // If you don't find any _FillValue through generic API.
                if(grid->getDataFields()[j]->haveAddedFillValue()) {
                    DBG(cout << "Has an added fill value." << endl);
                    float addedfillvalue = 
                        grid->getDataFields()[j]->getAddedFillValue();
                    int type = 
                        grid->getDataFields()[j]->getType();
                    DBG(cout <<"Added fill value = "<<addedfillvalue);
                    cf.write_attribute_FillValue(newfname, 
                                                 type, addedfillvalue);
                }
                string coordinate = grid->getDataFields()[j]->getCoordinate();
                DBG(cout <<"Coordinate attribute: " << coordinate <<endl);
                cf.write_attribute_coordinates(newfname, coordinate);
            }

            DBG(cout << "Original Field Name: " <<  fname << endl);
            DBG(cout << "Corrected Field Name: " <<  newfname << endl);
            // This will override _FillValue if it's defined on the field.
            cf.write_attribute(gname, fname, newfname, 
                               f->getGrids().size(), fieldtype);  
            // 1 is latitude.
            // 2 is longtitude.    
            // 3 is defined level.
            // 4 is an inserted natural number.
            // 5 is time.
            if(fieldtype > 0){

                // MOD13C2 is treated specially. 
                if(fieldtype == 1 && (grid->getDataFields()[j]->getSpecialLLFormat())==3)
                  tempstrflag = true;

                string tempunits = grid->getDataFields()[j]->getUnits();
                DBG(cout
                    << "fieldtype " << fieldtype 
                    << " units" << tempunits 
                    << endl);
                    cf.write_attribute_units(newfname, tempunits);
            }
	    //Rename attributes of MODIS products.
            AttrTable *at = das.get_table(newfname);
            if(mtype!=OTHER_TYPE && at!=NULL)
            {
                AttrTable::Attr_iter it = at->attr_begin();
                string  scale_factor_value="", add_offset_value="", fillvalue="", valid_range_value="";
                string scale_factor_type, add_offset_type, fillvalue_type, valid_range_type;
                while (it!=at->attr_end())
                {
                        if(at->get_name(it)=="scale_factor")
                        {
                                scale_factor_value = (*at->get_attr_vector(it)->begin());
                                scale_factor_type = at->get_type(it);
                        }
                        if(at->get_name(it)=="add_offset")
                        {
                                add_offset_value = (*at->get_attr_vector(it)->begin());
                                add_offset_type = at->get_type(it);
                        }
			if(at->get_name(it)=="_FillValue")
			{
				fillvalue =  (*at->get_attr_vector(it)->begin());
                        	fillvalue_type = at->get_type(it);
			}
			if(at->get_name(it)=="valid_range")
			{
				vector<string> *avalue = at->get_attr_vector(it);
				vector<string>::iterator ait = avalue->begin();
				while(ait!=avalue->end())
				{
					valid_range_value += *ait;
					ait++;	
					if(ait!=avalue->end())
						valid_range_value += ", ";
				}
				valid_range_type = at->get_type(it);
			}
			it++;
                }
		if(scale_factor_value.length()!=0 && add_offset_value.length()!=0)
		{
			if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) //Rename them.
                	{
                        	at->del_attr("scale_factor");
                        	at->del_attr("add_offset");
                        	at->append_attr("scale_factor_modis", scale_factor_type, scale_factor_value);
                        	at->append_attr("add_offset_modis", add_offset_type, add_offset_value);
                	}
		}
		if(change_data_type(newfname) && fillvalue.length()!=0 && fillvalue_type!="Float32" && fillvalue_type!="Float64") // Change attribute type.
		{
			at->del_attr("_FillValue");
			at->append_attr("_FillValue", "Float32", fillvalue);
		}
		if(change_data_type(newfname) && valid_range_value.length()!=0 && valid_range_type!="Float32" && valid_range_type!="Float64") //Change attribute type.
		{
			at->del_attr("valid_range");
			//at->append_attr("valid_range", "Float32", valid_range_value);
		}
            }

            ncml->set_variable_clear(fname);
            
        }
    }

    for (int i = 0; i < (int) f->getSwaths().size(); i++) {

        HDFEOS2::SwathDataset*  swath = f->getSwaths()[i];
        string gname = swath->getName();
        DBG(cout <<"Swath name: " << gname << endl);

        for (unsigned int j = 0; j < swath->getGeoFields().size(); ++j) {
            string fname = swath->getGeoFields()[j]->getName();
            string newfname = swath->getGeoFields()[j]->getNewName();
            DBG(cout << "Original Field name: " <<  fname << endl);
            DBG(cout << "Corrected Field name: " <<  newfname << endl);
            int dimlistsize = swath->getGeoFields()[j]->getDimensions().size();
            DBG(cout << "the original dimlist size: "<< dimlistsize << endl);
            for(unsigned int k = 0; 
                k < swath->getGeoFields()[j]->getDimensions().size();
                ++k) {
                string dimname = 
                    swath
                    ->getGeoFields()[j]->getDimensions()[k]->getName();
                int dimsize = 
                    swath
                    ->getGeoFields()[j]->getDimensions()[k]->getSize();
                DBG(cout << "Original dimension name: " <<  dimname << endl);
                DBG(cout << "Original dimension size: " <<  dimsize << endl);
            }

            dimlistsize = 
                swath->getGeoFields()[j]->getCorrectedDimensions().size();
            DBG(cout << "the corrected dimlist size: "<< dimlistsize << endl);

            for(unsigned int k = 0; 
                k < swath->getGeoFields()[j]->getCorrectedDimensions().size();
                ++k) {
                string dimname = 
                    swath
                    ->getGeoFields()[j]->getCorrectedDimensions()[k]->getName();
                int dimsize = 
                    swath
                    ->getGeoFields()[j]->getCorrectedDimensions()[k]->getSize();
                DBG(cout << "Corrected dimension name: " <<  dimname << endl);
                DBG(cout << "Corrected dimension size: " <<  dimsize << endl);
            }

            int fieldtype = swath->getGeoFields()[j]->getFieldType();
            if (fieldtype == 0){
                string coordinate = swath->getGeoFields()[j]->getCoordinate();
                DBG(cout <<"Coordinate attribute: " << coordinate <<endl);
                cf.write_attribute_coordinates(newfname, coordinate);
            }
            // 1 is latitude.
            // 2 is longitude.
            if(fieldtype >0){
                string tempunits = swath->getGeoFields()[j]->getUnits();
                DBG(cout 
                    << "fieldtype " << fieldtype 
                    << " units" << tempunits << endl);
                cf.write_attribute_units(newfname, tempunits);
                
            }
            DBG(cout << "Field Name: " <<  fname << endl);
            cf.write_attribute(gname, fname, newfname, 
                               f->getSwaths().size(), fieldtype); 
            ncml->set_variable_clear(fname);
        }

        for (unsigned int j = 0; j < swath->getDataFields().size(); ++j) {
            string fname = swath->getDataFields()[j]->getName();
            string newfname = swath->getDataFields()[j]->getNewName();
            DBG(cout << "Original Field Name: " <<  fname << endl);
            DBG(cout << "Corrected Field Name: " <<  newfname << endl);
            int dimlistsize = swath->getDataFields()[j]->getDimensions().size();
            DBG(cout << "the original dimlist size: "<< dimlistsize << endl);
            for(unsigned int k = 0; 
                k < swath->getDataFields()[j]->getDimensions().size();++k) {
                string dimname = 
                    swath->getDataFields()[j]->getDimensions()[k]->getName();
                int dimsize = 
                    swath->getDataFields()[j]->getDimensions()[k]->getSize();
                DBG(cout << "Original dimension Name: " <<  dimname << endl);
                DBG(cout << "Original dimension size: " <<  dimsize << endl);
            }

            dimlistsize = 
                swath->getDataFields()[j]->getCorrectedDimensions().size();
            DBG(cout << "the corrected dimlist size: " << dimlistsize << endl);

            for(unsigned int k = 0; 
                k < swath->getDataFields()[j]->getCorrectedDimensions().size();
                ++k) {
                string dimname = 
                    swath
                    ->getDataFields()[j]
                    ->getCorrectedDimensions()[k]->getName();
                int dimsize = 
                    swath
                    ->getDataFields()[j]
                    ->getCorrectedDimensions()[k]->getSize();
                DBG(cout << "Corrected dimension name: " << dimname << endl);
                DBG(cout << "Corrected  dimension size: " << dimsize << endl);
            }

            int fieldtype = swath->getDataFields()[j]->getFieldType();
            if (fieldtype == 0){
                string coordinate = swath->getDataFields()[j]->getCoordinate();
                DBG(cout <<"Coordinate attribute: " << coordinate <<endl);
                cf.write_attribute_coordinates(newfname, coordinate);
            }

            // For Swath, coordinate variables are only under the geolocation 
            // group.
            //We find inside many MODIS files, the third dimension is put
            // under the "data field" group. So release this for now and see
            // what we obtain. KY 2010-6-21 
            if(fieldtype > 0) {

                string tempunits = swath->getDataFields()[j]->getUnits();
                DBG(cout 
                    << "fieldtype " << fieldtype 
                    << " units" << tempunits << endl);
                cf.write_attribute_units(newfname, tempunits);
                
            }
#if 0
            if(fieldtype != 0) 
             throw InternalErr(__FILE__, __LINE__, 
                  "Coordinate variables for Swath should be under geolocation group");
#endif
            if(swath->getDataFields()[j]->haveAddedFillValue()){
                float addedfillvalue = 
                    swath->getDataFields()[j]->getAddedFillValue();
                int type = 
                    swath->getDataFields()[j]->getType();
                DBG(cout <<"Added fill value = "<<addedfillvalue);
                cf.write_attribute_FillValue(newfname, type, addedfillvalue);

            }
            DBG(cout << "Field Name: " <<  fname << endl);
            cf.write_attribute(gname, fname, newfname, 
                               f->getSwaths().size(), fieldtype);
            ncml->set_variable_clear(fname);

	    //Rename attributes of MODIS products.
	    AttrTable *at = das.get_table(newfname);
	    if(mtype!=OTHER_TYPE && at!=NULL) 
	    {
		AttrTable::Attr_iter it = at->attr_begin();
		string scale_factor_value="", add_offset_value="", fillvalue="", valid_range_value="";
		string scale_factor_type, add_offset_type, fillvalue_type, valid_range_type;
		while (it!=at->attr_end()) 
		{
			if(at->get_name(it)=="scale_factor")
			{
				scale_factor_value = (*at->get_attr_vector(it)->begin());
				scale_factor_type = at->get_type(it);
			}
			if(at->get_name(it)=="add_offset")
			{
				add_offset_value = (*at->get_attr_vector(it)->begin());
				add_offset_type = at->get_type(it);
			}
			if(at->get_name(it)=="_FillValue")
			{
                                fillvalue =  (*at->get_attr_vector(it)->begin());
                        	fillvalue_type = at->get_type(it);
			}
			if(at->get_name(it)=="valid_range")
                        {
                                vector<string> *avalue = at->get_attr_vector(it);
                                vector<string>::iterator ait = avalue->begin();
                                while(ait!=avalue->end())
                                {
                                        valid_range_value += *ait;
                                        ait++;
                                        if(ait!=avalue->end())
                                                valid_range_value += ", ";
                                }
				valid_range_type = at->get_type(it);
        }

			it++;
		}
		if(scale_factor_value.length()!=0 && add_offset_value.length()!=0)
                {
			if(!(atof(scale_factor_value.c_str())==1 && atof(add_offset_value.c_str())==0)) //Rename them.
			{
				at->del_attr("scale_factor");
				at->del_attr("add_offset");
				at->append_attr("scale_factor_modis", scale_factor_type, scale_factor_value);
				at->append_attr("add_offset_modis", add_offset_type, add_offset_value);
			}
		}
		if(fillvalue.length()!=0 && fillvalue_type!="Float32" && fillvalue_type!="Float64") // Change attribute type.
                {
                        at->del_attr("_FillValue");
			//--------
			int32 sdfileid;
        		sdfileid = SDstart(const_cast < char *>(filename.c_str ()), DFACC_READ);
        		int32 sdsindex, sdsid;
        		sdsindex = SDnametoindex(sdfileid, fname.c_str());
        		sdsid = SDselect(sdfileid, sdsindex);

        		char attrname[H4_MAX_NC_NAME + 1];
        		int32 attrtype, attrcount, attrindex;
        		vector<char> attrbuf;
        		float _fillvalue;
			attrindex = SDfindattr(sdsid, "_FillValue");
			intn ret;
               	 	ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
                	if (ret==FAIL)
                	{
                        	ostringstream eherr;
                        	eherr << "Attribute '_FillValue' in " << fname.c_str () << " cannot be obtained.";
                        	throw InternalErr (__FILE__, __LINE__, eherr.str ());
                	}
                	attrbuf.clear();
                	attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                	ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
                	if (ret==FAIL)
                	{
                        	ostringstream eherr;
                        	eherr << "Attribute '_FillValue' in " << fname.c_str () << " cannot be obtained.";
                        	throw InternalErr (__FILE__, __LINE__, eherr.str ());
                	}
			switch(attrtype)
                	{
#define GET_FILLVALUE_ATTR_VALUE(TYPE, CAST) \
        case DFNT_##TYPE: \
        { \
                CAST tmpvalue = *(CAST*)&attrbuf[0]; \
                _fillvalue = (float)tmpvalue; \
        } \
        break;
                        GET_FILLVALUE_ATTR_VALUE(INT8, int8);
                        GET_FILLVALUE_ATTR_VALUE(INT16, int16);
                        GET_FILLVALUE_ATTR_VALUE(INT32, int32);
                        GET_FILLVALUE_ATTR_VALUE(UINT8, uint8);
                        GET_FILLVALUE_ATTR_VALUE(UINT16, uint16);
                        GET_FILLVALUE_ATTR_VALUE(UINT32, uint32);
                	};
#undef GET_FILLVALUE_ATTR_VALUE
			SDend(sdfileid);
			std::ostringstream ss;
			ss << _fillvalue;
			fillvalue = ss.str();
			//--------
                        at->append_attr("_FillValue", "Float32", fillvalue);
                }
                if(valid_range_value.length()!=0 && valid_range_type!="Float32" && valid_range_type!="Float64") // Change attribute type.
                {
                        at->del_attr("valid_range");
                        //at->append_attr("valid_range", "Float32", valid_range_value);
                }
	    }
        }
    }
    
    write_metadata(das, cf, "CoreMetadata");
    write_metadata(das, cf, "coremetadata");
// This cause a problem for a MOD13C2 file, So turn it off temporarily. KY 2010-6-29
   if(!tempstrflag)
    write_metadata(das, cf, "StructMetadata");

    cf.close();
    delete f;
    
    if(ncml->_variables.size() > 0){
        ostringstream error;
        error << "Input file has invalid variable names:";
        for(unsigned int i =0; i < ncml->_variables.size(); i++){
            error << ncml->_variables.at(i) << " ";
        }
        throw InternalErr(__FILE__, __LINE__, error.str());
    }
    return true;

}    


//The wrapper of building HDF-EOS2 and special HDF4 files. 
void read_das_use_eos2lib(DAS & das, const string & filename,
                  HE2CFNcML* ncml,
                  HE2CFShortName* sn, HE2CFShortName* sn_dim,
                  HE2CFUniqName* un, HE2CFUniqName* un_dim)
{

    if(read_das_hdfeos2(das, filename, ncml, sn, sn_dim, un, un_dim)){
        return;
    }

    else if(read_das_hdfsp(das, filename,ncml,sn,sn_dim,un,un_dim)){
        return;
    }
    read_das(das, filename);
}



#endif // #ifdef USE_HDFEOS2_LIB


void read_dds(DDS & dds, const string & filename)
{
	// generate DDS, DAS
	DAS das;
	dds.set_dataset_name(basename(filename));
	build_descriptions(dds, das, filename);

	if (!dds.check_semantics()) {       // DDS didn't get built right
		// dds.print(cerr);
		THROW(dhdferr_ddssem);
	}
	return;
}

void read_das(DAS & das, const string & filename)
{
    // generate DDS, DAS
    DDS dds(NULL);
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

    accum_attr(const string & named):d_named(named) {
    }

    hdf_genvec & operator() (hdf_genvec & accum, const hdf_attr & attr) {
        // Assume that all fields with the same base name should be combined,
        // and assume that they are in order.
        DBG(cout << "attr.name: " << attr.name << endl);
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

    is_named(const string & named):d_named(named) {
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
                (*BESLog::TheLog()) << "unknown tag: " << tag << " ref: " << ref << endl;
            	// TODO: Make this an exception? jhrg 8/19/11
                // cerr << "unknown tag: " << tag << " ref: " << ref << endl;
                break;
            }	// switch (tag) 
        } //     for (uint32 i = 0; i < vg->tags.size(); i++) 
    } //   for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) 
    // Build DDS for all toplevel vgroups
    BaseType *pbt = 0;
    for (VGI v = vgmap.begin(); v != vgmap.end(); ++v) {
        if (!v->second.toplevel)
            continue;           // skip over non-toplevel vgroups
        pbt = NewStructureFromVgroup(v->second.vgroup,
                                     vgmap, sdmap, vdmap,
                                     grmap, filename);
        if (pbt != 0) {
            dds.add_var(pbt);
            delete pbt;
        }                        
            
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
    if (atp == 0) {
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
// TESTING BES DEbug
BESDEBUG("h4","Testing Debug message "<<endl);

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
                if (hdfeosparse(static_cast < void *>(&arg)) != 0)
                	throw Error("HDF-EOS parse error while processing a " + container_name + " HDFEOS attribute.");

                if (arg.status() == false) {
#if 0
                	cerr << "HDF-EOS parse error while processing a "
                    	<< container_name << " HDFEOS attribute. (2)" << endl
                    	<< arg.error()->get_error_message() << endl;
#endif
                	BESDEBUG("h4", "HDF-EOS parse error while processing a "
                		<< container_name << " HDFEOS attribute. (2)" << endl
                		<< arg.error()->get_error_message() << endl);
                }

                hdfeos_delete_buffer(buf);
            }
            else {
                if (attrtype == "String")
#ifdef ATTR_STRING_QUOTE_FIX
                    attv[j] = escattr(attv[j]);
#else
                attv[j] = "\"" + escattr(attv[j]) + "\"";
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
    if (atp == 0) {
        atp = new AttrTable;
        atp = das.add_table(varname, atp);
    }
    // add the annotations to the DAS
    string an;
    for (int i = 0; i < (int) anv.size(); ++i) {        // for each annotation
#ifdef ATTR_STRING_QUOTE_FIX
        an = escattr(anv[i]);     // quote strings
#else
        an = "\"" + escattr(anv[i]) + "\"";     // quote strings
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

