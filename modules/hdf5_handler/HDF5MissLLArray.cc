/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
// Currently, it provides the missing latitude,longitude fields for the HDF-EOS5 files for the default option.
/////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include "HDF5MissLLArray.h"
#include "HDF5CFUtil.h"
#if 0
#include "HDF5RequestHandler.h"
#endif

using namespace std;
using namespace libdap;

BaseType *HDF5MissLLArray::ptr_duplicate()
{
    auto HDF5MissLLArray_unique = make_unique<HDF5MissLLArray>(*this);
    return HDF5MissLLArray_unique.release();
}

bool HDF5MissLLArray::read()
{

    BESDEBUG("h5","Coming to HDF5MissLLArray read "<<endl);
    
    if (g_info.projection == HE5_GCTP_GEO) 
        read_data_geo();
    else 
        read_data_non_geo();

    return true;
}


bool HDF5MissLLArray::read_data_non_geo() {

    int64_t nelms = -1;
    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;

    if (rank <=  0)  {
       string msg = "The number of dimension of this variable should be greater than 0.";
       throw InternalErr (__FILE__, __LINE__, msg);
    }
    else {
         offset.resize(rank);
         count.resize(rank);
         step.resize(rank);
         nelms = format_constraint (offset.data(), step.data(), count.data());
    }

    if (nelms <= 0) { 
       string msg = "The number of elments is negative.";
       throw InternalErr (__FILE__, __LINE__, msg);
    }

    int64_t total_elms = g_info.xdim_size * g_info.ydim_size;
    if (total_elms > DODS_INT_MAX) {
       string msg = "Currently we cannot calculate lat/lon that is greater than 2G for HDF-EOS5.";
       throw InternalErr (__FILE__, __LINE__, msg);
    }

    if(g_info.ydim_size <=0 || g_info.xdim_size <=0) {
        string msg = "The number of elments at each dimension should be greater than 0.";
	throw InternalErr (__FILE__, __LINE__, msg);
    }
        
    vector<size_t>pos(rank,0);
    for (int i = 0; i< rank; i++)
        pos[i] = offset[i];

    vector<size_t>dimsizes;
    dimsizes.push_back(g_info.ydim_size);
    dimsizes.push_back(g_info.xdim_size);

    double upleft[2];
    double lowright[2];
    vector<int>rows;
    vector<int>cols;
    vector<double>lon;
    vector<double>lat;
    rows.resize(g_info.xdim_size * g_info.ydim_size);
    cols.resize(g_info.xdim_size * g_info.ydim_size);
    lon.resize(g_info.xdim_size * g_info.ydim_size);
    lat.resize(g_info.xdim_size * g_info.ydim_size);

    upleft[0] = g_info.point_left;
    upleft[1] = g_info.point_upper;
    lowright[0] = g_info.point_right;
    lowright[1] = g_info.point_lower;

    int r = -1;

    int k = 0;
    for (int j = 0; j < g_info.ydim_size; ++j) {
        for (int i = 0; i < g_info.xdim_size; ++i) {
            rows[k] = j;
            cols[k] = i;
            ++k;
        }
    }

    BESDEBUG("h5", " Before calling GDij2ll, check all projection parameters. "  << endl);
    BESDEBUG("h5", " eos5_projcode is "  << g_info.projection <<endl);
    BESDEBUG("h5", " eos5_zone is "  << g_info.zone <<endl);
    BESDEBUG("h5", " eos5_params[0] is "  << g_info.param[0] <<endl);
    BESDEBUG("h5", " eos5_params[1] is "  << g_info.param[1] <<endl);
    BESDEBUG("h5", " eos5_sphere is "  << g_info.sphere <<endl);
    BESDEBUG("h5", " xdimsize is "  << g_info.xdim_size <<endl);
    BESDEBUG("h5", " ydimsize is "  << g_info.ydim_size <<endl);
    BESDEBUG("h5", " eos5_pixelreg is "  << g_info.pixelregistration <<endl);
    BESDEBUG("h5", " eos5_origin is "  << g_info.gridorigin <<endl);
    BESDEBUG("h5", " upleft[0] is "  << upleft[0] <<endl);
    BESDEBUG("h5", " upleft[1] is "  << upleft[1] <<endl);
    BESDEBUG("h5", " lowright[0] is "  << lowright[0] <<endl);
    BESDEBUG("h5", " lowright[1] is "  << lowright[1] <<endl);
    
 
    // Calculate Lat/lon by using GCTP
    r = GDij2ll (g_info.projection, g_info.zone, g_info.param,g_info.sphere, g_info.xdim_size, g_info.ydim_size, upleft, lowright,
                 g_info.xdim_size * g_info.ydim_size, rows.data(), cols.data(), lon.data(), lat.data(), g_info.pixelregistration, g_info.gridorigin);
    if (r != 0) {
        string msg = "Cannot calculate grid latitude and longitude.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    BESDEBUG("h5", " The first value of lon is "  << lon[0] <<endl);
    BESDEBUG("h5", " The first value of lat is "  << lat[0] <<endl);

    
    if(is_lat) {
        if(total_elms == nelms)
            set_value_ll(lat.data(),total_elms);
        else {
            vector<double>val;
            subset<double>(
                           lat.data(),
                           rank,
                           dimsizes,
                           offset.data(),
                           step.data(),
                           count.data(),
                           &val,
                           pos,
                           0);
            set_value_ll(val.data(),nelms);
        }
    }
    else {

        if(total_elms == nelms)
            set_value_ll(lon.data(),total_elms);
        else {
            vector<double>val;
            subset<double>(
                           lon.data(),
                           rank,
                           dimsizes,
                           offset.data(),
                           step.data(),
                           count.data(),
                           &val,
                           pos,
                           0);
            set_value_ll(val.data(),nelms);
        }
    }
    return true;

}

bool HDF5MissLLArray::read_data_geo(){

    BESDEBUG("h5","HDF5MissLLArray: Coming to read_data_geo "<<endl);
    int64_t nelms = -1;
    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;


    if (rank <=  0) {
        string msg = "The number of dimension of this variable should be greater than 0.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);
    nelms = format_constraint (offset.data(), step.data(), count.data());

    if (nelms <= 0 || nelms >DODS_INT_MAX) {
        string msg = "The number of elements for geographic lat/lon is negative or greater than 2G.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    vector<float>val;
    val.resize(nelms);

    if (is_lat)
        read_data_geo_lat(nelms, offset, step, val) ;
    else
        read_data_geo_lon(nelms, offset,  step, val) ;
    set_value_ll(val.data(), nelms);
    
 
    return true;
}

void HDF5MissLLArray::read_data_geo_lat(int64_t nelms, const vector<int64_t> &offset,
                                          const vector<int64_t> &step, vector<float> &val) const
{

    float start = 0.0;
    float end = 0.0;

    if (HE5_HDFE_GD_UL == g_info.gridorigin || HE5_HDFE_GD_UR == g_info.gridorigin) {
    
        start = (float)(g_info.point_upper);
        end   = (float)(g_info.point_lower);
    
    }
    else {// (gridorigin == HE5_HDFE_GD_LL || gridorigin == HE5_HDFE_GD_LR)
    
        start = (float)(g_info.point_lower);
        end = (float)(g_info.point_upper);
    }
    
    if (g_info.ydim_size <=0){
        string msg = "The number of elements should be greater than 0.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }
    
    float lat_step = (end - start) /(float)(g_info.ydim_size);
    
    if ( HE5_HDFE_CENTER == g_info.pixelregistration ) {
        for (int i = 0; i < nelms; i++)
            val[i] = (((float)(offset[0]+i*step[0]) + 0.5F) * lat_step + start) / 1000000.0F;
    }
    else { // HE5_HDFE_CORNER
        for (int i = 0; i < nelms; i++)
            val[i] = ((float)(offset[0]+i * step[0])*lat_step + start) / 1000000.0F;
    }

}

void HDF5MissLLArray::read_data_geo_lon(int64_t nelms, const vector<int64_t> &offset, const vector<int64_t> &step,
                                        vector<float> &val) const {

    float start = 0.0;
    float end   = 0.0;

    if (HE5_HDFE_GD_UL == g_info.gridorigin || HE5_HDFE_GD_LL == g_info.gridorigin) {
        start = (float)(g_info.point_left);
        end   = (float)(g_info.point_right);
    }
    else {// (gridorigin == HE5_HDFE_GD_UR || gridorigin == HE5_HDFE_GD_LR)
        start = (float)(g_info.point_right);
        end = (float)(g_info.point_left);
    }
    
    if (g_info.xdim_size <=0) {
        string msg = "The number of elements should be greater than 0.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }
    
    float lon_step = (end - start) /(float)(g_info.xdim_size);
    
    if (HE5_HDFE_CENTER == g_info.pixelregistration) {
        for (int i = 0; i < nelms; i++)
        val[i] = (((float)(offset[0] + i *step[0]) + 0.5F) * lon_step + start ) / 1000000.0F;
    }
    else { // HE5_HDFE_CORNER
        for (int i = 0; i < nelms; i++)
        val[i] = ((float)(offset[0]+i*step[0]) * lon_step + start) / 1000000.0F;
    }

}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int64_t 
HDF5MissLLArray::format_constraint (int64_t *offset, int64_t *step, int64_t *count)
{
    int64_t nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int64_t start = dimension_start (p, true);
        int64_t stride = dimension_stride (p, true);
        int64_t stop = dimension_stop (p, true);

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

        BESDEBUG ("h5",
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

//! Getting a subset of a variable
//
//      \param input Input variable
//       \param dim dimension info of the input
//       \param start start indexes of each dim
//       \param stride stride of each dim
//       \param edge count of each dim
//       \param poutput output variable
//      \parrm index dimension index
//       \return 0 if successful. -1 otherwise.
//
template<typename T>
int HDF5MissLLArray::subset(
    void* input,
    int s_rank,
    const vector<size_t> & dim,
    int64_t start[],
    int64_t stride[],
    int64_t edge[],
    vector<T> *poutput,
    vector<size_t>& pos,
    int index)
{
    for(int k=0; k<edge[index]; k++)
    {
        pos[index] = start[index] + k*stride[index];
        if(index+1<s_rank)
            subset(input, s_rank, dim, start, stride, edge, poutput,pos,index+1);
        if(index==s_rank-1)
        {
            size_t cur_pos = INDEX_nD_TO_1D( dim, pos);
            auto tempbuf = (void*)((char*)input+cur_pos*sizeof(T));
            poutput->push_back(*(static_cast<T*>(tempbuf)));
        }
    } // end of for
    return 0;
} // end of template<typename T> static int subset

size_t HDF5MissLLArray::INDEX_nD_TO_1D (const std::vector < size_t > &dims,
                                 const std::vector < size_t > &pos) const {
    //
    //  "int a[10][20][30]  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3)"
    //  "int b[10][2] // &b[1][1] == b + (2*1 + 1)"
    //
    if (dims.size () != pos.size ()) {
        string msg = "Dimension error in INDEX_nD_TO_1D routine.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    size_t sum = 0;
    size_t  start = 1;

    for (const auto &p:pos) {
        size_t m = 1;

        for (size_t j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m *p; 
        start++;
    }
    return sum;
}

