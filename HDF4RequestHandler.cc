
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

extern void read_das(DAS & das, const string & cachedir,
                     const string & filename);
extern void read_dds(DDS & dds, const string & cachedir,
                     const string & filename);

string HDF4RequestHandler::_cachedir = "";

HDF4RequestHandler::HDF4RequestHandler(const string & name)
    :BESRequestHandler(name)
{
    add_handler(DAS_RESPONSE, HDF4RequestHandler::hdf4_build_das);
    add_handler(DDS_RESPONSE, HDF4RequestHandler::hdf4_build_dds);
    add_handler(DATA_RESPONSE, HDF4RequestHandler::hdf4_build_data);
    add_handler(HELP_RESPONSE, HDF4RequestHandler::hdf4_build_help);
    add_handler(VERS_RESPONSE, HDF4RequestHandler::hdf4_build_version);

    if (HDF4RequestHandler::_cachedir.empty()) {
        bool found = false;
        _cachedir = TheBESKeys::TheKeys()->get_key("HDF4.CacheDir", found);
#if 0
        if (!found || _cachedir == "")
            _cachedir = "/tmp";
#else
        // Turn caching off
        if (!found)
            _cachedir = "";
#endif
        if (!_cachedir.empty()) {
        string HDF4_file = _cachedir + "/HDF4XXXXXX";
        char *HDF4_temp = new char[HDF4_file.length() + 1];
	string::size_type len =
	HDF4_file.copy(HDF4_temp,HDF4_file.length());
	*(HDF4_temp+len) = '\0';
        mode_t original_mask = umask(077);
        int fd = mkstemp(HDF4_temp);
        (void)umask(original_mask);

        if (fd == -1) {
	    delete[] HDF4_temp;
#if 0
            if (_cachedir == "/tmp") {
		// fd is -1 so should not close here
                //close(fd);
#endif
                string err =
                    "Could not create a file in the cache directory (" +
                    _cachedir + ")";
                throw BESInternalError(err, __FILE__, __LINE__);
#if 0
            }
            _cachedir = "/tmp";
#endif
        }
	else
	{
	    // should only do this if we were successful in running
	    // mkstemp and got back a valid fd.
	    (void)unlink(HDF4_temp);
	    close(fd);
	    delete[] HDF4_temp;
	}
        }
    }
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
        read_das(*das, _cachedir, accessed);
	Ancillary::read_ancillary_das( *das, accessed ) ;

	bdas->clear_container() ;
    }
    catch(BESError & e) {
        throw e;
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
        read_dds(*dds, _cachedir, accessed);
	Ancillary::read_ancillary_dds( *dds, accessed ) ;

        DAS *das = new DAS ;
	BESDASResponse bdas( das ) ;
	bdas.set_container( dhi.container->get_symbolic_name() ) ;
        read_das( *das, _cachedir, accessed ) ;
	Ancillary::read_ancillary_das( *das, accessed ) ;
        dds->transfer_attributes( das ) ;

	bdds->set_constraint( dhi ) ;

	bdds->clear_container() ;
    }
    catch(BESError & e) {
        throw e;
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
        read_dds(*dds, _cachedir, accessed);
	Ancillary::read_ancillary_dds( *dds, accessed ) ;

        DAS *das = new DAS ;
	BESDASResponse bdas( das ) ;
	bdas.set_container( dhi.container->get_symbolic_name() ) ;
        read_das( *das, _cachedir, accessed ) ;
	Ancillary::read_ancillary_das( *das, accessed ) ;
        dds->transfer_attributes( das ) ;

	bdds->set_constraint( dhi ) ;

	bdds->clear_container() ;
    }
    catch(BESError & e) {
        throw e;
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
