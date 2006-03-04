// DODSStatusReturn.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#ifndef DODSStatusReturn_h_
#define DODSStatusReturn_h_ 1

#define DODS_EXECUTED_OK 0
#define DODS_TERMINATE_IMMEDIATE 1
#define DODS_REQUEST_INCORRECT 2
#define DODS_MEMORY_EXCEPTION 3
#define OPENDAP_DATABASE_FAILURE 4
#define DODS_CONTAINER_PERSISTENCE_ERROR 5
#define DODS_INITIALIZATION_FILE_PROBLEM 6
#define DODS_LOG_FILE_PROBLEM 7
#define DODS_DATA_HANDLER_FAILURE 8
#define DODS_AGGREGATION_EXCEPTION 9
#define OPeNDAP_FAILED_TO_EXECUTE_COMMIT_COMMAND 10
#define DODS_DATA_HANDLER_PROBLEM 11

#endif // DODSStatusReturn_h_

// $Log: DODSStatusReturn.h,v $
// Revision 1.4  2005/03/15 19:57:45  pwest
// new return status
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
