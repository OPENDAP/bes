/////////////////////////////////////////////////////////////////////////////
// retrieves the latitude and longitude of  the HDF-EOS2 Grid
//  Authors:   Kent Yang <myang6@hdfgroup.org> 
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2ArrayGridGeoField.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include "HDFEOS2.h"
#include <BESInternalError.h>
#include <BESDebug.h>
#include "HDFCFUtil.h"

#include "misrproj.h"
#include "errormacros.h"
#include <proj.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "BESH4MCache.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;

#define SIGNED_BYTE_TO_INT32 1

// These two functions are used to handle MISR products with the SOM projections.
extern "C" {
    int inv_init(int insys, int inzone, double *inparm, int indatum, char *fn27, char *fn83, int *iflg, int (*inv_trans[])(double, double, double*, double*));
    int sominv(double y, double x, double *lon, double *lat);
}

bool
HDFEOS2ArrayGridGeoField::read ()
{
    BESDEBUG("h4","Coming to HDFEOS2ArrayGridGeoField read "<<endl);
    if (length() == 0)
        return true; 

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // Currently The latitude and longitude rank from HDF-EOS2 grid must be either 1-D or 2-D.
    // However, For SOM projection the final rank will become 3. 
    if (rank < 1 || rank > 2) {
        throw BESInternalError("The rank of geo field is greater than 2, currently we don't support 3-D lat/lon cases.",__FILE__, __LINE__);
    }

    // MISR SOM file's final rank is 3. So declare a new variable. 
    int final_rank = -1;

    if (true == condenseddim)
        final_rank = 1;
    else if (4 == specialformat)// For the SOM projection, the final output of latitude/longitude rank should be 3.
        final_rank = 3;
    else 
        final_rank = rank;

    vector<int> offset;
    offset.resize(final_rank);
    vector<int> count;
    count.resize(final_rank);
    vector<int> step;
    step.resize(final_rank);

    int nelms = -1;

    // Obtain the number of the subsetted elements
    nelms = format_constraint (offset.data(), step.data(), count.data());

    // Define function pointers to handle both grid and swath Note: in
    // this code, we only handle grid, implementing this way is to
    // keep the same style as the read functions in other files.
    int32 (*openfunc) (char *, intn);
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);

    string datasetname;
    openfunc      = GDopen;
    attachfunc    = GDattach;
    detachfunc    = GDdetach;
    fieldinfofunc = GDfieldinfo;
    readfieldfunc = GDreadfield;
    datasetname   = gridname;

    int32 gfid   = -1;
    int32 gridid = -1;

    /* Declare projection code, zone, etc grid parameters. */
    int32 projcode = -1; 
    int32 zone     = -1;
    int32 sphere   = -1;
    float64 params[16];

    int32 xdim = 0;
    int32 ydim = 0;

    float64 upleft[2];
    float64 lowright[2];

    string cache_fpath="";
    bool use_cache = false;

    // Check if passing file IDs to data
    if (true == check_pass_fileid_key)
        gfid = gridfd;
    else {
        gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (gfid < 0) {
            string msg =  "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }

    // Attach the grid id; make the grid valid.
    gridid = attachfunc (gfid, const_cast < char *>(datasetname.c_str ()));
    if (gridid < 0) {
        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        string msg = "Grid " +  datasetname + " cannot be attached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (false == llflag) {

        // Cache
        // Check if a BES key H4.EnableEOSGeoCacheFile is true, if yes, we will check
        // if a lat/lon cache file exists and if we can read lat/lon from this file.

        if (true == HDF4RequestHandler::get_enable_eosgeo_cachefile()) {

            use_cache = true;
            BESH4Cache *llcache = BESH4Cache::get_instance();

            // Here we have a sanity check for the cached parameters:Cached directory,file prefix and cached directory size.
            // Supposedly Hyrax BES cache feature should check this and the code exists. However, the
            // current hyrax 1.9.7 doesn't provide this feature. KY 2014-10-24
            string bescachedir = HDF4RequestHandler::get_cache_latlon_path();
            string bescacheprefix = HDF4RequestHandler::get_cache_latlon_prefix();
            long cachesize = HDF4RequestHandler::get_cache_latlon_size();

            if (("" == bescachedir)||(""==bescacheprefix)||(cachesize <=0)){
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("Either the cached dir is empty or the prefix is nullptr or the cache size is not set.",__FILE__, __LINE__);
            }
            else {
                struct stat sb;
                if (stat(bescachedir.c_str(),&sb) !=0) {
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg="The cached directory " + bescachedir;
                    msg = msg + " doesn't exist.  ";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                    
                }
                else { 
                     if (true == S_ISDIR(sb.st_mode)) {
                        if (access(bescachedir.c_str(),R_OK|W_OK|X_OK) == -1) {
                            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                            string msg="The cached directory " + bescachedir;
                            msg = msg + " can NOT be read,written or executable.";
                            throw BESInternalError(msg,__FILE__,__LINE__);
                        }
                    }
                    else {
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg="The cached directory " + bescachedir;
                        msg = msg + " is not a directory.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }
                }
            }

            string cache_fname=HDF4RequestHandler::get_cache_latlon_prefix();

            intn r = -1;
            r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
            if (r!=0) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("GDprojinfo failed.",__FILE__, __LINE__);
            }

            // Retrieve dimensions and X-Y coordinates of corners
            if (GDgridinfo(gridid, &xdim, &ydim, upleft,
                           lowright) == -1) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("GDgridinfo failed.",__FILE__, __LINE__);
            }

            // Retrieve pixel registration information 
            int32 pixreg = 0; 
            r = GDpixreginfo (gridid, &pixreg);
            if (r != 0) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("Cannot obtain grid pixel registration info.",__FILE__, __LINE__);
            }

            //Retrieve grid pixel origin 
            int32 origin = 0; 
            r = GDorigininfo (gridid, &origin);
            if (r != 0) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("Cannot obtain grid origin info.",__FILE__, __LINE__);
            }


            // Projection code,zone,sphere,pix,origin
            cache_fname +=HDFCFUtil::get_int_str(projcode);
            cache_fname +=HDFCFUtil::get_int_str(zone);
            cache_fname +=HDFCFUtil::get_int_str(sphere);
            cache_fname +=HDFCFUtil::get_int_str(pixreg);
            cache_fname +=HDFCFUtil::get_int_str(origin);


            // Xdimsize and ydimsize. Although it is rare, need to consider dim major.
            // Whether latlon is ydim,xdim or xdim,ydim.
            if (ydimmajor) { 
                cache_fname +=HDFCFUtil::get_int_str(ydim);
                cache_fname +=HDFCFUtil::get_int_str(xdim);
                    
            }
            else {
                cache_fname +=HDFCFUtil::get_int_str(xdim);
                cache_fname +=HDFCFUtil::get_int_str(ydim);
            }

            // upleft,lowright
            // HDF-EOS upleft,lowright,params use DDDDMMMSSS.6 digits. So choose %17.6f.
            cache_fname +=HDFCFUtil::get_double_str(upleft[0],17,6);
            cache_fname +=HDFCFUtil::get_double_str(upleft[1],17,6);
            cache_fname +=HDFCFUtil::get_double_str(lowright[0],17,6);
            cache_fname +=HDFCFUtil::get_double_str(lowright[1],17,6);

            // According to HDF-EOS2 document, only 13 parameters are used.
            for(int ipar = 0; ipar<13;ipar++) 
                cache_fname+=HDFCFUtil::get_double_str(params[ipar],17,6);
            
            
            cache_fpath = bescachedir + "/"+ cache_fname;
            
            try {
                do  { // do while(0) is a trick to handle break; so ignore solarcloud's warning.
                    int expected_file_size = 0;
                    if (GCTP_CEA == projcode || GCTP_GEO == projcode) 
                        expected_file_size = (xdim+ydim)*sizeof(double);
                    else if (GCTP_SOM == projcode)
                        expected_file_size = xdim*ydim*NBLOCK*2*sizeof(double);
                    else
                        expected_file_size = xdim*ydim*2*sizeof(double);

                    int fd = 0;
                    bool latlon_from_cache = llcache->get_data_from_cache(cache_fpath, expected_file_size,fd);
                    if (false == latlon_from_cache) 
                        break;
 
                    // Get the offset of lat/lon in the cached file. Since lat is stored first and lon is stored second, 
                    // so offset_1d for lat/lon is different.   
                    // We still need to consider different projections. 1D,2D,3D reading.Need also to consider dim major and special format.
                    size_t offset_1d = 0;

                    // Get the count of the lat/lon from the cached file.
                    // Notice the data is read continuously. So starting from the offset point, we have to read all elements until the
                    // last points. The total count for the data point is bigger than the production of count and step.
                    int count_1d = 1;

                    if (GCTP_CEA == projcode|| GCTP_GEO== projcode) { 

                        // It seems that for 1-D lat/lon, regardless of xdimmajor or ydimmajor. It is always Lat[YDim],Lon[XDim], check getCorrectSubset
                        // So we don't need to consider the dimension major case.
                        offset_1d = (fieldtype == 1) ?offset[0] :(ydim+offset[0]);
                        count_1d = 1+(count[0]-1)*step[0];
                    }
                    else if (GCTP_SOM == projcode) {

                        if (true == ydimmajor) {
                            offset_1d = (fieldtype == 1)?(offset[0]*xdim*ydim+offset[1]*xdim+offset[2])
                                                        :(offset[0]*xdim*ydim+offset[1]*xdim+offset[2]+expected_file_size/2/sizeof(double));
                        }
                        else {
                            offset_1d = (fieldtype == 1)?(offset[0]*xdim*ydim+offset[1]*ydim+offset[2])
                                                        :(offset[0]*xdim*ydim+offset[1]*ydim+offset[2]+expected_file_size/2/sizeof(double));
                        }

                        int total_count_dim0  = (count[0]-1)*step[0];
                        int total_count_dim1  = (count[1]-1)*step[1];
                        int total_count_dim2  = (count[2]-1)*step[2];
                        int total_dim1 = (true ==ydimmajor)?ydim:xdim;
                        int total_dim2 = (true ==ydimmajor)?xdim:ydim;

                        // Flatten the 3-D index to 1-D 
                        // This calculation can be generalized from nD to 1D
                        // but since we only use it here. Just keep it this way.
                        count_1d = 1 + total_count_dim0*total_dim1*total_dim2 + total_count_dim1*total_dim2 + total_count_dim2;
                                  
                    }
                    else {// 2-D lat/lon case
                        if (true == ydimmajor) 
                            offset_1d = (fieldtype == 1) ?(offset[0] * xdim + offset[1]):(expected_file_size/2/sizeof(double)+offset[0]*xdim+offset[1]);
                        else 
                            offset_1d = (fieldtype == 1) ?(offset[0] * ydim + offset[1]):(expected_file_size/2/sizeof(double)+offset[0]*ydim+offset[1]);

                        // Flatten the 2-D index to 1-D 
                        int total_count_dim0  = (count[0]-1)*step[0];
                        int total_count_dim1  = (count[1]-1)*step[1];
                        int total_dim1 = (true ==ydimmajor)?xdim:ydim;

                        count_1d = 1 + total_count_dim0*total_dim1 + total_count_dim1;
                    }

                    // Assign a vector to store lat/lon
                    vector<double> latlon_1d;
                    latlon_1d.resize(count_1d);
                    
                    // Read lat/lon from the file. 
                    //int fd;
                    //fd = open(cache_fpath.c_str(),O_RDONLY,0666);
                    // TODO: Use BESLog to report that the cached file cannot be read.
                    off_t fpos = lseek(fd,sizeof(double)*offset_1d,SEEK_SET);
                    if (-1 == fpos) {
                        llcache->unlock_and_close(cache_fpath);
                        llcache->purge_file(cache_fpath);
                        break;
                    }
                    ssize_t read_size = HDFCFUtil::read_vector_from_file(fd,latlon_1d,sizeof(double));
                    llcache->unlock_and_close(cache_fpath);
                    if ((-1 == read_size) || ((size_t)read_size != count_1d*sizeof(double))) {
                        llcache->purge_file(cache_fpath);
                        break;
                    }
                    
                    int total_count = 1;
                    for (int i_rank = 0; i_rank<final_rank;i_rank++) 
                        total_count = total_count*count[i_rank];
                    
                    // We will see if there is a shortcut that the lat/lon is accessed with
                    // one-big-block. Actually this is the most common case. If we find
                    // such a case, we simply read the whole data into the latlon buffer and
                    // send it to BES.
                    if (total_count == count_1d) {
                        set_value((dods_float64*)latlon_1d.data(),nelms);
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        return false;
                    }
 
                    vector<double>latlon;
                    latlon.resize(total_count);

                    // Retrieve latlon according to the projection 
                    if (GCTP_CEA == projcode|| GCTP_GEO== projcode) { 
                        for (int i = 0; i <total_count;i++)
                            latlon[i] = latlon_1d[i*step[0]];

                    }
                    else if (GCTP_SOM == projcode) {
                        
                        for (int i =0; i<count[0];i++)
                            for(int j =0;j<count[1];j++)
                                for(int k=0;k<count[2];k++) 
                                    latlon[i*count[1]*count[2]+j*count[2]+k]=(true == ydimmajor)
                                         ?latlon_1d[i*ydim*xdim*step[0]+j*xdim*step[1]+k*step[2]]
                                         :latlon_1d[i*ydim*xdim*step[0]+j*ydim*step[1]+k*step[2]];
                    }
                    else {
                        for (int i =0; i<count[0];i++)
                            for(int j =0;j<count[1];j++)
                                    latlon[i*count[1]+j]=(true == ydimmajor)
                                    ?latlon_1d[i*xdim*step[0]+j*step[1]]
                                    :latlon_1d[i*ydim*step[0]+j*step[1]];
                    
                    }

                    set_value((dods_float64*)latlon.data(),nelms);
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    return false;

                } while (0);

            }
            catch(...) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw;
            }
        }
    }


    // In this step, if use_cache is true, we always need to write the lat/lon into the cache.
    // SOM projection should be calculated differently. If turning on the lat/lon cache feature, it also needs to be handled  differently. 
    if (specialformat == 4) {// SOM projection
        try {
            CalculateSOMLatLon(gridid, offset.data(), count.data(), step.data(), nelms,cache_fpath,use_cache);
        }
        catch(...) {
            detachfunc(gridid);
            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
            throw;
        }
        detachfunc(gridid);
        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        return false;
    }

    // We define offset,count and step in int32 datatype.
    vector<int32>offset32;
    offset32.resize(rank);

    vector<int32>count32;
    count32.resize(rank);

    vector<int32>step32;
    step32.resize(rank);


    // Obtain offset32 with the correct rank, the rank of lat/lon of
    // GEO and CEA projections in the file may be 2 instead of 1.
    try {
        getCorrectSubset (offset.data(), count.data(), step.data(), offset32.data(), count32.data(), step32.data(), condenseddim, ydimmajor, fieldtype, rank);
    }
    catch(...) {
        detachfunc(gridid);
        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        throw;
    }

    // The following case handles when the lat/lon is not provided.
    if (llflag == false) {		// We have to calculate the lat/lon

        vector<float64>latlon;
        latlon.resize(nelms);

        // If projection code etc. is not retrieved, retrieve them.
        // When specialformat is 3, the file is a file of which the project code is set to -1, we need to skip it. KY 2014-09-11
        if (projcode == -1 && specialformat !=3) {

            
            intn r = 0;
            r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
            if (r!=0) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("GDprojinfo failed.",__FILE__, __LINE__);
            }

            // Retrieve dimensions and X-Y coordinates of corners
            if (GDgridinfo(gridid, &xdim, &ydim, upleft,
                           lowright) == -1) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw BESInternalError("GDgridinfo failed.",__FILE__, __LINE__);
            }
       }

        
       // Handle LAMAZ projection first.
       if (GCTP_LAMAZ == projcode) { 
            try {
                vector<double>latlon_all;
                latlon_all.resize(xdim*ydim*2);

                CalculateLAMAZLatLon(gridid, fieldtype, latlon.data(), latlon_all.data(),offset.data(), count.data(), step.data(), use_cache);
                if (true == use_cache) {

                    BESH4Cache *llcache = BESH4Cache::get_instance();
                    llcache->write_cached_data(cache_fpath,xdim*ydim*2*sizeof(double),latlon_all);

                }
            }
            catch(...) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw;
            }
            set_value ((dods_float64 *) latlon.data(), nelms);
            detachfunc(gridid);
            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
            return false;
        }
         
        // Aim to handle large MCD Grid such as 21600*43200 lat,lon
        if (specialformat == 1) {

            try {
                vector<double>latlon_all;
                latlon_all.resize(xdim+ydim);

                CalculateLargeGeoLatLon(gridid, fieldtype,latlon.data(), latlon_all.data(),offset.data(), count.data(), step.data(), nelms,use_cache);
                if (true == use_cache) {

                    BESH4Cache *llcache = BESH4Cache::get_instance();
                    llcache->write_cached_data(cache_fpath,(xdim+ydim)*sizeof(double),latlon_all);

                }

            }
            catch(...) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw;
            }
            set_value((dods_float64 *)latlon.data(),nelms);
            detachfunc(gridid);
            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
            
            return false;
        }

        // Now handle other cases,note the values will be written after the if-block
        else if (specialformat == 3)	{// Have to provide latitude and longitude by ourselves
            try {
                CalculateSpeLatLon (gridid, fieldtype, latlon.data(), offset32.data(), count32.data(), step32.data());
            }
            catch(...) {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                throw;

            }
            detachfunc(gridid);
            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        }
        else {// This is mostly general case, it will calculate lat/lon with GDij2ll.

            // Cache: check the flag and decide whether to calculate the lat/lon.
            vector<double>latlon_all;

            if (GCTP_GEO == projcode || GCTP_CEA == projcode) 
                latlon_all.resize(xdim+ydim);
            else
                latlon_all.resize(xdim*ydim*2);
 
            CalculateLatLon (gridid, fieldtype, specialformat, latlon.data(),latlon_all.data(),
                             offset32.data(), count32.data(), step32.data(), nelms,use_cache);

            if (true == use_cache) {
                size_t num_item_expected = 0;
                if (GCTP_GEO == projcode || GCTP_CEA == projcode) 
                    num_item_expected = xdim + ydim;
                else
                    num_item_expected = xdim*ydim*2;

                BESH4Cache *llcache = BESH4Cache::get_instance();
                llcache->write_cached_data(cache_fpath,num_item_expected*sizeof(double),latlon_all);

            }

            // The longitude values changed in the cache file is implemented in CalculateLatLon.
            // Some longitude values need to be corrected.
            if (speciallon && fieldtype == 2) 
                CorSpeLon(latlon.data(), nelms);
            detachfunc(gridid);
            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        }

        set_value ((dods_float64 *) latlon.data(), nelms);

        return false;
    }


    // Now lat and lon are stored as HDF-EOS2 fields. We need to read the lat and lon values from the fields.
    int32 tmp_rank = -1;
    vector<int32> tmp_dims;
    tmp_dims.resize(rank);

    char tmp_dimlist[1024];
    int32 type     = -1;
    intn r         = -1;

    // Obtain field info.
    r = fieldinfofunc (gridid, const_cast < char *>(fieldname.c_str ()),
                       &tmp_rank, tmp_dims.data(), &type, tmp_dimlist);

    if (r != 0) {
        detachfunc(gridid);
        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        string msg = "Field " + fieldname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Retrieve dimensions and X-Y coordinates of corners
    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
        detachfunc(gridid);
        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        string msg = "Grid " + datasetname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Retrieve all GCTP projection information
    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r != 0) {
        detachfunc(gridid);
        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
        string msg = "Grid " + datasetname + " projection info. cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (projcode != GCTP_GEO) {	// Just retrieve the data like other fields
        // We have to loop through all datatype and read the lat/lon out.
        switch (type) {
        case DFNT_INT8:
            {
                vector<int8> val;
                val.resize(nelms);
                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                // DAP2 requires the map of SIGNED_BYTE to INT32 if
                // SIGNED_BYTE_TO_INT32 is defined.
#ifndef SIGNED_BYTE_TO_INT32
                set_value ((dods_byte *) val.data(), nelms);
#else
                vector<int32>newval;
                newval.resize(nelms);

                for (int counter = 0; counter < nelms; counter++)
                    newval[counter] = (int32) (val[counter]);

                set_value ((dods_int32 *) newval.data(), nelms);
#endif

            }
            break;
        case DFNT_UINT8:
        case DFNT_UCHAR8:

            {
                vector<uint8> val;
                val.resize(nelms);
                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                set_value ((dods_byte *) val.data(), nelms);

            }
            break;

        case DFNT_INT16:
            {
                vector<int16> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_int16 *) val.data(), nelms);

            }
            break;

        case DFNT_UINT16:
            {
                vector<uint16> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_uint16 *) val.data(), nelms);
            }
            break;

        case DFNT_INT32:
            {
                vector<int32> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_int32 *) val.data(), nelms);
            }
            break;

        case DFNT_UINT32:
            {
                vector<uint32> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }
                set_value ((dods_uint32 *) val.data(), nelms);
            }
            break;

        case DFNT_FLOAT32:
            {
                vector<float32> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_float32 *) val.data(), nelms);
            }
            break;

        case DFNT_FLOAT64:

            {
                vector<float64> val;
                val.resize(nelms);

                r = readfieldfunc (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                if (r != 0) {
                    detachfunc(gridid);
                    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                    string msg = "Field "+ fieldname + "cannot be read.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
                }

                set_value ((dods_float64 *) val.data(), nelms);
            }
            break;

        default: 
            {
                detachfunc(gridid);
                HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                string msg = "Unsupported data type.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }

        }
    }
    else {// Only handle special cases for the Geographic Projection 
        // We find that lat/lon of the geographic projection in some
        // files include fill values. So we recalculate lat/lon based
        // on starting value,step values and number of steps.
        // GDgetfillvalue will return 0 if having fill values. 
        // The other returned value indicates no fillvalue is found inside the lat or lon.
        switch (type) {
        case DFNT_INT8:
            {
                vector<int8> val;
                val.resize(nelms);

                int8 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {
                    int ifillvalue = fillvalue;

                    vector <int8> temp_total_val;
                    temp_total_val.resize(xdim*ydim);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        // Recalculate lat/lon for the geographic projection lat/lon that has fill values
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
			offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }
                }

                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);


#ifndef SIGNED_BYTE_TO_INT32
                set_value ((dods_byte *) val.data(), nelms);
#else
                vector<int32>newval;
                newval.resize(nelms);

                for (int counter = 0; counter < nelms; counter++)
                    newval[counter] = (int32) (val[counter]);

                set_value ((dods_int32 *) newval.data(), nelms);

#endif

            }
            break;

        case DFNT_UINT8:
        case DFNT_UCHAR8:
            {
                vector<uint8> val;
                val.resize(nelms);

                uint8 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);

                if (r == 0) {

                    int ifillvalue =  fillvalue;
                    vector <uint8> temp_total_val;
                    temp_total_val.resize(xdim*ydim);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }
                }
	    
                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);
                set_value ((dods_byte *) val.data(), nelms);

            }
            break;

        case DFNT_INT16:
            {
                vector<int16> val;
                val.resize(nelms);

                int16 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue =  fillvalue;
                    vector <int16> temp_total_val;
                    temp_total_val.resize(xdim*ydim);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }
                }

	    
                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);

                set_value ((dods_int16 *) val.data(), nelms);
            }
            break;

        case DFNT_UINT16:
            {
                uint16  fillvalue = 0;
                vector<uint16> val;
                val.resize(nelms);

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);

                if (r == 0) {

                    int ifillvalue =  fillvalue;

                    vector <uint16> temp_total_val;
                    temp_total_val.resize(xdim*ydim);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }
                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
			offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    } 
                }

                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);

                set_value ((dods_uint16 *) val.data(), nelms);

            }
            break;

        case DFNT_INT32:
            {
                vector<int32> val;
                val.resize(nelms);

                int32 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    int ifillvalue =  fillvalue;

                    vector <int32> temp_total_val;
                    temp_total_val.resize(xdim*ydim);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    } 

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);

                set_value ((dods_int32 *) val.data(), nelms);

            }
            break;

        case DFNT_UINT32:
            {
                vector<uint32> val;
                val.resize(nelms);

                uint32 fillvalue = 0;

                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    // this may cause overflow. Although we don't find the overflow in the NASA HDF products, may still fix it later. KY 2012-8-20
                    int ifillvalue = (int)fillvalue;
                    vector <uint32> temp_total_val;
                    temp_total_val.resize(xdim*ydim);
                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);

                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }
                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
			offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    } 

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);

                set_value ((dods_uint32 *) val.data(), nelms);

            }
            break;

        case DFNT_FLOAT32:
            {
                vector<float32> val;
                val.resize(nelms);

                float32 fillvalue =0;
                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);


                if (r == 0) {
                    // May cause overflow,not find this happen in NASA HDF files, may still need to handle later.
                    // KY 2012-08-20
                    auto ifillvalue =(int)fillvalue;
     
                    vector <float32> temp_total_val;
                    temp_total_val.resize(xdim*ydim);

                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }

                }
                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);

                set_value ((dods_float32 *) val.data(), nelms);

            }
            break;

        case DFNT_FLOAT64:

            {
                vector<float64> val;
                val.resize(nelms);

                float64 fillvalue = 0;
                r = GDgetfillvalue (gridid,
                    const_cast < char *>(fieldname.c_str ()),
                    &fillvalue);
                if (r == 0) {

                    // May cause overflow,not find this happen in NASA HDF files, may still need to handle later.
                    // KY 2012-08-20
                    auto ifillvalue = (int)fillvalue;
                    vector <float64> temp_total_val;
                    temp_total_val.resize(xdim*ydim);
                    r = readfieldfunc(gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        nullptr, nullptr, nullptr, (void *)(temp_total_val.data()));

                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    }

                    try {
                        HandleFillLatLon(temp_total_val, val.data(),ydimmajor,fieldtype,xdim,ydim,offset32.data(),count32.data(),step32.data(),ifillvalue);
                    }
                    catch(...) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        throw;
                    }

                }

                else {

                    r = readfieldfunc (gridid,
                        const_cast < char *>(fieldname.c_str ()),
                        offset32.data(), step32.data(), count32.data(), (void*)(val.data()));
                    if (r != 0) {
                        detachfunc(gridid);
                        HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
                        string msg = "Field "+ fieldname + "cannot be read.";
                        throw BESInternalError(msg,__FILE__,__LINE__);
                    } 

                }
                if (speciallon && fieldtype == 2)
                    CorSpeLon (val.data(), nelms);

                set_value ((dods_float64 *) val.data(), nelms);

            }
            break;
        default:
            detachfunc(gridid);
            HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);
            string msg = "Unsupported data type.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

    }

    r = detachfunc (gridid);
    if (r != 0) {
        string msg = "Grid "+ datasetname  + "cannot be detached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    HDFCFUtil::close_fileid(-1,-1,gfid,-1,check_pass_fileid_key);

    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArrayGridGeoField::format_constraint (int *offset, int *step,
                                             int *count)
{

    long nels = 1;
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
    }// end while 

    return (int)nels;
}


// Calculate lat/lon based on HDF-EOS2 APIs.
void
HDFEOS2ArrayGridGeoField::CalculateLatLon (int32 gridid, int g_fieldtype,
                                           int g_specialformat,
                                           float64 * outlatlon,float64* latlon_all,
                                           const int32 * offset, const int32 * count,
                                           const int32 * step, int nelms,bool write_latlon_cache)
{

    // Retrieve dimensions and X-Y coordinates of corners 
    int32 xdim = 0;
    int32 ydim = 0; 
    int r = -1;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
        string msg = "Cannot obtain grid information.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // The coordinate values(MCD products) are set to -180.0, -90.0, etc.
    // We have to change them to DDDMMMSSS.SS format, so 
    // we have to multiply them by 1000000.
    if (g_specialformat == 1) {
        upleft[0] = upleft[0] * 1000000;
        upleft[1] = upleft[1] * 1000000;
        lowright[0] = lowright[0] * 1000000;
        lowright[1] = lowright[1] * 1000000;
    }

    // The coordinate values(CERES TRMM) are set to default,which are zeros.
    // Based on the grid names and size, we find it covers the whole global.
    // So we set the corner coordinates to (-180000000.00,90000000.00) and
    // (180000000.00,-90000000.00).
    if (g_specialformat == 2) {
        upleft[0] = 0.0;
        upleft[1] = 90000000.0;
        lowright[0] = 360000000.0;
        lowright[1] = -90000000.0;
    }

    // Retrieve all GCTP projection information 
    int32 projcode = 0; 
    int32 zone = 0; 
    int32 sphere = 0;
    float64 params[16];

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r != 0) {
        string msg = "Cannot obtain grid projection information.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Retrieve pixel registration information 
    int32 pixreg = 0; 

    r = GDpixreginfo (gridid, &pixreg);
    if (r != 0) {
        string msg = "Cannot obtain grid pixel registration info.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    //Retrieve grid pixel origin 
    int32 origin = 0; 

    r = GDorigininfo (gridid, &origin);
    if (r != 0) {
        string msg = "Cannot obtain grid origin info.";
        throw BESInternalError(msg,__FILE__,__LINE__);
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

    if (ydimmajor) {
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
    }
    else {
        // rows             cols
        //   /- ydim  -/      /- ydim  -/
        //   0 1 2 ... y      0 0 0 ... y
        //   0 1 2 ... y      1 1 1 ... y
        //       ...              ...
        //   0 1 2 ... y      2 2 2 ... y

        for (k = j = 0; j < xdim; ++j) {
            for (i = 0; i < ydim; ++i) {
                rows[k] = i;
                cols[k] = j;
                ++k;
            }
        }
    }


    r = GDij2ll (projcode, zone, params, sphere, xdim, ydim, upleft, lowright,
                 xdim * ydim, rows.data(), cols.data(), lon.data(), lat.data(), pixreg, origin);

    if (r != 0) {
        string msg = "Cannot calculate grid latitude and longitude.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // ADDING CACHE file routine,save lon and lat to a cached file. lat first, lon second.
    if (true == write_latlon_cache) {
        if (GCTP_CEA == projcode || GCTP_GEO == projcode) {
            vector<double>temp_lat;
            vector<double>temp_lon;
            int32 temp_offset[2];
            int32 temp_count[2];
            int32 temp_step[2];
            temp_offset[0] = 0;
            temp_offset[1] = 0;
            temp_step[0]   = 1;
            temp_step[1]   = 1;
            if (ydimmajor) {
                // Latitude 
                temp_count[0] = ydim;
                temp_count[1] = 1;
                temp_lat.resize(ydim);
                LatLon2DSubset(temp_lat.data(),ydim,xdim,lat.data(),temp_offset,temp_count,temp_step);
              
                // Longitude
                temp_count[0] = 1;
                temp_count[1] = xdim;
                temp_lon.resize(xdim);
                LatLon2DSubset(temp_lon.data(),ydim,xdim,lon.data(),temp_offset,temp_count,temp_step);

                for(i = 0; i<ydim;i++)
                    latlon_all[i] = temp_lat[i];

                // Longitude values for the simple projections are mapped to 0 to 360 and we need to map them between -180 and 180.
                // The routine need to be called before the latlon_all to make sure the longitude value is changed. 
                // KY 2016-03-09, HFVHANDLER-301
                if (speciallon == true) {//Must also apply to the latitude case since the lat/lon is stored in one cached file
                    CorSpeLon(temp_lon.data(),xdim);
                }

                for(i = 0; i<xdim;i++) 
                    latlon_all[i+ydim] = temp_lon[i];

            }
            else {
                // Latitude 
                temp_count[1] = ydim;
                temp_count[0] = 1;
                temp_lat.resize(ydim);
                LatLon2DSubset(temp_lat.data(),xdim,ydim,lat.data(),temp_offset,temp_count,temp_step);
              
                // Longitude
                temp_count[1] = 1;
                temp_count[0] = xdim;
                temp_lon.resize(xdim);
                LatLon2DSubset(temp_lon.data(),xdim,ydim,lon.data(),temp_offset,temp_count,temp_step);

                for(i = 0; i<ydim;i++)
                    latlon_all[i] = temp_lat[i];

                // Longitude values for the simple projections are mapped to 0 to 360 and we need to map them between -180 and 180.
                // The routine need to be called before the latlon_all to make sure the longitude value is changed. 
                // KY 2016-03-09, HFVHANDLER-301
                if (speciallon == true) //Must also apply to the latitude case since the lat/lon is stored in one cached file
                    CorSpeLon(temp_lon.data(),xdim);

                for(i = 0; i<xdim;i++)
                    latlon_all[i+ydim] = temp_lon[i];

            }
        }
        else {
            memcpy((char*)(&latlon_all[0]),lat.data(),xdim*ydim*sizeof(double));
            memcpy((char*)(&latlon_all[0])+xdim*ydim*sizeof(double),lon.data(),xdim*ydim*sizeof(double));
           
        }
    }

    // 2-D Lat/Lon, need to decompose the data for subsetting.
    if (nelms == (xdim * ydim)) {	// no subsetting return all, for the performance reason.
        if (g_fieldtype == 1)
            memcpy (outlatlon, lat.data(), xdim * ydim * sizeof (double));
        else
            memcpy (outlatlon, lon.data(), xdim * ydim * sizeof (double));
    }
    else {	// Messy subsetting case, needs to know the major dimension
        if (ydimmajor) {
            if (g_fieldtype == 1) // Lat 
                LatLon2DSubset (outlatlon, ydim, xdim, lat.data(), offset, count,
                                step);
            else // Lon
                LatLon2DSubset (outlatlon, ydim, xdim, lon.data(), offset, count,
                                step);
        }
        else {
            if (g_fieldtype == 1) // Lat
                LatLon2DSubset (outlatlon, xdim, ydim, lat.data(), offset, count,
                                step);
            else // Lon
                LatLon2DSubset (outlatlon, xdim, ydim, lon.data(), offset, count,
                    step);
        }
    }
}


// Map the subset of the lat/lon buffer to the corresponding 2D array.
template<class T> void
HDFEOS2ArrayGridGeoField::LatLon2DSubset (T * outlatlon, int /*majordim */,
                                          int minordim, T * latlon,
                                          const int32 * offset, const int32 * count,
                                          const int32 * step) const
{
    int i = 0;
    int j = 0;

    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim0index[dim0count];
    int dim1index[dim1count];

    for (i = 0; i < count[0]; i++)	// count[0] is the least changing dimension 
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

// Some HDF-EOS2 geographic projection lat/lon fields have fill values. 
// This routine is used to replace those fill values by using the formula to calculate
// the lat/lon of the geographic projection.
template < class T > bool HDFEOS2ArrayGridGeoField::CorLatLon (T * latlon,
                                                               int g_fieldtype,
                                                               int elms,
                                                               int fv)
{

    // Since we only find the contiguous fill value of lat/lon from some position to the end 
    // So to speed up the performance, the following algorithm is limited to that case.

    // The first two values cannot be fill value.
    // We find a special case :the first latitude(index 0) is a special value.
    // So we need to have three non-fill values to calculate the increment.

    if (elms < 3) {
        for (int i = 0; i < elms; i++)
            if ((int) (latlon[i]) == fv)
                return false;
        return true;
    }

    // Number of elements is greater than 3.

    for (int i = 0; i < 3; i++)	// The first three elements should not include fill value.
        if ((int) (latlon[i]) == fv)
            return false;

    if ((int) (latlon[elms - 1]) != fv)
        return true;

    T increment = latlon[2] - latlon[1];

    int index = 0;

    // Find the first fill value
    index = findfirstfv (latlon, 0, elms - 1, fv);
    if (index < 2) {
        string msg = "Cannot calculate the fill value. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    for (int i = index; i < elms; i++) {

        latlon[i] = latlon[i - 1] + increment;

        // The latitude must be within (-90,90)
        if (i != (elms - 1) && (g_fieldtype == 1) &&
            ((float) (latlon[i]) < -90.0 || (float) (latlon[i]) > 90.0))
            return false;

        // For longitude, since some files use (0,360)
        // some files use (-180,180), for simple check
        // we just choose (-180,360). 
        // I haven't found longitude has missing values.
        if (i != (elms - 1) && (g_fieldtype == 2) &&
            ((float) (latlon[i]) < -180.0 || (float) (latlon[i]) > 360.0))
            return false;
    }
    if (g_fieldtype == 1) {
        if ((float) (latlon[elms - 1]) < -90.0)
            latlon[elms - 1] = (T)-90;
        if ((float) (latlon[elms - 1]) > 90.0)
            latlon[elms - 1] = (T)90;
    }

    if (g_fieldtype == 2) {
        if ((float) (latlon[elms - 1]) < -180.0)
            latlon[elms - 1] = (T)-180.0;
        if ((float) (latlon[elms - 1]) > 360.0)
            latlon[elms - 1] = (T)360.0;
    }
    return true;
}

// Make longitude (0-360) to (-180 - 180)
template < class T > void
HDFEOS2ArrayGridGeoField::CorSpeLon (T * lon, int xdim) const
{
    int i;
    float64 accuracy = 1e-3;	// in case there is a lon value = 180.0 in the middle, make the error to be less than 1e-3.
    float64 temp = 0;

    // Check if this lon. field falls to the (0-360) case.
    int speindex = -1;

    for (i = 0; i < xdim; i++) {
        if ((double) lon[i] < 180.0)
            temp = 180.0 - (double) lon[i];
        if ((double) lon[i] > 180.0)
            temp = (double) lon[i] - 180.0;

        if (temp < accuracy) {
            speindex = i;
            break;
        }
        else if ((static_cast < double >(lon[i]) < 180.0)
                 &&(static_cast<double>(lon[i + 1]) > 180.0)) {
            speindex = i;
            break;
        }
        else
            continue;
    }

    if (speindex != -1) {
        for (i = speindex + 1; i < xdim; i++) {
            lon[i] =
                static_cast < T > (static_cast < double >(lon[i]) - 360.0);
        }
    }
    return;
}

// Get correct subsetting indexes. This is especially useful when 2D lat/lon can be condensed to 1D.
void
HDFEOS2ArrayGridGeoField::getCorrectSubset (const int *offset, const int *count,
                                            const int *step, int32 * offset32,
                                            int32 * count32, int32 * step32,
                                            bool gf_condenseddim, bool gf_ydimmajor,
                                            int gf_fieldtype, int gf_rank) const
{

    if (gf_rank == 1) {
        offset32[0] = (int32) offset[0];
        count32[0] = (int32) count[0];
        step32[0] = (int32) step[0];
    }
    else if (gf_condenseddim) {

        // Since offset,count and step for some dimensions will always
        // be 1, so first assign offset32,count32,step32 to 1.
        for (int i = 0; i < gf_rank; i++) {
            offset32[i] = 0;
            count32[i] = 1;
            step32[i] = 1;
        }

        if (gf_ydimmajor && gf_fieldtype == 1) {// YDim major, User: Lat[YDim], File: Lat[YDim][XDim]
            offset32[0] = (int32) offset[0];
            count32[0] = (int32) count[0];
            step32[0] = (int32) step[0];
        }
        else if (gf_ydimmajor && gf_fieldtype == 2) {	// YDim major, User: Lon[XDim],File: Lon[YDim][XDim]
            offset32[1] = (int32) offset[0];
            count32[1] = (int32) count[0];
            step32[1] = (int32) step[0];
        }
        else if (!gf_ydimmajor && gf_fieldtype == 1) {// XDim major, User: Lat[YDim], File: Lat[XDim][YDim]
            offset32[1] = (int32) offset[0];
            count32[1] = (int32) count[0];
            step32[1] = (int32) step[0];
        }
        else if (!gf_ydimmajor && gf_fieldtype == 2) {// XDim major, User: Lon[XDim], File: Lon[XDim][YDim]
            offset32[0] = (int32) offset[0];
            count32[0] = (int32) count[0];
            step32[0] = (int32) step[0];
        }

        else {// errors
            throw BESInternalError("Lat/lon subset is wrong for condensed lat/lon.",__FILE__, __LINE__);
        }
    }
    else {
        for (int i = 0; i < gf_rank; i++) {
            offset32[i] = (int32) offset[i];
            count32[i] = (int32) count[i];
            step32[i] = (int32) step[i];
        }
    }
}

// Correct latitude and longitude that have fill values. Although I only found this
// happens for AIRS CO2 grids, I still implemented this as general as I can.

template <class T> void
HDFEOS2ArrayGridGeoField::HandleFillLatLon(vector<T> total_latlon, T* latlon,bool gf_ydimmajor, int gf_fieldtype, int32 xdim , int32 ydim, const int32* offset, const int32* count, const int32* step, int fv)  {

   class vector <T> temp_lat;
   class vector <T> temp_lon;

   if (true == gf_ydimmajor) {

        if (1 == gf_fieldtype) {
            temp_lat.resize(ydim);
            for (int i = 0; i <(int)ydim; i++)
                temp_lat[i] = total_latlon[i*xdim];

            if (false == CorLatLon(temp_lat.data(),gf_fieldtype,ydim,fv))
                throw BESInternalError("Cannot handle the fill values in lat/lon correctly.",__FILE__,__LINE__);
           
            for (int i = 0; i <(int)(count[0]); i++)
                latlon[i] = temp_lat[offset[0] + i* step[0]];
        }
        else {

            temp_lon.resize(xdim);
            for (int i = 0; i <(int)xdim; i++)
                temp_lon[i] = total_latlon[i];


            if (false == CorLatLon(temp_lon.data(),gf_fieldtype,xdim,fv))
                throw BESInternalError("Cannot handle the fill values in lat/lon correctly.",__FILE__,__LINE__);
           
            for (int i = 0; i <(int)(count[1]); i++)
                latlon[i] = temp_lon[offset[1] + i* step[1]];

        }
   }
   else {

        if (1 == gf_fieldtype) {
            temp_lat.resize(xdim);
            for (int i = 0; i <(int)xdim; i++)
                temp_lat[i] = total_latlon[i];

            if (false == CorLatLon(temp_lat.data(),gf_fieldtype,ydim,fv))
                throw BESInternalError("Cannot handle the fill values in lat/lon correctly.",__FILE__,__LINE__);
           
            for (int i = 0; i <(int)(count[1]); i++)
                latlon[i] = temp_lat[offset[1] + i* step[1]];
        }
        else {

            temp_lon.resize(ydim);
            for (int i = 0; i <(int)ydim; i++)
                temp_lon[i] = total_latlon[i*xdim];


            if (false == CorLatLon(temp_lon.data(),gf_fieldtype,xdim,fv))
                throw BESInternalError("Cannot handle the fill values in lat/lon correctly.",__FILE__,__LINE__);
           
            for (int i = 0; i <(int)(count[0]); i++)
                latlon[i] = temp_lon[offset[0] + i* step[0]];
        }

    }
}
            
// A helper recursive function to find the first filled value index.
template < class T > int
HDFEOS2ArrayGridGeoField::findfirstfv (T * array, int start, int end,
                                       int fillvalue)
{

    if (start == end || start == (end - 1)) {
        if (static_cast < int >(array[start]) == fillvalue)
            return start;
        else
            return end;
    }
    else {
        int current = (start + end) / 2;

        if (static_cast < int >(array[current]) == fillvalue)
            return findfirstfv (array, start, current, fillvalue);
        else
            return findfirstfv (array, current, end, fillvalue);
    }
}

// Calculate Special Latitude and Longitude.
//One MOD13C2 file doesn't provide projection code
// The upperleft and lowerright coordinates are all -1
// We have to calculate lat/lon by ourselves.
// Since it doesn't provide the project code, we double check their information
// and find that it covers the whole globe with 0.05 degree resolution.
// Lat. is from 90 to -90 and Lon is from -180 to 180.
void
HDFEOS2ArrayGridGeoField::CalculateSpeLatLon (int32 gridid, int gf_fieldtype,
                                              float64 * outlatlon,
                                              const int32 * offset32,
                                              const int32 * count32, const int32 * step32) const
{

    // Retrieve dimensions and X-Y coordinates of corners 
    int32 xdim = 0;
    int32 ydim = 0;
    int r = -1;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0) {
        string msg = "Cannot obtain grid information.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    //Since this is a special calcuation out of using the GDij2ll function,
    // the rank is always assumed to be 2 and we condense to 1. So the 
    // count for longitude should be count[1] instead of count[0]. See function GetCorSubset

    // Since the project parameters in StructMetadata are all set to be default, I will use
    // the default HDF-EOS2 cell center as the origin of the coordinate. See the HDF-EOS2 user's guide
    // for details. KY 2012-09-10

    if (0 == xdim || 0 == ydim) 
        throw BESInternalError("Xdim or ydim cannot be zero",__FILE__,__LINE__);

    if (gf_fieldtype == 1) {
        double latstep = 180.0 / ydim;

        for (int i = 0; i < (int) (count32[0]); i++)
            outlatlon[i] = 90.0 -latstep/2 - latstep * (offset32[0] + i * step32[0]);
    }
    else {// Longitude should use count32[1] etc. 
        double lonstep = 360.0 / xdim;

        for (int i = 0; i < (int) (count32[1]); i++)
            outlatlon[i] = -180.0 + lonstep/2 + lonstep * (offset32[1] + i * step32[1]);
    }
}

// Calculate latitude and longitude for the MISR SOM projection HDF-EOS2 product.
// since the latitude and longitude of the SOM projection are 3-D, so we need to handle this projection in a special way. 
// Based on our current understanding, the third dimension size is always 180. 
// This is according to the MISR Lat/lon calculation document 
// at http://eosweb.larc.nasa.gov/PRODOCS/misr/DPS/DPS_v50_RevS.pdf
void
HDFEOS2ArrayGridGeoField::CalculateSOMLatLon(int32 gridid, const int *start, const int *count, const int *step, int nelms,const string & cache_fpath,bool write_latlon_cache)
{
    int32 projcode = -1;
    int32 zone = -1; 
    int32 sphere = -1;
    float64 params[NPROJ];
    intn r = -1;

    r = GDprojinfo (gridid, &projcode, &zone, &sphere, params);
    if (r!=0)
        throw BESInternalError("GDprojinfo doesn't return the correct values",__FILE__, __LINE__);

    int MAXNDIM = 10;
    int32 dim[MAXNDIM];
    char dimlist[STRLEN];
    r = GDinqdims(gridid, dimlist, dim);
    // r is the number of dims. or 0.
    // So the valid returned value can be greater than 0. Only throw error when r is less than 0.
    if (r<0)
        throw BESInternalError("GDinqdims doesn't return the correct values.",__FILE__, __LINE__);

    bool is_block_180 = false;
    for(int i=0; i<MAXNDIM; i++)
    {
        if (dim[i]==NBLOCK)
        {
            is_block_180 = true;
            break;
        }
    }
    if (false == is_block_180) {
        string msg = "Number of Block is not " + to_string(NBLOCK)+"." ;
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    int32 xdim = 0; 
    int32 ydim = 0;
    float64 ulc[2];
    float64 lrc[2];

    r = GDgridinfo (gridid, &xdim, &ydim, ulc, lrc);
    if (r!=0) 
        throw BESInternalError("GDgridinfo doesn't return the correct values.",__FILE__,__LINE__);


    float32 offset[NOFFSET]; 
    char som_rw_code[]="r";
    r = GDblkSOMoffset(gridid, offset, NOFFSET, som_rw_code);
    if (r!=0) 
        throw BESInternalError("GDblkSOMoffset doesn't return the correct values.",__FILE__,__LINE__);

    int status = misr_init(NBLOCK, xdim, ydim, offset, ulc, lrc);
    if (status!=0) 
        throw BESInternalError("misr_init doesn't return the correct values.",__FILE__,__LINE__);

    int iflg = 0;
    int (*inv_trans[MAXPROJ+1])(double, double, double*, double*);
    inv_init((int)projcode, (int)zone, (double*)params, (int)sphere, nullptr, nullptr, (int*)&iflg, inv_trans);
    if (iflg) 
        throw BESInternalError("inv_init doesn't return correct values.",__FILE__,__LINE__);

    // Change to vector in the future. KY 2012-09-20
    double somx = 0.;
    double somy = 0.;
    double lat_r = 0.;
    double lon_r = 0.;
    int i = 0;
    int j = 0;
    int k = 0;
    int b = 0;
    int npts=0; 
    float l = 0;
    float s = 0;

    // Seems setting blockdim = 0 always, need to understand this more. KY 2012-09-20
    int blockdim=0; //20; //84.2115,84.2018, 84.192, ... //0 for all
    if (blockdim==0) //66.2263, 66.224, ....
    {

        if (true == write_latlon_cache) {
            vector<double>latlon_all;
            latlon_all.resize(xdim*ydim*NBLOCK*2);
            for(i =1; i <NBLOCK+1;i++)
                for(j=0;j<xdim;j++)
                    for(k=0;k<ydim;k++) 
            {
                b = i;
                l = (float)j;
                s = (float)k;
                misrinv(b, l, s, &somx, &somy); /* (b,l.l,s.s) -> (X,Y) */
                sominv(somx, somy, &lon_r, &lat_r); /* (X,Y) -> (lat,lon) */
                latlon_all[npts] = lat_r*R2D;
                latlon_all[xdim*ydim*NBLOCK+npts] = lon_r*R2D;
                npts++;
            
            }

            BESH4Cache *llcache = BESH4Cache::get_instance();           
            llcache->write_cached_data(cache_fpath,xdim*ydim*NBLOCK*2*sizeof(double),latlon_all);

            // Send the subset of latlon to DAP.
            vector<double>latlon;
            latlon.resize(nelms); 
          
            // Leave the following #if 0 #endif for possible future references. KY 2022-11-28
#if 0
            //double[180*xdim*ydim];
            //int s1=start[0]+1, e1=s1+count[0]*step[0];
            //int s2=start[1],   e2=s2+count[1]*step[1];
            //int s3=start[2],   e3=s3+count[2]*step[2];
            //int s1=start[0]+1; 
            //int s2=start[1];  
            //int s3=start[2]; 
#endif

            npts =0;
            for(i=0; i<count[0]; i++) //i = 1; i<180+1; i++)
                for(j=0; j<count[1]; j++)//j=0; j<xdim; j++)
                    for(k=0; k<count[2]; k++)//k=0; k<ydim; k++)
            {
                if (fieldtype == 1) {
                    latlon[npts] = latlon_all[start[0]*ydim*xdim+start[1]*ydim+start[2]+
                                                 i*ydim*xdim*step[0]+j*ydim*step[1]+k*step[2]];
                }
                else {
                    latlon[npts] = latlon_all[xdim*ydim*NBLOCK+start[0]*ydim*xdim+start[1]*ydim+start[2]+
                                                 i*ydim*xdim*step[0]+j*ydim*step[1]+k*step[2]];

                }
                npts++;
            }

            set_value ((dods_float64 *) latlon.data(), nelms); //(180*xdim*ydim)); //nelms);
        }
        else {
            vector<double>latlon;
            latlon.resize(nelms); //double[180*xdim*ydim];
            int s1=start[0]+1; 
            int e1=s1+count[0]*step[0];
            int s2=start[1];
            int e2=s2+count[1]*step[1];
            int s3=start[2];
            int e3=s3+count[2]*step[2];
            for(i=s1; i<e1; i+=step[0]) //i = 1; i<180+1; i++)
                for(j=s2; j<e2; j+=step[1])//j=0; j<xdim; j++)
                    for(k=s3; k<e3; k+=step[2])//k=0; k<ydim; k++)
            {
                        b = i;
                        l = j;
                        s = k;
                        misrinv(b, l, s, &somx, &somy); /* (b,l.l,s.s) -> (X,Y) */
                        sominv(somx, somy, &lon_r, &lat_r); /* (X,Y) -> (lat,lon) */
                        if (fieldtype==1)
                            latlon[npts] = lat_r*R2D;
                        else
                            latlon[npts] = lon_r*R2D;
                        npts++;
            }
                    set_value ((dods_float64 *) latlon.data(), nelms); //(180*xdim*ydim)); //nelms);
        }
    } 
}

// The following code aims to handle large MCD Grid(GCTP_GEO projection) such as 21600*43200 lat and lon.
// These MODIS MCD files don't follow standard format for lat/lon (DDDMMMSSS);
// they simply represent lat/lon as -180.0000000 or -90.000000.
// HDF-EOS2 library won't give the correct value based on these value.
// We need to calculate the latitude and longitude values.
void
HDFEOS2ArrayGridGeoField::CalculateLargeGeoLatLon(int32 gridid,  int gf_fieldtype, float64* latlon, float64* latlon_all,const int *start, const int *count, const int *step, int nelms,bool write_latlon_cache) const
{

    int32 xdim = 0;
    int32 ydim = 0;
    float64 upleft[2];
    float64 lowright[2];
    int r = 0;
    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r!=0) {
        throw BESInternalError("GDgridinfo failed.",__FILE__,__LINE__);
    }

    if (0 == xdim || 0 == ydim) {
        throw BESInternalError("Xdim or ydim should not be zero. ",__FILE__,__LINE__);
    }

    if (upleft[0]>180.0 || upleft[0] <-180.0 ||
        upleft[1]>90.0 || upleft[1] <-90.0 ||
        lowright[0] >180.0 || lowright[0] <-180.0 ||
        lowright[1] >90.0 || lowright[1] <-90.0) {
        throw BESInternalError("Lat/lon corner points are out of range. ",__FILE__,__LINE__);
    }

    if (count[0] != nelms) {
        throw BESInternalError("Rank is not 1. ",__FILE__,__LINE__);
    }
    float lat_step = (float)(lowright[1] - upleft[1])/ydim;
    float lon_step = (float)(lowright[0] - upleft[0])/xdim;

    if (true == write_latlon_cache) {

        for(int i = 0;i<ydim;i++)
            latlon_all[i] = upleft[1] + i*lat_step + lat_step/2;       

        for(int i = 0;i<xdim;i++)
            latlon_all[i+ydim] = upleft[0] + i*lon_step + lon_step/2;       

    }

    // Treat the origin of the coordinate as the center of the cell.
    // This has been the setting of MCD43 data.  KY 2012-09-10
    if (1 == gf_fieldtype) { //Latitude
        auto start_lat = (float)(upleft[1] + start[0] *lat_step + lat_step/2);
        float step_lat  = lat_step *step[0];
        for (int i = 0; i < count[0]; i++) 
            latlon[i] = start_lat +i *step_lat;
    }
    else { // Longitude
        auto start_lon = (float)(upleft[0] + start[0] *lon_step + lon_step/2);
        float step_lon  = lon_step *step[0];
        for (int i = 0; i < count[0]; i++) 
            latlon[i] = start_lon +i *step_lon;
    }

}


// Calculate latitude and longitude for LAMAZ projection lat/lon products.
// GDij2ll returns infinite numbers over the north pole or the south pole.
void
HDFEOS2ArrayGridGeoField::CalculateLAMAZLatLon(int32 gridid, int gf_fieldtype, float64* latlon, float64* latlon_all, const int *start, const int *count, const int *step, bool write_latlon_cache)
{
    int32 xdim = 0;
    int32 ydim = 0;
    intn r = 0;
    float64 upleft[2];
    float64 lowright[2];

    r = GDgridinfo (gridid, &xdim, &ydim, upleft, lowright);
    if (r != 0)
        throw BESInternalError("GDgridinfo failed.",__FILE__,__LINE__);

    vector<float64> tmp1;
    tmp1.resize(xdim*ydim);
    int32 tmp2[] = {0, 0};
    int32 tmp3[] = {xdim, ydim};
    int32 tmp4[] = {1, 1};
	
    CalculateLatLon (gridid, gf_fieldtype, specialformat, tmp1.data(), latlon_all, tmp2, tmp3, tmp4, xdim*ydim,write_latlon_cache);

    if (write_latlon_cache == true) {

        vector<float64> temp_lat_all;
        vector<float64> lat_all;
        temp_lat_all.resize(xdim*ydim);
        lat_all.resize(xdim*ydim);

        vector<float64> temp_lon_all;
        vector<float64> lon_all;
        temp_lon_all.resize(xdim*ydim);
        lon_all.resize(xdim*ydim);

        for(int w=0; w < xdim*ydim; w++){
            temp_lat_all[w] = latlon_all[w];
            lat_all[w]      = latlon_all[w];
            temp_lon_all[w] = latlon_all[w+xdim*ydim];
            lon_all[w]      = latlon_all[w+xdim*ydim];
        }

        // If we find infinite number among lat or lon values, we use the nearest neighbor method to calculate lat or lon.
        if (ydimmajor) {
            for(int i=0; i<ydim; i++)//Lat
                for(int j=0; j<xdim; j++)
                    if (isundef_lat(lat_all[i*xdim+j]))
                        lat_all[i*xdim+j]=nearestNeighborLatVal(temp_lat_all.data(), i, j, ydim, xdim);
            for(int i=0; i<ydim; i++)
                for(int j=0; j<xdim; j++)
                    if (isundef_lon(lon_all[i*xdim+j]))
                        lon_all[i*xdim+j]=nearestNeighborLonVal(temp_lon_all.data(), i, j, ydim, xdim);
        }
        else { 
            for(int i=0; i<xdim; i++)
                for(int j=0; j<ydim; j++)
                    if (isundef_lat(lat_all[i*ydim+j]))
                        lat_all[i*ydim+j]=nearestNeighborLatVal(temp_lat_all.data(), i, j, xdim, ydim);
         
            for(int i=0; i<xdim; i++)
                for(int j=0; j<ydim; j++)
                    if (isundef_lon(lon_all[i*ydim+j]))
                        lon_all[i*ydim+j]=nearestNeighborLonVal(temp_lon_all.data(), i, j, xdim, ydim);
        
        }

        for(int i = 0; i<xdim*ydim;i++) {
            latlon_all[i] = lat_all[i];
            latlon_all[i+xdim*ydim] = lon_all[i];
        }

    }

    // Need to optimize the access of LAMAZ subset
    vector<float64> tmp5;
    tmp5.resize(xdim*ydim);

    for(int w=0; w < xdim*ydim; w++)
        tmp5[w] = tmp1[w];

    // If we find infinite number among lat or lon values, we use the nearest neighbor method to calculate lat or lon.
    if (ydimmajor) {
        if (gf_fieldtype==1) {// Lat.
            for(int i=0; i<ydim; i++)
                for(int j=0; j<xdim; j++)
                    if (isundef_lat(tmp1[i*xdim+j]))
                        tmp1[i*xdim+j]=nearestNeighborLatVal(tmp5.data(), i, j, ydim, xdim);
        } else if (gf_fieldtype==2){ // Lon.
            for(int i=0; i<ydim; i++)
                for(int j=0; j<xdim; j++)
                    if (isundef_lon(tmp1[i*xdim+j]))
                        tmp1[i*xdim+j]=nearestNeighborLonVal(tmp5.data(), i, j, ydim, xdim);
        }
    } else { // end if (ydimmajor)
        if (gf_fieldtype==1) {
            for(int i=0; i<xdim; i++)
                for(int j=0; j<ydim; j++)
                    if (isundef_lat(tmp1[i*ydim+j]))
                        tmp1[i*ydim+j]=nearestNeighborLatVal(tmp5.data(), i, j, xdim, ydim);
            } else if (gf_fieldtype==2) {
                for(int i=0; i<xdim; i++)
                    for(int j=0; j<ydim; j++)
                        if (isundef_lon(tmp1[i*ydim+j]))
                            tmp1[i*ydim+j]=nearestNeighborLonVal(tmp5.data(), i, j, xdim, ydim);
            }
    }

    for(int i=start[0], k=0; i<start[0]+count[0]*step[0]; i+=step[0])
        for(int j=start[1]; j<start[1]+count[1]*step[1]; j+= step[1])
            latlon[k++] = tmp1[i*ydim+j];

}
#endif

