// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// Copyright (c) 1996, California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.
//
// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
// $RCSfile: hdf_handler.cc,v $ - server CGI for data transfer
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <sstream>
 
#include "DODSFilter.h"
#include "DAS.h"
#include "DDS.h"
#include "DataDDS.h"
#include "ConstraintEvaluator.h"

#include "debug.h"
#include "cgi_util.h"
#include "ObjectType.h"

#include "HDFTypeFactory.h"
#include "hcerr.h"
#include "dhdferr.h"

using namespace std;

extern void read_das(DAS& das, const string& cachedir, const string& filename);
extern void read_dds(DDS& dds, const string& cachedir, const string& filename);
extern void register_funcs(DDS& dds);

const string cgi_version = PACKAGE_VERSION;

int 
main(int argc, char *argv[])
{
    DBG(cerr << "Starting the HDF server." << endl);

    try {
	DODSFilter df(argc, argv);
	if (df.get_cgi_version() == "")
	    df.set_cgi_version(cgi_version);
            
       string cachedir = df.get_cache_dir();
       
       string dummy = df.get_cache_dir()+ "/dummy";
       int fd = open(dummy.c_str(),  O_CREAT|O_WRONLY|O_TRUNC);
       unlink(dummy.c_str());
       if (fd == -1) {
           cachedir = "";
           ErrMsgT(string("Could not create a file in the cache directory (")
                   + df.get_cache_dir() + ")");
       }
       close(fd);

	switch (df.get_response()) {
	  case DODSFilter::DAS_Response: {
	      DAS das;
	
	      read_das(das, cachedir, df.get_dataset_name());
	      df.read_ancillary_das(das);
	      df.send_das(das);
	      break;
	  }

	  case DODSFilter::DDS_Response: {
              HDFTypeFactory factory;
	      DDS dds(&factory);
              ConstraintEvaluator ce;

	      read_dds(dds, cachedir, df.get_dataset_name());
	      df.read_ancillary_dds(dds);
	      df.send_dds(dds, ce, true);
	      break;
	  }

	  case DODSFilter::DataDDS_Response: {
              HDFTypeFactory factory;
	      DDS dds(&factory);
              ConstraintEvaluator ce;

	      read_dds(dds, cachedir, df.get_dataset_name());
	      register_funcs(dds);
	      df.read_ancillary_dds(dds);
	      df.send_data(dds, ce, stdout);
	      break;
	  }

          case DODSFilter::DDX_Response: {
              HDFTypeFactory factory;
              DDS dds(&factory);
              ConstraintEvaluator ce;
              DAS das;

              dds.filename(df.get_dataset_name());

              read_dds(dds, cachedir, df.get_dataset_name());
              register_funcs(dds);
              df.read_ancillary_dds(dds);

              read_das(das, cachedir, df.get_dataset_name());
              df.read_ancillary_das(das);

              dds.transfer_attributes(&das);

              df.send_ddx(dds, ce, stdout);
              break;
          }

	  case DODSFilter::Version_Response: {
	      df.send_version_info();

	      break;
	  }

	  default:
	    df.print_usage();	// Throws Error
	}
    }
    catch (dhdferr &d) {
        ostringstream s;
	s << "hdf4 handler: " << d;
        ErrMsgT(s.str());
	Error e(unknown_error, d.errmsg());
	set_mime_text(stdout, dods_error, cgi_version);
	e.print(stdout);
	return 1;
    }
    catch (hcerr &h) {
        ostringstream s;
	s << "hdf4 handler: " << h;
        ErrMsgT(s.str());
	Error e(unknown_error, h.errmsg());
	set_mime_text(stdout, dods_error, cgi_version);
	e.print(stdout);
	return 1;
    }
    catch (Error &e) {
        string s;
	s = (string)"hdf4 handler: " + e.get_error_message() + "\n";
        ErrMsgT(s);
	set_mime_text(stdout, dods_error, cgi_version);
	e.print(stdout);
	return 1;
    }
    catch (...) {
        string s("hdf4 handler: Unknown exception");
	ErrMsgT(s);
	Error e(unknown_error, s);
	set_mime_text(stdout, dods_error, cgi_version);
	e.print(stdout);
	return 1;
    }
    
    DBG(cerr << "HDF server exitied successfully." << endl);
    return 0;
}
