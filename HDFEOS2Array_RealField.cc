/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real field values.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFEOS2Array_RealField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "HDFEOS2.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "HDFEOS2Util.h"

#include "hdfdesc.h"

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
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		throw InternalErr (__FILE__, __LINE__,
						   "It should be either grid or swath.");
	}

	// Note gfid and gridid represent either swath or grid.
	int32 gfid, gridid;

	// Obtain the EOS object ID(either grid or swath)
	gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

	if (gfid < 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "File " << filename.c_str () << " cannot be open.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}

	// Attach the EOS object ID
	gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));

	if (gridid < 0) {
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
	float scale=1.0, offset2=0.0, fillvalue;
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
                ret = SDreadattr(sdsid, attrindex2, (VOIDP)&attrbuf2[0]);
                if (ret==FAIL)
                {
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
			GET_FILLVALUE_ATTR_VALUE(INT8, int8);
                        GET_FILLVALUE_ATTR_VALUE(INT16, int16);
			GET_FILLVALUE_ATTR_VALUE(INT32, int32);
			GET_FILLVALUE_ATTR_VALUE(UINT8, uint8);
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

	// Obtain the field info. We mainly need to the datatype information 
	// to allocate the buffer to store the data
	r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
					   &tmp_rank, tmp_dims, &type, tmp_dimlist);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Field " << fieldname.
			c_str () << " information cannot be obtained.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	void *val;

	// We need to loop through all datatpes to allocate the memory buffer for the data.
#define RECALCULATE(CAST, DODS_CAST, VAL) \
{ \
	bool change_data_value = false; \
	if(mtype!=OTHER_TYPE) \
	{ \
		float *tmpval = new float[nelms]; \
        	CAST tmptr = (CAST)VAL; \
		for(int l=0; l<nelms; l++) \
			tmpval[l] = (float)tmptr[l]; \
		bool special_case = false; \
		if(scale_factor_attr_index==FAIL || add_offset_attr_index==FAIL) \
			if(attrcounts==1) \
				if(radiance_scales!=NULL && radiance_offsets!=NULL) \
				{ \
					scale = radiance_scales[0]; \
					offset2 = radiance_offsets[0];\
					special_case = true; \
				} \
		if((scale_factor_attr_index!=FAIL && add_offset_attr_index!=FAIL && !(scale==1 && offset2==0)) || special_case)  \
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

	switch (type) {

	case DFNT_INT8:
		{
			val = new int8[nelms];
			r = readfieldfunc (gridid,
							   const_cast < char *>(fieldname.c_str ()),
							   offset32, step32, count32, val);
			if (r != 0) {
				HDFEOS2Util::ClearMem (offset32, count32, step32, offset,
									   count, step);
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
			//set_value ((dods_int32 *) newval, nelms);
			delete[](int8 *) val;
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
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
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
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
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
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
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
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
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
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		RECALCULATE(uint32*, dods_uint32*, val);
		//set_value ((dods_uint32 *) val, nelms);
		//delete[](uint32 *) val;
		break;
	case DFNT_FLOAT32:
		val = new float32[nelms];
		r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}

		set_value ((dods_float32 *) val, nelms);
		delete[](float32 *) val;
		break;
	case DFNT_FLOAT64:
		val = new float64[nelms];
		r = readfieldfunc (gridid, const_cast < char *>(fieldname.c_str ()),
						   offset32, step32, count32, val);
		if (r != 0) {
			HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
								   step);
			ostringstream eherr;

			eherr << "field " << fieldname.c_str () << "cannot be read.";
			throw InternalErr (__FILE__, __LINE__, eherr.str ());
		}
		set_value ((dods_float64 *) val, nelms);
		delete[](float64 *) val;
		break;
	default:
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		InternalErr (__FILE__, __LINE__, "unsupported data type.");
	}

	r = detachfunc (gridid);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << datasetname.
			c_str () << " cannot be detached.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	r = closefunc (gfid);
	if (r != 0) {
		HDFEOS2Util::ClearMem (offset32, count32, step32, offset, count,
							   step);
		ostringstream eherr;

		eherr << "Grid/Swath " << filename.c_str () << " cannot be closed.";
		throw InternalErr (__FILE__, __LINE__, eherr.str ());
	}


	delete[]offset;
	delete[]count;
	delete[]step;

	delete[]offset32;
	delete[]count32;
	delete[]step32;

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
