/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF-EOS2 latitude/longtitude  values for a direct DMR-mapping DAP4 response.
// Each SDS will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////


#include "HDFDMRArray_EOS2LL.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"
#include <iostream>
#include <sstream>
#include <libdap/debug.h>
#include <libdap/InternalErr.h>
#include <BESDebug.h>

using namespace std;
using namespace libdap;


bool
HDFDMRArray_EOS2LL::read ()
{

    BESDEBUG("h4","Coming to HDFDMRArray_EOS2LL read "<<endl);
    if (length() == 0)
        return true; 

    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(ll_rank);
    vector<int>count;
    count.resize(ll_rank);
    vector<int>step;
    step.resize(ll_rank);

    string err_msg;

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());

    int32 gridfd = GDopen(const_cast < char *>(filename.c_str()), DFACC_READ);
    if (gridfd <0) {
        ostringstream eherr;
        err_msg = "HDF-EOS: GDopen failed";
        throw InternalErr (__FILE__, __LINE__, err_msg);
    }

    int32 gridid = GDattach(gridfd, const_cast<char *>(gridname.c_str()));
    if (gridid <0) {
        ostringstream eherr;
        err_msg = "HDF-EOS: GDattach failed to attach " + gridname;
        GDclose(gridfd);
        throw InternalErr (__FILE__, __LINE__, err_msg);
    }

    // Declare projection code, zone, etc grid parameters. 
    int32 projcode = -1; 
    int32 zone     = -1;
    int32 sphere   = -1;
    float64 params[16];

    int32 xdim = 0;
    int32 ydim = 0;

    float64 upleft[2];
    float64 lowright[2];

    if (GDprojinfo (gridid, &projcode, &zone, &sphere, params) <0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDprojinfo failed for grid name: " + gridname;
        throw InternalErr (__FILE__, __LINE__, err_msg);
    }

    // Retrieve dimensions and X-Y coordinates of corners
    if (GDgridinfo(gridid, &xdim, &ydim, upleft,
                   lowright) == -1) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDgridinfo failed for grid name: " + gridname;
        throw InternalErr (__FILE__, __LINE__, err_msg);
    }
 
    // Retrieve pixel registration information 
    int32 pixreg = 0; 
    intn r = GDpixreginfo (gridid, &pixreg);
    if (r != 0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDpixreginfo failed for grid name: " + gridname;
        throw InternalErr (__FILE__, __LINE__, err_msg);
    }

    //Retrieve grid pixel origin 
    int32 origin = 0; 
    r = GDorigininfo (gridid, &origin);
    if (r != 0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDoriginfo failed for grid name: " + gridname;
        throw InternalErr (__FILE__, __LINE__, err_msg);
    }

    vector<int32>rows;
    vector<int32>cols;
    vector<float64>lon;
    vector<float64>lat;
    rows.resize(xdim*ydim);
    cols.resize(xdim*ydim);
    lon.resize(xdim*ydim);
    lat.resize(xdim*ydim);


    int i = 0;
    int j = 0;
    int k = 0; 

    /* Fill two arguments, rows and columns */
    // rows             cols
    //   /- xdim  -/      /- xdim  -/
    //   0 0 0 ... 0      0 1 2 ... x
    //   1 1 1 ... 1      0 1 2 ... x
    //       ...              ...
    //   y y y ... y      0 1 2 ... x

    for (k = j = 0; j < ydim; ++j) {
        for (i = 0; i < xdim; ++i) {
            rows[k] = j;
            cols[k] = i;
            ++k;
        }
    }


    r = GDij2ll (projcode, zone, params, sphere, xdim, ydim, upleft, lowright,
                 xdim * ydim, rows.data(), cols.data(), lon.data(), lat.data(), pixreg, origin);

    if (r != 0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "cannot calculate grid latitude and longitude for grid name: " + gridname;
        return false;
    }
    
    if (is_lat) { 
        // Need to check if CEA or GEO
        if (projcode == GCTP_CEA || projcode == GCTP_GEO) {
            vector<float64>out_lat;
            out_lat.resize(ydim);
            j=0;
            for (i =0; i<xdim*ydim;i = i+xdim){
                out_lat[j] = lat[i];
                j++;
            }

            // Need to consider the subset case.
            vector<float64>out_lat_subset;
            out_lat_subset.resize(nelms);
            for (i=0;i<count[0];i++)
                out_lat_subset[i] = out_lat[offset[0]+i*step[0]];
            set_value(out_lat_subset.data(),nelms);
            
        }
        else  {
            vector<float64>out_lat_subset;
            out_lat_subset.resize(nelms);
            LatLon2DSubset(out_lat_subset.data(),xdim,lat.data(),offset.data(),count.data(),step.data());
            set_value(out_lat_subset.data(),nelms);
        }
    }
    else {
        if (projcode == GCTP_CEA || projcode == GCTP_GEO) {
            vector<float64>out_lon;
            out_lon.resize(xdim);
            for (i =0; i<xdim;i++)
                out_lon[i] = lon[i];

            // Need to consider the subset case.
            vector<float64>out_lon_subset;
            out_lon_subset.resize(nelms);
            for (i=0;i<count[0];i++)
                out_lon_subset[i] = out_lon[offset[0]+i*step[0]];
            set_value(out_lon_subset.data(),nelms);
 
        }
        else { 
            vector<float64>out_lon_subset;
            out_lon_subset.resize(nelms);
            LatLon2DSubset(out_lon_subset.data(),xdim,lon.data(),offset.data(),count.data(),step.data());
            set_value(out_lon_subset.data(),nelms);
        }

    }
    set_read_p(true);
 
    return true;
}
// Standard way to pass the coordinates of the subsetted region from the client to the handlers
// Returns the number of elements 
int
HDFDMRArray_EOS2LL::format_constraint (int *offset, int *step, int *count)
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

// Map the subset of the lat/lon buffer to the corresponding 2D array.
template<class T> void
HDFDMRArray_EOS2LL::LatLon2DSubset (T * outlatlon, 
                                          int minordim, T * latlon,
                                          const int * offset, const int * count,
                                          const int * step) const
{               
    int i = 0;      
    int j = 0;          
        
    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim0index[dim0count];
    int dim1index[dim1count];
            
    for (i = 0; i < count[0]; i++)      // count[0] is the least changing dimension
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
}



