// main.cc

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
#include"CSV_Obj.h"

using namespace std;

#define EXIT_ERROR 1

void printHeader(CSV_Obj* foo);
void printAllData(CSV_Obj* foo);

int main(int argc, char* argv[]) {
  
  CSV_Obj* foo = new CSV_Obj();
  char* filepath;
  
  if(argc > 1) { 
    filepath = argv[1]; 
  } else { 
    filepath = "/Users/zednik/data/temperature.csv"; 
  }
  
  if (!foo->open(filepath)) {  
    cout << "Could not open .CSV file." << endl; 
    return EXIT_ERROR;
  }

  foo->load();
  printHeader(foo);
  printAllData(foo);

  delete foo;
  return EXIT_SUCCESS;
}

void printAllData(CSV_Obj* foo) {
  int count = foo->getRecordCount();
  for(int index = 0; index < count; index++) {
    string recStr;
    vector<string> rec = foo->getRecord(index);
    for(vector<string>::iterator it = rec.begin(); it != rec.end(); it++) {
      recStr += *it + ", ";
    }
    recStr = recStr.substr(0, recStr.length() - 2);
    cout << recStr << endl;
  }
}

void printHeader(CSV_Obj* foo) {
  string FL = "Field List: ";
  vector<string> fList = foo->getFieldList();
  for(vector<string>::iterator it = fList.begin(); it != fList.end(); it++) {
    FL += *it + ", ";
  }
  FL = FL.substr(0, FL.length() - 2);
  cout << FL << endl;
}
