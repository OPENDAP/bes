// CSV_Header.cc

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

#include<iostream>
#include<sstream>
#include"CSV_Header.h"

CSV_Header::CSV_Header() {
  hdr = new map<string, CSV_Field*>;
  index2field = new map<int,string>;
}

CSV_Header::~CSV_Header() {
  delete hdr;
  delete index2field;
}

const bool CSV_Header::populate(vector<string>* foo) {
  
  string::size_type lastPos;

  string fieldName;
  string fieldType;
  int fieldIndex = 0;

  for(vector<string>::iterator it = foo->begin(); it != foo->end(); it++) {
    slim(*it);
    lastPos = (*it).find_first_of("<",0);
    fieldName = (*it).substr(0,lastPos);
    fieldType = (*it).substr(lastPos + 1,(*it).length() - lastPos - 2);
    
    CSV_Field* field = new CSV_Field();
    field->insertName(fieldName);
    field->insertType(fieldType);
    field->insertIndex(fieldIndex);
    
    hdr->insert(make_pair(fieldName,field));
    index2field->insert(make_pair(fieldIndex,fieldName));
    
    fieldIndex++;
  }

  return true;
}

CSV_Field* CSV_Header::getField(const int& index) throw(string) {
  if(index2field->find(index) != index2field->end()) {
    string fieldName = index2field->find(index)->second;
    return hdr->find(fieldName)->second;
  } else {
    ostringstream osstream;
    osstream << "Could not find field at index " << index << endl;
    throw osstream.str();
  }
}

CSV_Field* CSV_Header::getField(const string& fieldName) throw(string) {
  if(hdr->find(fieldName) != hdr->end()) {
    return hdr->find(fieldName)->second;
  } else {
    ostringstream osstream;
    osstream <<  "Could not find field \"" << fieldName << "\"\n";
    throw osstream.str();
  }
}

const string CSV_Header::getFieldType(const string& fieldName) {
  map<string,CSV_Field*>::iterator it = hdr->find(fieldName);
  
  if(it != hdr->end())
    return (it->second)->getType();
  else
    return "";
}

void CSV_Header::print() {
  for(unsigned int index = 0; index < index2field->size(); index++) {
    string field = index2field->find(index)->second;
    CSV_Field* csvField = hdr->find(field)->second;
    cout << csvField->getIndex() << "\t" << csvField->getType() 
	 << "\t" << csvField->getName() << "\n";
  }
}

void slim(string& str) {
  if(*(--str.end()) == '\"' and *str.begin() == '\"')
    str = str.substr(1,str.length() - 2);
}

vector<string> CSV_Header::getFieldList() {
  vector<string> fieldList;
  for(unsigned int index = 0; index < index2field->size(); index++) {
    fieldList.push_back(index2field->find(index)->second);
  }
  return fieldList;
}
