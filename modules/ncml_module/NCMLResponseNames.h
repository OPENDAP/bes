//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009-2010 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////
#ifndef __NCML_MODULE__NCML_RESPONSE_NAMES_H__
#define __NCML_MODULE__NCML_RESPONSE_NAMES_H__

#include <string>

namespace ncml_module {

struct ModuleConstants {
    /** The name used to specify an ncml file. */
    static const std::string NCML_NAME;

    /** URL with the location of the documentation Wiki */
    static const std::string DOC_WIKI_URL;

    /** Response name in the DHI for the cache of aggregations command */
    static const std::string CACHE_AGG_RESPONSE;

    /** Name of the attribute in the cacheAgg XML command where the
     * filename to do the caching on is located.
     */
    static const std::string CACHE_AGG_LOCATION_XML_ATTR;

    /** Key in the dhi.data[] map where the location is stored. */
    static const std::string CACHE_AGG_LOCATION_DATA_KEY;
};
}

#endif // __NCML_MODULE__NCML_RESPONSE_NAMES_H_

