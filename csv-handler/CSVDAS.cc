// CSVDAS.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#include <vector>

#include "CSVDAS.h"

#include "DAS.h"
#include "Error.h"

#include "CSV_Obj.h"

void csv_read_attributes(DAS &das, const string &filename) throw(Error) {

  vector<string> fieldList;
  AttrTable *attr_table_ptr = NULL;
  string type;

  CSV_Obj* csvObj = new CSV_Obj();
  csvObj->open(filename);  //on fail, throw error
  csvObj->load(); //on fail, throw error
  
  fieldList = csvObj->getFieldList();
  
  //loop through all the fields
  for(vector<string>::iterator it = fieldList.begin(); it != fieldList.end(); it++) {

    attr_table_ptr = das.get_table((string(*it)).c_str());

    if(!attr_table_ptr)
      attr_table_ptr = das.add_table((string(*it)).c_str(), new AttrTable);
    
    //only one attribute, field type, called "type"
    type = csvObj->getFieldType(*it);
    attr_table_ptr->append_attr("type",type,type);
  }

  delete csvObj;
}
