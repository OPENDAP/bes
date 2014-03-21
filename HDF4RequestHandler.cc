
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


extern void read_das(DAS & das, const string & filename);
extern void read_dds(DDS & dds, const string & filename);

extern bool read_dds_hdfsp(DDS & dds, const string & filename,int32 sdfd, int32 fileid);

extern bool read_das_hdfsp(DAS & das, const string & filename,int32 sdfd, int32 fileid);


#ifdef USE_HDFEOS2_LIB

void read_das_use_eos2lib(DAS & das, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd);
void read_dds_use_eos2lib(DDS & dds, const string & filename,int32 sdfd,int32 fileid, int32 gridfd, int32 swathfd);

#endif

HDF4RequestHandler::HDF4RequestHandler(const string & name) :
	BESRequestHandler(name) {
	add_handler(DAS_RESPONSE, HDF4RequestHandler::hdf4_build_das);
	add_handler(DDS_RESPONSE, HDF4RequestHandler::hdf4_build_dds);
	add_handler(DATA_RESPONSE, HDF4RequestHandler::hdf4_build_data);
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

            int32 sdfd   = -1;
            int32 fileid = -1;
            int32 gridfd  = -1;
            int32 swathfd = -1;

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
            
            try {
                read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd);
            }
            catch(...) {
                close_fileid(sdfd,fileid,gridfd,swathfd);
                throw;
                //throw InternalErr(__FILE__,__LINE__,"read_das_use_eos2lib error");
            }
            GDclose(gridfd);
            SWclose(swathfd);

#else
            try {
                read_das_hdfsp(*das,accessed,sdfd,fileid);
            }
            catch(...) {
               close_hdf4_fileid(sdfd,fileid); 
               throw;
               //throw InternalErr(__FILE__,__LINE__,"read_das_hdfsp error");
            }
#endif
            SDend(sdfd);
            Hclose(fileid);
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
            int32 gridfd  = -1;
            int32 swathfd = -1;

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
 
            try {
                read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd);
                Ancillary::read_ancillary_das(*das, accessed);

                read_dds_use_eos2lib(*dds, accessed,sdfd,fileid,gridfd,swathfd);
            }
            catch(...) {
                close_fileid(sdfd,fileid,gridfd,swathfd);
                throw;
                //throw InternalErr(__FILE__,__LINE__,"read_dds_use_eos2lib error");
            }
            GDclose(gridfd);
            SWclose(swathfd);

#else
            try {
                read_das_hdfsp(*das, accessed,sdfd,fileid);
                Ancillary::read_ancillary_das(*das, accessed);

                read_dds_hdfsp(*dds, accessed,sdfd,fileid);
            }
            catch(...) {
                close_hdf4_fileid(sdfd,fileid);
                throw;
                //throw InternalErr(__FILE__,__LINE__,"read_dds_hdfsp error");
            }

#endif

            SDend(sdfd);
            Hclose(fileid);
 
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
    int32 gridfd  = -1;
    int32 swathfd = -1;


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
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *> (response);

    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    try {
        bdds->set_container(dhi.container->get_symbolic_name());

        HDF4DDS *hdds = new HDF4DDS(bdds->get_dds());

        delete bdds->get_dds();

        bdds->set_dds(hdds);

        //ConstraintEvaluator & ce = bdds->get_ce();

        // Not sure why keep the following line, it is not used.
        //ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        hdds->filename(accessed);

        DAS *das = new DAS;
        BESDASResponse bdas(das);
        bdas.set_container(dhi.container->get_symbolic_name());

        if (true == usecf) {

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
 
            // No need to use a try-catch block since we will not close the file IDs.
            // The low-level error messages will just be popped out.
            read_das_use_eos2lib(*das, accessed,sdfd,fileid,gridfd,swathfd);
            Ancillary::read_ancillary_das(*das, accessed);

            read_dds_use_eos2lib(*hdds, accessed,sdfd,fileid,gridfd,swathfd);


#else
            hdds->setHDF4Dataset(sdfd,fileid);
            read_das_hdfsp(*das, accessed,sdfd,fileid);
            Ancillary::read_ancillary_das(*das, accessed);


            read_dds_hdfsp(*hdds, accessed,sdfd,fileid);
#endif


        }
        else {
            read_das(*das, accessed);
            Ancillary::read_ancillary_das(*das, accessed);
            read_dds(*hdds, accessed);
        }


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
        string s = "unknown exception caught building HDF4 DataDDS";
        throw BESDapError(s, true, unknown_error, __FILE__, __LINE__);
    }

    return true;
}

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

void close_fileid(int sdfd, int fileid,int gridfd, int swathfd) {

    if(sdfd != -1)
        SDend(sdfd);
    if(fileid != -1)
        Hclose(fileid);
#ifdef USE_HDFEOS2_LIB
    if(gridfd != -1)
        GDclose(gridfd);
    if(swathfd != -1)
        SWclose(swathfd);
#endif

}
void close_hdf4_fileid(int sdfd, int fileid) {

    if(sdfd != -1)
        SDend(sdfd);
    if(fileid != -1)
        Hclose(fileid);

}
