
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
//         James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include <sstream>

#include <BaseType.h>
#include <Str.h>
#include <Error.h>
#include <debug.h>
#include <ServerFunctionsList.h>

#include "LinearScaleFunction.h"

using namespace libdap;

namespace functions {

/** This server-side function returns version information for the server-side
 functions. Note that this function takes no arguments and returns a
 String using the BaseType value/result parameter.

 @param btpp A pointer to the return value; caller must delete.
 @TODO Change implementation to use libxml2 objects and NOT strings.
*/
void
function_dap2_version(int, BaseType *[], DDS &dds, BaseType **btpp)
{
    string xml_value = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    ServerFunction *sf;
    string functionType;

    ServerFunctionsList *sfList = libdap::ServerFunctionsList::TheList();
    std::multimap<string,libdap::ServerFunction *>::iterator begin = sfList->begin();
    std::multimap<string,libdap::ServerFunction *>::iterator end = sfList->end();
    std::multimap<string,libdap::ServerFunction *>::iterator sfit;

    xml_value += "<ds:functions xmlns:ds=\"http://xml.opendap.org/ns/DAP/4.0/dataset-services#\">\n";
    for(sfit=begin; sfit!=end; sfit++){
    	sf = sfList->getFunction(sfit);
    	if(sf->canOperateOn(dds)){
			xml_value += "     <ds:function  name=\"" + sf->getName() +"\""+
						 " version=\"" + sf->getVersion() + "\""+
						 " type=\"" + sf->getTypeString() + "\""+
						 " role=\"" + sf->getRole() + "\""+
						 " >\n" ;
			xml_value += "        <ds:Description href=\"" + sf->getDocUrl() + "\">" + sf->getDescriptionString() + "</ds:Description>\n";
			xml_value += "    </ds:function>\n";
    	}
    }
	xml_value += "</functions>\n";

    Str *response = new Str("version");

    response->set_value(xml_value);
    *btpp = response;
    return;
}

/** This server-side function returns version information for the server-side
 functions. Note that this function takes no arguments and returns a
 String using the BaseType value/result parameter.

 @param btpp A pointer to the return value; caller must delete.
 @TODO Change implementation to use libxml2 objects and NOT strings.
*/

BaseType *function_dap4_version(D4RValueList *, DMR &dmr)
{
    string xml_value = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    ServerFunction *sf;
    string functionType;

    ServerFunctionsList *sfList = libdap::ServerFunctionsList::TheList();
    std::multimap<string,libdap::ServerFunction *>::iterator begin = sfList->begin();
    std::multimap<string,libdap::ServerFunction *>::iterator end = sfList->end();
    std::multimap<string,libdap::ServerFunction *>::iterator sfit;

    xml_value += "<ds:functions xmlns:ds=\"http://xml.opendap.org/ns/DAP/4.0/dataset-services#\">\n";
    for(sfit=begin; sfit!=end; sfit++){
    	sf = sfList->getFunction(sfit);
    	if(sf->canOperateOn(dmr)){
			xml_value += "     <ds:function  name=\"" + sf->getName() +"\""+
						 " version=\"" + sf->getVersion() + "\""+
						 " type=\"" + sf->getTypeString() + "\""+
						 " role=\"" + sf->getRole() + "\""+
						 " >\n" ;
			xml_value += "        <ds:Description href=\"" + sf->getDocUrl() + "\">" + sf->getDescriptionString() + "</ds:Description>\n";
			xml_value += "    </ds:function>\n";
    	}
    }
	xml_value += "</functions>\n";

    Str *response = new Str("version");

    response->set_value(xml_value);
    return response;
}

} // namespace functions
