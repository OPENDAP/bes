// CSVDAS.cc

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include <vector>
#include <memory>

#include "CSVDAS.h"

#include "DAS.h"
#include "Error.h"

#include "CSV_Obj.h"

#include <BESNotFoundError.h>
#include <BESDebug.h>

void csv_read_attributes(DAS &das, const string &filename)
{
#if 0
    AttrTable *attr_table_ptr = NULL;
    string type;
#endif

#if 0
    CSV_Obj* csvObj = new CSV_Obj();
#endif

    auto_ptr<CSV_Obj> csvObj(new CSV_Obj);

    if (!csvObj->open(filename)) {
#if 0
        string err = (string) "Unable to open file " + filename;
        delete csvObj;
#endif

        throw BESNotFoundError(string("Unable to open file ").append(filename), __FILE__, __LINE__);
    }

    csvObj->load();

    BESDEBUG( "csv", "File Loaded:" << endl << *csvObj << endl );

    vector<string> fieldList;
    csvObj->getFieldList(fieldList);

    //loop through all the fields
    vector<string>::iterator it = fieldList.begin();
    vector<string>::iterator et = fieldList.end();
    for (; it != et; it++) {
        AttrTable *attr_table_ptr = das.get_table((string(*it)).c_str());

        if (!attr_table_ptr) attr_table_ptr = das.add_table(string(*it), new AttrTable);

        //only one attribute, field type, called "type"
        string type = csvObj->getFieldType(*it);
        attr_table_ptr->append_attr("type", "String", type);
    }

#if 0
    delete csvObj;
#endif

}

