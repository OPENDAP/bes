
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implmentation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

/** ais_tool is a simple proxy device for DODS URLs. Given a URL, ais_tool
    dereferences it and merges in any AIS resources that the current ais
    database returns for the URL.

    @author: jhrg */

#include "config.h"

static char rcsid[] not_used = {"$Id$"};

#include <stdio.h>

#include <iostream>
#include <string>

#include <GetOpt.h>

#define DODS_DEBUG

#include "AISConnect.h"
#include "../../../usage/old/ais-tool/AISDODSFilter.h"
#include "DAS.h"
#include "DDS.h"
#include "DataDDS.h"
#include "ObjectType.h"
#include "mime_util.h"
#include "debug.h"

static void
usage(string name)
{
    cerr << "Usage: " << name
	 << " [madDV] -- <url>" << endl
	 << "       m: Output a MIME header." << endl
	 << "       a: Get a DAS object from the URL." << endl
	 << "       s: Get a DDS object from the URL." << endl
	 << "       D: Get a DataDDS object from the URL." << endl
	 << "       B: Use this XML as the ais dataBase." << endl
	 << "       V: Print the version number and exit" << endl
	 << endl;
}

int
main(int argc, char * argv[])
{
    DBG(cerr << "Entering main... " << endl);

    // Put all the options for DODSFilter *and* this tool here, that way
    // DODSFilter's options won't get flagged as errors. It's OK to process
    // the options twice (once here and once in DODSFilter's ctor).
    AISDODSFilter df(argc, argv);

    try {
	if (df.get_ais_db() == "")
	    throw Error("The AIS database was not specified.\n\
The AIS proxy server has not been configured correctly.\n\
In the DODS server script (nph-ais), set the name of the AIS database.");

	AISConnect *url = new AISConnect(df.get_dataset_name(),
					 df.get_ais_db());

	// *** Set accept_deflate ???
	url->set_cache_enabled(false);

	switch(df.get_response()) {
	  case dods_das: {
	    DAS das;
	    DBG(cerr << "About to get the DAS object." << endl);
	    url->request_das(das);
	    DBG(cerr << "Get das instance from server." << endl);
	    df.send_das(stdout, das);
	    break;
	  }

	  case dods_dds: {
            BaseTypeFactory btf;
	    DDS dds(&btf, df.get_dataset_name());
	    url->request_dds(dds);
            ConstraintEvaluator ce;
	    df.send_dds(stdout, dds, ce);
	    break;
	  }

	  case dods_data: {
            BaseTypeFactory btf;
	    DataDDS dds(&btf, df.get_dataset_name());
	    DBG(cerr << "URL: " << url->URL(false) << endl);
	    DBG(cerr << "CE: " << df.get_ce() << endl);
	    url->request_data(dds, df.get_ce()); // other requests ignore this
	    DBG(cerr << "Successfully got data from the server..." << endl);

	    // Before sending, mark all the variables as read so that the
	    // read() methods don't get called by serialize().
	    for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); ++i)
		(*i)->set_read_p(true);
	    df.set_ce("");	// zero CE to avoid re-applying

            ConstraintEvaluator ce;
	    df.send_data(dds, ce, stdout);
	    break;
	  }

	  default:
	    throw Error("The AIS proxy server only knows how to process simple OPeNDAP objects.");
	}
    }
    catch (Error &e) {
	DBG(cerr << "Caught an Error: " << e.get_error_message()
	    << endl);
	set_mime_text(stdout, dods_error, df.get_cgi_version());
	e.print(stdout);
	return 1;
    }

    DBG(cerr << "exiting." << endl);

    return 0;
}

// $Log: ais_tool.cc,v $
// Revision 1.3  2003/03/14 19:59:56  jimg
// Once the client part of the gateway has a full DataDDS, the AISDODSFilter
// object's constraint expression must be zeroed before calling
// DODSFilter::send_data().
//
// Revision 1.2  2003/03/14 17:22:55  jimg
// Added call to set_read_p() for all the variables in a DataDDS before the call
// to send_data(). This prevents the read() methods from being called.
//
// Revision 1.1  2003/03/13 23:37:32  jimg
// Added.
//
