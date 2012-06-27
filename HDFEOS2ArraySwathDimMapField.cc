/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

// Currently the handling of swath data fields with dimension maps is the same as other data fields(HDFEOS2Array_RealField.cc etc)
// The reason to keep it in separate is, in theory, that data fields with dimension map may need special handlings.
// So we will leave it here for this release(2010-8), it may be removed in the future. HDFEOS2Array_RealField.cc may be used.

#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include "BESDebug.h"
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"
#include "HDFEOS2.h"
#include "HDFEOS2ArraySwathDimMapField.h"
#define SIGNED_BYTE_TO_INT32 1


bool
HDFEOS2ArraySwathDimMapField::read ()
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

	for (int i = 0; i < rank; i++) {
		offset32[i] = (int32) offset[i];
		count32[i] = (int32) count[i];
		step32[i] = (int32) step[i];
	}

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
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		throw InternalErr (__FILE__, __LINE__,
						   "It should be either grid or swath.");
	}

	// Swath ID, swathid is actually in this case only the id of latitude and longitude.
	int32 sfid, swathid;

	// Open, attach and obtain datatype information based on HDF-EOS2 APIs.
	sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (sfid < 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	swathid = attachfunc (sfid, const_cast < char *>(datasetname.c_str ()));

	if (swathid < 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << datasetname.
			c_str () << " cannot be attached.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	int32 tmp_rank, tmp_dims[rank];
	char tmp_dimlist[1024];
	int32 type;
	intn r;

	// Obtain attribute values.
        int32 sdfileid;
        sdfileid = SDstart(const_cast < char *>(filename.c_str ()), DFACC_READ);
        int32 sdsindex, sdsid;
        sdsindex = SDnametoindex(sdfileid, fieldname.c_str());
        sdsid = SDselect(sdfileid, sdsindex);

        char attrname[H4_MAX_NC_NAME + 1];
        int32 attrtype, attrcount, attrcounts=0, attrindex, attrindex2;
	int32 scale_factor_attr_index, add_offset_attr_index;
        vector<char> attrbuf, attrbuf2;
        float scale=1, offset2=0, fillvalue;
        float *reflectance_offsets=NULL, *reflectance_scales=NULL;
        float *radiance_offsets=NULL, *radiance_scales=NULL;

        attrindex = SDfindattr(sdsid, "radiance_scales");
        attrindex2 = SDfindattr(sdsid, "radiance_offsets");
	if(mtype!=OTHER_TYPE && attrindex!=FAIL && attrindex2!=FAIL)
        {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'radiance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'radiance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                ret = SDattrinfo(sdsid, attrindex2, attrname, &attrtype, &attrcount);
		if (ret==FAIL)
                {
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
        if(mtype!=OTHER_TYPE && attrindex!=FAIL && attrindex2!=FAIL)
        {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'reflectance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'reflectance_scales' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
		ret = SDattrinfo(sdsid, attrindex2, attrname, &attrtype, &attrcount);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'reflectance_offsets' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf2.clear();
                attrbuf2.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex2, (VOIDP)&attrbuf2[0]);
                if (ret==FAIL)
                {
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
        if(mtype!=OTHER_TYPE && scale_factor_attr_index!=FAIL)
        {
                intn ret;
                ret = SDattrinfo(sdsid, scale_factor_attr_index, attrname, &attrtype, &attrcount);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'scale_factor' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, scale_factor_attr_index, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
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
        if(mtype!=OTHER_TYPE && add_offset_attr_index!=FAIL)
        {
                intn ret;
                ret = SDattrinfo(sdsid, add_offset_attr_index, attrname, &attrtype, &attrcount);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute 'add_offset' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, add_offset_attr_index, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
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
        if(mtype!=OTHER_TYPE && attrindex!=FAIL)
        {
                intn ret;
                ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
                if (ret==FAIL)
                {
                        ostringstream eherr;
                        eherr << "Attribute '_FillValue' in " << fieldname.c_str () << " cannot be obtained.";
                        throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }
                attrbuf.clear();
                attrbuf.resize(DFKNTsize(attrtype)*attrcount);
                ret = SDreadattr(sdsid, attrindex, (VOIDP)&attrbuf[0]);
                if (ret==FAIL)
                {
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
	
	SDend(sdfileid);

	r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
					   &tmp_rank, tmp_dims, &type, tmp_dimlist);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
                detachfunc (swathid);
                closefunc (sfid);
		ostringstream eherr;

		eherr << "Field " << fieldname.
			c_str () << " information cannot be obtained.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	void *val;

#define RECALCULATE(CAST, DODS_CAST, VAL) \
{ \
        bool change_data_value = false; \
        if(mtype!=OTHER_TYPE) \
        { \
                float *tmpval = new float[nelms]; \
                CAST tmptr = (CAST)VAL; \
                for(int l=0; l<nelms; l++) \
                        tmpval[l] = (float)tmptr[l]; \
		if(scale_factor_attr_index!=FAIL && !(scale==1 && offset2==0)) \
                { \
                        for(size_t l=0; l<nelms; l++) \
                                if(attrindex!=FAIL && ((float)tmptr[l])!=fillvalue) \
                                { \
                                        if(mtype==MODIS_TYPE1) \
                                                tmpval[l] = (tmptr[l]-offset2)*scale; \
                                        else if(mtype==MODIS_TYPE2) \
                                                tmpval[l] = tmptr[l]*scale + offset2; \
                                        else if(mtype==MODIS_TYPE3) \
                                                tmpval[l] = (tmptr[l]-offset2)/scale; \
                                } \
                        change_data_value = true; \
			set_value((dods_float32 *)tmpval, nelms); \
                        delete[] tmpval; \
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
                        size_t nr_elems = nelms/count32[dimindex]; \
                        start_index = offset32[dimindex]; \
                        end_index = start_index+step32[dimindex]*(count32[dimindex]-1); \
			for(size_t k=start_index; k<=end_index; k+=step32[dimindex]) \
                        { \
                                float tmpscale = (fieldname.find("Emissive")!=string::npos)? radiance_scales[k]: reflectance_scales[k]; \
                                float tmpoffset = (fieldname.find("Emissive")!=string::npos)? radiance_offsets[k]: reflectance_offsets[k]; \
                                for(size_t l=0; l<nr_elems; l++) \
                                { \
                                        size_t index = l+k*nr_elems; \
                                        if(attrindex!=FAIL && ((float)tmptr[index])!=fillvalue) \
                                                if(mtype==MODIS_TYPE1) \
                                                        tmpval[index] = (tmptr[index]-tmpoffset)*tmpscale; \
                                                else if(mtype==MODIS_TYPE2) \
                                                        tmpval[index] = tmptr[index]*tmpscale+tmpoffset; \
                                                else if(mtype==MODIS_TYPE2) \
                                                        tmpval[index] = (tmptr[index]-tmpoffset)/tmpscale; \
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

   int32 majordimsize, minordimsize;

	// Loop through the data type. 
	switch (type) {

	case DFNT_INT8:
		{
        // Obtaining the total value and interpolating the data according to dimension map
        std::vector < int8 > total_val8;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val8,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int8* val8 = new int8[nelms];

        FieldSubset (val8, majordimsize, minordimsize,
                           &total_val8[0], offset32, count32, step32);


#ifndef SIGNED_BYTE_TO_INT32
			RECALCULATE(int8*, dods_byte*, val8);
			//set_value ((dods_byte *) val, nelms);
			//delete[](int8 *) val;
#else
			int32 *newval;
			int8 *newval8;

			newval = new int32[nelms];
			newval8 = val8;
			for (int counter = 0; counter < nelms; counter++)
				newval[counter] = (int32) (newval8[counter]);

			RECALCULATE(int32*, dods_int32*, newval);
			//set_value ((dods_int32 *) newval, nelms);
			//delete[](int8 *) val;
                        delete[]val8;
			//delete[]newval;
#endif
		}
		break;
	case DFNT_UINT8:
	case DFNT_UCHAR8:
	case DFNT_CHAR8:
               {
               // Obtaining the total value and interpolating the data according to dimension map
        std::vector < uint8 > total_val_u8;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u8,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        uint8* val_u8 = new uint8[nelms];

        FieldSubset (val_u8, majordimsize, minordimsize,
                           &total_val_u8[0], offset32, count32, step32);
        RECALCULATE(uint8*, dods_byte*, val_u8);
	//set_value ((dods_byte *) val, nelms);
	//delete[](int8 *) val;


                }
		break;
	case DFNT_INT16:
        {    
// Obtaining the total value and interpolating the data according to dimension map
        std::vector < int16 > total_val16;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val16,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int16* val16 = new int16[nelms];

        FieldSubset (val16, majordimsize, minordimsize,
                           &total_val16[0], offset32, count32, step32);

	RECALCULATE(int16*, dods_int16*, val16);
		//set_value ((dods_int16 *) val, nelms);
		//delete[](int16 *) val;
         }
		break;
	case DFNT_UINT16:
        {
        // Obtaining the total value and interpolating the data according to dimension map
        std::vector < uint16 > total_val_u16;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u16,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        uint16* val_u16 = new uint16[nelms];

        FieldSubset (val_u16, majordimsize, minordimsize,
                           &total_val_u16[0], offset32, count32, step32);

	RECALCULATE(uint16*, dods_uint16*, val_u16);
		//set_value ((dods_int16 *) val, nelms);
		//delete[](int16 *) val;
 

         }
		break;
	case DFNT_INT32:
        {
        // Obtaining the total value and interpolating the data according to dimension map
        std::vector < int32 > total_val32;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val32,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int32* val32 = new int32[nelms];

        FieldSubset (val32, majordimsize, minordimsize,
                           &total_val32[0], offset32, count32, step32);

	RECALCULATE(int32*, dods_int32*, val32);
		//set_value ((dods_int16 *) val, nelms);
		//delete[](int16 *) val;
 

        }
		break;
	case DFNT_UINT32:
        {
        // Obtaining the total value and interpolating the data according to dimension map
        std::vector < uint32 > total_val_u32;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u32,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        uint32* val_u32 = new uint32[nelms];

        FieldSubset (val_u32, majordimsize, minordimsize,
                           &total_val_u32[0], offset32, count32, step32);

	RECALCULATE(uint32*, dods_uint32*, val_u32);
		//set_value ((dods_int16 *) val, nelms);
		//delete[](int16 *) val;
 
        }
                break;
	case DFNT_FLOAT32:
        {
// Obtaining the total value and interpolating the data according to dimension map
        std::vector < float32 > total_val_f32;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f32,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        float32* val_f32 = new float32[nelms];

        FieldSubset (val_f32, majordimsize, minordimsize,
                           &total_val_f32[0], offset32, count32, step32);

	RECALCULATE(float32*, dods_float32*, val_f32);
		//set_value ((dods_int16 *) val, nelms);
		//delete[](int16 *) val;
 

        }
		break;
	case DFNT_FLOAT64:
        {
// Obtaining the total value and interpolating the data according to dimension map
        std::vector < float64 > total_val_f64;
        r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f64,
                                                   &majordimsize, &minordimsize);
        if (r != 0) {
            HDFEOS2Util::ClearMem (offset32, count32, step32, offset,count,step);
            detachfunc (swathid);
            closefunc (sfid);
            ostringstream eherr;

            eherr << "field " << fieldname.c_str () << "cannot be read.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        float64* val_f64 = new float64[nelms];

        FieldSubset (val_f64, majordimsize, minordimsize,
                           &total_val_f64[0], offset32, count32, step32);

	RECALCULATE(float64*, dods_float64*, val_f64);
		//set_value ((dods_int16 *) val, nelms);
		//delete[](int16 *) val;
 
        }
		break;
	default:
                {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,step);
	        detachfunc (swathid);
                closefunc (sfid);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
                }
	}

	r = detachfunc (swathid);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
                closefunc(sfid);
		ostringstream eherr;

		eherr << "Grid/Swath " << datasetname.
			c_str () << " cannot be detached.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = closefunc (sfid);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


        HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count, step);
	return false;
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFEOS2ArraySwathDimMapField::format_constraint (int *offset, int *step,
												 int *count)
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
		count[id] = ((stop - start) / stride) + 1;	// count of elements
		nels *= count[id];		// total number of values for variable

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
		   std::vector < T > &vals, int32 * ydim, int32 * xdim)
{

	int32
		ret;
	int32
		size;
	int32
		rank,
		dims[130],
		type;

	// Two dimensions for lat/lon; each dimension name is < 64 characters,
	// The dimension names are separated by a comma.
	char
		dimlist[130];
	ret =
		SWfieldinfo (swathid, const_cast < char *>(geofieldname.c_str ()),
					 &rank, dims, &type, dimlist);
	if (ret != 0)
		return -1;

	size = 1;
	for (int i = 0; i < rank; i++)
		size *= dims[i];

	vals.resize (size);

	ret =
		SWreadfield (swathid, const_cast < char *>(geofieldname.c_str ()),
					 NULL, NULL, NULL, (void *) &vals[0]);
	if (ret != 0)
		return -1;

	std::vector < std::string > dimname;
	HDFEOS2::Utility::Split (dimlist, ',', dimname);

	for (int i = 0; i < rank; i++) {
		std::vector < struct dimmap_entry >::iterator
			it;

		for (it = dimmaps.begin (); it != dimmaps.end (); it++) {
			if (it->geodim == dimname[i]) {
				int32
					ddimsize =
					SWdiminfo (swathid, (char *) it->datadim.c_str ());
				if (ddimsize == -1)
					return -1;
				int
					r;

				r = _expand_dimmap_field (&vals, rank, dims, i, ddimsize,
										  it->offset, it->inc);
				if (r != 0)
					return -1;
			}
		}
	}
	*ydim = dims[0];
	*xdim = dims[1];
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

				if (i * inc + offset == j)	// perfect match
				{
					f = (v[i]);
				}
				else {
					int32 i1, i2, j1, j2;

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

// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
template < class T >
	bool HDFEOS2ArraySwathDimMapField::FieldSubset (T * outlatlon,
							  int majordim,
							  int minordim,
							  T * latlon,
							  int32 * offset,
							  int32 * count,
							  int32 * step)
{

//          float64 templatlon[majordim][minordim];
	T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
	int
		i,
		j,
		k;

	// do subsetting
	// Find the correct index
	int
		dim0count = count[0];
	int
		dim1count = count[1];

	int
		dim0index[dim0count],
		dim1index[dim1count];

	for (i = 0; i < count[0]; i++)	// count[0] is the least changing dimension
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
