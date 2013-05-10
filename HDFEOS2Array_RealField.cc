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
//cerr<<"coming to HDF real array read" <<endl;

#if 0
    int *offset = new int[rank];
    int *count = new int[rank];
    int *step = new int[rank];
#endif

    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    int nelms = 0;

    nelms = format_constraint (&offset[0], &step[0], &count[0]);

#if 0
    int32 *offset32 = new int32[rank];
    int32 *count32 = new int32[rank];
    int32 *step32 = new int32[rank];
#endif

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

    int32 tmp_rank = 0;
    int32 tmp_dims[rank];
    char tmp_dimlist[1024];
    int32 type = 0;
    intn r = 0;

    // All the following variables are used by the RECALCULATE macro. So 
    // ideally they should not be here. 
    float *reflectance_offsets=NULL, *reflectance_scales=NULL;
    float *radiance_offsets=NULL, *radiance_scales=NULL;

    int32 attrtype, attrcount, attrcounts=0, attrindex = 0, attrindex2 = 0;
    int32 scale_factor_attr_index = 0, add_offset_attr_index = 0;
    float scale=1, offset2=0, fillvalue = 0;
    float orig_valid_min = 0;
    float orig_valid_max = 0;
    bool  has_Key_attr = false;

    if(sotype!=DEFAULT_CF_EQU) {

        bool field_is_vdata = false;

        if (""==gridname) {

            r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &tmp_rank, tmp_dims, &type, tmp_dimlist);
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
            int32 sdfileid;
            sdfileid = SDstart(const_cast < char *>(filename.c_str ()), DFACC_READ);

            if (FAIL == sdfileid) {

                detachfunc(gridid);
                closefunc(gfid);
                ostringstream eherr;
                eherr << "Cannot Start the SD interface for the file " << filename <<endl;
            }

	    int32 sdsindex, sdsid;
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
            attrindex = SDfindattr(sdsid, "radiance_scales");
            attrindex2 = SDfindattr(sdsid, "radiance_offsets");
            if(attrindex!=FAIL && attrindex2!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
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
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
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
                ret = SDattrinfo(sdsid, attrindex2, attrname, &attrtype, &attrcount);
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
                attrbuf2.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex2, (VOIDP)&attrbuf2[0]);
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
		switch(attrtype)
                {
#define GET_RADIANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)&attrbuf[0]; \
            CAST *ptr2 = (CAST*)&attrbuf2[0]; \
            radiance_scales = new float[attrcount]; \
            radiance_offsets = new float[attrcount]; \
            for(int l=0; l<attrcount; l++) \
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
                attrcounts = attrcount; // Store the count of attributes.
            }

            attrindex = SDfindattr(sdsid, "reflectance_scales");
            attrindex2 = SDfindattr(sdsid, "reflectance_offsets");
            if(attrindex!=FAIL && attrindex2!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
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
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
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
		
                ret = SDattrinfo(sdsid, attrindex2, attrname, &attrtype, &attrcount);
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
                attrbuf2.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex2, (VOIDP)&attrbuf2[0]);
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
		switch(attrtype)
                {
#define GET_REFLECTANCE_SCALES_OFFSETS_ATTR_VALUES(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST *ptr = (CAST*)&attrbuf[0]; \
            CAST *ptr2 = (CAST*)&attrbuf2[0]; \
            reflectance_scales = new float[attrcount]; \
            reflectance_offsets = new float[attrcount]; \
            for(int l=0; l<attrcount; l++) \
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
                attrcounts = attrcount; // Store the count of attributes. 
            }

            scale_factor_attr_index = SDfindattr(sdsid, "scale_factor");
            if(scale_factor_attr_index!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, scale_factor_attr_index, attrname, &attrtype, &attrcount);
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
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
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
 
                switch(attrtype)
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

            add_offset_attr_index = SDfindattr(sdsid, "add_offset");
            if(add_offset_attr_index!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, add_offset_attr_index, attrname, &attrtype, &attrcount);
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
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
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

                switch(attrtype)
                {
#define GET_ADD_OFFSET_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)&attrbuf[0]; \
            offset2 = (float)tmpvalue; \
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

            attrindex = SDfindattr(sdsid, "_FillValue");
            if(attrindex!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
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
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
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

                switch(attrtype)
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

//#if 0

            attrindex = SDfindattr(sdsid, "valid_range");
            if(attrindex!=FAIL)
            {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
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
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
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

                switch(attrtype) {
                
                    case DFNT_CHAR:
                    {                   
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
                        if (attrcount >2) {

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
                        else if (2 == attrcount) {
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
                        if (attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_UINT8 type.");
                        unsigned char* temp_valid_range = (unsigned char *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_INT16:
                    {
                        if (attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_INT16 type.");
                        short* temp_valid_range = (short *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_UINT16:
                    {
                        if (attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_UINT16 type.");
                        unsigned short* temp_valid_range = (unsigned short *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;
 
                    case DFNT_INT32:
                    {
                        if (attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_INT32 type.");
                        int* temp_valid_range = (int *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_UINT32:
                    {
                        if (attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_UINT32 type.");
                        unsigned int* temp_valid_range = (unsigned int *)&attrbuf[0]; 
                        orig_valid_min = (float)(temp_valid_range[0]);
                        orig_valid_max = (float)(temp_valid_range[1]);
                    }
                    break;

                    case DFNT_FLOAT32:
                    {
                        if (attrcount != 2) 
                            throw InternalErr(__FILE__,__LINE__,"The number of attribute count should be 2 for the DFNT_FLOAT32 type.");
                        float* temp_valid_range = (float *)&attrbuf[0]; 
                        orig_valid_min = temp_valid_range[0];
                        orig_valid_max = temp_valid_range[1];
                    }
                    break;

                    case DFNT_FLOAT64:
                    {
                        if (attrcount != 2)
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
//#endif

            // Check if the data has the "Key" attribute. We found that some NSIDC MODIS data(MOD29) used "Key" to identify some special values.
            // To get the values that are within the range identified by the "Key", scale offset rules also need to be applied to those values
            // outside the original valid range. KY 2013-02-25

            int32 attrindex3 = SUCCEED;
            attrindex3 = SDfindattr(sdsid, "Key");
            if(attrindex3 !=FAIL) 
                has_Key_attr = true;

            // For testing only. Use BESDEBUG later. 
            //cerr << "scale=" << scale << endl;	
            //cerr << "offset=" << offset2 << endl;
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
        &tmp_rank, tmp_dims, &type, tmp_dimlist);
    if (r != 0) {
        detachfunc(gridid);
        closefunc(gfid);   
        ostringstream eherr;

        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    //void *val; //[LD Comment 11/12/2012]

    // We need to loop through all datatpes to allocate the memory buffer for the data.
// It is hard to add comments to the macro. We may need to change them to general routines in the future.
// Some MODIS products use both valid_range(valid_min, valid_max) and fillvalues for data fields. When do recalculating,
// I check fillvalue first, then check valid_min and valid_max if they are available. 
// The middle check is_special_value addresses the MODIS L1B special value. 
//**************************************************************************************
//****    if((float)tmptr[l] != fillvalue ) \
//                    { \
//                        if(false == HDFCFUtil::is_special_value(type,fillvalue,tmptr[l]))\
 //                       { \
//                            if (orig_valid_min<tmpval[l] && orig_valid_max>tmpval[l] \
//                            if(sotype==MODIS_MUL_SCALE) \
//                                tmpval[l] = (tmptr[l]-offset2)*scale; \
//                            else if(sotype==MODIS_EQ_SCALE) \
//                                tmpval[l] = tmptr[l]*scale + offset2; \
//                            else if(sotype==MODIS_DIV_SCALE) \
//                                tmpval[l] = (tmptr[l]-offset2)/scale; \
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
            if(attrcounts==1) \
                if(radiance_scales!=NULL && radiance_offsets!=NULL) \
                { \
                    scale = radiance_scales[0]; \
                    offset2 = radiance_offsets[0];\
                    special_case = true; \
                } \
        if((scale_factor_attr_index!=FAIL && !(scale==1 && offset2==0)) || special_case)  \
        { \
            for(int l=0; l<nelms; l++) \
            { \
                if(attrindex!=FAIL) \
                { \
                    if((float)tmptr[l] != fillvalue ) \
                    { \
                        if(false == HDFCFUtil::is_special_value(type,fillvalue,tmptr[l]))\
                        { \
                            if ((orig_valid_min<=tmpval[l] && orig_valid_max>=tmpval[l]) || (true==has_Key_attr))\
                            { \
                                if(sotype==MODIS_MUL_SCALE) \
                                        tmpval[l] = (tmptr[l]-offset2)*scale; \
                                else if(sotype==MODIS_EQ_SCALE) \
                                        tmpval[l] = tmptr[l]*scale + offset2; \
                                else if(sotype==MODIS_DIV_SCALE) \
                                        tmpval[l] = (tmptr[l]-offset2)/scale;\
                           } \
                        } \
                    } \
                } \
            } \
            change_data_value = true; \
            set_value((dods_float32 *)tmpval, nelms); \
            delete[] tmpval; \
        } else 	if(attrcounts>1 && (radiance_scales!=NULL && radiance_offsets!=NULL) || (reflectance_scales!=NULL && reflectance_offsets!=NULL)) \
        { \
            size_t dimindex=0; \
            if( attrcounts!=tmp_dims[dimindex]) \
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
                    if(attrindex!=FAIL) \
                    { \
                        if(((float)tmptr[index])!=fillvalue) \
                        { \
                            if(false == HDFCFUtil::is_special_value(type,fillvalue,tmptr[index]))\
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

    switch (type) {
        case DFNT_INT8:
        {

            vector<int8>val;
            val.resize(nelms);
            //val = new int8[nelms];
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
            //set_value ((dods_byte *) val, nelms);
            //delete[](int8 *) val;
#else

            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val[counter]);

            RECALCULATE(int32*, dods_int32*, &newval[0]);
            //delete[](int8 *) val;
            //set_value ((dods_int32 *) newval, nelms);
            //delete[]newval;
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
            //set_value ((dods_byte *) val, nelms);
            //delete[](uint8 *) val;
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
            //set_value ((dods_int16 *) val, nelms);
            //delete[](int16 *) val;
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
            //set_value ((dods_uint16 *) val, nelms);
            //delete[](uint16 *) val;
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
            //set_value ((dods_int32 *) val, nelms);
            //delete[](int32 *) val;
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
            //set_value ((dods_uint32 *) val, nelms);
            //delete[](uint32 *) val;
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
            //set_value ((dods_float32 *) &val[0], nelms);
            //delete[](float32 *) val;
        }
            break;
        case DFNT_FLOAT64:
        {
            //val = new float64[nelms];
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
            //RECALCULATE(float32*, dods_float32*, &val[0]);
            set_value ((dods_float64 *) &val[0], nelms);
            //delete[](float64 *) val;
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

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
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
