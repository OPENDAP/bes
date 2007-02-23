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
