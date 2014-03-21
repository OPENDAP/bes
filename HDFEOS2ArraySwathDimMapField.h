/////////////////////////////////////////////////////////////////////////////
// Retrieves the latitude and longitude of  the HDF-EOS2 Swath with dimension map
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

// Currently the handling of swath data fields with dimension maps is the same as other data fields(HDFEOS2Array_RealField.cc etc)
// The reason to keep it in separate is, in theory, that data fields with dimension map may need special handlings.
// So we will leave it here for this release(2010-8), it may be removed in the future. HDFEOS2Array_RealField.cc may be used. 

#ifdef USE_HDFEOS2_LIB
#ifndef HDFEOS2ARRAYSWATHDIMMAPFIELD_H
#define HDFEOS2ARRAYSWATHDIMMAPFIELD_H

#include "Array.h"
using namespace libdap;

#include "HDFCFUtil.h"
#include "HdfEosDef.h"

#include "HDFEOS2EnumType.h"

class HDFEOS2ArraySwathDimMapField:public Array
{
    public:
    HDFEOS2ArraySwathDimMapField (int rank,  const std::string & filename, bool isgeofile, const int sdfd, const int swathfd, const std::string & gridname, const std::string & swathname, const std::string & fieldname, const std::vector < struct dimmap_entry >&dimmaps, SOType sotype, const string & n = "", BaseType * v = 0):
        Array (n, v),
        rank (rank),
        filename(filename),
        isgeofile(isgeofile),
        sdfd(sdfd),
        swfd(swathfd),
        gridname (gridname),
        swathname (swathname), 
        fieldname (fieldname), 
        dimmaps(dimmaps),
        sotype(sotype){
        }
        virtual ~ HDFEOS2ArraySwathDimMapField ()
        {
        }

        // Standard way to pass the coordinates of the subsetted region from the client to the handlers
        int format_constraint (int *cor, int *step, int *edg);

#if 0
        // Obtain Field value
        template < class T > int GetFieldValue (int32, const std::string &,std::vector < struct dimmap_entry >&, std::vector < T > &, std::vector<int32>&);

        // The internal routine to do the interpolation
        template < class T > int _expand_dimmap_field (std::vector < T > *pvals, int32 rank, int32 dimsa[], int dimindex, int32 ddimsize, int32 offset, int32 inc);

        // subsetting routine to ensure the subsetted field to be returned.
        template < class T > bool FieldSubset (T * outlatlon, std::vector<int32>&newdims, T * latlon, int32 * offset, int32 * count, int32 * step);
        // subsetting routine to ensure the subsetted 1D field to be returned.
        template < class T > bool Field1DSubset (T * outlatlon, int majordim, T * latlon, int32 * offset, int32 * count, int32 * step);

        // subsetting routine to ensure the subsetted 2D field to be returned.
        template < class T > bool Field2DSubset (T * outlatlon, int majordim, int minordim, T * latlon, int32 * offset, int32 * count, int32 * step);
         // subsetting routine to ensure the subsetted 2D field to be returned.
        template < class T > bool Field3DSubset (T * outlatlon, std::vector<int32>& newdims, T * latlon, int32 * offset, int32 * count, int32 * step);

#endif
        BaseType *ptr_duplicate ()
        {
            return new HDFEOS2ArraySwathDimMapField (*this);
        }

       // Read the data
       virtual bool read ();

    private:

        // Field array rank
        int rank;

        // HDF-EOS2 file name
        std::string filename;

        bool isgeofile;

        int sdfd;

        int swfd;

        // HDF-EOS2 grid name
        std::string gridname;

        // HDF-EOS2 swath name
        std::string swathname;

        // HDF-EOS2 field name
        std::string fieldname;

        // Swath dimmap info.
        std::vector < struct dimmap_entry >dimmaps;

        // MODIS scale and offset type
        // Some MODIS files don't use the CF linear equation y = scale * x + offset,
        // the scaletype distinguishs products following different scale and offset rules. 
        SOType sotype;

        int write_dap_data_scale_comp(int32 swid, const int nelms,   std::vector<int32> &offset32, std::vector<int32> &count32, std::vector<int32> &step32);
        int write_dap_data_disable_scale_comp(int32 swid, const int nelms,  std::vector<int32> &offset32,std::vector<int32> &count32,std::vector<int32> &step32);
        // Obtain Field value
        template < class T > int GetFieldValue (int32, const std::string &,std::vector < struct dimmap_entry >&, std::vector < T > &, std::vector<int32>&);

        // The internal routine to do the interpolation
        template < class T > int _expand_dimmap_field (std::vector < T > *pvals, int32 rank, int32 dimsa[], int dimindex, int32 ddimsize, int32 offset, int32 inc);

        // subsetting routine to ensure the subsetted field to be returned.
        template < class T > bool FieldSubset (T * outlatlon, const std::vector<int32>&newdims, T * latlon, int32 * offset, int32 * count, int32 * step);
        // subsetting routine to ensure the subsetted 1D field to be returned.
        template < class T > bool Field1DSubset (T * outlatlon, const int majordim, T * latlon, int32 * offset, int32 * count, int32 * step);

        // subsetting routine to ensure the subsetted 2D field to be returned.
        template < class T > bool Field2DSubset (T * outlatlon, const int majordim, const int minordim, T * latlon, int32 * offset, int32 * count, int32 * step);
         // subsetting routine to ensure the subsetted 2D field to be returned.
        template < class T > bool Field3DSubset (T * outlatlon, const std::vector<int32>& newdims, T * latlon, int32 * offset, int32 * count, int32 * step);


        // Close file IDs for swath and SDS. This routine is mostly for cleaning up resources when errors occur.
        void close_fileid(const int32 swid, const int32 sfid);

        // Check is the retrieved number of elements is smaller or equal to the total number of elements in an array.
        bool check_num_elems_constraint(const int num_elems,const std::vector<int32>&newdims);
};


#endif
#endif
