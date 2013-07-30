/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB

#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>
#include <BESLog.h>

#include "HDFCFUtil.h"
#include "HDFEOS2Array_RealField.h"


#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2Array_RealField::read ()
{

    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = 0;
    nelms = format_constraint (&offset[0], &step[0], &count[0]);

    // Just declare offset,count and step in the int32 type.
    vector<int32>offset32;
    offset32.resize(rank);
    vector<int32>count32;
    count32.resize(rank);
    vector<int32>step32;
    step32.resize(rank);

    // Just obtain the offset,count and step in the datatype of int32.
    for (int i = 0; i < rank; i++) {
        offset32[i] = (int32) offset[i];
        count32[i] = (int32) count[i];
        step32[i] = (int32) step[i];
    }

    // Define function pointers to handle both grid and swath
    int32 (*openfunc) (char *, intn);
    intn (*closefunc) (int32);
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);

    intn (*attrinfofunc) (int32, char *, int32 *, int32 *);
    intn (*readattrfunc) (int32, char *, void*);

    std::string datasetname;
    if (swathname == "") {
        openfunc = GDopen;
        closefunc = GDclose;
        attachfunc = GDattach;
        detachfunc = GDdetach;
        fieldinfofunc = GDfieldinfo;
        readfieldfunc = GDreadfield;
        datasetname = gridname;

        attrinfofunc = GDattrinfo;
        readattrfunc = GDreadattr;
    }
    else if (gridname == "") {
        openfunc = SWopen;
        closefunc = SWclose;
        attachfunc = SWattach;
        detachfunc = SWdetach;
        fieldinfofunc = SWfieldinfo;
        readfieldfunc = SWreadfield;
        datasetname = swathname;

        attrinfofunc = SWattrinfo;
        readattrfunc = SWreadattr;
    }
    else 
        throw InternalErr (__FILE__, __LINE__, "It should be either grid or swath.");

    // Note gfid and gridid represent either swath or grid.
    int32 gfid = 0;
    int32 gridid = 0;

    // Obtain the EOS object ID(either grid or swath)
    gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
    if (gfid < 0) {
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Attach the EOS object ID
    gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));
    if (gridid < 0) {
        closefunc(gfid);
        ostringstream eherr;
        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // tmp_rank and tmp_dimlist are two dummy variables that are only used when calling fieldinfo.
    int32 tmp_rank = 0;
    char  tmp_dimlist[1024];

    // field dimension sizes
    int32 tmp_dims[rank];

    // field data type
    int32 field_dtype = 0;

    // returned value of HDF4 and HDF-EOS2 APIs
    intn  r = 0;
    
    // MODIS level 1B reflectance and radiance fields have scale/offset arrays rather than one scale/offset value.
    // So we need to handle these fields specially.
    float *reflectance_offsets =NULL;
    float *reflectance_scales  =NULL;
    float *radiance_offsets    =NULL;
    float *radiance_scales     =NULL;

    // Attribute datatype, reused for several attributes
    int32 attr_dtype = 0; 

    // Number of elements for an attribute, reused 
    int32 temp_attrcount  = 0;

    // Number of elements in an attribute
    int32 num_eles_of_an_attr = 0; 

    // Attribute(radiance_scales) index 
    int32 cf_general_attrindex  = 0; 

    // Attribute (radiance_offsets) index
    int32 cf_general_attrindex2 = 0;

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
    // Since the valid_range attribute in these products may treat values identified in the Key attribute
    // as invalid, we need to handle them in a special way.So set a flag here.
    bool  has_Key_attr = false;

    if(sotype!=DEFAULT_CF_EQU) {

        bool field_is_vdata = false;

        // HDF-EOS2 swath maps 1-D field as vdata. So we need to check if this field is vdata or SDS.
        // Essentially we only call SDS attribute routines to retrieve MODIS scale,offset and fillvalue attributes since we don't
        // find 1-D MODIS field has scale,offset and fillvalue attributes. We may need to visit this again in the future to see
        // if we also need to support the handling of scale,offset,fillvalue via vdata routines. KY 2013-07-15
        if (""==gridname) {

            r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);
                ostringstream eherr;

                eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            if (1 == tmp_rank) 
                field_is_vdata = true;
        }

        // For swath, we don't see any MODIS 1-D fields that have scale,offset and fill value attributes that need to be changed.
        // So currently, we don't need to handle the vdata handling. Another reason is the possible change of the implementation
        // of the SDS attribute handlings. KY 2012-07-31

        if( false == field_is_vdata) {

            // Obtain attribute values.
            int32 sdfileid = -1;
            sdfileid = SDstart(const_cast < char *>(filename.c_str ()), DFACC_READ);
            if (FAIL == sdfileid) {

                detachfunc(gridid);
                closefunc(gfid);
                ostringstream eherr;
                eherr << "Cannot Start the SD interface for the file " << filename <<endl;
            }

	    int32 sdsindex = -1;
            int32 sdsid;
	    sdsindex = SDnametoindex(sdfileid, fieldname.c_str());
            if (FAIL == sdsindex) {
                detachfunc(gridid);
                closefunc(gfid);
                SDend(sdfileid);
                ostringstream eherr;
                eherr << "Cannot obtain the index of " << fieldname;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            sdsid = SDselect(sdfileid, sdsindex);
            if (FAIL == sdsid) {
                detachfunc(gridid);
                closefunc(gfid);
                SDend(sdfileid);
                ostringstream eherr;
                eherr << "Cannot obtain the SDS ID  of " << fieldname;
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
	
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
            if(cf_general_attrindex!=FAIL && cf_general_attrindex2!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, cf_general_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'radiance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);

                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'radiance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                ret = SDattrinfo(sdsid, cf_general_attrindex2, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'radiance_offsets' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex2, (VOIDP)&attrbuf2[0]);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'radiance_offsets' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                // The following macro will obtain radiance_scales and radiance_offsets.
                // Although the code is compact, it may not be easy to follow. The similar macro can also be found later.
		switch(attr_dtype)
                {
#define GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)&attrbuf[0]; \
            CAST *ptr2 = (CAST*)&attrbuf2[0]; \
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
                };
#undef GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES
                num_eles_of_an_attr = temp_attrcount; // Store the count of attributes.
            }

            // Obtain attribute values of reflectance_scales and reflectance_offsets if they are available.
            cf_general_attrindex = SDfindattr(sdsid, "reflectance_scales");
            cf_general_attrindex2 = SDfindattr(sdsid, "reflectance_offsets");
            if(cf_general_attrindex!=FAIL && cf_general_attrindex2!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, cf_general_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'reflectance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'reflectance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
		
                ret = SDattrinfo(sdsid, cf_general_attrindex2, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'reflectance_offsets' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex2, (VOIDP)&attrbuf2[0]);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'reflectance_offsets' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
		switch(attr_dtype)
                {
#define GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)&attrbuf[0]; \
            CAST *ptr2 = (CAST*)&attrbuf2[0]; \
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
                };
#undef GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES
                num_eles_of_an_attr = temp_attrcount; // Store the count of attributes. 
            }

            // Obtain the value of attribute scale_factor.
            scale_factor_attr_index = SDfindattr(sdsid, "scale_factor");
            if(scale_factor_attr_index!=FAIL)
            {
                intn ret = -1;
                ret = SDattrinfo(sdsid, scale_factor_attr_index, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL) 
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'scale_factor' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, scale_factor_attr_index, (VOIDP)&attrbuf[0]);
                if (ret==FAIL) 
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'scale_factor' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
 
                switch(attr_dtype)
                {
#define GET_SCALE_FACTOR_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)&attrbuf[0]; \
            scale = (float)tmpvalue; \
        } \
        break;
                    GET_SCALE_FACTOR_ATTR_VALUE(INT8, int8);
                    GET_SCALE_FACTOR_ATTR_VALUE(CHAR,int8);
                    GET_SCALE_FACTOR_ATTR_VALUE(UINT8, uint8);
                    GET_SCALE_FACTOR_ATTR_VALUE(UCHAR,uint8);
                    GET_SCALE_FACTOR_ATTR_VALUE(INT16, int16);
                    GET_SCALE_FACTOR_ATTR_VALUE(UINT16, uint16);
                    GET_SCALE_FACTOR_ATTR_VALUE(INT32, int32);
                    GET_SCALE_FACTOR_ATTR_VALUE(UINT32, uint32);
                    GET_SCALE_FACTOR_ATTR_VALUE(FLOAT32, float);
                    GET_SCALE_FACTOR_ATTR_VALUE(FLOAT64, double);
                    
                };
#undef GET_SCALE_FACTOR_ATTR_VALUE
            }

            // Obtain the value of attribute add_offset
            add_offset_attr_index = SDfindattr(sdsid, "add_offset");
            if(add_offset_attr_index!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, add_offset_attr_index, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL) 
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'add_offset' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, add_offset_attr_index, (VOIDP)&attrbuf[0]);
                if (ret==FAIL) 
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute 'add_offset' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                switch(attr_dtype)
                {
#define GET_ADD_OFFSET_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)&attrbuf[0]; \
            field_offset = (float)tmpvalue; \
        } \
        break;
                    GET_ADD_OFFSET_ATTR_VALUE(INT8, int8);
                    GET_ADD_OFFSET_ATTR_VALUE(CHAR,int8);
                    GET_ADD_OFFSET_ATTR_VALUE(UINT8, uint8);
                    GET_ADD_OFFSET_ATTR_VALUE(UCHAR,uint8);
                    GET_ADD_OFFSET_ATTR_VALUE(INT16, int16);
                    GET_ADD_OFFSET_ATTR_VALUE(UINT16, uint16);
                    GET_ADD_OFFSET_ATTR_VALUE(INT32, int32);
                    GET_ADD_OFFSET_ATTR_VALUE(UINT32, uint32);
                    GET_ADD_OFFSET_ATTR_VALUE(FLOAT32, float);
                    GET_ADD_OFFSET_ATTR_VALUE(FLOAT64, double);
                };
#undef GET_ADD_OFFSET_ATTR_VALUE
            }

            // Obtain the value of the attribute _FillValue
            cf_general_attrindex = SDfindattr(sdsid, "_FillValue");
            if(cf_general_attrindex!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, cf_general_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute '_FillValue' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute '_FillValue' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                switch(attr_dtype)
                {
#define GET_FILLVALUE_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)&attrbuf[0]; \
            fillvalue = (float)tmpvalue; \
        } \
        break;
                    GET_FILLVALUE_ATTR_VALUE(INT8, int8);
                    GET_FILLVALUE_ATTR_VALUE(CHAR, int8);
                    GET_FILLVALUE_ATTR_VALUE(INT16, int16);
                    GET_FILLVALUE_ATTR_VALUE(INT32, int32);
                    GET_FILLVALUE_ATTR_VALUE(UINT8, uint8);
                    GET_FILLVALUE_ATTR_VALUE(UCHAR, uint8);
                    GET_FILLVALUE_ATTR_VALUE(UINT16, uint16);
                    GET_FILLVALUE_ATTR_VALUE(UINT32, uint32);
                };
#undef GET_FILLVALUE_ATTR_VALUE
            }

            // Retrieve valid_range,valid_range is normally represented as (valid_min,valid_max)
            // for non-CF scale and offset rules, the data is always float. So we only
            // need to change the data type to float.
            cf_general_attrindex = SDfindattr(sdsid, "valid_range");
            if(cf_general_attrindex!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, cf_general_attrindex, attrname, &attr_dtype, &temp_attrcount);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute '_FillValue' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attr_dtype)*temp_attrcount);
                ret = SDreadattr(sdsid, cf_general_attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
                    detachfunc(gridid);
                    closefunc(gfid);
                    SDendaccess(sdsid);
                    SDend(sdfileid);
                    ostringstream eherr;
                    eherr << "Attribute '_FillValue' in " << fieldname.c_str () << " cannot be obtained.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                string attrbuf_str(attrbuf.begin(),attrbuf.end());

                switch(attr_dtype) {
                
                    case DFNT_CHAR:
                    {                   
                        // We need to treat the attribute data as characters or string.
                        // So find the separator.
                        size_t found = attrbuf_str.find_first_of(",");
                        size_t found_from_end = attrbuf_str.find_last_of(",");
                        
                        if (string::npos == found)
                            throw InternalErr(__FILE__,__LINE__,"should find the separator ,");
                        if (found != found_from_end)
                            throw InternalErr(__FILE__,__LINE__,"Only one separator , should be available.");

                        //istringstream(attrbuf_str.substr(0,found))>> orig_valid_min;
                        //istringstream(attrbuf_str.substr(found+1))>> orig_valid_max;
                        
                        orig_valid_min = atof((attrbuf_str.substr(0,found)).c_str());
                        orig_valid_max = atof((attrbuf_str.substr(found+1)).c_str());
                    
                    }
                    break;
                    case DFNT_INT8:
                    {
                        // We find a special case that even valid_range is logically interpreted as two elements,
                        // but the count of attribute elements is more than 2. The count actually is the number of
                        // characters stored as the attribute value. So we need to find the separator "," and then
                        // change the string before and after the separator into float numbers.
                        if (temp_attrcount >2) {

                            size_t found = attrbuf_str.find_first_of(",");
                            size_t found_from_end = attrbuf_str.find_last_of(",");

                            if (string::npos == found)
                                throw InternalErr(__FILE__,__LINE__,"should find the separator ,");
                            if (found != found_from_end)
                                throw InternalErr(__FILE__,__LINE__,"Only one separator , should be available.");

                            //istringstream(attrbuf_str.substr(0,found))>> orig_valid_min;
                            //istringstream(attrbuf_str.substr(found+1))>> orig_valid_max;

                            orig_valid_min = atof((attrbuf_str.substr(0,found)).c_str());
                            orig_valid_max = atof((attrbuf_str.substr(found+1)).c_str());

                        }
                        else if (2 == temp_attrcount) {
                            orig_valid_min = (float)attrbuf[0];
                            orig_valid_max = (float)attrbuf[1]; 
                        }
                        else 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be greater than 1.");

                    }
                    break;

                    case DFNT_UINT8:
                    case DFNT_UCHAR:
                    {
                        if (temp_attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_UINT8 type.");

                        unsigned char* temp_valid_range = (unsigned char *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_INT16:
                    {
                        if (temp_attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_INT16 type.");

                        short* temp_valid_range = (short *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_UINT16:
                    {
                        if (temp_attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_UINT16 type.");

                        unsigned short* temp_valid_range = (unsigned short *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;
 
                    case DFNT_INT32:
                    {
                        if (temp_attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_INT32 type.");

                        int* temp_valid_range = (int *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_UINT32:
                    {
                        if (temp_attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_UINT32 type.");

                        unsigned int* temp_valid_range = (unsigned int *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_FLOAT32:
                    {
                        if (temp_attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_FLOAT32 type.");

                        float* temp_valid_range = (float *)&attrbuf[0]; 
                        orig_valid_min = temp_valid_range[0];
                        orig_valid_max = temp_valid_range[1];
                    }
                    break;

                    case DFNT_FLOAT64:
                    {
                        if (temp_attrcount != 2)
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_FLOAT32 type.");
                        double* temp_valid_range = (double *)&attrbuf[0];

                        // Notice: this approach will lose precision and possibly overflow. Fortunately it is not a problem for MODIS data.
                        // This part of code may not be called. If it is called, mostly the value is within the floating-point range.
                        // KY 2013-01-29
                        orig_valid_min = temp_valid_range[0];
                        orig_valid_max = temp_valid_range[1];
                    }
                    break;
                    default:
                        throw InternalErr(__FILE__,__LINE__,"Unsupported data type.");
                }
            }

            // Check if the data has the "Key" attribute. We found that some NSIDC MODIS data(MOD29) used "Key" to identify some special values.
            // To get the values that are within the range identified by the "Key", scale offset rules also need to be applied to those values
            // outside the original valid range. KY 2013-02-25
            int32 cf_general_attrindex3 = SUCCEED;
            cf_general_attrindex3 = SDfindattr(sdsid, "Key");
            if(cf_general_attrindex3 !=FAIL) {
                has_Key_attr = true;
            }

            // For testing only. Use BESDEBUG later. 
            //cerr << "scale=" << scale << endl;	
            //cerr << "offset=" << field_offset << endl;
            //cerr << "fillvalue=" << fillvalue << endl;

            SDendaccess(sdsid);
	    SDend(sdfileid);
        }
    }

    // According to our observations, it seems that MODIS products ALWAYS use the "scale" factor to 
    // make bigger values smaller.
    // So for MODIS_MUL_SCALE products, if the scale of some variable is greater than 1, 
    // it means that for this variable, the MODIS type for this variable may be MODIS_DIV_SCALE.
    // For the similar logic, we may need to change MODIS_DIV_SCALE to MODIS_MUL_SCALE and MODIS_EQ_SCALE
    // to MODIS_DIV_SCALE.
    // We indeed find such a case. HDF-EOS2 Grid MODIS_Grid_1km_2D of MOD(or MYD)09GA is a MODIS_EQ_SCALE.
    // However,
    // the scale_factor of the variable Range_1 in the MOD09GA product is 25. According to our observation,
    // this variable should be MODIS_DIV_SCALE.We verify this is true according to MODIS 09 product document
    // http://modis-sr.ltdri.org/products/MOD09_UserGuide_v1_3.pdf.
    // Since this conclusion is based on our observation, we would like to add a BESlog to detect if we find
    // the similar cases so that we can verify with the corresponding product documents to see if this is true.
    // 

    if (MODIS_EQ_SCALE == sotype || MODIS_MUL_SCALE == sotype) {
        if (scale > 1) {
            sotype = MODIS_DIV_SCALE;
            (*BESLog::TheLog())<< "The field " << fieldname << " scale factor is "<< scale << endl
                               << " But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE. " << endl
                               << " Now change it to MODIS_DIV_SCALE. "<<endl;
        }
    }
    
    if (MODIS_DIV_SCALE == sotype) {
        if (scale < 1) {
            sotype = MODIS_MUL_SCALE;
            (*BESLog::TheLog())<< "The field " << fieldname << " scale factor is "<< scale << endl
                               << " But the original scale factor type is MODIS_DIV_SCALE. " << endl
                               << " Now change it to MODIS_MUL_SCALE. "<<endl;
        }
    }
    


    // Obtain the field info. We mainly need the datatype information 
    // to allocate the buffer to store the data
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        detachfunc(gridid);
        closefunc(gfid);   
        ostringstream eherr;

        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    // We need to loop through all datatpes to allocate the memory buffer for the data.
// It is hard to add comments to the macro. We may need to change them to general routines in the future.
// Some MODIS products use both valid_range(valid_min, valid_max) and fillvalues for data fields. When do recalculating,
// I check fillvalue first, then check valid_min and valid_max if they are available. 
// The middle check is_special_value addresses the MODIS L1B special value. 
//**************************************************************************************
//****    if((float)tmptr[l] != fillvalue ) \
//                    { \
//                        if(false == HDFCFUtil::is_special_value(field_dtype,fillvalue,tmptr[l]))\
 //                       { \
//                            if (orig_valid_min<tmpval[l] && orig_valid_max>tmpval[l] \
//                            if(sotype==MODIS_MUL_SCALE) \
//                                tmpval[l] = (tmptr[l]-field_offset)*scale; \
//                            else if(sotype==MODIS_EQ_SCALE) \
//                                tmpval[l] = tmptr[l]*scale + field_offset; \
//                            else if(sotype==MODIS_DIV_SCALE) \
//                                tmpval[l] = (tmptr[l]-field_offset)/scale; \
//                        } \
//*******************************************************************************

#define RECALCULATE(CAST, DODS_CAST, VAL) \
{ \
    bool change_data_value = false; \
    if(sotype!=DEFAULT_CF_EQU) \
    { \
        float *tmpval = new float[nelms]; \
        CAST tmptr = (CAST)VAL; \
        for(int l=0; l<nelms; l++) \
            tmpval[l] = (float)tmptr[l]; \
        bool special_case = false; \
        if(scale_factor_attr_index==FAIL) \
            if(num_eles_of_an_attr==1) \
                if(radiance_scales!=NULL && radiance_offsets!=NULL) \
                { \
                    scale = radiance_scales[0]; \
                    field_offset = radiance_offsets[0];\
                    special_case = true; \
                } \
        if((scale_factor_attr_index!=FAIL && !(scale==1 && field_offset==0)) || special_case)  \
        { \
            for(int l=0; l<nelms; l++) \
            { \
                if(cf_general_attrindex!=FAIL) \
                { \
                    if((float)tmptr[l] != fillvalue ) \
                    { \
                        if(false == HDFCFUtil::is_special_value(field_dtype,fillvalue,tmptr[l]))\
                        { \
                            if ((orig_valid_min<=tmpval[l] && orig_valid_max>=tmpval[l]) || (true==has_Key_attr))\
                            { \
                                if(sotype==MODIS_MUL_SCALE) \
                                        tmpval[l] = (tmptr[l]-field_offset)*scale; \
                                else if(sotype==MODIS_EQ_SCALE) \
                                        tmpval[l] = tmptr[l]*scale + field_offset; \
                                else if(sotype==MODIS_DIV_SCALE) \
                                        tmpval[l] = (tmptr[l]-field_offset)/scale;\
                           } \
                        } \
                    } \
                } \
            } \
            change_data_value = true; \
            set_value((dods_float32 *)tmpval, nelms); \
            delete[] tmpval; \
        } else 	if(num_eles_of_an_attr>1 && (radiance_scales!=NULL && radiance_offsets!=NULL) || (reflectance_scales!=NULL && reflectance_offsets!=NULL)) \
        { \
            size_t dimindex=0; \
            if( num_eles_of_an_attr!=tmp_dims[dimindex]) \
            { \
                ostringstream eherr; \
                eherr << "The number of Z-Dimension scale attribute is not equal to the size of the first dimension in " << fieldname.c_str() << ". These two values must be equal."; \
                throw InternalErr (__FILE__, __LINE__, eherr.str ()); \
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
                    if(cf_general_attrindex!=FAIL) \
                    { \
                        if(((float)tmptr[index])!=fillvalue) \
                        { \
                            if(false == HDFCFUtil::is_special_value(field_dtype,fillvalue,tmptr[index]))\
                            { \
                                if(sotype==MODIS_MUL_SCALE) \
                                    tmpval[index] = (tmptr[index]-tmpoffset)*tmpscale; \
                                else if(sotype==MODIS_EQ_SCALE) \
                                    tmpval[index] = tmptr[index]*tmpscale+tmpoffset; \
                                else if(sotype==MODIS_DIV_SCALE) \
                                    tmpval[index] = (tmptr[index]-tmpoffset)/tmpscale; \
                            } \
                        } \
                    } \
                    index++; \
                } \
            } \
            change_data_value = true; \
            set_value((dods_float32 *)tmpval, nelms); \
            delete[] tmpval; \
        } \
    } \
    if(!change_data_value) \
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
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);   
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

#ifndef SIGNED_BYTE_TO_INT32
            RECALCULATE(int8*, dods_byte*, &val[0]);
#else

            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val[counter]);

            RECALCULATE(int32*, dods_int32*, &newval[0]);
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
        {

            vector<uint8>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid); 

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(uint8*, dods_byte*, &val[0]);
        }
            break;

        case DFNT_INT16:
        {
            vector<int16>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &offset32[0], &step32[0], &count32[0], &val[0]);

            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            RECALCULATE(int16*, dods_int16*, &val[0]);
        }
            break;
        case DFNT_UINT16:
        {
            vector<uint16>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(uint16*, dods_uint16*, &val[0]);
        }
            break;
        case DFNT_INT32:
        {
            vector<int32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(int32*, dods_int32*, &val[0]);
        }
            break;
        case DFNT_UINT32:
        {
            vector<uint32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(uint32*, dods_uint32*, &val[0]);
        }
            break;
        case DFNT_FLOAT32:
        {
            vector<float32>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            // Recalculate seems not necessary.
            RECALCULATE(float32*, dods_float32*, &val[0]);
        }
            break;
        case DFNT_FLOAT64:
        {
            vector<float64>val;
            val.resize(nelms);
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &offset32[0], &step32[0], &count32[0], &val[0]);
            if (r != 0) {
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            set_value ((dods_float64 *) &val[0], nelms);
        }
            break;
        default:
            detachfunc(gridid);
            closefunc(gfid);

            InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

    r = detachfunc (gridid);
    if (r != 0) {
        closefunc(gfid);
        ostringstream eherr;

        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be detached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    r = closefunc (gfid);
    if (r != 0) {
        ostringstream eherr;

        eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    if(reflectance_scales!=NULL)
    {
        delete[] reflectance_offsets;
        delete[] reflectance_scales;
    }

    if(radiance_scales!=NULL)
    {
        delete[] radiance_offsets;
        delete[] radiance_scales;
    }

    return false;
}

// Standard way to pass the coordinates of the subsetted region from the client to the handlers
// Return the number of elements to read.
int
HDFEOS2Array_RealField::format_constraint (int *offset, int *step, int *count)
{
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);


        // Check for illegical  constraint
        if (stride < 0 || start < 0 || stop < 0 || start > stop) {
            ostringstream oss;

            oss << "Array/Grid hyperslab indices are bad: [" << start <<
                ":" << stride << ":" << stop << "]";
            throw Error (malformed_expr, oss.str ());
        }

        // Check for an empty constraint and use the whole dimension if so.
        if (start == 0 && stop == 0 && stride == 0) {
            start = dimension_start (p, false);
            stride = dimension_stride (p, false);
            stop = dimension_stop (p, false);
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;// count of elements
        nels *= count[id];// total number of values for variable

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
#endif
