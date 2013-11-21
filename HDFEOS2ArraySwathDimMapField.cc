/////////////////////////////////////////////////////////////////////////////

// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

// Currently the handling of swath data fields with dimension maps is the same as other data fields(HDFEOS2Array_RealField.cc etc)
// The reason to keep it in separate is, in theory, that data fields with dimension map may need special handlings.
// So we will leave it here for this release(2010-8), it may be removed in the future. HDFEOS2Array_RealField.cc may be used.
#ifdef USE_HDFEOS2_LIB

#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include "BESDebug.h"
#include <BESLog.h>
#include "HDFEOS2ArraySwathDimMapField.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFEOS2ArraySwathDimMapField::read ()
{

    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);

    vector<int>count;
    count.resize(rank);

    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(&offset[0],&step[0],&count[0]);

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

    std::string datasetname;

    if (swathname == "") {
        openfunc = GDopen;
        closefunc = GDclose;
        attachfunc = GDattach;
        detachfunc = GDdetach;
        fieldinfofunc = GDfieldinfo;
        readfieldfunc = GDreadfield;
        datasetname = gridname;
    }
    else if (gridname == "") {
        openfunc = SWopen;
        closefunc = SWclose;
        attachfunc = SWattach;
        detachfunc = SWdetach;
        fieldinfofunc = SWfieldinfo;
        readfieldfunc = SWreadfield;
        datasetname = swathname;
    }
    else {
        throw InternalErr (__FILE__, __LINE__, "It should be either grid or swath.");
    }

    // Swath ID, swathid is actually in this case only the id of latitude and longitude.
    int32 sfid = -1;
    int32 swathid = -1; 

    // Open, attach and obtain datatype information based on HDF-EOS2 APIs.
    sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

    if (sfid < 0) {
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    swathid = attachfunc (sfid, const_cast < char *>(datasetname.c_str ()));

    if (swathid < 0) {
        closefunc (sfid);
        ostringstream eherr;
        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // dimmaps was set to be empty in hdfdesc.cc if the extra geolocation file also uses the dimension map
    // This is because the dimmaps may be different in the MODIS geolocation file. So we cannot just pass
    // the dimmaps to this class.
    // Here we then obtain the dimension map info. in the geolocation file.
    if(true == dimmaps.empty()) {
    
        int32 nummaps = 0;
        int32 bufsize = 0;

        // Obtain number of dimension maps and the buffer size.
        if ((nummaps = SWnentries(swathid, HDFE_NENTMAP, &bufsize)) == -1)
            throw InternalErr (__FILE__, __LINE__, "cannot obtain the number of dimmaps");

        if (nummaps <= 0)
            throw InternalErr (__FILE__,__LINE__,"Number of dimension maps should be greater than 0");

        vector<char> namelist;
        vector<int32> offset, increment;

        namelist.resize(bufsize + 1);
        offset.resize(nummaps);
        increment.resize(nummaps);
        if (SWinqmaps(swathid, &namelist[0], &offset[0], &increment[0])
            == -1)
            throw InternalErr (__FILE__,__LINE__,"fail to inquiry dimension maps");

        vector<std::string> mapnames;
        HDFCFUtil::Split(&namelist[0], bufsize, ',', mapnames);
        int count = 0;
        for (std::vector<std::string>::const_iterator i = mapnames.begin();
            i != mapnames.end(); ++i) {
            vector<std::string> parts;
            HDFCFUtil::Split(i->c_str(), '/', parts);
            if (parts.size() != 2)
                throw InternalErr (__FILE__,__LINE__,"the dimmaps should only include two parts");

            struct dimmap_entry tempdimmap;
            tempdimmap.geodim = parts[0];
            tempdimmap.datadim = parts[1];
            tempdimmap.offset = offset[count];
            tempdimmap.inc    = increment[count];

            dimmaps.push_back(tempdimmap);
            ++count;
        }

    }

    // The code that handles the MODIS scale/offset/valid_range is very similar with the code
    // in HDFEOS2Array_Realfield.cc. They may be combined in the future(jira ticket HFRHANDLER-166).
    // So don't revise comments in the following code. KY 2013-07-15
    // tmp_rank and tmp_dimlist are two dummy variables that are only used when calling fieldinfo.
    int32 tmp_rank = -1;
    char tmp_dimlist[1024];

    // field dimension sizes
    int32 tmp_dims[rank]; 
    int32 type = -1;
    intn r = -1;

    // All the following variables are used by the RECALCULATE macro. So 
    // ideally they should not be here. 
    float *reflectance_offsets=NULL, *reflectance_scales=NULL;
    float *radiance_offsets=NULL, *radiance_scales=NULL;

    int32 attrtype = -1, attrcount = -1, attrcounts=0;
    int32 attrindex = -1, attrindex2 = -1;
    int32 scale_factor_attr_index = -1, add_offset_attr_index =-1;
    float scale=1, offset2=0, fillvalue = 0.;

    //cerr<<"sotype "<<sotype <<endl;
    if (sotype!=DEFAULT_CF_EQU) {

        // Obtain attribute values.
        int32 sdfileid = -1;
        sdfileid = SDstart(const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (FAIL == sdfileid) {
            detachfunc(swathid);
            closefunc(sfid);
            ostringstream eherr;
            eherr << "Cannot Start the SD interface for the file " << filename <<endl;
        }

        int32 sdsindex = -1, sdsid = -1; 
        sdsindex = SDnametoindex(sdfileid, fieldname.c_str());
        if (FAIL == sdsindex) {
            detachfunc (swathid);
            closefunc (sfid);
            SDend(sdfileid);
            ostringstream eherr;
            eherr << "Cannot obtain the index of " << fieldname;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        sdsid = SDselect(sdfileid, sdsindex);
        if (FAIL == sdsid) {
            detachfunc(swathid);
            closefunc(sfid);
            SDend(sdfileid);
            ostringstream eherr;
            eherr << "Cannot obtain the SDS ID  of " << fieldname;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        char attrname[H4_MAX_NC_NAME + 1];
        vector<char> attrbuf, attrbuf2;

        attrindex = SDfindattr(sdsid, "radiance_scales");
        attrindex2 = SDfindattr(sdsid, "radiance_offsets");
        if(attrindex!=FAIL && attrindex2!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
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
                detachfunc(swathid);
                closefunc(sfid);
                SDendaccess(sdsid);
                SDend(sdfileid);
                ostringstream eherr;
                eherr << "Attribute 'radiance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            ret = SDattrinfo(sdsid, attrindex2, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
                SDendaccess(sdsid);
                SDend(sdfileid);
                ostringstream eherr;
                eherr << "Attribute 'radiance_offsets' in " << fieldname.c_str () << " cannot be obtained.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            attrbuf2.clear();
            attrbuf2.resize(DFKNTsize(attrtype)*attrcount);
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
            intn ret = 0;
            ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
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
                detachfunc(swathid);
                closefunc(sfid);
                SDendaccess(sdsid);
                SDend(sdfileid);
                ostringstream eherr;
                eherr << "Attribute 'reflectance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            ret = SDattrinfo(sdsid, attrindex2, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
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
                detachfunc(swathid);
                closefunc(sfid);
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
        if(sotype!=DEFAULT_CF_EQU && scale_factor_attr_index!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, scale_factor_attr_index, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
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
                detachfunc(swathid);
                closefunc(sfid);
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
        if(sotype!=DEFAULT_CF_EQU && add_offset_attr_index!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, add_offset_attr_index, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
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
                detachfunc(swathid);
                closefunc(sfid);
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
        if(sotype!=DEFAULT_CF_EQU && attrindex!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(swathid);
                closefunc(sfid);
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
                detachfunc(swathid);
                closefunc(sfid);
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
                    GET_FILLVALUE_ATTR_VALUE(INT8,   int8);
                    GET_FILLVALUE_ATTR_VALUE(INT16,  int16);
                    GET_FILLVALUE_ATTR_VALUE(INT32,  int32);
                    GET_FILLVALUE_ATTR_VALUE(UINT8,  uint8);
                    GET_FILLVALUE_ATTR_VALUE(UINT16, uint16);
                    GET_FILLVALUE_ATTR_VALUE(UINT32, uint32);
                };
#undef GET_FILLVALUE_ATTR_VALUE
        }

        // For testing only.
        //cerr << "scale=" << scale << endl;
        //cerr << "offset=" << offset2 << endl;
        //cerr << "fillvalue=" << fillvalue << endl;
	
        SDendaccess(sdsid);
        SDend(sdfileid);
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

    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
                       &tmp_rank, tmp_dims, &type, tmp_dimlist);
    if (r != 0) {
        detachfunc (swathid);
        closefunc (sfid);
        ostringstream eherr;
        eherr << "Field " << fieldname.c_str () << " information cannot be obtained.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }



#define RECALCULATE(CAST, DODS_CAST, VAL) \
{ \
    bool change_data_value = false; \
    if(sotype!=DEFAULT_CF_EQU) \
    { \
        vector<float>tmpval; \
        tmpval.resize(nelms); \
        CAST tmptr = (CAST)VAL; \
        for(int l=0; l<nelms; l++) \
            tmpval[l] = (float)tmptr[l]; \
        if(scale_factor_attr_index!=FAIL && !(scale==1 && offset2==0)) \
        { \
            for(int l=0; l<nelms; l++) \
                if(attrindex!=FAIL && ((float)tmptr[l])!=fillvalue) \
                { \
                    if(sotype==MODIS_MUL_SCALE) \
                        tmpval[l] = (tmptr[l]-offset2)*scale; \
                    else if(sotype==MODIS_EQ_SCALE) \
                        tmpval[l] = tmptr[l]*scale + offset2; \
                    else if(sotype==MODIS_DIV_SCALE) \
                        tmpval[l] = (tmptr[l]-offset2)/scale; \
                } \
                change_data_value = true; \
                set_value((dods_float32 *)&tmpval[0], nelms); \
        } else  if((radiance_scales!=NULL && radiance_offsets!=NULL) || (reflectance_scales!=NULL && reflectance_offsets!=NULL)) \
        { \
            size_t dimindex=0; \
            if (attrcounts!=tmp_dims[dimindex]) \
            { \
                ostringstream eherr; \
                eherr << "The number of Z-Dimension scale attribute is not equal to the size of the first dimension in " << fieldname.c_str() << ". These two values must be equal."; \
                throw InternalErr (__FILE__, __LINE__, eherr.str ()); \
            } \
            size_t start_index, end_index; \
            int nr_elems = nelms/count32[dimindex]; \
            start_index = offset32[dimindex]; \
            end_index = start_index+step32[dimindex]*(count32[dimindex]-1); \
            for(size_t k=start_index; k<=end_index; k+=step32[dimindex]) \
            { \
                float tmpscale = (fieldname.find("Emissive")!=string::npos)? radiance_scales[k]: reflectance_scales[k]; \
                float tmpoffset = (fieldname.find("Emissive")!=string::npos)? radiance_offsets[k]: reflectance_offsets[k]; \
                for(int l=0; l<nr_elems; l++) \
                { \
                    int index = l+k*nr_elems; \
                    if(attrindex!=FAIL && ((float)tmptr[index])!=fillvalue) \
                        if(sotype==MODIS_MUL_SCALE) \
                            tmpval[index] = (tmptr[index]-tmpoffset)*tmpscale; \
                        else if(sotype==MODIS_EQ_SCALE) \
                            tmpval[index] = tmptr[index]*tmpscale+tmpoffset; \
                        else if(sotype==MODIS_DIV_SCALE) \
                            tmpval[index] = (tmptr[index]-tmpoffset)/tmpscale; \
                } \
            } \
            change_data_value = true; \
            set_value((dods_float32 *)&tmpval[0], nelms); \
        } \
    } \
    if(!change_data_value) \
    { \
        set_value ((DODS_CAST)VAL, nelms); \
    } \
}

    // int32 majordimsize, minordimsize;
    vector<int32> newdims;
    newdims.resize(rank);

    // Loop through the data type. 
    switch (type) {

        case DFNT_INT8:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < int8 > total_val8;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val8, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<int8>val8;
            val8.resize(nelms);
            FieldSubset (&val8[0], newdims, &total_val8[0], &offset32[0], &count32[0], &step32[0]);


#ifndef SIGNED_BYTE_TO_INT32
            RECALCULATE(int8*, dods_byte*, &val8[0]);
#else
            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val8[counter]);

            RECALCULATE(int32*, dods_int32*, &newval[0]);
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_CHAR8:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < uint8 > total_val_u8;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u8, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;
                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<uint8>val_u8;
            val_u8.resize(nelms);

            FieldSubset (&val_u8[0], newdims, &total_val_u8[0], &offset32[0], &count32[0], &step32[0]);
            RECALCULATE(uint8*, dods_byte*, &val_u8[0]);
        }
            break;
        case DFNT_INT16:
        {    
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < int16 > total_val16;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val16, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }  

            vector<int16>val16;
            val16.resize(nelms);

            FieldSubset (&val16[0], newdims, &total_val16[0], &offset32[0], &count32[0], &step32[0]);

            RECALCULATE(int16*, dods_int16*, &val16[0]);
        }
            break;
        case DFNT_UINT16:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < uint16 > total_val_u16;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u16, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<uint16>val_u16;
            val_u16.resize(nelms);

            FieldSubset (&val_u16[0], newdims, &total_val_u16[0], &offset32[0], &count32[0], &step32[0]);
            RECALCULATE(uint16*, dods_uint16*, &val_u16[0]);

        }
            break;
        case DFNT_INT32:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < int32 > total_val32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val32, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<int32> val32;
            val32.resize(nelms);

            FieldSubset (&val32[0], newdims, &total_val32[0], &offset32[0], &count32[0], &step32[0]);

            RECALCULATE(int32*, dods_int32*, &val32[0]);
        }
            break;
        case DFNT_UINT32:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            // Notice the total_val_u32 is allocated inside the GetFieldValue.
            std::vector < uint32 > total_val_u32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u32, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<uint32>val_u32;
            val_u32.resize(nelms);

            FieldSubset (&val_u32[0], newdims, &total_val_u32[0], &offset32[0], &count32[0], &step32[0]);
            RECALCULATE(uint32*, dods_uint32*, &val_u32[0]);
 
        }
            break;
        case DFNT_FLOAT32:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < float32 > total_val_f32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f32, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<float32>val_f32;
            val_f32.resize(nelms);

            FieldSubset (&val_f32[0], newdims, &total_val_f32[0], &offset32[0], &count32[0], &step32[0]);

            RECALCULATE(float32*, dods_float32*, &val_f32[0]);
        }
            break;
        case DFNT_FLOAT64:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            std::vector < float64 > total_val_f64;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f64, newdims);
            if (r != 0) {
                detachfunc (swathid);
                closefunc (sfid);
                ostringstream eherr;

                eherr << "field " << fieldname.c_str () << "cannot be read.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            vector<float64>val_f64;
            val_f64.resize(nelms);
            FieldSubset (&val_f64[0], newdims, &total_val_f64[0], &offset32[0], &count32[0], &step32[0]);
            RECALCULATE(float64*, dods_float64*, &val_f64[0]);
 
        }
            break;
        default:
        {
            detachfunc (swathid);
            closefunc (sfid);
            InternalErr (__FILE__, __LINE__, "unsupported data type.");
        }
    }

    r = detachfunc (swathid);
    if (r != 0) {
        closefunc(sfid);
        ostringstream eherr;

        eherr << "Grid/Swath " << datasetname.c_str () << " cannot be detached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    r = closefunc (sfid);
    if (r != 0) {
        ostringstream eherr;

        eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }


    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArraySwathDimMapField::format_constraint (int *offset, int *step, int *count)
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
        count[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= count[id]; // total number of values for variable

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

// Get latitude and longitude fields. It will call expand_dimmap_field to interpolate latitude and longitude.
template < class T > int
HDFEOS2ArraySwathDimMapField::
GetFieldValue (int32 swathid, const std::string & geofieldname,
    std::vector < struct dimmap_entry >&dimmaps,
    std::vector < T > &vals, std::vector<int32>&newdims)
{

    int32 ret = -1; 
    int32 size = -1;
    int32 rank = -1, dims[130], type = -1;

    // Two dimensions for lat/lon; each dimension name is < 64 characters,
    // The dimension names are separated by a comma.
    char dimlist[130];
    ret = SWfieldinfo (swathid, const_cast < char *>(geofieldname.c_str ()),
        &rank, dims, &type, dimlist);
    if (ret != 0)
        return -1;

    size = 1;
    for (int i = 0; i < rank; i++)
        size *= dims[i];

    vals.resize (size);

    ret = SWreadfield (swathid, const_cast < char *>(geofieldname.c_str ()),
        NULL, NULL, NULL, (void *) &vals[0]);
    if (ret != 0)
        return -1;

    std::vector < std::string > dimname;
    HDFCFUtil::Split (dimlist, ',', dimname);

    for (int i = 0; i < rank; i++) {
        std::vector < struct dimmap_entry >::iterator it;

        for (it = dimmaps.begin (); it != dimmaps.end (); it++) {
            if (it->geodim == dimname[i]) {
                int32 ddimsize = SWdiminfo (swathid, (char *) it->datadim.c_str ());
                if (ddimsize == -1)
                    return -1;
                int r;

                r = _expand_dimmap_field (&vals, rank, dims, i, ddimsize, it->offset, it->inc);
                if (r != 0)
                    return -1;
            }
        }
    }

    // dims[] are expanded already.
    for (int i = 0; i < rank; i++) { 
        //cerr<<"i "<< i << " "<< dims[i] <<endl;
        if (dims[i] < 0)
            return -1;
        newdims[i] = dims[i];
    }

    return 0;
}

// expand the dimension map field.
template < class T > int
HDFEOS2ArraySwathDimMapField::_expand_dimmap_field (std::vector < T >
                                                    *pvals, int32 rank,
                                                    int32 dimsa[],
                                                    int dimindex,
                                                    int32 ddimsize,
                                                    int32 offset,
                                                    int32 inc)
{
    std::vector < T > orig = *pvals;
    std::vector < int32 > pos;
    std::vector < int32 > dims;
    std::vector < int32 > newdims;
    pos.resize (rank);
    dims.resize (rank);

    for (int i = 0; i < rank; i++) {
        pos[i] = 0;
        dims[i] = dimsa[i];
    }
    newdims = dims;
    newdims[dimindex] = ddimsize;
    dimsa[dimindex] = ddimsize;

    int newsize = 1;

    for (int i = 0; i < rank; i++) {
        newsize *= newdims[i];
    }
    pvals->clear ();
    pvals->resize (newsize);

    for (;;) {
        // if end
        if (pos[0] == dims[0]) {
            // we past then end
            break;
        }
        else if (pos[dimindex] == 0) {
            // extract 1D values
            std::vector < T > v;
            for (int i = 0; i < dims[dimindex]; i++) {
                pos[dimindex] = i;
                v.push_back (orig[INDEX_nD_TO_1D (dims, pos)]);
            }
            // expand them

            std::vector < T > w;
            for (int32 j = 0; j < ddimsize; j++) {
                int32 i = (j - offset) / inc;
                T f;

                if (i * inc + offset == j) // perfect match
                {
                    f = (v[i]);
                }
                else {
                    int32 i1, i2, j1, j2; //[LD Comment 11/13/2012]

                    if (i <= 0) {
                        i1 = 0;
                        i2 = 1;
                    }
                    if ((unsigned int) i + 1 >= v.size ()) {
                        i1 = v.size () - 2;
                        i2 = v.size () - 1;
                    }
                    else {
                        i1 = i;
                        i2 = i + 1;
                    }
                    j1 = i1 * inc + offset;
                    j2 = i2 * inc + offset;
                    f = (((j - j1) * v[i2] + (j2 - j) * v[i1]) / (j2 - j1));
                }
                w.push_back (f);
                pos[dimindex] = j;
                (*pvals)[INDEX_nD_TO_1D (newdims, pos)] = f;
            }
            pos[dimindex] = 0;
        }
        // next pos
        pos[rank - 1]++;
        for (int i = rank - 1; i > 0; i--) {
            if (pos[i] == dims[i]) {
                pos[i] = 0;
                pos[i - 1]++;
            }
        }
    }

    return 0;
}

template < class T >
bool HDFEOS2ArraySwathDimMapField::FieldSubset (T * outlatlon,
                                                vector<int32>&newdims,
                                                T * latlon,
                                                int32 * offset,
                                                int32 * count,
                                                int32 * step)
{

    if (newdims.size() == 1) 
        Field1DSubset(outlatlon,newdims[0],latlon,offset,count,step);
    else if (newdims.size() == 2)
        Field2DSubset(outlatlon,newdims[0],newdims[1],latlon,offset,count,step);
    else if (newdims.size() == 3)
        Field3DSubset(outlatlon,newdims,latlon,offset,count,step);
    else 
        throw InternalErr(__FILE__, __LINE__,
                          "Currently doesn't support rank >3 when interpolating with dimension map");

    return true;
}

// Subset of 1-D field to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathDimMapField::Field1DSubset (T * outlatlon,
                                                  int majordim,
                                                  T * latlon,
                                                  int32 * offset,
                                                  int32 * count,
                                                  int32 * step)
{
    if (majordim < count[0]) 
        throw InternalErr(__FILE__, __LINE__,
                          "The number of elements is greater than the total dimensional size");

    for (int i = 0; i < count[0]; i++) 
        outlatlon[i] = latlon[offset[0]+i*step[0]];
    return true;

}
// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathDimMapField::Field2DSubset (T * outlatlon,
                                                  int majordim,
                                                  int minordim,
                                                  T * latlon,
                                                  int32 * offset,
                                                  int32 * count,
                                                  int32 * step)
{


    // float64 templatlon[majordim][minordim];
    T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
    int	i = 0, j =0, k = 0; 

    // do subsetting
    // Find the correct index
    int	dim0count = count[0];
    int dim1count = count[1];

    int	dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++) // count[0] is the least changing dimension
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = (*templatlonptr)[dim0index[i]][dim1index[j]];
            k++;
        }
    }
    return true;
}

// Subsetting the field  to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathDimMapField::Field3DSubset (T * outlatlon,
                                                  vector<int32>& newdims,
                                                  T * latlon,
                                                  int32 * offset,
                                                  int32 * count,
                                                  int32 * step)
{


    // float64 templatlon[majordim][minordim];
    if (newdims.size() !=3) 
        throw InternalErr(__FILE__, __LINE__,
                          "the rank must be 3 to call this function");

    T (*templatlonptr)[newdims[0]][newdims[1]][newdims[2]] = (typeof templatlonptr) latlon;
    int i,j,k,l;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim2count = count[2];

    int dim0index[dim0count], dim1index[dim1count],dim2index[dim2count];

    for (i = 0; i < count[0]; i++) // count[0] is the least changing dimension
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    for (k = 0; k < count[2]; k++)
        dim2index[k] = offset[2] + k * step[2];

    // Now assign the subsetting data
    l = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            for ( k =0;k<count[2];k++) {
                outlatlon[l] = (*templatlonptr)[dim0index[i]][dim1index[j]][dim2index[k]];
                l++;
            }
        }
    }
    return true;
}
#endif
