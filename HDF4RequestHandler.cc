
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf4_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
// Author: Muqun Yang <myang6@opendap.org>
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// HDF4RequestHandler.cc

#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include <DMR.h>
#include <D4BaseTypeFactory.h>
#include <BESDMRResponse.h>
#include <mime_util.h>
#include <InternalErr.h>
#include <Ancillary.h>
#include <debug.h>

#include "HDF4RequestHandler.h"
#include <BESResponseNames.h>
#include <BESDapNames.h>
#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESInfo.h>
#include <BESResponseHandler.h>
#include <BESVersionInfo.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <TheBESKeys.h>
#include <InternalErr.h>
#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESStopWatch.h>
#include <BESDebug.h>
#include "BESDataNames.h"
//#include <ConstraintEvaluator.h>
#include <Ancillary.h>
#include "config_hdf.h"

#define HDF4_NAME "h4"
#include "HE2CF.h"
#include "HDF4_DDS.h"

#include "HDF4_DMR.h"

//#include "HDFCFUtil.h"
#include "HDFFloat32.h"
#include "HDFSPArray_RealField.h"

#include "dodsutil.h"
//#if 0
#include <sys/time.h>
//#endif

using namespace std;
using namespace libdap;

bool check_beskeys(const string);
bool get_beskeys(const string,string &);

extern void read_das(DAS & das, const string & filename);
extern void read_dds(DDS & dds, const string & filename);

extern bool read_dds_hdfsp(DDS & dds, const string & filename,int32 sdfd, int32 fileid,HDFSP::File*h4file);

extern bool read_das_hdfsp(DAS & das, const string & filename,int32 sdfd, int32 fileid,HDFSP::File**h4fileptr);

extern void read_das_sds(DAS & das, const string & filename,int32 sdfd, bool ecs_metadata,HDFSP::File**h4fileptr);
extern void read_dds_sds(DDS &dds, const string & filename,int32 sdfd, HDFSP::File*h4file,bool dds_set_cache);

#ifdef USE_HDFEOS2_LIB

void read_das_use_eos2lib(DAS & das, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,bool ecs_metadata,HDFSP::File**h4file,HDFEOS2::File**eosfile);
void read_dds_use_eos2lib(DDS & dds, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,HDFSP::File*h4file,HDFEOS2::File*eosfile);
void close_fileid(const int sdfd, const int fileid,const int gridfd, const int swathfd,HDFSP::File*h4file,HDFEOS2::File*eosfile); 

#endif

void close_hdf4_fileid(const int sdfd,const int fileid,HDFSP::File*h4file);
bool rw_das_cache_file(const string & filename, DAS *das_ptr,bool rw_flag);
bool r_dds_cache_file(const string & cache_filename, DDS *dds_ptr,const string & hdf4_filename);

// CF key
bool HDF4RequestHandler::_usecf                    = false;

// Keys to tune the performance -general
bool HDF4RequestHandler::_pass_fileid              = false;
bool HDF4RequestHandler::_disable_structmeta       = false;
bool HDF4RequestHandler::_enable_special_eos       = false;
bool HDF4RequestHandler::_disable_scaleoffset_comp = false;
bool HDF4RequestHandler::_disable_ecsmetadata_min  = false;
bool HDF4RequestHandler::_disable_ecsmetadata_all  = false;


// Keys to tune the performance - cache
bool HDF4RequestHandler::_enable_eosgeo_cachefile  = false;
bool HDF4RequestHandler::_enable_data_cachefile    = false;
bool HDF4RequestHandler::_enable_metadata_cachefile= false;

// Keys to handle vdata and vgroups
bool HDF4RequestHandler::_enable_hybrid_vdata              = false;
bool HDF4RequestHandler::_enable_ceres_vdata               = false;
bool HDF4RequestHandler::_enable_vdata_attr                = false;
bool HDF4RequestHandler::_enable_vdata_desc_attr           = false;
bool HDF4RequestHandler::_disable_vdata_nameclashing_check = false;
bool HDF4RequestHandler::_enable_vgroup_attr               = false;

// Misc. keys
bool HDF4RequestHandler::_enable_check_modis_geo_file      = false;
bool HDF4RequestHandler::_enable_swath_grid_attr           = false;
bool HDF4RequestHandler::_enable_ceres_merra_short_name    = false;
bool HDF4RequestHandler::_enable_check_scale_offset_type   = false;
bool HDF4RequestHandler::_disable_swath_dim_map            = false;

// Cache path,prefix and size
bool HDF4RequestHandler::_cache_latlon_path_exist          =false;
string HDF4RequestHandler::_cache_latlon_path              ="";
bool HDF4RequestHandler::_cache_latlon_prefix_exist        =false;
string HDF4RequestHandler::_cache_latlon_prefix            ="";
bool HDF4RequestHandler::_cache_latlon_size_exist          =false;
long HDF4RequestHandler::_cache_latlon_size       =0;
bool HDF4RequestHandler::_cache_metadata_path_exist        =false;
string HDF4RequestHandler::_cache_metadata_path            ="";

HDF4RequestHandler::HDF4RequestHandler(const string & name) :
	BESRequestHandler(name) {
	add_handler(DAS_RESPONSE, HDF4RequestHandler::hdf4_build_das);
	add_handler(DDS_RESPONSE, HDF4RequestHandler::hdf4_build_dds);
	add_handler(DATA_RESPONSE, HDF4RequestHandler::hdf4_build_data);
	add_handler(DMR_RESPONSE, HDF4RequestHandler::hdf4_build_dmr);
	add_handler(DAP4DATA_RESPONSE, HDF4RequestHandler::hdf4_build_dmr);
	add_handler(HELP_RESPONSE, HDF4RequestHandler::hdf4_build_help);
	add_handler(VERS_RESPONSE, HDF4RequestHandler::hdf4_build_version);

        _usecf = check_beskeys("H4.EnableCF");

        // The following keys are only effective when usecf is true.
        // Keys to tune the performance -general
        _pass_fileid                       = check_beskeys("H4.EnablePassFileID");
        _disable_structmeta                = check_beskeys("H4.DisableStructMetaAttr");
        _enable_special_eos                = check_beskeys("H4.EnableSpecialEOS");
        _disable_scaleoffset_comp          = check_beskeys("H4.DisableScaleOffsetComp");
        _disable_ecsmetadata_min           = check_beskeys("H4.DisableECSMetaDataMin");
        _disable_ecsmetadata_all           = check_beskeys("H4.DisableECSMetaDataAll");


        // Keys to tune the performance - cache
        _enable_eosgeo_cachefile           = check_beskeys("H4.EnableEOSGeoCacheFile");
        _enable_data_cachefile             = check_beskeys("H4.EnableDataCacheFile");
        _enable_metadata_cachefile         = check_beskeys("H4.EnableMetaDataCacheFile");

        // Keys to handle vdata and vgroups
        _enable_hybrid_vdata               = check_beskeys("H4.EnableHybridVdata");
        _enable_ceres_vdata                = check_beskeys("H4.EnableCERESVdata");
        _enable_vdata_attr                 = check_beskeys("H4.EnableVdata_to_Attr");
        _enable_vdata_desc_attr            = check_beskeys("H4.EnableVdataDescAttr");
        _disable_vdata_nameclashing_check  = check_beskeys("H4.DisableVdataNameclashingCheck");
        _enable_vgroup_attr                = check_beskeys("H4.EnableVgroupAttr");

        // Misc. keys
        _enable_check_modis_geo_file       = check_beskeys("H4.EnableCheckMODISGeoFile");
        _enable_swath_grid_attr            = check_beskeys("H4.EnableSwathGridAttr");
        _enable_ceres_merra_short_name     = check_beskeys("H4.EnableCERESMERRAShortName");
        _enable_check_scale_offset_type    = check_beskeys("H4.EnableCheckScaleOffsetType");

        _disable_swath_dim_map             = check_beskeys("H4.DisableSwathDimMap");

        // Cache path etc. 
        _cache_latlon_path_exist           =get_beskeys("HDF4.Cache.latlon.path",_cache_latlon_path);
        _cache_latlon_prefix_exist         =get_beskeys("HDF4.Cache.latlon.prefix",_cache_latlon_prefix);
        string temp_cache_latlon_size;
        _cache_latlon_size_exist           =get_beskeys("HDF4.Cache.latlon.size",temp_cache_latlon_size);
        if(_cache_latlon_size_exist == true) {
            istringstream iss(temp_cache_latlon_size);
            iss >> _cache_latlon_size;
        }

        _cache_metadata_path_exist        =get_beskeys("H4.Cache.metadata.path",_cache_metadata_path);

}

HDF4RequestHandler::~HDF4RequestHandler() {
}

bool HDF4RequestHandler::hdf4_build_das(BESDataHandlerInterface & dhi) {


    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY))
        sw.start("HDF4RequestHandler::hdf4_build_das", dhi.data[REQUEST_ID]);

    if(true == _usecf) {

        // Build DAP response only based on the HDF4 SD interfaces. Doing this 
        // way will save the use of other library open calls. Other library open 
        // calls  may be expensive
        // for an HDF4 file that only has variables created by SD interfaces.
        // This optimization may be very useful for the aggreagation case that
        // has many variables.
        // Currently we only handle AIRS version 6 products. AIRS products
        // are identified by their file names.
        // We only obtain the filename. The path is stripped off.

        string base_file_name = basename(dhi.container->access());

        //AIRS.2015.01.24.L3.RetStd_IR008.v6.0.11.0.G15041143400.hdf
        // Identify this file from product name: AIRS, product level: .L3. or .L2. and version .v6. 
        if((base_file_name.size() >12) && (base_file_name.compare(0,4,"AIRS") == 0)
                && ((base_file_name.find(".L3.")!=string::npos) || (base_file_name.find(".L2.")!=string::npos))
                && (base_file_name.find(".v6.")!=string::npos)) {
                return hdf4_build_das_cf_sds(dhi);

        }
    }

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();

        string base_file_name = basename(dhi.container->access());

        string accessed = dhi.container->access();
         
        if (true == _usecf) {

            int32 sdfd    = -1;
            int32 fileid  = -1;

            HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd){
                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }
 
            // H open
            fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                string invalid_file_msg="HDF4 Hopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

#ifdef USE_HDFEOS2_LIB

            int32 gridfd  = -1;
            int32 swathfd = -1;
            HDFEOS2::File *eosfile = NULL;
            // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            
            // Grid open 
            gridfd = GDopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == gridfd) {
                SDend(sdfd);
                Hclose(fileid);
                string invalid_file_msg="HDF-EOS GDopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                string invalid_file_msg="HDF-EOS SWopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }
            
            try {
                bool ecs_metadata = !(_disable_ecsmetadata_all);
#if 0
if(ecs_metadata == true)
cerr<<"output ecs metadata "<<endl;
else
cerr<<"Don't output ecs metadata "<<endl;
#endif
                read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
            }
            catch(...) {
                close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
                throw;
#if 0
                //throw InternalErr(__FILE__,__LINE__,"read_das_use_eos2lib error");
#endif
            }
            if(eosfile != NULL)
                delete eosfile;
            GDclose(gridfd);
            SWclose(swathfd);
 
#else
            try {
                read_das_hdfsp(*das,accessed,sdfd,fileid,&h4file);
            }
            catch(...) {
                close_hdf4_fileid(sdfd,fileid,h4file); 
                throw;
                //throw InternalErr(__FILE__,__LINE__,"read_das_hdfsp error");
            }
#endif
            close_hdf4_fileid(sdfd,fileid,h4file);
        }
        else 
            read_das(*das,accessed);

        Ancillary::read_ancillary_das(*das, accessed);
        bdas->clear_container();
    } 

    catch (BESError & e) {
        throw;
    } 
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
				__FILE__, __LINE__);
    } 
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
				__FILE__, __LINE__);
    } 
    catch (...) {
        string s = "unknown exception caught building HDF4 DAS";
	throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_dds(BESDataHandlerInterface & dhi) {



    BESStopWatch sw;
        if (BESDebug::IsSet(TIMING_LOG_KEY))
                sw.start("HDF4RequestHandler::hdf4_build_das", dhi.data[REQUEST_ID]);


    if(true == _usecf) {
        // Build DAP response only based on the HDF4 SD interfaces. Doing this 
        // way will save the use of other library open calls. Other library open 
        // calls  may be expensive
        // for an HDF4 file that only has variables created by SD interfaces.
        // This optimization may be very useful for the aggreagation case that
        // has many variables.
        // Currently we only handle AIRS version 6 products. AIRS products
        // are identified by their file names.
        // We only obtain the filename. The path is stripped off.

        string base_file_name = basename(dhi.container->access());

        //AIRS.2015.01.24.L3.RetStd_IR008.v6.0.11.0.G15041143400.hdf
        // Identify this file from product name: AIRS, product level: .L3. or .L2. and version .v6. 
        if((base_file_name.size() >12) && (base_file_name.compare(0,4,"AIRS") == 0)
                && ((base_file_name.find(".L3.")!=string::npos) || (base_file_name.find(".L2.")!=string::npos))
                && (base_file_name.find(".v6.")!=string::npos)) {
                return hdf4_build_dds_cf_sds(dhi);

        }
    }

// This is for the performance check. Leave it now for future use. KY 2014-10-23
#ifdef KENT2
struct timeval start_time,end_time;
gettimeofday(&start_time,NULL);
#endif

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *> (response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());
        DDS *dds = bdds->get_dds();
#if 0
        //ConstraintEvaluator & ce = bdds->get_ce();
#endif

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

        if (true == _usecf) {

            int32 sdfd   = -1;
            int32 fileid = -1;
            HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd){
                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // H open
            fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                string invalid_file_msg="HDF4 Hopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

#ifdef USE_HDFEOS2_LIB        

            int32 gridfd  = -1;
            int32 swathfd = -1;
 
            HDFEOS2::File *eosfile = NULL;

            // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            // Grid open 
            gridfd = GDopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == gridfd) {
                SDend(sdfd);
                Hclose(fileid);
                string invalid_file_msg="HDF-EOS GDopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                string invalid_file_msg="HDF-EOS SWopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }
 
            try {
                bool ecs_metadata = !(_disable_ecsmetadata_all);
                read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
                Ancillary::read_ancillary_das(*das, accessed);

                // Pass file pointer(h4file, eosfile) from DAS to DDS.
                read_dds_use_eos2lib(*dds, accessed,sdfd,fileid,gridfd,swathfd,h4file,eosfile);
            }
            catch(...) {
                close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
                throw;
            }

            if(eosfile != NULL)
                delete eosfile;

            GDclose(gridfd);
            SWclose(swathfd);

#else
            try {
                read_das_hdfsp(*das, accessed,sdfd,fileid,&h4file);
                Ancillary::read_ancillary_das(*das, accessed);

                // Pass file pointer(h4file) from DAS to DDS.
                read_dds_hdfsp(*dds, accessed,sdfd,fileid,h4file);
            }
            catch(...) {
                close_hdf4_fileid(sdfd,fileid,h4file);
                throw;
            }

#endif
            close_hdf4_fileid(sdfd,fileid,h4file);
        }
        else {
            read_das(*das, accessed);
            Ancillary::read_ancillary_das(*das, accessed);
            read_dds(*dds, accessed);
        }

// Leave it for future performance tests. KY 2014-10-23
#ifdef KENT2
gettimeofday(&end_time,NULL);
int total_time_spent = (end_time.tv_sec - start_time.tv_sec)*1000000 +end_time.tv_usec-start_time.tv_usec;
cerr<<"total time spent for DDS buld is "<<total_time_spent<< "micro seconds "<<endl;
#endif

	Ancillary::read_ancillary_dds(*dds, accessed);

	dds->transfer_attributes(das);

	bdds->set_constraint(dhi);

	bdds->clear_container();
    } 
    catch (BESError & e) {
        throw;
    } 
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
				__FILE__, __LINE__);
    } 
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
				__FILE__, __LINE__);
    } 
    catch (...) {
        string s = "unknown exception caught building HDF4 DDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_data(BESDataHandlerInterface & dhi) {


    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY))
        sw.start("HDF4RequestHandler::hdf4_build_data", dhi.data[REQUEST_ID]);


    int32 sdfd   = -1;
    int32 fileid = -1;

    // Since passing file IDs requires to use the derived class and it
    // causes the management of code structure messy, we first handle this with
    // another method.
    if(true == _usecf) {
        // Build DAP response only based on the HDF4 SD interfaces. Doing this 
        // way will save the use of other library open calls. Other library open 
        // calls  may be expensive
        // for an HDF4 file that only has variables created by SD interfaces.
        // This optimization may be very useful for the aggreagation case that
        // has many variables.
        // Currently we only handle AIRS version 6 products. AIRS products
        // are identified by their file names.
        // We only obtain the filename. The path is stripped off.

        string base_file_name = basename(dhi.container->access());
        //AIRS.2015.01.24.L3.RetStd_IR008.v6.0.11.0.G15041143400.hdf
        // Identify this file from product name: AIRS, product level: .L3. or .L2. and version .v6. 
        if((base_file_name.size() >12) && (base_file_name.compare(0,4,"AIRS") == 0)
                && ((base_file_name.find(".L3.")!=string::npos) || (base_file_name.find(".L2.")!=string::npos))
                && (base_file_name.find(".v6.")!=string::npos)) {

            BESDEBUG("h4", "Coming to read the data of AIRS level 3 or level 2 products." << endl);

            if(true == _pass_fileid) 
                return hdf4_build_data_cf_sds_with_IDs(dhi);
            else 
                return hdf4_build_data_cf_sds(dhi);

        }

        if(true == _pass_fileid)
            return hdf4_build_data_with_IDs(dhi);

    }

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        DDS *dds = bdds->get_dds();

        // Not sure why keep the following line, it is not used.
        //ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

        if (true == _usecf) {

             HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd) {
                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // H open
            fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                string invalid_file_msg="HDF4 Hopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }


#ifdef USE_HDFEOS2_LIB        

            int32 gridfd  = -1;
            int32 swathfd = -1;
            HDFEOS2::File *eosfile = NULL;
            // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            
            // Grid open 
            gridfd = GDopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == gridfd) {
                SDend(sdfd);
                Hclose(fileid);
                string invalid_file_msg="HDF-EOS GDopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                string invalid_file_msg="HDF-EOS SWopen error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
 
            }

            try {

                // Here we will check if ECS_Metadata key if set. For DataDDS, 
                // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
                // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
                bool ecs_metadata = true;
#if 0
                if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
                    || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
                    ecs_metadata = false;
#endif
                if((true == _disable_ecsmetadata_min) 
                    || (true == _disable_ecsmetadata_all)) 
                    ecs_metadata = false;

                read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
                Ancillary::read_ancillary_das(*das, accessed);
                
                // Pass file pointer(h4file, eosfile) from DAS to DDS.
                read_dds_use_eos2lib(*dds, accessed,sdfd,fileid,gridfd,swathfd,h4file,eosfile);
            }
            catch(...) {
                close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
                throw;
            }

            if(eosfile != NULL)
                delete eosfile;
            GDclose(gridfd);
            SWclose(swathfd);

#else
            try {
                read_das_hdfsp(*das, accessed,sdfd,fileid,&h4file);
                Ancillary::read_ancillary_das(*das, accessed);

                // Pass file pointer(h4file) from DAS to DDS.
                read_dds_hdfsp(*dds, accessed,sdfd,fileid,h4file);
            }
            catch(...) {
                close_hdf4_fileid(sdfd,fileid,h4file);
                throw;
            }
#endif
            close_hdf4_fileid(sdfd,fileid,h4file);
        }
        else {
            read_das(*das, accessed);
            Ancillary::read_ancillary_das(*das, accessed);
            read_dds(*dds, accessed);
        }

        Ancillary::read_ancillary_dds(*dds, accessed);
        dds->transfer_attributes(das);
        bdds->set_constraint(dhi);
        bdds->clear_container();

    }

    catch (BESError & e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
        string s = "unknown exception caught building DAP2 Data Response from an HDF4 data resource";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_data_with_IDs(BESDataHandlerInterface & dhi) {

    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY))
        sw.start("HDF4RequestHandler::hdf4_build_data_with_IDs", dhi.data[REQUEST_ID]);

    int32 sdfd   = -1;
    int32 fileid = -1;
    HDFSP::File *h4file = NULL;
#ifdef USE_HDFEOS2_LIB
    int32 gridfd  = -1;
    int32 swathfd = -1;
    HDFEOS2::File *eosfile = NULL;
#endif

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        // Create a new HDF4DDS object.
        HDF4DDS *hdds = new HDF4DDS(bdds->get_dds());

        // delete the old object.
        delete bdds->get_dds();

        bdds->set_dds(hdds);

#if 0
        //ConstraintEvaluator & ce = bdds->get_ce();
#endif

        string accessed = dhi.container->access();
        hdds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());


        // Obtain HDF4 file IDs
        //SDstart
        sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
        if( -1 == sdfd) {
            string invalid_file_msg="HDF4 SDstart error for the file ";
            invalid_file_msg +=accessed;
            invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }
        // H open
        fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
        if (-1 == fileid) {
            SDend(sdfd);
            string invalid_file_msg="HDF4 Hopen error for the file ";
            invalid_file_msg +=accessed;
            invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

#ifdef USE_HDFEOS2_LIB        


        // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            
        // Grid open 
        gridfd = GDopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
        if (-1 == gridfd) {
            SDend(sdfd);
            Hclose(fileid);
            string invalid_file_msg="HDF-EOS GDopen error for the file ";
            invalid_file_msg +=accessed;
            invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        // Swath open 
        swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
        if (-1 == swathfd) {
            SDend(sdfd);
            Hclose(fileid);
            GDclose(gridfd);
            string invalid_file_msg="HDF-EOS SWopen error for the file ";
            invalid_file_msg +=accessed;
            invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        hdds->setHDF4Dataset(sdfd,fileid,gridfd,swathfd);

        // Here we will check if ECS_Metadata key if set. For DataDDS, 
        // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
        // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
        bool ecs_metadata = true;
#if 0
        if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
            || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
#endif
        if((true == _disable_ecsmetadata_min) 
                 || (true == _disable_ecsmetadata_all))
            ecs_metadata = false;

        read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);

#if 0
        //read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,true);
#endif

        Ancillary::read_ancillary_das(*das, accessed);

        // Pass file pointer(h4file, eosfile) from DAS to DDS.
        read_dds_use_eos2lib(*hdds, accessed,sdfd,fileid,gridfd,swathfd,h4file,eosfile);

        if(eosfile != NULL)
            delete eosfile;

#else
        hdds->setHDF4Dataset(sdfd,fileid);
        read_das_hdfsp(*das, accessed,sdfd,fileid,&h4file);
        Ancillary::read_ancillary_das(*das, accessed);

        // Pass file pointer(h4file) from DAS to DDS.
        read_dds_hdfsp(*hdds, accessed,sdfd,fileid,h4file);
#endif
        if(h4file != NULL)
            delete h4file;

        Ancillary::read_ancillary_dds(*hdds, accessed);

        hdds->transfer_attributes(das);

        bdds->set_constraint(dhi);

        bdds->clear_container();


// File IDs are closed by the derived class.
#if 0
        if(true == usecf) {     
#ifdef USE_HDFEOS2_LIB
            GDclose(gridfd);
            SWclose(swathfd);

#endif
            SDend(sdfd);  
            Hclose(fileid);
        }
#endif
    }

    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
#ifdef USE_HDFEOS2_LIB
        close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
#else
        close_hdf4_fileid(sdfd,fileid,h4file);
#endif
        string s = "unknown exception caught building DAP2 data response from an HDF4 data resource";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

// Special function to build DDS for HDF4 SDS-only DDS. One can turn
// on cache for this function. Currently only AIRS version 6 is supported.
bool HDF4RequestHandler::hdf4_build_dds_cf_sds(BESDataHandlerInterface &dhi){

    int32 sdfd   = -1;
    HDFSP::File *h4file = NULL;

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        DDS *dds = bdds->get_dds();

#if 0
        //BaseTypeFactory* factory = new BaseTypeFactory ;
        //dds->set_factory( factory ) ;
#endif

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);

        // Check and set up dds and das cache files.
        string base_file_name = basename(dhi.container->access());
        bool das_set_cache = false;
        bool dds_set_cache = false;
        bool dds_das_get_cache = false;
        string das_filename;
        string dds_filename;

        // Check if the EnableMetaData key is set. 
#if 0
        string check_enable_mcache_key="H4.EnableMetaDataCacheFile";
        bool turn_on_enable_mcache_key= false;
        turn_on_enable_mcache_key = HDFCFUtil::check_beskeys(check_enable_mcache_key);
        if(true == turn_on_enable_mcache_key) {// the EnableMetaData key is set
#endif
        if(true == _enable_metadata_cachefile) {// the EnableMetaData key is set


#if 0
            string md_cache_dir;
            bool found = false;
            string key = "H4.Cache.metadata.path";
            TheBESKeys::TheKeys()->get_value(key,md_cache_dir,found);
#endif
            if(true == _cache_metadata_path_exist) {// the metadata.path key is set

                // Create DAS and DDS cache file names
                das_filename = _cache_metadata_path + "/" + base_file_name +"_das";
                dds_filename = _cache_metadata_path + "/" + base_file_name +"_dds";

                // When the returned value das_set_cache is false, read DAS from the cache file.
                // Otherwise, do nothing.
                das_set_cache = rw_das_cache_file(das_filename,das,false);

                // When the returned value dds_set_cache is false, read DDS from the cache file.
                // Otherwise, do nothing.
                dds_set_cache = r_dds_cache_file(dds_filename,dds,accessed);

                // Set the flag to obtain  DDS and DAS from the cache files.
                // Here, we require that both DDS and DAS should be cached. 
                // We don't support caching one and not caching another.
                if((false == das_set_cache)&&(false == dds_set_cache))
                    dds_das_get_cache = true;
            }
        }


        // We need to go back to retrieve DDS, DAS from the HDF file.
        if(false == dds_das_get_cache) { 

            // Obtain SD ID, this is the only ID we need to use.
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd) {

                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }


            // Here we will check if ECS_Metadata key if set. For DDS and DAS, 
            // only when the DisableECSMetaDataAll key is set, ecs_metadata is off.
            bool ecs_metadata = !(_disable_ecsmetadata_all);
#if 0
        bool ecs_metadata = true;
        if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
            || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
            ecs_metadata = false;
#endif
               
            read_das_sds(*das, accessed,sdfd,ecs_metadata,&h4file);

            Ancillary::read_ancillary_das(*das, accessed);

            // Pass file pointer(h4file) from DAS to DDS, also pass the flag
            // dds_set_cache to indicate if the handler needs to cache DDS.
            read_dds_sds(*dds, accessed,sdfd,h4file,dds_set_cache);

            // We also need to cache DAS if das_set_cache is set.
            if(true == das_set_cache) {
                if(das_filename =="")
                    throw InternalErr(__FILE__,__LINE__,"DAS cache file name should be set ");
                rw_das_cache_file(das_filename,das,true);
            }

        }

        Ancillary::read_ancillary_dds(*dds, accessed);
        dds->transfer_attributes(das);
        bdds->set_constraint(dhi);

        bdds->clear_container();

        if(h4file != NULL)
            delete h4file;

        if(sdfd != -1)
            SDend(sdfd); 
#if 0
        // Cannot(should not) delete factory here. factory will be released by DAP? or errors occur.
        //if(factory != NULL)
        //    delete factory;
#endif

    }

    catch (BESError & e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
        if(sdfd != -1)
            SDend(sdfd);
        if(h4file != NULL)
            delete h4file;
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

   return true;
}

// Special function to build DAS for HDF4 SDS-only DDS. One can turn
// on cache for this function. Currently only AIRS version 6 is supported.
bool HDF4RequestHandler::hdf4_build_das_cf_sds(BESDataHandlerInterface &dhi){

    int32 sdfd   = -1;
    HDFSP::File *h4file = NULL;

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);

    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdas->set_container(dhi.container->get_symbolic_name());

        DAS *das = bdas->get_das();
        string base_file_name = basename(dhi.container->access());

        string accessed = dhi.container->access();

        // Check if the enable metadata cache key is set and set the key appropriate.
        bool das_set_cache = false;
        bool das_get_cache = false;
        string das_filename;
#if 0
        string check_enable_mcache_key="H4.EnableMetaDataCacheFile";
        bool turn_on_enable_mcache_key= false;
        turn_on_enable_mcache_key = HDFCFUtil::check_beskeys(check_enable_mcache_key);
        if(true == turn_on_enable_mcache_key) { // find the metadata cache key.
#endif
        if(true == _enable_metadata_cachefile) { // find the metadata cache key.
#if 0
            string md_cache_dir;
            string key = "H4.Cache.metadata.path";
            bool found = false;
            TheBESKeys::TheKeys()->get_value(key,md_cache_dir,found);
            if(true == found) { // Also find the metadata cache.
#endif
            if(true == _cache_metadata_path_exist) {

                // Create the DAS cache file name.
                das_filename = _cache_metadata_path + "/" + base_file_name +"_das";

                // Read the DAS from the cached file, if das_set_cache is false.
                // When the das_set_cache is true, need to create a das cache file.
                das_set_cache = rw_das_cache_file(das_filename,das,false);

                // Not set cache, must get the das from cache, so das_get_cache should be true.
                if(false == das_set_cache)
                    das_get_cache = true;
            }
        }

        // Need to retrieve DAS from the HDF4 file.
        if(false == das_get_cache) {
            // Obtain SD ID.
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd){
                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }


#if 0
        bool ecs_metadata = true;
        if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
            || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
            ecs_metadata = false;
#endif
            // Here we will check if ECS_Metadata key if set. For DDS and DAS, 
            // only when the DisableECSMetaDataAll key is set, ecs_metadata is off.
            bool ecs_metadata = !(_disable_ecsmetadata_all);
               
            read_das_sds(*das, accessed,sdfd,ecs_metadata,&h4file);
            Ancillary::read_ancillary_das(*das, accessed);

            // Generate DAS cache file if the cache flag is set.
            if(true == das_set_cache) 
                rw_das_cache_file(das_filename,das,true);
 
        }

        bdas->clear_container();

        if(h4file != NULL)
            delete h4file;

        if(sdfd != -1)
           SDend(sdfd);  

    }

    catch (BESError & e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
        if(sdfd != -1)
            SDend(sdfd);
        if(h4file != NULL)
            delete h4file;
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }
    return true;
}
// Special function to build data for HDF4 SDS-only DDS. One can turn
// on cache for this function. Currently only AIRS version 6 is supported.
bool HDF4RequestHandler::hdf4_build_data_cf_sds(BESDataHandlerInterface &dhi){

    int32 sdfd   = -1;
    HDFSP::File *h4file = NULL;

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        DDS *dds = bdds->get_dds();

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

#if 0
        //BaseTypeFactory* factory = new BaseTypeFactory ;
        //dds->set_factory( factory ) ;
#endif

        string base_file_name = basename(dhi.container->access());

        bool das_set_cache = false;
        bool dds_set_cache = false;
        bool dds_das_get_cache = false;
        string das_filename;
        string dds_filename;

#if 0
        string check_enable_mcache_key="H4.EnableMetaDataCacheFile";
        bool _disable_ecsmetadata_allturn_on_enable_mcache_key= false;
        turn_on_enable_mcache_key = HDFCFUtil::check_beskeys(check_enable_mcache_key);
        if(true == turn_on_enable_mcache_key) {
#endif
        if(true == _enable_metadata_cachefile) {

            if(true == _cache_metadata_path_exist) {
                BESDEBUG("h4", "H4.Cache.metadata.path key is set and metadata cache key is set." << endl);

                // Notice, since the DAS output may be different between DAS/DDS service and DataDDS service.
                // See comments about ecs_metadata below.
                // So we create a different DAS file. This can be optimized in the future if necessary.
                das_filename = _cache_metadata_path + "/" + base_file_name +"_das_dd";
                dds_filename = _cache_metadata_path + "/" + base_file_name +"_dds";

                // If das_set_cache is true, data is read from the DAS cache.
                das_set_cache = rw_das_cache_file(das_filename,das,false);

                // If dds_set_cache is true, data is read from the DDS cache.
                dds_set_cache = r_dds_cache_file(dds_filename,dds,accessed);
                // Need to set a flag to generate DAS and DDS cache files 
                if((false == das_set_cache)&&(false == dds_set_cache))
                    dds_das_get_cache = true;
            }
        }
        if(false == dds_das_get_cache) { 

            // Obtain HDF4 SD ID
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd){
                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=accessed;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }


            // Here we will check if ECS_Metadata key if set. For DataDDS, 
            // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
            // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
            bool ecs_metadata = true;
#if 0
            if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
                || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
#endif
            if((true == _disable_ecsmetadata_min) 
                    || (true == _disable_ecsmetadata_all))
                ecs_metadata = false;
               
            read_das_sds(*das, accessed,sdfd,ecs_metadata,&h4file);

            Ancillary::read_ancillary_das(*das, accessed);

            // Need to write DAS to a cache file.
            if(true == das_set_cache) {
                rw_das_cache_file(das_filename,das,true);
            }

            // Pass file pointer h4file from DAS to DDS. Also need to pass the
            // flag that indicates if a DDS file needs to be created.
            read_dds_sds(*dds, accessed,sdfd,h4file,dds_set_cache);

        }

        Ancillary::read_ancillary_dds(*dds, accessed);

        dds->transfer_attributes(das);

        bdds->set_constraint(dhi);

        bdds->clear_container();

        if(h4file != NULL)
            delete h4file;

        if(sdfd != -1)
            SDend(sdfd);  
    }

    catch (BESError & e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
        if(sdfd != -1)
            SDend(sdfd);
        if(h4file != NULL)
            delete h4file;
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }


    return true;
}

// Notice the metadata cache doesn't apply when the EnableFileID key is set.
// This is because the file ID generated by creating DDS/DAS WO cache is needed to access the data.
// So to make this work at any time, we have to create SDS ID even when the EnableFileID key is set.
// This is against the purpose of EnableFileID key. To acheieve the same purpose for AIRS,
// one can set the DataCache key, the performance is similar or even better than just using the EnableFileID key.
bool HDF4RequestHandler::hdf4_build_data_cf_sds_with_IDs(BESDataHandlerInterface &dhi){

    int32 sdfd   = -1;
    HDFSP::File *h4file = NULL;

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        HDF4DDS *hdds = new HDF4DDS(bdds->get_dds());

        delete bdds->get_dds();

        bdds->set_dds(hdds);


        string accessed = dhi.container->access();
        hdds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

        //Obtain SD ID. 
        sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
        if( -1 == sdfd) {
            string invalid_file_msg="HDF4 SDstart error for the file ";
            invalid_file_msg +=accessed;
            invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        hdds->setHDF4Dataset(sdfd,-1);

        // Here we will check if ECS_Metadata key if set. For DataDDS, 
        // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
        // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
        bool ecs_metadata = true;
#if 0
        if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
            || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
#endif
        if((true == _disable_ecsmetadata_min) 
            || (true == _disable_ecsmetadata_all)) 
            ecs_metadata = false;
               
        read_das_sds(*das, accessed,sdfd,ecs_metadata,&h4file);
#if 0
        //read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,true);
#endif

        Ancillary::read_ancillary_das(*das, accessed);

        // Pass file pointer(h4file, eosfile) from DAS to DDS.
        read_dds_sds(*hdds, accessed,sdfd,h4file,false);

        if(h4file != NULL)
            delete h4file;

        Ancillary::read_ancillary_dds(*hdds, accessed);

        hdds->transfer_attributes(das);

        bdds->set_constraint(dhi);

        bdds->clear_container();

    }

    catch (BESError & e) {
        throw;
    }
    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
        if(sdfd != -1)
            SDend(sdfd);
        if(h4file != NULL)
            delete h4file;
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }


    return true;
}
bool HDF4RequestHandler::hdf4_build_dmr(BESDataHandlerInterface &dhi)
{

    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY))
        sw.start("HDF4RequestHandler::hdf4_build_dmr", dhi.data[REQUEST_ID]);

    // Because this code does not yet know how to build a DMR directly, use
    // the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
    // First step, build the 'full DDS'
    string data_path = dhi.container->access();

    BaseTypeFactory factory;
    DDS dds(&factory, name_path(data_path), "3.2");
    dds.filename(data_path);

    DAS das;

    int32 sdfd   = -1;
    int32 fileid = -1;

    // Since passing file IDs requires to use the derived class and it
    // causes the management of code structure messy, we first handle this with
    // another method.
    if(true == _usecf) {
       
        if(true == _pass_fileid)
            return hdf4_build_dmr_with_IDs(dhi);

    }


    try {

        if (true == _usecf) {

            HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(data_path.c_str()), DFACC_READ);
            if( -1 == sdfd){
                string invalid_file_msg="HDF4 SDstart error for the file ";
                invalid_file_msg +=data_path;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // H open
            fileid = Hopen(const_cast<char *>(data_path.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                string invalid_file_msg="HDF4 Hopen error for the file ";
                invalid_file_msg +=data_path;
                invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

#ifdef USE_HDFEOS2_LIB        

           int32 gridfd  = -1;
           int32 swathfd = -1;

           HDFEOS2::File *eosfile = NULL;

            // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            // Grid open 
            gridfd = GDopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
            if (-1 == gridfd) {
                SDend(sdfd);
                Hclose(fileid);
                string invalid_file_msg="HDF-EOS GDopen error for the file ";
                invalid_file_msg +=data_path;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                string invalid_file_msg="HDF-EOS SWopen error for the file ";
                invalid_file_msg +=data_path;
                invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

 
            // Here we will check if ECS_Metadata key if set. For DAP4's DMR, 
            // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
            // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
            // This is one difference between DAP2 and DAP4 mapping. Since 
            // people can use BES key to turn on the ECS metadata, so this is okay.
            // KY 2014-10-23
            bool ecs_metadata = true;
#if 0
            if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
                || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
#endif
            if((true == _disable_ecsmetadata_min) 
                || (true == _disable_ecsmetadata_all)) 
               ecs_metadata = false;
               
            try {

                read_das_use_eos2lib(das, data_path,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
#if 0
                //read_das_use_eos2lib(*das, data_path,sdfd,fileid,gridfd,swathfd,true);
#endif
                Ancillary::read_ancillary_das(das, data_path);

                // Pass file pointer(h4file, eosfile) from DAS to DDS
                read_dds_use_eos2lib(dds, data_path,sdfd,fileid,gridfd,swathfd,h4file,eosfile);
            }

            catch(...) {
                close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
                throw;
            }
            if(eosfile != NULL)
                delete eosfile;
            GDclose(gridfd);
            SWclose(swathfd);

#else
            try {
                read_das_hdfsp(das, data_path,sdfd,fileid,&h4file);
                Ancillary::read_ancillary_das(das, data_path);

                // Pass file pointer(h4file) from DAS to DDS.
                read_dds_hdfsp(dds, data_path,sdfd,fileid,h4file);
            }
            catch(...) {
                close_hdf4_fileid(sdfd,fileid,h4file);
                throw;
                //throw InternalErr(__FILE__,__LINE__,"read_dds_hdfsp error");
            }
#endif
            close_hdf4_fileid(sdfd,fileid,h4file);

        }
        else {
            read_das(das, data_path);
            Ancillary::read_ancillary_das(das, data_path);
            read_dds(dds, data_path);
        }


        Ancillary::read_ancillary_dds(dds, data_path);

        dds.transfer_attributes(&das);

    }

    catch (InternalErr & e) {
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (Error & e) {
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                                __FILE__, __LINE__);
    }
    catch (...) {
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

#if 0
    //dds.print(cout);
    //dds.print_das(cout);
#endif
    //
    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    DMR *dmr = bes_dmr.get_dmr();

    D4BaseTypeFactory MyD4TypeFactory;
    dmr->set_factory(&MyD4TypeFactory);
#if 0
    //dmr->set_factory(new D4BaseTypeFactory);
#endif
    dmr->build_using_dds(dds);

#if 0
    //dmr->print(cout);
#endif

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr.set_dap4_constraint(dhi);
    bes_dmr.set_dap4_function(dhi);
    dmr->set_factory(0);

    return true;
}

bool HDF4RequestHandler::hdf4_build_dmr_with_IDs(BESDataHandlerInterface & dhi) {

    BESStopWatch sw;
        if (BESDebug::IsSet(TIMING_LOG_KEY))
                sw.start("HDF4RequestHandler::hdf4_build_dmr_with_IDs", dhi.data[REQUEST_ID]);

    // Because this code does not yet know how to build a DMR directly, use
    // the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
    // First step, build the 'full DDS'
    string data_path = dhi.container->access();

    BaseTypeFactory factory;
    DDS dds(&factory, name_path(data_path), "3.2");
    dds.filename(data_path);

    DAS das;

    int32 sdfd   = -1;
    int32 fileid = -1;

    HDFSP::File *h4file = NULL;

    // Obtain HDF4 file IDs
    //SDstart
    sdfd = SDstart (const_cast < char *>(data_path.c_str()), DFACC_READ);
    if( -1 == sdfd){
        string invalid_file_msg="HDF4 SDstart error for the file ";
        invalid_file_msg +=data_path;
        invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
        throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
    }

    // H open
    fileid = Hopen(const_cast<char *>(data_path.c_str()), DFACC_READ,0);
    if (-1 == fileid) {
        SDend(sdfd);
        string invalid_file_msg="HDF4 SDstart error for the file ";
        invalid_file_msg +=data_path;
        invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
        throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
    }


#ifdef USE_HDFEOS2_LIB        

   int32 gridfd  = -1;
   int32 swathfd = -1;

   HDFEOS2::File *eosfile = NULL;
    // Obtain HDF-EOS2 file IDs  with the file open APIs. 
    // Grid open 
    gridfd = GDopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
    if (-1 == gridfd) {
        SDend(sdfd);
        Hclose(fileid);
        string invalid_file_msg="HDF4 SDstart error for the file ";
        invalid_file_msg +=data_path;
        invalid_file_msg +=". It is very possible that this file is not an HDF4 file. ";
        throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);

    }

    // Swath open 
    swathfd = SWopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
    if (-1 == swathfd) {
        SDend(sdfd);
        Hclose(fileid);
        GDclose(gridfd);
        string invalid_file_msg="HDF-EOS SWopen error for the file ";
        invalid_file_msg +=data_path;
        invalid_file_msg +=". It is very possible that this file is not an HDF-EOS2 file. ";
        throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);

    }
 
    // Here we will check if ECS_Metadata key if set. For DAP4's DMR, 
    // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
    // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
    // This is one difference between DAP2 and DAP4 mapping. Since 
    // people can use BES key to turn on the ECS metadata, so this is okay.
    // KY 2014-10-23
    bool ecs_metadata = true;
#if 0
    if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
       || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
#endif
    if((true == _disable_ecsmetadata_min) 
        || (true == _disable_ecsmetadata_all)) 
        ecs_metadata = false;
               
    try {

        read_das_use_eos2lib(das, data_path,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
#if 0
        //read_das_use_eos2lib(*das, data_path,sdfd,fileid,gridfd,swathfd,true);
#endif
        Ancillary::read_ancillary_das(das, data_path);

        // Pass file pointer(h4file, eosfile) from DAS to DDS.
        read_dds_use_eos2lib(dds, data_path,sdfd,fileid,gridfd,swathfd,h4file,eosfile);
    }

    catch(...) {
        close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
        throw;
    }
    if(eosfile != NULL)
        delete eosfile;

#else
    try {
        read_das_hdfsp(das, data_path,sdfd,fileid,&h4file);
        Ancillary::read_ancillary_das(das, data_path);

        // Pass file pointer(h4file) from DAS to DDS.
        read_dds_hdfsp(dds, data_path,sdfd,fileid,h4file);
    }
    catch(...) {
        close_hdf4_fileid(sdfd,fileid,h4file);
        throw;
    }
#endif
    if(h4file != NULL)
        delete h4file;

    Ancillary::read_ancillary_dds(dds, data_path);

    dds.transfer_attributes(&das);


// File IDs are closed by the derived class.
#if 0
        if(true == usecf) {     
#ifdef USE_HDFEOS2_LIB
            GDclose(gridfd);
            SWclose(swathfd);

#endif
            SDend(sdfd);  
            Hclose(fileid);
        }

    //dds.print(cout);
    //dds.print_das(cout);
#endif
    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    // In this handler we use a different pattern since the handler specializes the DDS/DMR.
    // First, build the DMR adding the open handle to the HDF4 dataset, then free the DMR
    // the BES built and add this one. The HDF4DMR object will close the open dataset when
    // the BES runs the DMR's destructor.
    DMR *dmr = bes_dmr.get_dmr();
    D4BaseTypeFactory MyD4TypeFactory;
    dmr->set_factory(&MyD4TypeFactory);
    dmr->build_using_dds(dds);
    HDF4DMR *hdf4_dmr = new HDF4DMR(dmr);
#ifdef USE_HDFEOS2_LIB
    hdf4_dmr->setHDF4Dataset(sdfd,fileid,gridfd,swathfd);
#else
    hdf4_dmr->setHDF4Dataset(sdfd,fileid);
#endif
    delete dmr;     // The call below will make 'dmr' unreachable; delete it now to avoid a leak.
    bes_dmr.set_dmr(hdf4_dmr); // BESDMRResponse will delete hdf4_dmr

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr.set_dap4_constraint(dhi);
    bes_dmr.set_dap4_function(dhi);
    hdf4_dmr->set_factory(0);

    return true;

}

bool HDF4RequestHandler::hdf4_build_help(BESDataHandlerInterface & dhi) {
	BESResponseObject *response = dhi.response_handler->get_response_object();
	BESInfo *info = dynamic_cast<BESInfo *> (response);
	if (!info)
		throw BESInternalError("cast error", __FILE__, __LINE__);

	map < string, string > attrs;
        attrs["name"] = MODULE_NAME ;
        attrs["version"] = MODULE_VERSION ;
	list < string > services;
	BESServiceRegistry::TheRegistry()->services_handled(HDF4_NAME, services);
	if (services.size() > 0) {
		string handles = BESUtil::implode(services, ',');
		attrs["handles"] = handles;
	}
	info->begin_tag("module", &attrs);
	info->end_tag("module");

	return true;
}

bool HDF4RequestHandler::hdf4_build_version(BESDataHandlerInterface & dhi) {
	BESResponseObject *response = dhi.response_handler->get_response_object();
	BESVersionInfo *info = dynamic_cast<BESVersionInfo *> (response);
	if (!info)
		throw BESInternalError("cast error", __FILE__, __LINE__);

	info->add_module(MODULE_NAME, MODULE_VERSION);

	return true;
}

#ifdef USE_HDFEOS2_LIB
void close_fileid(int sdfd, int fileid,int gridfd, int swathfd,HDFSP:: File* h4file, HDFEOS2::File*eosfile) {
    if(h4file !=NULL)
        delete h4file;
    if(sdfd != -1)
        SDend(sdfd);
    if(fileid != -1)
        Hclose(fileid);


    if(eosfile !=NULL)
        delete eosfile;
    if(gridfd != -1)
        GDclose(gridfd);
    if(swathfd != -1)
        SWclose(swathfd);

}
#endif
void close_hdf4_fileid(int sdfd, int fileid,HDFSP::File*h4file) {

    if(h4file !=NULL)
        delete h4file;

    if(sdfd != -1)
        SDend(sdfd);
    if(fileid != -1)
        Hclose(fileid);

}

// Handling DAS and DDS cache. Currently we only apply the cache to special products(AIRS version 6 level 3 or 2).
// read or write DAS from/to a cache file.
bool rw_das_cache_file(const string & filename, DAS *das_ptr,bool w_flag) {

    bool das_set_cache = false;
    FILE *das_file = NULL;
    
    if(false == w_flag)  // open a cache file for reading.
        das_file = fopen(filename.c_str(),"r");
    else 
        das_file = fopen(filename.c_str(),"w");

    if(NULL == das_file) {
        if(ENOENT == errno) {
            // Since the das service always tries to read the data from a cache and if the cache file doesn't exist,
            // it will generates a cache file, so here we set a flag to indicate if a cache file needs to be generated.
            if(false == w_flag) {
                BESDEBUG("h4", "DAS set cache key is true." << endl);
                das_set_cache = true;
            }
        }
        else 
            throw BESInternalError( "An error occurred trying to open a das cache file  " + get_errno(), __FILE__, __LINE__);
    }
    else {

        int fd_das = fileno(das_file);

        // Set a corresponding read(shared) or write(exclusive) lock.
        struct flock *l_das;
        if(false == w_flag)
            l_das = lock(F_RDLCK);
        else
            l_das = lock(F_WRLCK);

        // Hold a lock.
        if(fcntl(fd_das,F_SETLKW,l_das) == -1) {
            fclose(das_file);
            ostringstream oss;
            oss << "cache process: " << l_das->l_pid << " triggered a locking error: " << get_errno();
            throw BESInternalError( oss.str(), __FILE__, __LINE__);
        }

        if(false == w_flag){
            // Read DAS from a cache file
            BESDEBUG("h4", "Obtaining DAS from the cache file" << endl);
            try {
                das_ptr->parse(das_file);
            }
            catch(...) {
                if(fcntl(fd_das,F_SETLK,lock(F_UNLCK)) == -1) {
                    fclose(das_file);
                    throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
                }
                fclose(das_file);
                throw InternalErr(__FILE__,__LINE__,"Fail to parse the das from a das file.");
            }
        }
        else  {
            // Write DAS to a cache file
            BESDEBUG("h4", "write DAS to a cache file" << endl);
            try {
                das_ptr->print(das_file);
            }
            catch(...) {
                if(fcntl(fd_das,F_SETLK,lock(F_UNLCK)) == -1) {
                    fclose(das_file);
                    throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
                }
                fclose(das_file);
                throw InternalErr(__FILE__,__LINE__,"Fail to generate a das cache file.");
            }

        }

        // Unlock the cache file
        if(fcntl(fd_das,F_SETLK,lock(F_UNLCK)) == -1) { 
            fclose(das_file);
            throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
        }
        fclose(das_file);

    }

    return das_set_cache;

}

// Read dds from a cache file.
bool r_dds_cache_file(const string & cache_filename, DDS *dds_ptr,const string & hdf4_filename) {

    bool dds_set_cache = false;
    FILE *dds_file = NULL;
    dds_file = fopen(cache_filename.c_str(),"rb");

    if(NULL == dds_file) {
        if(ENOENT == errno) {
            // Since the das service always tries to read the data from a cache and if the cache file doesn't exist,
            // it is supposed that the handler should  generate a cache file, 
            // so set a flag to indicate if a cache file needs to be generated.
            dds_set_cache = true;
        }
        else 
            throw BESInternalError( "An error occurred trying to open a dds cache file  " + get_errno(), __FILE__, __LINE__);
    }
    else {

        int fd_dds = fileno(dds_file);
        struct flock *l_dds;
        l_dds = lock(F_RDLCK);

        // hold a read(shared) lock to read dds from a file.
        if(fcntl(fd_dds,F_SETLKW,l_dds) == -1) {
            fclose(dds_file);
            ostringstream oss;
            oss << "cache process: " << l_dds->l_pid << " triggered a locking error: " << get_errno();
            throw BESInternalError( oss.str(), __FILE__, __LINE__);
        }

        try {
            HDFCFUtil::read_sp_sds_dds_cache(dds_file,dds_ptr,cache_filename,hdf4_filename);
        }
        catch(...) {
            if(fcntl(fd_dds,F_SETLK,lock(F_UNLCK)) == -1) { 
                fclose(dds_file);
                throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
            }

            fclose(dds_file);
            throw InternalErr(__FILE__,__LINE__,"Fail to generate a dds cache file.");
        }

        if(fcntl(fd_dds,F_SETLK,lock(F_UNLCK)) == -1) { 
            fclose(dds_file);
            throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
        }

        fclose(dds_file);

    }

    return dds_set_cache;

}

bool check_beskeys(const string key) {

    bool found = false;
    string doset ="";
    const string dosettrue ="true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found ) {
        doset = BESUtil::lowercase( doset ) ;
        if( dosettrue == doset  || dosetyes == doset )
            return true;
    }
    return false;

}

bool get_beskeys(const string key,string &key_value) {

    bool found = false;

    TheBESKeys::TheKeys()->get_value( key, key_value, found ) ;
    return found;

}

#if 0
int get_cachekey_int(const string key) {

    bool found = false;
    int ret_value = 0;
    string doset ="";

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found ) {
        istringstream iss(doset);
        iss >>ret_value;
    }
    return ret_value;

}
#endif
#if 0
void test_func(HDFSP::File**h4file) {
cerr<<"OK to pass pointer of a NULL pointer "<<endl;

}
#endif
