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

#include <iostream>
#include <string>
#include <sstream>

#include "DODSFilter.h"
#include "DAS.h"
#include "DDS.h"
#include "DataDDS.h"

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

	switch (df.get_response()) {
	  case DODSFilter::DAS_Response: {
	      DAS das;
	
	      read_das(das, df.get_cache_dir(), df.get_dataset_name());
	      df.read_ancillary_das(das);
	      df.send_das(das);
	      break;
	  }

	  case DODSFilter::DDS_Response: {
              HDFTypeFactory factory;
	      DDS dds(&factory);

	      read_dds(dds, df.get_cache_dir(), df.get_dataset_name());
	      df.read_ancillary_dds(dds);
	      df.send_dds(dds, true);
	      break;
	  }

	  case DODSFilter::DataDDS_Response: {
              HDFTypeFactory factory;
	      DDS dds(&factory);

	      read_dds(dds, df.get_cache_dir(), df.get_dataset_name());
	      register_funcs(dds);
	      df.read_ancillary_dds(dds);
	      df.send_data(dds, stdout);
	      break;
	  }

          case DODSFilter::DDX_Response: {
              HDFTypeFactory factory;
              DDS dds(&factory);
              DAS das;

              dds.filename(df.get_dataset_name());

              read_dds(dds, df.get_cache_dir(), df.get_dataset_name());
              register_funcs(dds);
              df.read_ancillary_dds(dds);

              read_das(das, df.get_cache_dir(), df.get_dataset_name());
              df.read_ancillary_das(das);

              dds.transfer_attributes(&das);

              df.send_ddx(dds, stdout);
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

// $Log: hdf_handler.cc,v $
// Revision 1.1  2003/05/14 21:28:10  jimg
// Added.
//
// Revision 1.17  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
//
// $Log: hdf_handler.cc,v $
// Revision 1.1  2003/05/14 21:28:10  jimg
// Added.
//
// Revision 1.17  2003/01/31 02:08:36  jimg
// Merged with release-3-2-7.
//
// Revision 1.15.4.3  2003/01/30 22:40:28  jimg
// Fixed the --with-test-url argument to configure so that it works. This
// after two days of trying to figure out why the tests failes, geturl failed
// but running the filters (hdf_das, ...) worked... it was using the wrong
// URL! Arrgh.
//
// Revision 1.15.4.2  2002/12/18 23:32:50  pwest
// gcc3.2 compile corrections, mainly regarding the using statement. Also,
// missing semicolon in .y file
//
// Revision 1.16  2001/08/27 17:21:34  jimg
// Merged with version 3.2.2
//
// Revision 1.15.4.1  2001/06/14 23:58:32  jimg
// Changed to the new way DODSFilter handles version requests.
//
// Revision 1.15  2000/10/09 19:46:20  jimg
// Moved the CVS Log entries to the end of each file.
// Added code to catch Error objects thrown by the dap library.
// Changed the read() method's definition to match the dap library.
//
// Revision 1.14  1999/05/06 03:45:42  jimg
// Replaced the 1.11 changes jake added that were removed by accident.
//
// Revision 1.13  1999/05/06 03:23:36  jimg
// Merged changes from no-gnu branch
//
// Revision 1.12  1999/01/21 01:45:15  jimg
// Added call to set filename in the DDS. This is required for proper operation
// of the projection functions.
//
// Revision 1.11.6.1  1999/05/06 00:27:24  jimg
// Jakes String --> string changes
//
// Revision 1.11  1998/09/10 20:33:50  jehamby
// Updates to add cache directory and HDF-EOS support.  Also, the number of
// CGI's is reduced from three (hdf_das, hdf_dds, hdf_dods) to one (hdf_dods)
// which is symlinked to the other two locations to save disk space.
//
// Revision 1.10  1998/04/03 18:34:28  jimg
// Fixes for vgroups and Sequences from Jake Hamby
//
// Revision 1.9  1998/03/20 00:31:44  jimg
// Changed calls to the set_mime_*() to match the new parameters
//
// Revision 1.8  1998/02/05 20:14:31  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.7  1997/12/30 23:51:38  jimg
// Changed from the old-style filter program to one based on the new DODSFilter
// class.
//
// Revision 1.6  1997/08/05 22:43:13  jimg
// Changed type of option_char in `option_char = getopt(...)' from char to
// int. This fixes a bug that shows up on the SGI (where char is unsigned).
//
// Revision 1.5  1997/06/06 03:51:58  jimg
// Rewrote to use the new helper functions in cgi_util.cc. Now handles the -c
// and -v options like all the other *_dods filter programs._
//
// Revision 1.3  1997/06/05 17:20:49  jimg
// Rewrote main() so that it uses functions added to cgi_util.cc for the 2.14
// release. These function help to unify the *_dods filters so that they all
// respond more uniformly.
//
// Revision 1.4  1997/03/10 22:45:54  jimg
// Update for 2.12
//
// Revision 1.2  1997/03/05 23:23:01  todd
// Added register_funcs() call to register any server functions.
//
// Revision 1.1  1996/09/24 22:34:26  todd
// Initial revision
//
//
