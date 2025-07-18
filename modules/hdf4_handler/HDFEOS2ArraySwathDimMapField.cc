/////////////////////////////////////////////////////////////////////////////

// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

// Currently the handling of swath data fields with dimension maps is the same as 
// other data fields(HDFEOS2Array_RealField.cc etc)
// The reason to keep it in separate is, in theory, that data fields with dimension map 
// may need special handlings.
// So we will leave it this way for now.  It may be removed in the future. 
// HDFEOS2Array_RealField.cc may be used.
// KY 2014-02-19

#ifdef USE_HDFEOS2_LIB
#include "config.h"
#include "config_hdf.h"

#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include <BESInternalError.h>
#include "BESDebug.h"
#include <BESLog.h>
#include "HDFEOS2ArraySwathDimMapField.h"
#include "HDF4RequestHandler.h"
#define SIGNED_BYTE_TO_INT32 1

using namespace std;
using namespace libdap;
bool
HDFEOS2ArraySwathDimMapField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2ArraySwathDimMapField read "<<endl);
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
    int nelms = format_constraint(offset.data(),step.data(),count.data());

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

    string datasetname;

    if (swathname == "") 
        throw BESInternalError("It should be either grid or swath.",__FILE__, __LINE__);
    else if (gridname == "") {
        openfunc = SWopen;
        closefunc = SWclose;
        attachfunc = SWattach;
        detachfunc = SWdetach;
        datasetname = swathname;
    }
    else 
        throw BESInternalError("It should be either grid or swath.",__FILE__, __LINE__);

    // Swath ID, swathid is actually in this case only the id of latitude and longitude.
    int32 sfid = -1;
    int32 swathid = -1; 

    if (true == isgeofile || false == check_pass_fileid_key) {

        // Open, attach and obtain datatype information based on HDF-EOS2 APIs.
        sfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);

        if (sfid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
    else
        sfid = swfd;

    swathid = attachfunc (sfid, const_cast < char *>(datasetname.c_str ()));
    if (swathid < 0) {
        close_fileid (sfid,-1);
        string msg = "Cannot attach swath " + datasetname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // dimmaps was set to be empty in hdfdesc.cc if the extra geolocation file also 
    // uses the dimension map
    // This is because the dimmaps may be different in the MODIS geolocation file. 
    // So we cannot just pass
    // the dimmaps to this class.
    // Here we then obtain the dimension map info. in the geolocation file.
    if (true == dimmaps.empty()) {
    
        int32 nummaps = 0;
        int32 bufsize = 0;

        // Obtain number of dimension maps and the buffer size.
        if ((nummaps = SWnentries(swathid, HDFE_NENTMAP, &bufsize)) == -1){
            detachfunc(swathid);
            close_fileid(sfid,-1);
            throw BESInternalError("Cannot obtain the number of dimmaps.",__FILE__, __LINE__);
        }

        if (nummaps <= 0){
            detachfunc(swathid);
            close_fileid(sfid,-1);
            throw BESInternalError("Number of dimension maps should be greater than 0.",__FILE__,__LINE__);
        }

        vector<char> namelist;
        vector<int32> map_offset;
        vector<int32> increment;

        namelist.resize(bufsize + 1);
        map_offset.resize(nummaps);
        increment.resize(nummaps);
        if (SWinqmaps(swathid, namelist.data(), map_offset.data(), increment.data())
            == -1) {
            detachfunc(swathid);
            close_fileid(sfid,-1);
            throw BESInternalError("Fail to inquiry dimension maps.",__FILE__,__LINE__);
        }

        vector<string> mapnames;
        HDFCFUtil::Split(namelist.data(), bufsize, ',', mapnames);
        int map_count = 0;
        for (auto const &mapname:mapnames) {
            vector<string> parts;
            HDFCFUtil::Split(mapname.c_str(), '/', parts);
            if (parts.size() != 2){
                detachfunc(swathid);
                close_fileid(sfid,-1);
                throw BESInternalError("The dimmaps should only include two parts.",__FILE__,__LINE__);
            }
            struct dimmap_entry tempdimmap;
            tempdimmap.geodim = parts[0];
            tempdimmap.datadim = parts[1];
            tempdimmap.offset = map_offset[map_count];
            tempdimmap.inc    = increment[map_count];
            dimmaps.push_back(tempdimmap);
            ++map_count;
        }
    }

    if (sotype!=SOType::DEFAULT_CF_EQU) {

        if ("MODIS_SWATH_Type_L1B" == swathname) {

            string emissive_str = "Emissive";
            string RefSB_str = "RefSB";
            bool is_emissive_field = false;
            bool is_refsb_field = false;

            if (fieldname.find(emissive_str)!=string::npos) {
                if (0 == fieldname.compare(fieldname.size()-emissive_str.size(),
                                          emissive_str.size(),emissive_str))
                    is_emissive_field = true;
            }

            if (fieldname.find(RefSB_str)!=string::npos) {
                if (0 == fieldname.compare(fieldname.size()-RefSB_str.size(),
                                          RefSB_str.size(),RefSB_str))
                    is_refsb_field = true;
            }

            if ((true == is_emissive_field) || (true == is_refsb_field)) {
                detachfunc(swathid);
                close_fileid(sfid,-1);
                throw BESInternalError("Currently don't support MODIS Level 1B swath dim. map for data.",__FILE__, __LINE__);
            }
        }
    }

    bool is_modis1b = false;
    if ("MODIS_SWATH_Type_L1B" == swathname) 
        is_modis1b = true;

    try {
        if (true == HDF4RequestHandler::get_disable_scaleoffset_comp() && false== is_modis1b) 
            write_dap_data_disable_scale_comp(swathid,nelms,offset32,count32,step32);
        else
            write_dap_data_scale_comp(swathid,nelms,offset32,count32,step32);
    }
    catch(...) {
        detachfunc(swathid);
        close_fileid(sfid,-1);
        throw;
    }

    intn r = 0;
    r = detachfunc (swathid);
    if (r != 0) {
        close_fileid(sfid,-1);
        string msg = "Grid/Swath " + datasetname + " cannot be detached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    if (true == isgeofile || false == check_pass_fileid_key) {
        r = closefunc (sfid);
        if (r != 0) {
            string msg = "Grid/Swath " + filename + " cannot be closed.";
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }

    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArraySwathDimMapField::format_constraint (int *offset, int *step, int *count)
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

// Get latitude and longitude fields. 
// It will call expand_dimmap_field to interpolate latitude and longitude.
template < class T > int
HDFEOS2ArraySwathDimMapField::
GetFieldValue (int32 swathid, const string & geofieldname,
    const vector < struct dimmap_entry >&sw_dimmaps,
    vector < T > &vals, vector<int32>&newdims)
{

    int32 ret = -1; 
    int32 size = -1;
    int32 sw_rank = -1;
    int32 dims[130];
    int32  type = -1;

    // Two dimensions for lat/lon; each dimension name is < 64 characters,
    // The dimension names are separated by a comma.
    char dimlist[130];
    ret = SWfieldinfo (swathid, const_cast < char *>(geofieldname.c_str ()),
        &sw_rank, dims, &type, dimlist);
    if (ret != 0)
        return -1;

    size = 1;
    for (int i = 0; i <sw_rank; i++)
        size *= dims[i];

    vals.resize (size);

    ret = SWreadfield (swathid, const_cast < char *>(geofieldname.c_str ()),
                       nullptr, nullptr, nullptr, (void *) vals.data());
    if (ret != 0)
        return -1;

    vector < string > dimname;
    HDFCFUtil::Split (dimlist, ',', dimname);

    for (int i = 0; i < sw_rank; i++) {

        for (const auto & dmap:sw_dimmaps) {
            if (dmap.geodim == dimname[i]) {
                int32 ddimsize = SWdiminfo (swathid, (char *) dmap.datadim.c_str ());
                if (ddimsize == -1)
                    return -1;
                int r;

                r = _expand_dimmap_field (&vals, sw_rank, dims, i, ddimsize, dmap.offset, dmap.inc);
                if (r != 0)
                    return -1;
            }
        }
    }

    // dims[] are expanded already.
    for (int i = 0; i < sw_rank; i++) { 
        if (dims[i] < 0)
            return -1;
        newdims[i] = dims[i];
    }

    return 0;
}

// expand the dimension map field.
template < class T > int
HDFEOS2ArraySwathDimMapField::_expand_dimmap_field (vector < T >
                                                    *pvals, int32 sw_rank,
                                                    int32 dimsa[],
                                                    int dimindex,
                                                    int32 ddimsize,
                                                    int32 offset,
                                                    int32 inc) const  
{
    vector < T > orig = *pvals;
    vector < int32 > pos;
    vector < int32 > dims;
    vector < int32 > newdims;
    pos.resize (sw_rank);
    dims.resize (sw_rank);

    for (int i = 0; i < sw_rank; i++) {
        pos[i] = 0;
        dims[i] = dimsa[i];
    }
    newdims = dims;
    newdims[dimindex] = ddimsize;
    dimsa[dimindex] = ddimsize;

    int newsize = 1;

    for (int i = 0; i < sw_rank; i++) {
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
            vector < T > v;
            for (int i = 0; i < dims[dimindex]; i++) {
                pos[dimindex] = i;
                v.push_back (orig[INDEX_nD_TO_1D (dims, pos)]);
            }
            // expand them

            vector < T > w;
            for (int32 j = 0; j < ddimsize; j++) {
                int32 i = (j - offset) / inc;
                T f;

                if (i * inc + offset == j) // perfect match
                {
                    f = (v[i]);
                }
                else {
                    int32 i1 = 0;
                    int32 i2 = (i<=0)?1:0;
                    int32 j1 = 0;
                    int32 j2 = 0; 

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
        pos[sw_rank - 1]++;
        for (int i = sw_rank - 1; i > 0; i--) {
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
                                                const vector<int32>&newdims,
                                                T * latlon,
                                                const int32 * offset,
                                                const int32 * count,
                                                const int32 * step) 
{

    if (newdims.size() == 1) 
        Field1DSubset(outlatlon,newdims[0],latlon,offset,count,step);
    else if (newdims.size() == 2)
        Field2DSubset(outlatlon,newdims[0],newdims[1],latlon,offset,count,step);
    else if (newdims.size() == 3)
        Field3DSubset(outlatlon,newdims,latlon,offset,count,step);
    else { 
        string msg = "Currently doesn't support rank >3 when interpolating with dimension map.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    return true;
}

// Subset of 1-D field to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathDimMapField::Field1DSubset (T * outlatlon,
                                                  const int majordim,
                                                  T * latlon,
                                                  const int32 * offset,
                                                  const int32 * count,
                                                  const int32 * step)
{
    if (majordim < count[0]) {
        string msg = "The number of elements is greater than the total dimensional size.";
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    for (int i = 0; i < count[0]; i++) 
        outlatlon[i] = latlon[offset[0]+i*step[0]];
    return true;

}
// Subset of latitude and longitude to follow the parameters 
// from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathDimMapField::Field2DSubset (T * outlatlon,
                                                  const int ,
                                                  const int minordim,
                                                  T * latlon,
                                                  const int32 * offset,
                                                  const int32 * count,
                                                  const int32 * step)
{
    int	i = 0;
    int j = 0; 

    // do subsetting
    // Find the correct index
    int	dim0count = count[0];
    int dim1count = count[1];

    vector<int>dim0index(dim0count);
    vector<int>dim1index(dim1count);

    for (i = 0; i < count[0]; i++) // count[0] is the least changing dimension
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    int k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = *(latlon + (dim0index[i] * minordim) + dim1index[j]);
            k++;
        }
    }
    return true;
}

// Subsetting the field  to follow the parameters from the DAP expression constraint
template < class T >
bool HDFEOS2ArraySwathDimMapField::Field3DSubset (T * outlatlon,
                                                  const vector<int32>& newdims,
                                                  T * latlon,
                                                  const int32 * offset,
                                                  const int32 * count,
                                                  const int32 * step)
{
    if (newdims.size() !=3) 
        throw BESInternalError("The rank must be 3 to call this function.",__FILE__, __LINE__);
    int i = 0;
    int j = 0;
    int k = 0;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim2count = count[2];

    vector<int> dim0index(dim0count);
    vector<int> dim1index(dim1count);
    vector<int> dim2index(dim2count);

    for (i = 0; i < count[0]; i++) // count[0] is the least changing dimension
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    for (k = 0; k < count[2]; k++)
        dim2index[k] = offset[2] + k * step[2];

    // Now assign the subsetting data
    int l = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            for (k =0; k < count[2]; k++) {
                outlatlon[l] = *(latlon + (dim0index[i] * newdims[1] * newdims[2]) + (dim1index[j] * newdims[2])+ dim2index[k]);
                l++;
            }
        }
    }
    return true;
}

int 
HDFEOS2ArraySwathDimMapField::write_dap_data_scale_comp(int32 swathid, 
                                                        int nelms, 
                                                        vector<int32>& offset32, 
                                                        vector<int32>& count32,
                                                        vector<int32>& step32) {


    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

  // Define function pointers to handle both grid and swath
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);


    fieldinfofunc = SWfieldinfo;

    int32 attrtype = -1;
    int32 attrcount = -1;
    int32 attrindex = -1;

    int32 scale_factor_attr_index = -1;
    int32 add_offset_attr_index =-1;

    float scale=1;
    float offset2=0;
    float fillvalue = 0.;

    if (sotype!=SOType::DEFAULT_CF_EQU) {

        // Obtain attribute values.
        int32 sdfileid = -1;

        if (true == isgeofile || false == check_pass_fileid_key) {
            sdfileid = SDstart(filename.c_str (), DFACC_READ);
            if (FAIL == sdfileid) {
                string msg = "Cannot Start the SD interface for the file "+ filename +".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
        }
        else
            sdfileid = sdfd;

        int32 sdsindex = -1;
        int32 sdsid = -1; 

        sdsindex = SDnametoindex(sdfileid, fieldname.c_str());
        if (FAIL == sdsindex) {
            if (true == isgeofile || false == check_pass_fileid_key) 
                SDend(sdfileid);
            string msg = "Cannot obtain the index of " + fieldname +".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        sdsid = SDselect(sdfileid, sdsindex);
        if (FAIL == sdsid) {
            if (true == isgeofile || false == check_pass_fileid_key)
                SDend(sdfileid);
            string msg = "Cannot obtain the SDS ID  of " + fieldname + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        char attrname[H4_MAX_NC_NAME + 1];
        vector<char> attrbuf;
        vector<char> attrbuf2;

        scale_factor_attr_index = SDfindattr(sdsid, "scale_factor");
        if (scale_factor_attr_index!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, scale_factor_attr_index, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                SDendaccess(sdsid);
                if (true == isgeofile || false == check_pass_fileid_key)
                    SDend(sdfileid);
                string msg = "Attribute 'scale_factor' in " + fieldname + " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            attrbuf.clear();
            attrbuf.resize(DFKNTsize(attrtype)*attrcount);
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

            // Appears that the assumption for the datatype of scale_factor 
            // is either float or double
            // for this type of MODIS files. So far we haven't found any problems. 
            // Maybe this is okay.
            // KY 2013-12-19
            switch(attrtype)
            {
#define GET_SCALE_FACTOR_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)attrbuf.data(); \
            scale = (float)tmpvalue; \
        } \
        break;
                    GET_SCALE_FACTOR_ATTR_VALUE(FLOAT32, float)
                    GET_SCALE_FACTOR_ATTR_VALUE(FLOAT64, double)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);

            }
            
#undef GET_SCALE_FACTOR_ATTR_VALUE
        }

        add_offset_attr_index = SDfindattr(sdsid, "add_offset");
        if (add_offset_attr_index!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, add_offset_attr_index, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                SDendaccess(sdsid);
                if (true == isgeofile || false == check_pass_fileid_key)
                    SDend(sdfileid);
                string msg = "Attribute 'add_offset' information of  " + fieldname;
                msg += " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            attrbuf.clear();
            attrbuf.resize(DFKNTsize(attrtype)*attrcount);
            ret = SDreadattr(sdsid, add_offset_attr_index, (VOIDP)attrbuf.data());
            if (ret==FAIL)
            {
                SDendaccess(sdsid);
                if (true == isgeofile || false == check_pass_fileid_key)
                    SDend(sdfileid);
                string msg = "Attribute 'add_offset' of " + fieldname;
                msg += " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            switch(attrtype)
                {
#define GET_ADD_OFFSET_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)attrbuf.data(); \
            offset2 = (float)tmpvalue; \
        } \
        break;
                    GET_ADD_OFFSET_ATTR_VALUE(FLOAT32, float)
                    GET_ADD_OFFSET_ATTR_VALUE(FLOAT64, double)
                    default:
                        throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);
                }
#undef GET_ADD_OFFSET_ATTR_VALUE
        }
	
        attrindex = SDfindattr(sdsid, "_FillValue");
        if (sotype!=SOType::DEFAULT_CF_EQU && attrindex!=FAIL)
        {
            intn ret = 0;
            ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                SDendaccess(sdsid);
                if (true == isgeofile || false == check_pass_fileid_key)
                    SDend(sdfileid);
                string msg = "The information of attribute '_FillValue' of "; 
                msg = msg + fieldname + " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            attrbuf.clear();
            attrbuf.resize(DFKNTsize(attrtype)*attrcount);
            ret = SDreadattr(sdsid, attrindex, (VOIDP)attrbuf.data());
            if (ret==FAIL)
            {
                SDendaccess(sdsid);
                if (true == isgeofile || false == check_pass_fileid_key)
                    SDend(sdfileid);
                string msg = "Attribute '_FillValue' in " + fieldname;
                msg += " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
		
            switch(attrtype)
                {
#define GET_FILLVALUE_ATTR_VALUE(TYPE, CAST) \
    case DFNT_##TYPE: \
        { \
            CAST tmpvalue = *(CAST*)attrbuf.data(); \
            fillvalue = (float)tmpvalue; \
        } \
        break;
                    GET_FILLVALUE_ATTR_VALUE(INT8,   int8)
                    GET_FILLVALUE_ATTR_VALUE(INT16,  int16)
                    GET_FILLVALUE_ATTR_VALUE(INT32,  int32)
                    GET_FILLVALUE_ATTR_VALUE(UINT8,  uint8)
                    GET_FILLVALUE_ATTR_VALUE(UINT16, uint16)
                    GET_FILLVALUE_ATTR_VALUE(UINT32, uint32)
                    // Float and double are not considered. Handle them later.
                    default:
                        ;

                }
#undef GET_FILLVALUE_ATTR_VALUE
        }

    // There is a controversy if we need to apply the valid_range to the data, for the time being comment this out.
    // Leave the following code for possible quick references in the future.
#if 0

    //  KY 2013-12-19
        float orig_valid_min = 0.;
        float orig_valid_max = 0.;


        // Retrieve valid_range,valid_range is normally represented as (valid_min,valid_max)
        // for non-CF scale and offset rules, the data is always float. So we only
        // need to change the data type to float.
        attrindex = SDfindattr(sdsid, "valid_range");
        if (attrindex!=FAIL)
        {
            intn ret;
            ret = SDattrinfo(sdsid, attrindex, attrname, &attrtype, &attrcount);
            if (ret==FAIL)
            {
                detachfunc(gridid);
                closefunc(gfid);
                SDendaccess(sdsid);
                SDend(sdfileid);
                string msg = "The information of attribute 'valid_range' of "; 
                msg = msg + fieldname + " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            attrbuf.clear();
            attrbuf.resize(DFKNTsize(attrtype)*attrcount);
            ret = SDreadattr(sdsid, attrindex, (VOIDP)attrbuf.data());
            if (ret==FAIL)
            {
                detachfunc(gridid);
                closefunc(gfid);
                SDendaccess(sdsid);
                SDend(sdfileid);
                string msg = "Attribute 'valid_range' of " + fieldname + " cannot be obtained.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            string attrbuf_str(attrbuf.begin(),attrbuf.end());

            switch(attrtype) {
                
                case DFNT_CHAR:
                {                   
                    // We need to treat the attribute data as characters or string.
                    // So find the separator.
                    size_t found = attrbuf_str.find_first_of(",");
                    size_t found_from_end = attrbuf_str.find_last_of(",");
                        
                    if (string::npos == found)
                        throw BESInternalError("Should find the separator ','.",__FILE__,__LINE__);
                    if (found != found_from_end)
                        throw BESInternalError("Only one separator , should be available.",__FILE__,__LINE__);

                    orig_valid_min = atof((attrbuf_str.substr(0,found)).c_str());
                    orig_valid_max = atof((attrbuf_str.substr(found+1)).c_str());
                    
                }
                break;

                case DFNT_INT8:
                {
                    if (2 == temp_attrcount) {
                            orig_valid_min = (float)attrbuf[0];
                            orig_valid_max = (float)attrbuf[1]; 
                    }
                    else 
                        throw BESInternalError("The number of attribute count should be greater than 1.",__FILE__,__LINE__);
                }
                break;

                case DFNT_UINT8:
                case DFNT_UCHAR:
                {
                    if (temp_attrcount != 2) 
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT8 type.",__FILE__,__LINE__);

                    unsigned char* temp_valid_range = (unsigned char *)attrbuf.data();
                    orig_valid_min = (float)(temp_valid_range[0]);
                    orig_valid_max = (float)(temp_valid_range[1]);
                }
                break;

                case DFNT_INT16:
                {
                    if (temp_attrcount != 2) 
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT16 type.",__FILE__,__LINE__);

                    short* temp_valid_range = (short *)attrbuf.data();
                    orig_valid_min = (float)(temp_valid_range[0]);
                    orig_valid_max = (float)(temp_valid_range[1]);
                }
                break;

                case DFNT_UINT16:
                {
                    if (temp_attrcount != 2) 
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT16 type.",__FILE__,__LINE__);

                    unsigned short* temp_valid_range = (unsigned short *)attrbuf.data();
                    orig_valid_min = (float)(temp_valid_range[0]);
                    orig_valid_max = (float)(temp_valid_range[1]);
                }
                break;
 
                case DFNT_INT32:
                {
                    if (temp_attrcount != 2) 
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_INT32 type.",__FILE__,__LINE__);

                    int* temp_valid_range = (int *)attrbuf.data();
                    orig_valid_min = (float)(temp_valid_range[0]);
                    orig_valid_max = (float)(temp_valid_range[1]);
                }
                break;

                case DFNT_UINT32:
                {
                    if (temp_attrcount != 2) 
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_UINT32 type.",__FILE__,__LINE__);

                    unsigned int* temp_valid_range = (unsigned int *)attrbuf.data();
                    orig_valid_min = (float)(temp_valid_range[0]);
                    orig_valid_max = (float)(temp_valid_range[1]);
                }
                break;

                case DFNT_FLOAT32:
                {
                    if (temp_attrcount != 2) 
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_FLOAT32 type.",__FILE__,__LINE__);

                    float* temp_valid_range = (float *)attrbuf.data();
                    orig_valid_min = temp_valid_range[0];
                    orig_valid_max = temp_valid_range[1];
                }
                break;

                case DFNT_FLOAT64:
                {
                    if (temp_attrcount != 2)
                        throw BESInternalError("The number of attribute count should be 2 for the DFNT_FLOAT64 type.",__FILE__,__LINE__);
                    double* temp_valid_range = (double *)attrbuf.data();

                    // Notice: this approach will lose precision and possibly overflow. Fortunately it is not a problem for MODIS data.
                    // This part of code may not be called. If it is called, mostly the value is within the floating-point range.
                    // KY 2013-01-29
                    orig_valid_min = temp_valid_range[0];
                    orig_valid_max = temp_valid_range[1];
                }
                break;
                default:
                    throw BESInternalError("Unsupported data type.",__FILE__,__LINE__);
            }
        }

#endif


	
        SDendaccess(sdsid);
        if (true == isgeofile || false == check_pass_fileid_key)
            SDend(sdfileid);
    }

    // According to our observations, it seems that MODIS products ALWAYS 
    // use the "scale" factor to make bigger values smaller.
    // So for MODIS_MUL_SCALE products, if the scale of some variable is greater than 1, 
    // it means that for this variable, the MODIS type for this variable may be MODIS_DIV_SCALE.
    // For the similar logic, we may need to change MODIS_DIV_SCALE to 
    // MODIS_MUL_SCALE and MODIS_EQ_SCALE to MODIS_DIV_SCALE.
    // We indeed find such a case. HDF-EOS2 Grid MODIS_Grid_1km_2D of MOD(or MYD)09GA is 
    // a MODIS_EQ_SCALE.
    // However,
    // the scale_factor of the variable Range_1 in the MOD09GA product is 25. 
    // According to our observation, this variable should be MODIS_DIV_SCALE.
    // We verify this is true according to MODIS 09 product document
    // http://modis-sr.ltdri.org/products/MOD09_UserGuide_v1_3.pdf.
    // Since this conclusion is based on our observation, we would like to add 
    // a BESlog to detect if we find
    // the similar cases so that we can verify with the corresponding product 
    // documents to see if this is true.
    // 
    // More information, 
    // We just verified with the MOD09 data producer, the scale_factor for Range_1 
    // and Range_c is 25 but the
    // equation is still multiplication instead of division. So we have to make this 
    // as a special case that we don't want to change the scale and offset settings.
    // KY 2014-01-13
    // However, since this function only handles swath and we haven't found an outlier 
    // for a swath case, we still keep the old ways.


    if (SOType::MODIS_EQ_SCALE == sotype || SOType::MODIS_MUL_SCALE == sotype) {
        if (scale > 1) {
            sotype = SOType::MODIS_DIV_SCALE;
            INFO_LOG("The field " + fieldname + " scale factor is "
             + std::to_string(scale) + ". But the original scale factor type is MODIS_MUL_SCALE or MODIS_EQ_SCALE. "
             + " Now change it to MODIS_DIV_SCALE. ");
        }
    }
    
    if (SOType::MODIS_DIV_SCALE == sotype) {
        if (scale < 1) {
            sotype = SOType::MODIS_MUL_SCALE;
            INFO_LOG("The field " + fieldname + " scale factor is " + std::to_string(scale) +
                + " But the original scale factor type is MODIS_DIV_SCALE. "
                + " Now change it to MODIS_MUL_SCALE.");
        }
    }


#define RECALCULATE(CAST, DODS_CAST, VAL) \
{ \
    bool change_data_value = false; \
    if (sotype!=SOType::DEFAULT_CF_EQU) \
    { \
        if (scale_factor_attr_index!=FAIL && !(scale==1 && offset2==0)) \
        { \
            vector<float>tmpval; \
            tmpval.resize(nelms); \
            CAST tmptr = (CAST)VAL; \
            for(int l=0; l<nelms; l++) \
                tmpval[l] = (float)tmptr[l]; \
            float temp_scale = scale; \
            float temp_offset = offset2; \
            if (sotype==SOType::MODIS_MUL_SCALE) \
                temp_offset = -1. *offset2*temp_scale;\
            else if (sotype==SOType::MODIS_DIV_SCALE) \
            {\
                temp_scale = 1/scale; \
                temp_offset = -1. *temp_scale *offset2;\
            }\
            for(int l=0; l<nelms; l++) \
                if (attrindex!=FAIL && ((float)tmptr[l])!=fillvalue) \
                        tmpval[l] = tmptr[l]*temp_scale + temp_offset; \
                change_data_value = true; \
                set_value((dods_float32 *)tmpval.data(), nelms); \
        } \
    } \
    if (!change_data_value) \
    { \
        set_value ((DODS_CAST)VAL, nelms); \
    } \
}

    // tmp_rank and tmp_dimlist are two dummy variables that are 
    // only used when calling fieldinfo.
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
    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        string msg = "Field " + fieldname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    vector<int32> newdims;
    newdims.resize(rank);

    // Loop through the data type. 
    switch (field_dtype) {

        case DFNT_INT8:
        {
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            vector < int8 > total_val8;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val8, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);

            vector<int8>val8;
            val8.resize(nelms);

            FieldSubset (val8.data(), newdims, total_val8.data(),
                         offset32.data(), count32.data(), step32.data());

#ifndef SIGNED_BYTE_TO_INT32
            RECALCULATE(int8*, dods_byte*, val8.data());
#else
            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val8[counter]);

            RECALCULATE(int32*, dods_int32*, newval.data())
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            vector < uint8 > total_val_u8;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u8, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<uint8>val_u8;
            val_u8.resize(nelms);

            FieldSubset (val_u8.data(), newdims, total_val_u8.data(),
                         offset32.data(), count32.data(), step32.data());
            RECALCULATE(uint8*, dods_byte*, val_u8.data())
        }
            break;
        case DFNT_INT16:
        {    
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            vector < int16 > total_val16;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val16, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }  

            check_num_elems_constraint(nelms,newdims);
            vector<int16>val16;
            val16.resize(nelms);

            FieldSubset (val16.data(), newdims, total_val16.data(),
                         offset32.data(), count32.data(), step32.data());

            RECALCULATE(int16*, dods_int16*, val16.data())
        }
            break;
        case DFNT_UINT16:
        {
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            vector < uint16 > total_val_u16;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u16, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<uint16>val_u16;
            val_u16.resize(nelms);

            FieldSubset (val_u16.data(), newdims, total_val_u16.data(),
                         offset32.data(), count32.data(), step32.data());
            RECALCULATE(uint16*, dods_uint16*, val_u16.data())

        }
            break;
        case DFNT_INT32:
        {
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            vector < int32 > total_val32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val32, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<int32> val32;
            val32.resize(nelms);

            FieldSubset (val32.data(), newdims, total_val32.data(),
                         offset32.data(), count32.data(), step32.data());

            RECALCULATE(int32*, dods_int32*, val32.data())
        }
            break;
        case DFNT_UINT32:
        {
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            // Notice the total_val_u32 is allocated inside the GetFieldValue.
            vector < uint32 > total_val_u32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u32, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<uint32>val_u32;
            val_u32.resize(nelms);

            FieldSubset (val_u32.data(), newdims, total_val_u32.data(),
                         offset32.data(), count32.data(), step32.data());
            RECALCULATE(uint32*, dods_uint32*, val_u32.data())
 
        }
            break;
        case DFNT_FLOAT32:
        {
            // Obtaining the total value and interpolating the data 
            // according to dimension map
            vector < float32 > total_val_f32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f32, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<float32>val_f32;
            val_f32.resize(nelms);

            FieldSubset (val_f32.data(), newdims, total_val_f32.data(),
                         offset32.data(), count32.data(), step32.data());
            RECALCULATE(float32*, dods_float32*, val_f32.data())
        }
            break;
        case DFNT_FLOAT64:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < float64 > total_val_f64;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f64, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + " cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<float64>val_f64;
            val_f64.resize(nelms);
            FieldSubset (val_f64.data(), newdims, total_val_f64.data(),
                         offset32.data(), count32.data(), step32.data());
            RECALCULATE(float64*, dods_float64*, val_f64.data())
 
        }
            break;
        default:
        {
            throw BESInternalError("Unsupported data type.",__FILE__, __LINE__);
        }
    }

    return 0;
}

int
HDFEOS2ArraySwathDimMapField::write_dap_data_disable_scale_comp(int32 swathid, 
                                                                int nelms,  
                                                                vector<int32>& offset32,
                                                                vector<int32>& count32, 
                                                                vector<int32>& step32) {

     // Define function pointers to handle both grid and swath
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);

    fieldinfofunc = SWfieldinfo;


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
    r = fieldinfofunc (swathid, const_cast < char *>(fieldname.c_str ()),
        &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        string msg = "Field " + fieldname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    // int32 majordimsize, minordimsize;
    vector<int32> newdims;
    newdims.resize(rank);

    // Loop through the data type. 
    switch (field_dtype) {

        case DFNT_INT8:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < int8 > total_val8;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val8, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);

            vector<int8>val8;
            val8.resize(nelms);
            FieldSubset (val8.data(), newdims, total_val8.data(),
                         offset32.data(), count32.data(), step32.data());


#ifndef SIGNED_BYTE_TO_INT32
           set_value((dods_byte*)val8.data(),nelms);
#else
            vector<int32>newval;
            newval.resize(nelms);

            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (val8[counter]);

            set_value ((dods_int32 *) newval.data(), nelms);
#endif
        }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < uint8 > total_val_u8;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u8, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<uint8>val_u8;
            val_u8.resize(nelms);

            FieldSubset (val_u8.data(), newdims, total_val_u8.data(),
                         offset32.data(), count32.data(), step32.data());
            set_value ((dods_byte *) val_u8.data(), nelms);
        }
            break;
        case DFNT_INT16:
        {    
            // Obtaining the total value and interpolating the data according to dimension map
            vector < int16 > total_val16;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val16, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }  

            check_num_elems_constraint(nelms,newdims);
            vector<int16>val16;
            val16.resize(nelms);

            FieldSubset (val16.data(), newdims, total_val16.data(),
                         offset32.data(), count32.data(), step32.data());

            set_value ((dods_int16 *) val16.data(), nelms);
        }
            break;
        case DFNT_UINT16:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < uint16 > total_val_u16;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u16, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<uint16>val_u16;
            val_u16.resize(nelms);

            FieldSubset (val_u16.data(), newdims, total_val_u16.data(),
                         offset32.data(), count32.data(), step32.data());
            set_value ((dods_uint16 *) val_u16.data(), nelms);

        }
            break;
        case DFNT_INT32:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < int32 > total_val32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val32, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<int32> val32;
            val32.resize(nelms);

            FieldSubset (val32.data(), newdims, total_val32.data(),
                         offset32.data(), count32.data(), step32.data());
            set_value ((dods_int32 *) val32.data(), nelms);
        }
            break;
        case DFNT_UINT32:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            // Notice the total_val_u32 is allocated inside the GetFieldValue.
            vector < uint32 > total_val_u32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_u32, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<uint32>val_u32;
            val_u32.resize(nelms);

            FieldSubset (val_u32.data(), newdims, total_val_u32.data(),
                         offset32.data(), count32.data(), step32.data());
            set_value ((dods_uint32 *) val_u32.data(), nelms);
 
        }
            break;
        case DFNT_FLOAT32:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < float32 > total_val_f32;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f32, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<float32>val_f32;
            val_f32.resize(nelms);

            FieldSubset (val_f32.data(), newdims, total_val_f32.data(),
                         offset32.data(), count32.data(), step32.data());

            set_value ((dods_float32 *) val_f32.data(), nelms);
        }
            break;
        case DFNT_FLOAT64:
        {
            // Obtaining the total value and interpolating the data according to dimension map
            vector < float64 > total_val_f64;
            r = GetFieldValue (swathid, fieldname, dimmaps, total_val_f64, newdims);
            if (r != 0) {
                string msg = "Field " + fieldname + "cannot be read.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

            check_num_elems_constraint(nelms,newdims);
            vector<float64>val_f64;
            val_f64.resize(nelms);
            FieldSubset (val_f64.data(), newdims, total_val_f64.data(),
                         offset32.data(), count32.data(), step32.data());
            set_value ((dods_float64 *) val_f64.data(), nelms);
 
        }
            break;
        default:
        {
            throw BESInternalError("Unsupported data type. ", __FILE__, __LINE__);
        }
    }

    return 0;
}

void HDFEOS2ArraySwathDimMapField::close_fileid(const int32 swfileid, const int32 sdfileid) {


    if (true == isgeofile || false == HDF4RequestHandler::get_pass_fileid()) {

        if (sdfileid != -1)
            SDend(sdfileid);

        if (swfileid != -1)
            SWclose(swfileid);
        
    }

}

bool HDFEOS2ArraySwathDimMapField::check_num_elems_constraint(const int num_elems,
                                                              const vector<int32>&newdims) const {

    int total_dim_size = 1;
    for (int i =0;i<rank;i++)
        total_dim_size*=newdims[i];

    if (total_dim_size < num_elems) {
        ostringstream eherr;
        eherr << "The total number of elements for the array " << total_dim_size
              << "is less than the user-requested number of elements " << num_elems;
        string msg = "The total number of elements for the array " + to_string(total_dim_size);
        msg = msg + " is less than the user-requested number of elements " + to_string(num_elems) + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    return false;

}
#endif
