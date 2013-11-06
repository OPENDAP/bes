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
 * show
 *     catalog
 *     info
 * @endverbatim
 */

#define OPENDAP_SERVICE "dap"
#define DAP2_FORMAT "dap2"

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

#if 0
#define DMR_RESPONSE "get.dmr"
#define DMR_SERVICE "dmr"
#define DMR_DESCRIPT "OPeNDAP Data DMR Structure"
#define DMR_RESPONSE_STR "getDMR"

#define DAP4DATA_RESPONSE "get.dap4data"
#define DAP4DATA_SERVICE "dapdata"
#define DAP4DATA_DESCRIPT "OPeNDAP Data DAP4DATA Structure"
#define DAP4DATA_RESPONSE_STR "getDAP4DATA"
#endif
/*
 * DataDDX data names
 */
#define DATADDX_STARTID "dataddx_startid"
#define DATADDX_BOUNDARY "dataddx_boundary"



#endif // E_BESDapNames_H

