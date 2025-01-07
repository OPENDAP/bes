// BESDapNames.h

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

#ifndef D_BESDapNames_H
#define D_BESDapNames_H 1

/** @brief macros representing the default response objects handled
 *
 * These include
 * @verbatim
 * get
 *     das
 *     dds
 *     ddx
 *     dods
 *     dap4data
 * show
 *     catalog
 *     info
 * @endverbatim
 */

#define OPENDAP_SERVICE "dap"
#define DAP_FORMAT "dap2"

// Use this to indicate that the BESDapModule should be used; it will then use the DAP2 or DAP4 format as appropriate.
// jhrg 1/6/25
#define DAP_FORMAT "dap"

#define DAS_RESPONSE "get.das"
#define DAS_SERVICE "das"
#define DAS_DESCRIPT "OPeNDAP Data Attribute Structure"
#define DAS_RESPONSE_STR "getDAS"

#define DDS_RESPONSE "get.dds"
#define DDS_SERVICE "dds"
#define DDS_DESCRIPT "OPeNDAP Data Description Structure"
#define DDS_RESPONSE_STR "getDDS"

#define DDX_RESPONSE "get.ddx"
#define DDX_SERVICE "ddx"
#define DDX_DESCRIPT "OPeNDAP Data Description and Attribute XML Document"
#define DDX_RESPONSE_STR "getDDX"

#define DATA_RESPONSE "get.dods"
#define DATA_SERVICE "dods"
#define DATA_DESCRIPT "OPeNDAP Data Object"
#define DATA_RESPONSE_STR "getDODS"

#define DATADDX_RESPONSE "get.dataddx"
#define DATADDX_SERVICE "dataddx"
#define DATADDX_DESCRIPT "OPeNDAP Data Description and Attributes in DDX format and Data Object"
#define DATADDX_RESPONSE_STR "getDataDDX"

#define DMR_RESPONSE "get.dmr"
#define DMR_SERVICE "dmr"
#define DMR_DESCRIPT "OPeNDAP Data DMR Structure"
#define DMR_RESPONSE_STR "getDMR"

#define DAP4DATA_RESPONSE "get.dap"
#define DAP4DATA_SERVICE "dap"
#define DAP4DATA_DESCRIPT "OPeNDAP DAP4 Data Structure"
#define DAP4DATA_RESPONSE_STR "getDAP"

// DataDDX data names
#define DATADDX_STARTID "dataddx_startid"
#define DATADDX_BOUNDARY "dataddx_boundary"

// Container attribute used to signal the DMR++ handler to look in the
// MDS for a DMR++ response. jhrg 5/31/18
#define MDS_HAS_DMRPP "MDS_HAS_DMRPP"
#define USE_DMRPP_KEY "DAP.Use.Dmrpp"
#define DMRPP_NAME_KEY "DAP.Dmrpp.Name"
#define DMRPP_DEFAULT_NAME "dmrpp"

#define DODS_EXTRA_ATTR_TABLE "DODS_EXTRA"
#define DODS_EXTRA_ANNOTATION_ATTR "AnnotationService"

#endif // E_BESDapNames_H

