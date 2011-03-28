
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <ConstraintEvaluator.h>
#include <Ancillary.h>
#include "config_hdf.h"

#define HDF4_NAME "h4"

extern void read_das(DAS & das, const string & filename);
extern void read_dds(DDS & dds, const string & filename);


#ifdef USE_HDFEOS2_LIB
#include "HE2CFNcML.h"
#include "HE2CFShortName.h"
#include "HE2CF.h"
void
read_conf_xml(DAS & das, const string & filename,
              HE2CFNcML* ncml,
              HE2CFShortName* sn, HE2CFShortName* sn_dim,
              HE2CFUniqName* un, HE2CFUniqName* un_dim);
void
set_counters(HE2CFShortName* sn, HE2CFShortName* sn_dim,
             HE2CFUniqName* un, HE2CFUniqName* un_dim);
void
read_das_use_eos2lib(DAS & das, const string & filename,
             HE2CFNcML* ncml,
             HE2CFShortName* sn, HE2CFShortName* sn_dim,
             HE2CFUniqName* un, HE2CFUniqName* un_dim); 

void
read_dds_use_eos2lib(DDS & dds, const string & filename,
             HE2CFNcML* ncml,
             HE2CFShortName* sn, HE2CFShortName* sn_dim,
             HE2CFUniqName* un, HE2CFUniqName* un_dim);
#endif


HDF4RequestHandler::HDF4RequestHandler(const string & name)
    :BESRequestHandler(name)
{
    add_handler(DAS_RESPONSE, HDF4RequestHandler::hdf4_build_das);
    add_handler(DDS_RESPONSE, HDF4RequestHandler::hdf4_build_dds);
    add_handler(DATA_RESPONSE, HDF4RequestHandler::hdf4_build_data);
    add_handler(HELP_RESPONSE, HDF4RequestHandler::hdf4_build_help);
    add_handler(VERS_RESPONSE, HDF4RequestHandler::hdf4_build_version);
}

HDF4RequestHandler::~HDF4RequestHandler()
{
}

bool HDF4RequestHandler::hdf4_build_das(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(response) ;
    if( !bdas )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
	bdas->set_container( dhi.container->get_symbolic_name() ) ;
	DAS *das = bdas->get_das();

        string accessed = dhi.container->access();
#ifdef USE_HDFEOS2_LIB
        HE2CFNcML ncml;         // for conf input file
        HE2CFShortName sn;      // for variable 
        HE2CFShortName sn_dim;  // for dimension
        HE2CFUniqName un;       // for variable name clashing
        HE2CFUniqName un_dim;   // for dimension name clashing        
        read_conf_xml(*das, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);
        read_das_use_eos2lib(*das, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);
#else
        read_das(*das, accessed); 
#endif
	Ancillary::read_ancillary_das( *das, accessed ) ;

	bdas->clear_container() ;
    }
    catch(BESError & e) {
        throw;
    }
    catch(InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(...) {
        string s = "unknown exception caught building HDF4 DAS";
        BESDapError ex(s, true, unknown_error, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_dds(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(response);
    if( !bdds )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
	bdds->set_container( dhi.container->get_symbolic_name() ) ;
	DDS *dds = bdds->get_dds();
	ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS ;
	BESDASResponse bdas( das ) ;
	bdas.set_container( dhi.container->get_symbolic_name() ) ;
#ifdef USE_HDFEOS2_LIB        
        HE2CFNcML ncml;         // for conf input file
        HE2CFShortName sn;      // for variable
        HE2CFShortName sn_dim;  // for dimension
        HE2CFUniqName un;       // for variable name clashing
        HE2CFUniqName un_dim;   // for dimension name clashing        
        read_conf_xml(*das, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);
        read_das_use_eos2lib(*das, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);        
#else
        read_das( *das, accessed );
#endif
	Ancillary::read_ancillary_das( *das, accessed ) ;

#ifdef USE_HDFEOS2_LIB        
        // Reset all counters to 0.
        set_counters(&sn, &sn_dim, &un, &un_dim);
        read_dds_use_eos2lib(*dds, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);
#else
        read_dds( *dds, accessed );
#endif

	Ancillary::read_ancillary_dds( *dds, accessed ) ;

        dds->transfer_attributes( das ) ;

	bdds->set_constraint( dhi ) ;

	bdds->clear_container() ;
    }
    catch(BESError & e) {
        throw;
    }
    catch(InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(...) {
        string s = "unknown exception caught building HDF4 DDS";
        BESDapError ex(s, true, unknown_error, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_data(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
	bdds->set_container( dhi.container->get_symbolic_name() ) ;
	DataDDS *dds = bdds->get_dds();
	ConstraintEvaluator & ce = bdds->get_ce();

        string accessed = dhi.container->access();
        dds->filename(accessed);

        DAS *das = new DAS ;
	BESDASResponse bdas( das ) ;
	bdas.set_container( dhi.container->get_symbolic_name() ) ;
#ifdef USE_HDFEOS2_LIB        
        HE2CFNcML ncml;         // for conf input file
        HE2CFShortName sn;      // for variable
        HE2CFShortName sn_dim;  // for dimension
        HE2CFUniqName un;       // for variable name clashing
        HE2CFUniqName un_dim;   // for dimension name clashing                
        read_conf_xml(*das, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);
        read_das_use_eos2lib(*das, accessed, &ncml, &sn, &sn_dim, &un, &un_dim) ;
#else
        read_das( *das, accessed );
#endif
	Ancillary::read_ancillary_das( *das, accessed ) ;
#ifdef USE_HDFEOS2_LIB                
        // Reset all counters to 0.
        set_counters(&sn, &sn_dim, &un, &un_dim);
        read_dds_use_eos2lib(*dds, accessed, &ncml, &sn, &sn_dim, &un, &un_dim);
#else
        read_dds(*dds, accessed );
#endif
	Ancillary::read_ancillary_dds( *dds, accessed ) ;

        dds->transfer_attributes( das ) ;

	bdds->set_constraint( dhi ) ;

	bdds->clear_container() ;
    }
    catch(BESError & e) {
        throw;
    }
    catch(InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(...) {
        string s = "unknown exception caught building HDF4 DataDDS";
        BESDapError ex(s, true, unknown_error, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF4RequestHandler::hdf4_build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    map<string,string> attrs ;
    attrs["name"] = PACKAGE_NAME ;
    attrs["version"] = PACKAGE_VERSION ;
    list<string> services ;
    BESServiceRegistry::TheRegistry()->services_handled( HDF4_NAME, services );
    if( services.size() > 0 )
    {
	string handles = BESUtil::implode( services, ',' ) ;
	attrs["handles"] = handles ;
    }
    info->begin_tag( "module", &attrs ) ;
    info->end_tag( "module" ) ;

    return true;
}

bool HDF4RequestHandler::hdf4_build_version(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast < BESVersionInfo * >(response);
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    info->add_module( PACKAGE_NAME, PACKAGE_VERSION ) ;

    return true;
}
