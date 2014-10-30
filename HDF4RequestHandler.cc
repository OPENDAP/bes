
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf4_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
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
#include <unistd.h>

//#undef USE_DAP4 
//#define USE_DAP4 1
#ifdef USE_DAP4
#include <DMR.h>
#include <D4BaseTypeFactory.h>
#include <BESDMRResponse.h>
#endif
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
//#include <ConstraintEvaluator.h>
#include <Ancillary.h>
#include "config_hdf.h"

#define HDF4_NAME "h4"
#include "HE2CF.h"
#include "HDF4_DDS.h"

#ifdef USE_DAP4
#include "HDF4_DMR.h"
#endif 

#include "HDFCFUtil.h"

//#define KENT2
#undef KENT2
#ifdef KENT2
#include <sys/time.h>
#endif

extern void read_das(DAS & das, const string & filename);
extern void read_dds(DDS & dds, const string & filename);

extern bool read_dds_hdfsp(DDS & dds, const string & filename,int32 sdfd, int32 fileid,HDFSP::File*h4file);

extern bool read_das_hdfsp(DAS & das, const string & filename,int32 sdfd, int32 fileid,HDFSP::File**h4fileptr);


#ifdef USE_HDFEOS2_LIB

void read_das_use_eos2lib(DAS & das, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,bool ecs_metadata,HDFSP::File**h4file,HDFEOS2::File**eosfile);
void read_dds_use_eos2lib(DDS & dds, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd,HDFSP::File*h4file,HDFEOS2::File*eosfile);
void close_fileid(const int sdfd, const int fileid,const int gridfd, const int swathfd,HDFSP::File*h4file,HDFEOS2::File*eosfile); 

#endif

void close_hdf4_fileid(const int sdfd,const int fileid,HDFSP::File*h4file);
//void test_func(HDFSP::File**h4file);

HDF4RequestHandler::HDF4RequestHandler(const string & name) :
	BESRequestHandler(name) {
	add_handler(DAS_RESPONSE, HDF4RequestHandler::hdf4_build_das);
	add_handler(DDS_RESPONSE, HDF4RequestHandler::hdf4_build_dds);
	add_handler(DATA_RESPONSE, HDF4RequestHandler::hdf4_build_data);
#ifdef USE_DAP4
	add_handler(DMR_RESPONSE, HDF4RequestHandler::hdf4_build_dmr);
	add_handler(DAP4DATA_RESPONSE, HDF4RequestHandler::hdf4_build_dmr);
#endif
	add_handler(HELP_RESPONSE, HDF4RequestHandler::hdf4_build_help);
	add_handler(VERS_RESPONSE, HDF4RequestHandler::hdf4_build_version);
}

HDF4RequestHandler::~HDF4RequestHandler() {
}

bool HDF4RequestHandler::hdf4_build_das(BESDataHandlerInterface & dhi) {

    bool found = false;
    bool usecf = false;

    string key="H4.EnableCF";
    string doset;

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found )
    {
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) {

           // This is the CF option, go to the CF function
            usecf = true;
        }
    }

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDASResponse *bdas = dynamic_cast<BESDASResponse *> (response);
    if (!bdas)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdas->set_container(dhi.container->get_symbolic_name());
        DAS *das = bdas->get_das();

        string accessed = dhi.container->access();
         
        if (true == usecf) {

            int32 sdfd    = -1;
            int32 fileid  = -1;

            HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd)
                throw Error(cannot_read_file,"HDF4 SDstart error");

            // H open
            fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                throw Error(cannot_read_file,"HDF4 Hopen error");
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
                throw Error(cannot_read_file,"HDF-EOS GDopen error");
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                throw Error(cannot_read_file,"HDF-EOS SWopen error");
            }
            
            try {
                bool ecs_metadata = !(HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"));
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
                //throw InternalErr(__FILE__,__LINE__,"read_das_use_eos2lib error");
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

    bool found = false;
    bool usecf = false;

    string key="H4.EnableCF";
    string doset;

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found )
    {
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) {
           // This is the CF option, go to the CF function
            usecf = true;
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
        //ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

        if (true == usecf) {

            int32 sdfd   = -1;
            int32 fileid = -1;
            HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd)
                throw Error(cannot_read_file,"HDF4 SDstart error");

            // H open
            fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                throw Error(cannot_read_file,"HDF4 Hopen error");
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
                throw Error(cannot_read_file,"HDF-EOS GDopen error");
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                throw Error(cannot_read_file,"HDF-EOS SWopen error");
            }
 
            try {
                bool ecs_metadata = !(HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"));
                read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
                //read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,true);
                Ancillary::read_ancillary_das(*das, accessed);

#if 0
if(eosfile == NULL) 
cerr<<"HDFEOS2 file pointer is NULL "<<endl;
if(h4file == NULL)
cerr<<"HDF4 file pointer is NULL"<<endl;
#endif
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
                //throw InternalErr(__FILE__,__LINE__,"read_dds_hdfsp error");
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

    bool found = false;
    bool usecf = false;

    string key="H4.EnableCF";
    string doset;

    int32 sdfd   = -1;
    int32 fileid = -1;


    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found )
    {
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) {

           // This is the CF option, go to the CF function
            usecf = true;
        }
    }

    // Since passing file IDs requires to use the derived class and it
    // causes the management of code structure messy, we first handle this with
    // another method.
    if(true == usecf) {
       
        if(true == HDFCFUtil::check_beskeys("H4.EnablePassFileID"))
            return hdf4_build_data_with_IDs(dhi);

    }

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        DataDDS *dds = bdds->get_dds();

        // Not sure why keep the following line, it is not used.
        //ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

        if (true == usecf) {

             HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
            if( -1 == sdfd)
                throw Error(cannot_read_file,"HDF4 SDstart error");

            // H open
            fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                throw Error(cannot_read_file,"HDF4 Hopen error");
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
                throw Error(cannot_read_file,"HDF-EOS GDopen error");
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                throw Error(cannot_read_file,"HDF-EOS SWopen error");
            }

            try {

                // Here we will check if ECS_Metadata key if set. For DataDDS, 
                // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
                // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
                bool ecs_metadata = true;
                if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
                    || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
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
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_data_with_IDs(BESDataHandlerInterface & dhi) {


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

        HDF4DDS *hdds = new HDF4DDS(bdds->get_dds());

        delete bdds->get_dds();

        bdds->set_dds(hdds);

        //ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        hdds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());


        // Obtain HDF4 file IDs
        //SDstart
        sdfd = SDstart (const_cast < char *>(accessed.c_str()), DFACC_READ);
        if( -1 == sdfd)
            throw Error(cannot_read_file,"HDF4 SDstart error");

        // H open
        fileid = Hopen(const_cast<char *>(accessed.c_str()), DFACC_READ,0);
        if (-1 == fileid) {
            SDend(sdfd);
            throw Error(cannot_read_file,"HDF4 Hopen error");
        }

#ifdef USE_HDFEOS2_LIB        


        // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            
        // Grid open 
        gridfd = GDopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
        if (-1 == gridfd) {
            SDend(sdfd);
            Hclose(fileid);
            throw Error(cannot_read_file,"HDF-EOS GDopen error");
        }

        // Swath open 
        swathfd = SWopen(const_cast < char *>(accessed.c_str()), DFACC_READ);
        if (-1 == swathfd) {
            SDend(sdfd);
            Hclose(fileid);
            GDclose(gridfd);
            throw Error(cannot_read_file,"HDF-EOS SWopen error");
        }

        hdds->setHDF4Dataset(sdfd,fileid,gridfd,swathfd);

        // Here we will check if ECS_Metadata key if set. For DataDDS, 
        // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
        // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
        bool ecs_metadata = true;
        if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
            || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
            ecs_metadata = false;
               

        read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
        //read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd,true);

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
#ifdef USE_HDFEOS2_LIB
        close_fileid(sdfd,fileid,gridfd,swathfd,h4file,eosfile);
#else
        close_hdf4_fileid(sdfd,fileid,h4file);
#endif
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

#ifdef USE_DAP4
bool HDF4RequestHandler::hdf4_build_dmr(BESDataHandlerInterface &dhi)
{

    // Because this code does not yet know how to build a DMR directly, use
    // the DMR ctor that builds a DMR using a 'full DDS' (a DDS with attributes).
    // First step, build the 'full DDS'
    string data_path = dhi.container->access();

    BaseTypeFactory factory;
    DDS dds(&factory, name_path(data_path), "3.2");
    dds.filename(data_path);

    DAS das;

    bool found = false;
    bool usecf = false;

    string key="H4.EnableCF";
    string doset;

    int32 sdfd   = -1;
    int32 fileid = -1;
    int32 gridfd  = -1;
    int32 swathfd = -1;


    // Check if CF option is turned on. 
    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found )
    {
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) {

           // This is the CF option, go to the CF function
            usecf = true;
        }
    }
    // Since passing file IDs requires to use the derived class and it
    // causes the management of code structure messy, we first handle this with
    // another method.
    if(true == usecf) {
       
        if(true == HDFCFUtil::check_beskeys("H4.EnablePassFileID"))
            return hdf4_build_dmr_with_IDs(dhi);

    }


    try {

        if (true == usecf) {

            HDFSP::File *h4file = NULL;

            // Obtain HDF4 file IDs
            //SDstart
            sdfd = SDstart (const_cast < char *>(data_path.c_str()), DFACC_READ);
            if( -1 == sdfd)
                throw Error(cannot_read_file,"HDF4 SDstart error");

            // H open
            fileid = Hopen(const_cast<char *>(data_path.c_str()), DFACC_READ,0);
            if (-1 == fileid) {
                SDend(sdfd);
                throw Error(cannot_read_file,"HDF4 Hopen error");
            }

#ifdef USE_HDFEOS2_LIB        

            HDFEOS2::File *eosfile = NULL;

            // Obtain HDF-EOS2 file IDs  with the file open APIs. 
            // Grid open 
            gridfd = GDopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
            if (-1 == gridfd) {
                SDend(sdfd);
                Hclose(fileid);
                throw Error(cannot_read_file,"HDF-EOS GDopen error");
            }

            // Swath open 
            swathfd = SWopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
            if (-1 == swathfd) {
                SDend(sdfd);
                Hclose(fileid);
                GDclose(gridfd);
                throw Error(cannot_read_file,"HDF-EOS SWopen error");
            }

 
            // Here we will check if ECS_Metadata key if set. For DAP4's DMR, 
            // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
            // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
            // This is one difference between DAP2 and DAP4 mapping. Since 
            // people can use BES key to turn on the ECS metadata, so this is okay.
            // KY 2014-10-23
            bool ecs_metadata = true;
            if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
                || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
                ecs_metadata = false;
               
            try {

                read_das_use_eos2lib(das, data_path,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);

                //read_das_use_eos2lib(*das, data_path,sdfd,fileid,gridfd,swathfd,true);
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
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }


//dds.print(cout);
//dds.print_das(cout);
    //
    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    DMR *dmr = bes_dmr.get_dmr();
#if 0
    D4BaseTypeFactory* MyD4TypeFactory = NULL;
    MyD4TypeFactory = new D4BaseTypeFactory;
    dmr->set_factory(MyD4TypeFactory);
#endif

    D4BaseTypeFactory MyD4TypeFactory;
    dmr->set_factory(&MyD4TypeFactory);
    //dmr->set_factory(new D4BaseTypeFactory);
    dmr->build_using_dds(dds);

//dmr->print(cout);

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr.set_dap4_constraint(dhi);
    bes_dmr.set_dap4_function(dhi);
    dmr->set_factory(0);

#if 0
    if(MyD4TypeFactory !=NULL)
    delete MyD4TypeFactory;
#endif

    return true;
}

bool HDF4RequestHandler::hdf4_build_dmr_with_IDs(BESDataHandlerInterface & dhi) {

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
    int32 gridfd  = -1;
    int32 swathfd = -1;

    HDFSP::File *h4file = NULL;

    // Obtain HDF4 file IDs
    //SDstart
    sdfd = SDstart (const_cast < char *>(data_path.c_str()), DFACC_READ);
    if( -1 == sdfd)
        throw Error(cannot_read_file,"HDF4 SDstart error");

    // H open
    fileid = Hopen(const_cast<char *>(data_path.c_str()), DFACC_READ,0);
    if (-1 == fileid) {
        SDend(sdfd);
        throw Error(cannot_read_file,"HDF4 Hopen error");
    }


#ifdef USE_HDFEOS2_LIB        

    HDFEOS2::File *eosfile = NULL;
    // Obtain HDF-EOS2 file IDs  with the file open APIs. 
    // Grid open 
    gridfd = GDopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
    if (-1 == gridfd) {
        SDend(sdfd);
        Hclose(fileid);
        throw Error(cannot_read_file,"HDF-EOS GDopen error");
    }

    // Swath open 
    swathfd = SWopen(const_cast < char *>(data_path.c_str()), DFACC_READ);
    if (-1 == swathfd) {
        SDend(sdfd);
        Hclose(fileid);
        GDclose(gridfd);
        throw Error(cannot_read_file,"HDF-EOS SWopen error");
    }
 
    // Here we will check if ECS_Metadata key if set. For DAP4's DMR, 
    // if either H4.DisableECSMetaDataMin or H4.DisableECSMetaDataAll is set,
    // the HDF-EOS2 coremetadata or archivemetadata will not be passed to DAP.
    // This is one difference between DAP2 and DAP4 mapping. Since 
    // people can use BES key to turn on the ECS metadata, so this is okay.
    // KY 2014-10-23
    bool ecs_metadata = true;
    if((true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataMin")) 
       || (true == HDFCFUtil::check_beskeys("H4.DisableECSMetaDataAll"))) 
        ecs_metadata = false;
               
    try {

        read_das_use_eos2lib(das, data_path,sdfd,fileid,gridfd,swathfd,ecs_metadata,&h4file,&eosfile);
        //read_das_use_eos2lib(*das, data_path,sdfd,fileid,gridfd,swathfd,true);
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
#endif

    //dds.print(cout);
    //dds.print_das(cout);
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

#endif
bool HDF4RequestHandler::hdf4_build_help(BESDataHandlerInterface & dhi) {
	BESResponseObject *response = dhi.response_handler->get_response_object();
	BESInfo *info = dynamic_cast<BESInfo *> (response);
	if (!info)
		throw BESInternalError("cast error", __FILE__, __LINE__);

	map < string, string > attrs;
	attrs["name"] = PACKAGE_NAME;
	attrs["version"] = PACKAGE_VERSION;
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

	info->add_module(PACKAGE_NAME, PACKAGE_VERSION);

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


#if 0
void test_func(HDFSP::File**h4file) {
cerr<<"OK to pass pointer of a NULL pointer "<<endl;

}
#endif
