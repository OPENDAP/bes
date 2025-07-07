/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values.
//  Authors:   Kent Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB
#include "config.h"
#include "config_hdf.h"

#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include <libdap/InternalErr.h>
#include <BESDebug.h>
#include <BESLog.h>

#include "HDFCFUtil.h"
#include "HDFEOS2Array_RealField.h"
#include "dodsutil.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;

#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2Array_RealField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2_Array_RealField read "<<endl);
    if (length() == 0)
        return true;

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = 0;
    nelms = format_constraint (offset.data(), step.data(), count.data());

    // Just declare offset,count and step in the int32 type.
    vector<int32>offset32;
    offset32.resize(rank);
    vector<int32>count32;
    count32.resize(rank);
    vector<int32>step32;
    step32.resize(rank);

    // Just obtain the offset,count and step in the datatype of int32.
    for (int i = 0; i < rank; i++) {
        offset32[i] = offset[i];
        count32[i] = count[i];
        step32[i] = step[i];
    }

    // Define function pointers to handle both grid and swath
    int32 (*openfunc) (char *, intn);
    intn (*closefunc) (int32);
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);

    string datasetname;
    if (swathname == "") {
        openfunc = GDopen;
        closefunc = GDclose;
        attachfunc = GDattach;
        detachfunc = GDdetach;
        fieldinfofunc = GDfieldinfo;
        datasetname = gridname;
    }
    else if (gridname == "") {
        openfunc = SWopen;
        closefunc = SWclose;
        attachfunc = SWattach;
        detachfunc = SWdetach;
        fieldinfofunc = SWfieldinfo;
        datasetname = swathname;
    }
    else 
        throw BESInternalError("It should be either grid or swath.",__FILE__, __LINE__);

    // Note gfid and gridid represent either swath or grid.
    int32 gfid = 0;
    int32 gridid = 0;

    if (true == isgeofile || false == check_pass_fileid_key) {

        // Obtain the EOS object ID(either grid or swath)
        gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (gfid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
    else 
        gfid = gsfd;

    // Attach the EOS object ID
    gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));
    if (gridid < 0) {
        close_fileid(gfid,-1);
        string msg = "Grid/Swath " + datasetname + " cannot be attached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    bool is_modis_l1b = false;
    if ("MODIS_SWATH_Type_L1B" == swathname)
        is_modis_l1b = true;

    bool is_modis_vip = false;
    if ("VIP_CMG_GRID" == gridname)
        is_modis_vip = true;

    bool field_is_vdata = false;

    // HDF-EOS2 swath maps 1-D field as vdata. So we need to check if this field is vdata or SDS.
    // Essentially we only call SDS attribute routines to retrieve MODIS scale,offset and 
    // fillvalue attributes since we don't
    // find 1-D MODIS field has scale,offset and fillvalue attributes. We may need to visit 
    // this again in the future to see if we also need to support the handling of 
    // scale,offset,fillvalue via vdata routines. KY 2013-07-15
    if (""==gridname) {

        int32 tmp_rank = 0;
        char tmp_dimlist[1024];
        int32 tmp_dims[rank];
        int32 field_dtype = 0;
        intn r = 0;

        r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
        if (r != 0) {
            detachfunc(gridid);
            close_fileid(gfid,-1);
            string msg = "Field " +  fieldname + " information cannot be obtained.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        if (1 == tmp_rank) 
            field_is_vdata = true;
    }


    bool has_Key_attr = false;

    if (false == field_is_vdata) {

        // Obtain attribute values.
        int32 sdfileid = -1;

        if (true == isgeofile || false == check_pass_fileid_key)  {

            sdfileid = SDstart(filename.c_str (), DFACC_READ);

            if (FAIL == sdfileid) {
                detachfunc(gridid);
                close_fileid(gfid,-1);
                string msg = "Cannot Start the SD interface for the file " + filename + ".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
        }
        else 
            sdfileid = sdfd;

        int32 sdsindex = -1;
        int32 sdsid = -1;
        sdsindex = SDnametoindex(sdfileid, fieldname.c_str());
        if (FAIL == sdsindex) {
            detachfunc(gridid);
            close_fileid(gfid,sdfileid);
            string msg = "Cannot obtain the index of " + fieldname + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        sdsid = SDselect(sdfileid, sdsindex);
        if (FAIL == sdsid) {
            detachfunc(gridid);
            close_fileid(gfid,sdfileid);
            string msg = "Cannot obtain the SDS ID  of " + fieldname + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
	
        // Here we cannot check if SDfindattr fails since even SDfindattr fails it doesn't mean
        // errors happen. If no such attribute can be found, SDfindattr still returns FAIL.
        // The correct way is to use SDgetinfo and SDattrinfo to check if attributes 
        // "radiance_scales" etc exist.
        // For the time being, I won't do this, due to the performance reason and code simplicity and also the
        // very small chance of real FAIL for SDfindattr.
        if (SDfindattr(sdsid, "Key")!=FAIL) 
            has_Key_attr = true;

        // Close the interfaces
        SDendaccess(sdsid);
        if (true == isgeofile || false == check_pass_fileid_key)
            SDend(sdfileid);
    }

    // USE a try-catch block to release the resources.
    try {
        if ((false == is_modis_l1b) && (false == is_modis_vip)
           &&(false == has_Key_attr) && (true == HDF4RequestHandler::get_disable_scaleoffset_comp()))
            write_dap_data_disable_scale_comp(gridid,nelms,offset32.data(),count32.data(),step32.data());
        else 
            write_dap_data_scale_comp(gridid,nelms,offset32,count32,step32);
    }
    catch(...) {
        detachfunc(gridid);
        close_fileid(gfid,-1);
        throw;
    }

    int32 r = -1;
    r = detachfunc (gridid);
    if (r != 0) {
        close_fileid(gfid,-1);
        string msg = "Grid/Swath " + datasetname + " cannot be detached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    if (true == isgeofile || false == check_pass_fileid_key) {
        r = closefunc (gfid);
        if (r != 0) {
            string msg = "Grid/Swath " + filename + " cannot be closed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }

    return false;
}

int 
HDFEOS2Array_RealField::write_dap_data_scale_comp(int32 gridid, 
                                                  int nelms, 
                                                  vector<int32>& offset32,
                                                  vector<int32>& count32,
                                                  vector<int32>& step32) {
    

    BESDEBUG("h4",
             "coming to HDFEOS2Array_RealField write_dap_data_scale_comp "
             <<endl);

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // Define function pointers to handle both grid and swath
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);


    if (swathname == "") {
        fieldinfofunc = GDfieldinfo;
        readfieldfunc = GDreadfield;
    }
    else if (gridname == "") {
        fieldinfofunc = SWfieldinfo;
        readfieldfunc = SWreadfield;
    }
    else 
        throw BESInternalError ("It should be either grid or swath.",__FILE__, __LINE__);

    // tmp_rank and tmp_dimlist are two dummy variables that are only used when calling fieldinfo.
    int32 tmp_rank = 0;
    char  tmp_dimlist[1024];

    // field dimension sizes
    int32 tmp_dims[rank];

    // field data type
    int32 field_dtype = 0;

    // returned value of HDF4 and HDF-EOS2 APIs
    intn  r = 0;

    // Obtain the field info. We mainly need the datatype information 
    // to allocate the buffer to store the data
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        string msg = "Field " + fieldname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // The following chunk of code until switch(field_dtype) handles MODIS level 1B, 
    // MOD29E1D Key and VIP products. The reason to keep the code this way is due to 
    // use of RECALCULATE macro. It is too much work to change it now. KY 2013-12-17
    // MODIS level 1B reflectance and radiance fields have scale/offset arrays rather 
    // than one scale/offset value.
    // So we need to handle these fields specially.
    float *reflectance_offsets =nullptr;
    float *reflectance_scales  =nullptr;
    float *radiance_offsets    =nullptr;
    float *radiance_scales     =nullptr;

    // Attribute datatype, reused for several attributes
    int32 attr_dtype = 0; 

    // Number of elements for an attribute, reused 
    int32 temp_attrcount  = 0;

    // Number of elements in an attribute
    int32 num_eles_of_an_attr = 0; 

    // Attribute(radiance_scales, reflectance_scales) index 
    int32 cf_modl1b_rr_attrindex  = 0; 

    // Attribute (radiance_offsets) index
    int32 cf_modl1b_rr_attrindex2 = 0;

    // Attribute valid_range index
    int32 cf_vr_attrindex = 0;

    // Attribute fill value index
    int32 cf_fv_attrindex = 0;

    // Scale factor attribute index
    int32 scale_factor_attr_index = 0;

    // Add offset attribute index
    int32 add_offset_attr_index = 0;

    // Initialize scale
    float scale = 1;

    // Intialize field_offset
    float field_offset = 0;

    // Initialize fillvalue
    float fillvalue = 0;

    // Initialize the original valid_min
    float orig_valid_min = 0;

    // Initialize the original valid_max
    float orig_valid_max = 0;

    // Some NSIDC products use the "Key" attribute to identify 
    // the discrete valid values(land, cloud etc).
    // Since the valid_range attribute in these products may treat values 
    // identified by the Key attribute as invalid,
    // we need to handle them in a special way.So set a flag here.
    bool  has_Key_attr = false;

    int32 sdfileid = -1;
    if (sotype!=SOType::DEFAULT_CF_EQU) {

        bool field_is_vdata = false;

        // HDF-EOS2 swath maps 1-D field as vdata. So we need to check 
        // if this field is vdata or SDS.
        // Essentially we only call SDS attribute routines to retrieve MODIS scale,
        // offset and fillvalue 
        // attributes since we don't find 1-D MODIS field has scale,offset and 
        // fillvalue attributes. 
        // We may need to visit this again in the future to see
        // if we also need to support the handling of scale,offset,fillvalue via 
        // vdata routines. KY 2013-07-15
        if (""==gridname) {

            r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
            if (r != 0) {
                string msg = "Field " + fieldname + " information cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            if (1 == tmp_rank) 
                field_is_vdata = true;
        }

        // For swath, we don't see any MODIS 1-D fields that have scale,offset 
        // and fill value attributes that need to be changed.
        // So now we don't need to handle the vdata handling. 
        // Another reason is the possible change of the implementation
        // of the SDS attribute handlings. That may be too costly. 
        // KY 2012-07-31

        if ( false == field_is_vdata) {

            char attrname[H4_MAX_NC_NAME + 1];
            vector<char>  attrbuf;

            // Obtain attribute values.
            if (false == isgeofile || false == check_pass_fileid_key) 
                sdfileid = sdfd;
            else {
                sdfileid = SDstart(filename.c_str (), DFACC_READ);
                if (FAIL == sdfileid) {
                    string msg = "Cannot Start the SD interface for the file "  + filename +".";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
            }

	    int32 sdsindex = -1;
            int32 sdsid;
	    sdsindex = SDnametoindex(sdfileid, fieldname.c_str());
            if (FAIL == sdsindex) {
                if (true == isgeofile || false == check_pass_fileid_key) 
                    SDend(sdfileid);
                string msg = "Cannot obtain the index of " + fieldname;
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            sdsid = SDselect(sdfileid, sdsindex);
            if (FAIL == sdsid) {
                if (true == isgeofile || false == check_pass_fileid_key)
                    SDend(sdfileid);
                string msg = "Cannot obtain the SDS ID  of " + fieldname;
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
	
#if 0
            char attrname[H4_MAX_NC_NAME + 1];
            vector<char> attrbuf, attrbuf2;

            // Here we cannot check if SDfindattr fails or not since even SDfindattr fails it doesn't mean
            // errors happen. If no such attribute can be found, SDfindattr still returns FAIL.
            // The correct way is to use SDgetinfo and SDattrinfo to check if attributes "radiance_scales" etc exist.
            // For the time being, I won't do this, due to the performance reason and code simplity and also the
            // very small chance of real FAIL for SDfindattr.
            cf_general_attrindex = SDfindattr(sdsid, "radiance_scales");
            cf_general_attrindex2 = SDfindattr(sdsid, "radiance_offsets");

            // Obtain the values of radiance_scales and radiance_offsets if they are available.
            if (cf_general_attrindex!=FAIL && cf_general_attrindex2!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, cf_general_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                        SDend(sdfileid);
                    string msg = Attribute 'radiance_scales' in " + fieldname  + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex, (VOIDP)attrbuf.data());
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                        SDend(sdfileid);
                    string msg = "Attribute 'radiance_scales' in "+ fieldname +  " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                ret = SDattrinfo(sdsid, cf_general_attrindex2, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                        SDend(sdfileid);
                    string msg = "Information of attribute 'radiance_offsets' in " + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex2, (VOIDP)attrbuf2.data());
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                        SDend(sdfileid);
                    string msg = "Attribute 'radiance_offsets' in " + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                // The following macro will obtain radiance_scales and radiance_offsets.
                // Although the code is compact, it may not be easy to follow. The similar macro can also be found later.
		switch(attr_dtype)
                {
#define GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)attrbuf.data(); \
            CAST *ptr2 = (CAST*)attrbuf2.data(); \
            radiance_scales = new float[temp_attrcount]; \
            radiance_offsets = new float[temp_attrcount]; \
            for(int l=0; l<temp_attrcount; l++) \
            { \
                radiance_scales[l] = ptr[l]; \
                radiance_offsets[l] = ptr2[l]; \
            } \
        } \
        break;
                    GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT32, float);
                    GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT64, double);
                }
#undef GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES
                num_eles_of_an_attr = temp_attrcount; // Store the count of attributes.
            }

            // Obtain attribute values of reflectance_scales and reflectance_offsets if they are available.
            cf_general_attrindex = SDfindattr(sdsid, "reflectance_scales");
            cf_general_attrindex2 = SDfindattr(sdsid, "reflectance_offsets");
            if (cf_general_attrindex!=FAIL && cf_general_attrindex2!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, cf_general_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                       SDend(sdfileid);
                    string msg = "Information of attribute 'reflectance_scales' in " + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex, (VOIDP)attrbuf.data());
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                        SDend(sdfileid);
                    string msg = "Attribute 'reflectance_scales' in " + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
		
                ret = SDattrinfo(sdsid, cf_general_attrindex2, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                       SDend(sdfileid);
                    string msg = "Information of attribute 'reflectance_offsets' in " + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex2, (VOIDP)attrbuf2.data());
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile)
                        SDend(sdfileid);
                    string msg = "Attribute 'reflectance_offsets' in " + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
		switch(attr_dtype)
                {
#define GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)attrbuf.data(); \
            CAST *ptr2 = (CAST*)attrbuf2.data(); \
            reflectance_scales = new float[temp_attrcount]; \
            reflectance_offsets = new float[temp_attrcount]; \
            for(int l=0; l<temp_attrcount; l++) \
            { \
                reflectance_scales[l] = ptr[l]; \
                reflectance_offsets[l] = ptr2[l]; \
            } \
        } \
        break;
                    GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT32, float);
                    GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT64, double);
                }
#undef GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES
                num_eles_of_an_attr = temp_attrcount; // Store the count of attributes. 
            }

#endif
            // Obtain the value of attribute scale_factor.
            scale_factor_attr_index = SDfindattr(sdsid, "scale_factor");
            if (scale_factor_attr_index!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, scale_factor_attr_index, attrname, 
                                 &attr_dtype, &temp_attrcount);
                if (ret==FAIL) 
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'scale_factor' in " + fieldname; 
                    msg += " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, scale_factor_attr_index, (VOIDP)attrbuf.data());
                if (ret==FAIL) 
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'scale_factor' in ";
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
 
                switch(attr_dtype)
                {
#define GET_SCALE_FACTOR_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)attrbuf.data(); \
            scale = (float)tmpvalue; \
        } \
        break;
                    GET_SCALE_FACTOR_ATTR_VALUE(INT8, int8)
                    GET_SCALE_FACTOR_ATTR_VALUE(CHAR,int8)
                    GET_SCALE_FACTOR_ATTR_VALUE(UINT8, uint8)
                    GET_SCALE_FACTOR_ATTR_VALUE(UCHAR,uint8)
                    GET_SCALE_FACTOR_ATTR_VALUE(INT16, int16)
                    GET_SCALE_FACTOR_ATTR_VALUE(UINT16, uint16)
                    GET_SCALE_FACTOR_ATTR_VALUE(INT32, int32)
                    GET_SCALE_FACTOR_ATTR_VALUE(UINT32, uint32)
                    GET_SCALE_FACTOR_ATTR_VALUE(FLOAT32, float)
                    GET_SCALE_FACTOR_ATTR_VALUE(FLOAT64, double)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);

                    
                }
#undef GET_SCALE_FACTOR_ATTR_VALUE
            }

            // Obtain the value of attribute add_offset
            add_offset_attr_index = SDfindattr(sdsid, "add_offset");
            if (add_offset_attr_index!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, add_offset_attr_index, attrname, 
                                 &attr_dtype, &temp_attrcount);
                if (ret==FAIL) 
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'add_offset' in " + fieldname;
                    msg +=" cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, add_offset_attr_index, (VOIDP)attrbuf.data());
                if (ret==FAIL) 
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'add_offset' in " + fieldname;
                    msg +=" cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                switch(attr_dtype)
                {
#define GET_ADD_OFFSET_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)attrbuf.data(); \
            field_offset = (float)tmpvalue; \
        } \
        break;
                    GET_ADD_OFFSET_ATTR_VALUE(INT8, int8)
                    GET_ADD_OFFSET_ATTR_VALUE(CHAR,int8)
                    GET_ADD_OFFSET_ATTR_VALUE(UINT8, uint8)
                    GET_ADD_OFFSET_ATTR_VALUE(UCHAR,uint8)
                    GET_ADD_OFFSET_ATTR_VALUE(INT16, int16)
                    GET_ADD_OFFSET_ATTR_VALUE(UINT16, uint16)
                    GET_ADD_OFFSET_ATTR_VALUE(INT32, int32)
                    GET_ADD_OFFSET_ATTR_VALUE(UINT32, uint32)
                    GET_ADD_OFFSET_ATTR_VALUE(FLOAT32, float)
                    GET_ADD_OFFSET_ATTR_VALUE(FLOAT64, double)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);

                }
#undef GET_ADD_OFFSET_ATTR_VALUE
            }

            // Obtain the value of the attribute _FillValue
            cf_fv_attrindex = SDfindattr(sdsid, "_FillValue");
            if (cf_fv_attrindex!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, cf_fv_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute '_FillValue' in " + fieldname;
                    msg += " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_fv_attrindex, (VOIDP)attrbuf.data());
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute '_FillValue' in " + fieldname; 
                    msg +=" cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                switch(attr_dtype)
                {
#define GET_FILLVALUE_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)attrbuf.data(); \
            fillvalue = (float)tmpvalue; \
        } \
        break;
                    GET_FILLVALUE_ATTR_VALUE(INT8, int8)
                    GET_FILLVALUE_ATTR_VALUE(CHAR, int8)
                    GET_FILLVALUE_ATTR_VALUE(INT16, int16)
                    GET_FILLVALUE_ATTR_VALUE(INT32, int32)
                    GET_FILLVALUE_ATTR_VALUE(UINT8, uint8)
                    GET_FILLVALUE_ATTR_VALUE(UCHAR, uint8)
                    GET_FILLVALUE_ATTR_VALUE(UINT16, uint16)
                    GET_FILLVALUE_ATTR_VALUE(UINT32, uint32)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);

                }
#undef GET_FILLVALUE_ATTR_VALUE
            }

            // Retrieve valid_range,valid_range is normally represented as (valid_min,valid_max)
            // for non-CF scale and offset rules, the data is always float. So we only
            // need to change the data type to float.
            cf_vr_attrindex = SDfindattr(sdsid, "valid_range");
            if (cf_vr_attrindex!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, cf_vr_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Information of attribute '_FillValue' in " + fieldname;
                    msg += " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_vr_attrindex, (VOIDP)attrbuf.data());
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute '_FillValue' in " + fieldname;
                    msg += " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                string attrbuf_str(attrbuf.begin(),attrbuf.end());

                switch(attr_dtype) {
                
                    case DFNT_CHAR:
                    {                   
                        // We need to treat the attribute data as characters or string.
                        // So find the separator.
                        size_t found = attrbuf_str.find_first_of(",");
                        size_t found_from_end = attrbuf_str.find_last_of(",");
                        
                        if (string::npos == found){
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("Cannot find the separator ,",__FILE__,__LINE__);
                        }
                        if (found != found_from_end){
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("Only one separator , should be available.",__FILE__,__LINE__);
                        }

                        orig_valid_min = (float)(atof((attrbuf_str.substr(0,found)).c_str()));
                        orig_valid_max = (float)(atof((attrbuf_str.substr(found+1)).c_str()));
                    
                    }
                    break;
                    case DFNT_INT8:
                    {
                        // We find a special case that even valid_range is logically 
                        // interpreted as two elements,
                        // but the count of attribute elements is more than 2. The count 
                        // actually is the number of
                        // characters stored as the attribute value. So we need to find 
                        // the separator "," and then
                        // change the string before and after the separator into float numbers.
                        // 
                        if (temp_attrcount >2) {

                            size_t found = attrbuf_str.find_first_of(",");
                            size_t found_from_end = attrbuf_str.find_last_of(",");

                            if (string::npos == found){
                                SDendaccess(sdsid);
                                if (true == isgeofile || false == check_pass_fileid_key)
                                    SDend(sdfileid);
                                throw BESInternalError("Cannot find the separator ,",__FILE__,__LINE__);
                            }
                            if (found != found_from_end){
                                SDendaccess(sdsid);
                                if (true == isgeofile || false == check_pass_fileid_key)
                                    SDend(sdfileid);
                                throw BESInternalError("Only one separator , should be available.",__FILE__,__LINE__);
                            }

                            orig_valid_min = (float)(atof((attrbuf_str.substr(0,found)).c_str()));
                            orig_valid_max = (float)(atof((attrbuf_str.substr(found+1)).c_str()));

                        }
                        else if (2 == temp_attrcount) {
                            orig_valid_min = (float)attrbuf[0];
                            orig_valid_max = (float)attrbuf[1]; 
                        }
                        else {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be greater than 1.",__FILE__,__LINE__);
                        }

                    }
                    break;

                    case DFNT_UINT8:
                    case DFNT_UCHAR:
                    {
                        if (temp_attrcount != 2) {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT8 type.",__FILE__,__LINE__);
                        }

                        auto temp_valid_range = (unsigned char *)attrbuf.data();
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_INT16:
                    {
                        if (temp_attrcount != 2) {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_INT16 type.",__FILE__,__LINE__);
                        }

                        auto temp_valid_range = (short *)attrbuf.data();
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_UINT16:
                    {
                        if (temp_attrcount != 2) {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT16 type.",__FILE__,__LINE__);
                        }

                        auto temp_valid_range = (unsigned short *)attrbuf.data();
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;
 
                    case DFNT_INT32:
                    {
                        if (temp_attrcount != 2) {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_INT32 type.",__FILE__,__LINE__);
                        }

                        auto temp_valid_range = (int *)attrbuf.data();
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_UINT32:
                    {
                        if (temp_attrcount != 2) {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT32 type.",__FILE__,__LINE__);
                        }
                        auto temp_valid_range = (unsigned int *)attrbuf.data();
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_FLOAT32:
                    {
                        if (temp_attrcount != 2) {
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_FLOAT32 type.",__FILE__,__LINE__);
                        }
                        auto temp_valid_range = (float *)attrbuf.data();
                        orig_valid_min = temp_valid_range[0];
                        orig_valid_max = temp_valid_range[1];
                    }
                    break;

                    case DFNT_FLOAT64:
                    {
                        if (temp_attrcount != 2){
                            SDendaccess(sdsid);
                            if (true == isgeofile || false == check_pass_fileid_key)
                                SDend(sdfileid);
                            throw BESInternalError("The number of attribute count should be 2 for the DFNT_FLOAT64 type.",__FILE__,__LINE__);
                        }
                        auto temp_valid_range = (double *)attrbuf.data();

                        // Notice: this approach will lose precision and possibly overflow. 
                        // Fortunately it is not a problem for MODIS data.
                        // This part of code may not be called. 
                        // If it is called, mostly the value is within the floating-point range.
                        // KY 2013-01-29
                        orig_valid_min = (float)temp_valid_range[0];
                        orig_valid_max = (float)temp_valid_range[1];
                    }
                    break;
                    default: {
                        SDendaccess(sdsid);
                        if (true == isgeofile || false == check_pass_fileid_key)
                            SDend(sdfileid);
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);
                    }
                }
            }

            // Check if the data has the "Key" attribute. 
            // We found that some NSIDC MODIS data(MOD29) used "Key" to identify some special values.
            // To get the values that are within the range identified by the "Key", 
            // scale offset rules also need to be applied to those values
            // outside the original valid range. KY 2013-02-25
            int32 cf_mod_key_attrindex = SUCCEED;
            cf_mod_key_attrindex = SDfindattr(sdsid, "Key");
            if (cf_mod_key_attrindex !=FAIL) {
                has_Key_attr = true;
            }

            attrbuf.clear();
            vector<char> attrbuf2;

            // Here we cannot check if SDfindattr fails since even SDfindattr fails it doesn't mean
            // errors happen. If no such attribute can be found, SDfindattr still returns FAIL.
            // The correct way is to use SDgetinfo and SDattrinfo to check if attributes 
            // "radiance_scales" etc exist.
            // For the time being, I won't do this, due to the performance reason and code simplity 
            // and also the very small chance of real FAIL for SDfindattr.
            cf_modl1b_rr_attrindex = SDfindattr(sdsid, "radiance_scales");
            cf_modl1b_rr_attrindex2 = SDfindattr(sdsid, "radiance_offsets");

            // Obtain the values of radiance_scales and radiance_offsets if they are available.
            if (cf_modl1b_rr_attrindex!=FAIL && cf_modl1b_rr_attrindex2!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, cf_modl1b_rr_attrindex, attrname, 
                                 &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "The information of attribute 'radiance_scales' in " + fieldname;
                    msg +=" cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_modl1b_rr_attrindex, (VOIDP)attrbuf.data());
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'radiance_scales' in " + fieldname;
                    msg += "cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                ret = SDattrinfo(sdsid, cf_modl1b_rr_attrindex2, attrname, 
                                 &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "The information of attribute 'radiance_offsets' in ";
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_modl1b_rr_attrindex2, (VOIDP)attrbuf2.data());
                if (ret==FAIL)
                {
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "The attribute 'radiance_offsets' in ";
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                // The following macro will obtain radiance_scales and radiance_offsets.
                // Although the code is compact, it may not be easy to follow. 
                // The similar macro can also be found later.
		switch(attr_dtype)
                {
#define GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)attrbuf.data(); \
            CAST *ptr2 = (CAST*)attrbuf2.data(); \
            radiance_scales = new float[temp_attrcount]; \
            radiance_offsets = new float[temp_attrcount]; \
            for(int l=0; l<temp_attrcount; l++) \
            { \
                radiance_scales[l] = ptr[l]; \
                radiance_offsets[l] = ptr2[l]; \
            } \
        } \
        break;
                    GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT32, float)
                    GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT64, double)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);

                }
#undef GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES
                // Store the count of attributes.
                num_eles_of_an_attr = temp_attrcount; 
            }

            // Obtain attribute values of reflectance_scales 
            // and reflectance_offsets if they are available.
            cf_modl1b_rr_attrindex = SDfindattr(sdsid, "reflectance_scales");
            cf_modl1b_rr_attrindex2 = SDfindattr(sdsid, "reflectance_offsets");
            if (cf_modl1b_rr_attrindex!=FAIL && cf_modl1b_rr_attrindex2!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, cf_modl1b_rr_attrindex, attrname, 
                                 &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,
                                      radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                       SDend(sdfileid);
                    string msg = "The information of attribute 'reflectance_scales' in ";
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_modl1b_rr_attrindex, (VOIDP)attrbuf.data());
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,
                                      radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'reflectance_scales' in ";
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
		
                ret = SDattrinfo(sdsid, cf_modl1b_rr_attrindex2, attrname, 
                                 &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,
                                      radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                       SDend(sdfileid);
                    string msg = "The information of attribute 'reflectance_offsets' in "; 
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_modl1b_rr_attrindex2, (VOIDP)attrbuf2.data());
                if (ret==FAIL)
                {
                    release_mod1b_res(reflectance_scales,reflectance_offsets,
                                      radiance_scales,radiance_offsets);
                    SDendaccess(sdsid);
                    if (true == isgeofile || false == check_pass_fileid_key)
                        SDend(sdfileid);
                    string msg = "Attribute 'reflectance_offsets' in "; 
                    msg = msg + fieldname + " cannot be obtained.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
		switch(attr_dtype)
                {
#define GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)attrbuf.data(); \
            CAST *ptr2 = (CAST*)attrbuf2.data(); \
            reflectance_scales = new float[temp_attrcount]; \
            reflectance_offsets = new float[temp_attrcount]; \
            for(int l=0; l<temp_attrcount; l++) \
            { \
                reflectance_scales[l] = ptr[l]; \
                reflectance_offsets[l] = ptr2[l]; \
            } \
        } \
        break;
                    GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT32, float)
                    GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(FLOAT64, double)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);

                }
#undef GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES
                num_eles_of_an_attr = temp_attrcount; // Store the count of attributes. 
            }

            SDendaccess(sdsid);
 
            BESDEBUG("h4","scale is "<<scale <<endl);
            BESDEBUG("h4","offset is "<<field_offset <<endl);
            BESDEBUG("h4","fillvalue is "<<fillvalue <<endl);
        }
    }

    // According to our observations, it seems that MODIS products ALWAYS 
    // use the "scale" factor to make bigger values smaller.
    // So for MODIS_MUL_SCALE products, if the scale of some variable is greater than 1, 
    // it means that for this variable, the MODIS type for this variable may be MODIS_DIV_SCALE.
    // For the similar logic, we may need to change MODIS_DIV_SCALE to MODIS_MUL_SCALE 
    // and MODIS_EQ_SCALE to MODIS_DIV_SCALE.
    // We indeed find such a case. HDF-EOS2 Grid MODIS_Grid_1km_2D of MOD(or MYD)09GA is 
    // a MODIS_EQ_SCALE.
    // However,
    // the scale_factor of the variable Range_1 in the MOD09GA product is 25. 
    // According to our observation,
    // this variable should be MODIS_DIV_SCALE.We verify this is true according to 
    // MODIS 09 product document
    // http://modis-sr.ltdri.org/products/MOD09_UserGuide_v1_3.pdf.
    // Since this conclusion is based on our observation, we would like to add a BESlog to detect 
    // if we find
    // the similar cases so that we can verify with the corresponding product documents to see if 
    // this is true.
    // More updated information, 
    // We just verified with the MOD09 data producer, the scale_factor for Range_1 is 25 
    // but the equation is still multiplication instead of division.
    // So we have to make this as a special case and don't change the scale and offset settings
    // for Range_1 of MOD09 products.
    // KY 2014-01-13

    if (SOType::MODIS_EQ_SCALE == sotype || SOType::MODIS_MUL_SCALE == sotype) {
        if (scale > 1) {
            bool need_change_scale = true;           
            if (gridname!="") {
              
                string temp_filename;
                if (filename.find("#") != string::npos)
                    temp_filename =filename.substr(filename.find_last_of("#") + 1);
                 else
                    temp_filename = filename.substr(filename.find_last_of("/") +1);

                if ((temp_filename.size() >5) && ((temp_filename.compare(0,5,"MOD09") == 0)
                    ||(temp_filename.compare(0,5,"MYD09") == 0))) {
                    if ((fieldname.size() >5) && fieldname.compare(0,5,"Range") == 0)
                        need_change_scale = false;
                }
                // MOD16A2
                else if ((temp_filename.size() >7)&&
                        ((temp_filename.compare(0,7,"MOD16A2") == 0)|| (temp_filename.compare(0,7,"MYD16A2")==0)||
                         (temp_filename.compare(0,7,"MOD16A3") == 0)|| (temp_filename.compare(0,7,"MYD16A3")==0)))
                        need_change_scale = false;
                
 
            }
            if (true == need_change_scale) { 
                sotype = SOType::MODIS_DIV_SCALE;
                INFO_LOG( "The field " + fieldname + " scale factor is "
                 + std::to_string(scale) + " . But the original scale factor type is MODIS_MUL_SCALE"
                 + " or MODIS_EQ_SCALE.  Now change it to MODIS_DIV_SCALE.");
            }
        }
    }
    
    if (SOType::MODIS_DIV_SCALE == sotype) {
        if (scale < 1) {
            sotype = SOType::MODIS_MUL_SCALE;
            INFO_LOG("The field " + fieldname + " scale factor is "
                   + std::to_string(scale) + ". But the original scale factor type is MODIS_DIV_SCALE. "
                   + " Now change it to MODIS_MUL_SCALE.");
        }
    }
    
// We need to loop through all datatpes to allocate the memory buffer for the data.
// It is hard to add comments to the macro. We may need to change them to general 
// routines in the future.
// Some MODIS products use both valid_range(valid_min, valid_max) and fillvalues for data fields. 
// When do recalculating,
// I check fillvalue first, then check valid_min and valid_max if they are available. 
// The middle check is_special_value addresses the MODIS L1B special value. 
// Updated: just find that the RECALCULATE will be done only when the valid_range
// attribute is present(if cf_vr_attrindex!=FAIL). 
// This restriction is in theory not necessary, but for more MODIS data products,
// this restriction may be valid since valid_range pairs with scale/offset to identify
// the valid data values. KY 2014-02-19
//
#if 0
/*    if ((float)tmptr[l] != fillvalue ) \
//                    { \
//          f(false == HDFCFUtil::is_special_value(field_dtype,fillvalue,tmptr[l]))\
//                       { \
//                            if (orig_valid_min<tmpval[l] && orig_valid_max>tmpval[l] \
//                            if (sotype==MODIS_MUL_SCALE) \
//                                tmpval[l] = (tmptr[l]-field_offset)*scale; \
//                            else if (sotype==MODIS_EQ_SCALE) \
//                                tmpval[l] = tmptr[l]*scale + field_offset; \
//                            else if (sotype==MODIS_DIV_SCALE) \
//                                tmpval[l] = (tmptr[l]-field_offset)/scale; \
//                        } \
*/
#endif 
#define RECALCULATE(CAST, DODS_CAST, VAL) \
{ \
    bool change_data_value = false; \
    if (sotype!=SOType::DEFAULT_CF_EQU) \
    { \
        vector<float>tmpval; \
        tmpval.resize(nelms); \
        CAST tmptr = (CAST)VAL; \
        for(int l=0; l<nelms; l++) \
            tmpval[l] = (float)tmptr[l]; \
        bool special_case = false; \
        if (scale_factor_attr_index==FAIL) \
            if (num_eles_of_an_attr==1) \
                if ((radiance_scales!=nullptr) && (radiance_offsets!=nullptr)) \
                { \
                    scale = radiance_scales[0]; \
                    field_offset = radiance_offsets[0];\
                    special_case = true; \
                } \
        if (((scale_factor_attr_index!=FAIL) && !((scale==1) && (field_offset==0))) || special_case)  \
        { \
            float temp_scale = scale; \
            float temp_offset = field_offset; \
            if (sotype==SOType::MODIS_MUL_SCALE) \
                temp_offset = -1. *field_offset*temp_scale;\
            else if (sotype==SOType::MODIS_DIV_SCALE) \
            {\
                temp_scale = 1/scale; \
                temp_offset = -1. *field_offset*temp_scale;\
            }\
            for(int l=0; l<nelms; l++) \
            { \
                if (cf_vr_attrindex!=FAIL) \
                { \
                    if ((float)tmptr[l] != fillvalue ) \
                    { \
                        if (false == HDFCFUtil::is_special_value(field_dtype,fillvalue,tmptr[l]))\
                        { \
                            if (((orig_valid_min<=tmpval[l]) && (orig_valid_max>=tmpval[l])) || (true==has_Key_attr))\
                            { \
                                        tmpval[l] = tmptr[l]*temp_scale + temp_offset; \
                           } \
                        } \
                    } \
                } \
            } \
            change_data_value = true; \
            set_value((dods_float32 *)tmpval.data(), nelms); \
        } else 	if ((num_eles_of_an_attr>1) && (((radiance_scales!=nullptr) && (radiance_offsets!=nullptr)) || ((reflectance_scales!=nullptr) && (reflectance_offsets!=nullptr)))) \
        { \
            size_t dimindex=0; \
            if ( num_eles_of_an_attr!=tmp_dims[dimindex]) \
            { \
                release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets); \
                string msg = "The number of Z-Dimension scale attribute is not equal to the size of the first dimension in "; \
                msg = msg + fieldname + ". These two values must be equal."; \
                throw BESInternalError(msg,__FILE__,__LINE__); \
            } \
            size_t start_index, end_index; \
            size_t nr_elems = nelms/count32[dimindex]; \
            start_index = offset32[dimindex]; \
            end_index = start_index+step32[dimindex]*(count32[dimindex]-1); \
            size_t index = 0;\
            for(size_t k=start_index; k<=end_index; k+=step32[dimindex]) \
            { \
                float tmpscale = (fieldname.find("Emissive")!=string::npos)? radiance_scales[k]: reflectance_scales[k]; \
                float tmpoffset = (fieldname.find("Emissive")!=string::npos)? radiance_offsets[k]: reflectance_offsets[k]; \
                for(size_t l=0; l<nr_elems; l++) \
                { \
                    if (cf_vr_attrindex!=FAIL) \
                    { \
                        if (((float)tmptr[index])!=fillvalue) \
                        { \
                            if (false == HDFCFUtil::is_special_value(field_dtype,fillvalue,tmptr[index]))\
                            { \
                                if (sotype==SOType::MODIS_MUL_SCALE) \
                                    tmpval[index] = (tmptr[index]-tmpoffset)*tmpscale; \
                                else if (sotype==SOType::MODIS_EQ_SCALE) \
                                    tmpval[index] = tmptr[index]*tmpscale+tmpoffset; \
                                else if (sotype==SOType::MODIS_DIV_SCALE) \
                                    tmpval[index] = (tmptr[index]-tmpoffset)/tmpscale; \
                            } \
                        } \
                    } \
                    index++; \
                } \
            } \
            change_data_value = true; \
            set_value((dods_float32 *)tmpval.data(), nelms); \
        } \
    } \
    if (!change_data_value) \
    { \
        set_value ((DODS_CAST)VAL, nelms); \
    } \
}
    switch (field_dtype) {
        case DFNT_INT8:
        {

            vector<int8>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

#ifndef SIGNED_BYTE_TO_INT32
            RECALCULATE(int8*, dods_byte*, val.data());
#else

            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val[counter]);

            RECALCULATE(int32*, dods_int32*, newval.data())
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {

            vector<uint8>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            RECALCULATE(uint8*, dods_byte*, val.data())
        }
            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32.data(), step32.data(), count32.data(), val.data());

            if (r != 0) {

                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            RECALCULATE(int16*, dods_int16*, val.data())
        }
            break;
        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);

            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {
                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            RECALCULATE(uint16*, dods_uint16*, val.data())
        }
            break;
        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {

                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            RECALCULATE(int32*, dods_int32*, val.data())
        }
            break;
        case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {

                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            RECALCULATE(uint32*, dods_uint32*, val.data())
        }
            break;
        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {

                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            // Recalculate seems not necessary.
            RECALCULATE(float32*, dods_float32*, val.data())
            //set_value((dods_float32*)val.data(),nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32.data(), step32.data(), count32.data(), val.data());
            if (r != 0) {

                release_mod1b_res(reflectance_scales,reflectance_offsets,
                                  radiance_scales,radiance_offsets);
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value ((dods_float64 *) val.data(), nelms);
        }
            break;
        default:
            release_mod1b_res(reflectance_scales,reflectance_offsets,
                              radiance_scales,radiance_offsets);
            throw InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

    release_mod1b_res(reflectance_scales,reflectance_offsets,radiance_scales,radiance_offsets);

    // Somehow the macro RECALCULATE causes the interaction between gridid and sdfileid. SO
    // If I close the sdfileid earlier, gridid becomes invalid. So close the sdfileid now. KY 2014-10-24
    if (true == isgeofile || false == check_pass_fileid_key)
                SDend(sdfileid);
    //
    return false;
    
}


int
HDFEOS2Array_RealField::write_dap_data_disable_scale_comp(int32 gridid, 
                                                          int nelms, 
                                                          int32 *offset32,
                                                          int32*count32,
                                                          int32*step32) {


    BESDEBUG("h4",
             "Coming to HDFEOS2_Array_RealField: write_dap_data_disable_scale_comp"
             <<endl);

    // Define function pointers to handle both grid and swath
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);


    if (swathname == "") {
        fieldinfofunc = GDfieldinfo;
        readfieldfunc = GDreadfield;

    }
    else if (gridname == "") {
        fieldinfofunc = SWfieldinfo;
        readfieldfunc = SWreadfield;

    }
    else 
        throw InternalErr (__FILE__, __LINE__, "It should be either grid or swath.");


    // tmp_rank and tmp_dimlist are two dummy variables 
    // that are only used when calling fieldinfo.
    int32 tmp_rank = 0;
    char  tmp_dimlist[1024];

    // field dimension sizes
    int32 tmp_dims[rank];

    // field data type
    int32 field_dtype = 0;

    // returned value of HDF4 and HDF-EOS2 APIs
    intn  r = 0;

    // Obtain the field info. We mainly need the datatype information 
    // to allocate the buffer to store the data
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        string msg = "Field " + fieldname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    switch (field_dtype) {
        case DFNT_INT8:
        {
            vector<int8>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val.data());
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

#ifndef SIGNED_BYTE_TO_INT32
            set_value((dods_byte*)val.data(),nelms);
#else

            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val[counter]);

            set_value((dods_int32*)newval.data(),nelms);
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {

            vector<uint8>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val.data());
            if (r != 0) {

                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value((dods_byte*)val.data(),nelms);
        }
            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val.data());

            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
           set_value((dods_int16*)val.data(),nelms);
        }
            break;
        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val.data());
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value((dods_uint16*)val.data(),nelms);
        }
            break;
        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val.data());
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value((dods_int32*)val.data(),nelms);
        }
            break;
        case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val.data());
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            set_value((dods_uint32*)val.data(),nelms);
        }
            break;
        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val.data());
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            // Recalculate seems not necessary.
            set_value((dods_float32*)val.data(),nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val.data());
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value ((dods_float64 *) val.data(), nelms);
        }
            break;
        default:
            throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);
    }
    return 0;
}

// Standard way to pass the coordinates of the subsetted region from the client to the handlers
// Return the number of elements to read.
int
HDFEOS2Array_RealField::format_constraint (int *offset, int *step, int *count)
{
    int nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();
    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h4",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }

    return nels;
}


void HDFEOS2Array_RealField::close_fileid(const int gsfileid, const int sdfileid) const{

    if (true == isgeofile || false == HDF4RequestHandler::get_pass_fileid()) {

        if (sdfileid != -1)
            SDend(sdfileid);

        if (gsfileid != -1){
            if (""==gridname) 
                SWclose(gsfileid);
            if (""==swathname)
                GDclose(gsfileid);
        }

    }

}

void HDFEOS2Array_RealField::release_mod1b_res(float*ref_scale,
                                               float*ref_offset,
                                               float*rad_scale,
                                               float*rad_offset) {

    if (ref_scale != nullptr)
        delete[] ref_scale;
    if (ref_offset != nullptr)
        delete[] ref_offset;
    if (rad_scale != nullptr)
        delete[] rad_scale;
    if (rad_offset != nullptr)
        delete[] rad_offset;

}


#endif
