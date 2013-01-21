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

#include "HDFCFUtil.h"
#include "HDFEOS2Array_RealField.h"


#define SIGNED_BYTE_TO_INT32 1

bool
HDFEOS2Array_RealField::read ()
{


    int *offset = new int[rank];
    int *count = new int[rank];
    int *step = new int[rank];
    int nelms;

    try {
        nelms = format_constraint (offset, step, count);
    }
    catch (...) {
        delete[]offset;
        delete[]step;
        delete[]count;
        throw;
    }

    int32 *offset32 = new int32[rank];
    int32 *count32 = new int32[rank];
    int32 *step32 = new int32[rank];

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
    else {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        throw InternalErr (__FILE__, __LINE__, "It should be either grid or swath.");
    }

    // Note gfid and gridid represent either swath or grid.
    int32 gfid = 0;
    int32 gridid = 0;

    // Obtain the EOS object ID(either grid or swath)
    gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
    if (gfid < 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Attach the EOS object ID
    gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));

    if (gridid < 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
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

    if(sotype!=DEFAULT_CF_EQU) {

        bool field_is_vdata = false;

        if (""==gridname) {

            r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                &tmp_rank, tmp_dims, &type, tmp_dimlist);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count,
                                                           step);

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
                    GET_FILLVALUE_ATTR_VALUE(INT16, int16);
                    GET_FILLVALUE_ATTR_VALUE(INT32, int32);
                    GET_FILLVALUE_ATTR_VALUE(UINT8, uint8);
                    GET_FILLVALUE_ATTR_VALUE(UINT16, uint16);
                    GET_FILLVALUE_ATTR_VALUE(UINT32, uint32);
                };
#undef GET_FILLVALUE_ATTR_VALUE
            }

            // For testing only. Use BESDEBUG later. 
            //cerr << "scale=" << scale << endl;	
            //cerr << "offset=" << offset2 << endl;
            //cerr << "fillvalue=" << fillvalue << endl;

            SDendaccess(sdsid);
	    SDend(sdfileid);
        }
    }

    // Obtain the field info. We mainly need the datatype information 
    // to allocate the buffer to store the data
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &type, tmp_dimlist);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        detachfunc(gridid);
        closefunc(gfid);   
        ostringstream eherr;

        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    void *val; //[LD Comment 11/12/2012]

    // We need to loop through all datatpes to allocate the memory buffer for the data.
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
                            if(sotype==MODIS_MUL_SCALE) \
                                tmpval[l] = (tmptr[l]-offset2)*scale; \
                            else if(sotype==MODIS_EQ_SCALE) \
                                tmpval[l] = tmptr[l]*scale + offset2; \
                            else if(sotype==MODIS_DIV_SCALE) \
                                tmpval[l] = (tmptr[l]-offset2)/scale; \
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
    delete[](CAST) VAL; \
}

    switch (type) {
        case DFNT_INT8:
        {
            val = new int8[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);   
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

#ifndef SIGNED_BYTE_TO_INT32
            RECALCULATE(int8*, dods_byte*, val);
            //set_value ((dods_byte *) val, nelms);
            //delete[](int8 *) val;
#else
            int32 *newval;
            int8 *newval8;

            newval = new int32[nelms];
            newval8 = (int8 *) val;
            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (newval8[counter]);

            RECALCULATE(int32*, dods_int32*, newval);
            delete[](int8 *) val;
            //set_value ((dods_int32 *) newval, nelms);
            //delete[]newval;
#endif
        }

            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
            val = new uint8[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid); 

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(uint8*, dods_byte*, val);
            //set_value ((dods_byte *) val, nelms);
            //delete[](uint8 *) val;
            break;

        case DFNT_INT16:
            val = new int16[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val);

            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            RECALCULATE(int16*, dods_int16*, val);
            //set_value ((dods_int16 *) val, nelms);
            //delete[](int16 *) val;
            break;
        case DFNT_UINT16:
            val = new uint16[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(uint16*, dods_uint16*, val);
            //set_value ((dods_uint16 *) val, nelms);
            //delete[](uint16 *) val;
            break;
        case DFNT_INT32:
            val = new int32[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(int32*, dods_int32*, val);
            //set_value ((dods_int32 *) val, nelms);
            //delete[](int32 *) val;
            break;
        case DFNT_UINT32:
            val = new uint32[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()), 
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            RECALCULATE(uint32*, dods_uint32*, val);
            //set_value ((dods_uint32 *) val, nelms);
            //delete[](uint32 *) val;
            break;
        case DFNT_FLOAT32:
        {
            val = new float32[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            // Recalculate seems not necessary.
            // RECALCULATE(float32*, dods_float32*, val);
            set_value ((dods_float32 *) val, nelms);
            delete[](float32 *) val;
        }
            break;
        case DFNT_FLOAT64:
            val = new float64[nelms];
            r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
                offset32, step32, count32, val);
            if (r != 0) {
                HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
                detachfunc(gridid);
                closefunc(gfid);

                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            set_value ((dods_float64 *) val, nelms);
            delete[](float64 *) val;
            break;
        default:
            HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
            detachfunc(gridid);
            closefunc(gfid);

            InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

    r = detachfunc (gridid);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        closefunc(gfid);
        ostringstream eherr;

        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be detached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    r = closefunc (gfid);
    if (r != 0) {
        HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);
        ostringstream eherr;

        eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    HDFCFUtil::ClearMem (offset32, count32, step32, offset, count, step);

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
