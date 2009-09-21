// dao_csv_handler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Stephan Zednik <zednik@ucar.edu> and Patrick West <pwest@ucar.edu>
// and Jose Garcia <jgarcia@ucar.edu>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//	zednik      Stephan Zednik <zednik@ucar.edu>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include<string>

#include"DODSFilter.h"
#include"DAS.h"
#include"DDS.h"
#include"DataDDS.h"

#include"ObjectType.h"
#include"mime_util.h"
#include"ConstraintEvaluator.h"

#include"CSVDDS.h"
#include"CSVDAS.h"

using namespace std;

const string cgi_version = PACKAGE_VERSION;

int main(int argc, char* argv[]) {

  try {
    DODSFilter df(argc, argv);
    //...

    //is this only for server3?

  } catch(Error& e) {
    set_mime_text(cout, dods_error, cgi_version);
    e.print(cout);
    return EXIT_ERROR;
  }

  return EXIT_SUCCESS;
}
