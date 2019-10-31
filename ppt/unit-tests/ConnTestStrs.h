// ConnTestStrs.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef E_ConnTestStrs_h
#define E_ConnTestStrs_h 1

#include <string>

#include "PPTProtocol.h"

//class ConnTestStrs
//{
//public:
static string test_str[] = { (string)"This is a test",
			     (string)"define d as c with c.constraint=\"a[0,1,10]\";"
			   } ;

static string test_exp[] = { (string)"000000e"+"d"+"This is a test",
			     (string)"0000000"+"d",
			     (string)"000000fxvar1=val1;var2;",
			     (string)"000002cddefine d as c with c.constraint=\"a[0,1,10]\";",
			     (string)"0000000d"
			   } ;
//} ;

#endif // E_ConnTestStrs_h
