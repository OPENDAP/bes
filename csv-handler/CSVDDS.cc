#include <vector>
#include <string>

#include "CSVDDS.h"
#include "BESHandlerException.h"
#include "BaseTypeFactory.h"
#include "DDS.h"
#include "Error.h"

#include "Str.h"
#include "Int16.h"
#include "Int32.h"
#include "Float32.h"
#include "Float64.h"

#include "Array.h"

#include "CSV_Obj.h"

void csv_read_descriptors(DDS &dds, const string &filename) {
  
  vector<string> fieldList;
  string type;
  int recordCount;
  int index;

  Array* ar;
  void* data;
  BaseType *bt = NULL;

  CSV_Obj* csvObj = new CSV_Obj();
  csvObj->open(filename);
  csvObj->load();

  dds.set_dataset_name(filename);

  fieldList = csvObj->getFieldList();
  recordCount = csvObj->getRecordCount();

  for(vector<string>::iterator it = fieldList.begin(); it != fieldList.end(); it++) {
    type = csvObj->getFieldType(*it);
    ar = dds.get_factory()->NewArray(string(*it));
    data = csvObj->getFieldData(*it);

    if(type.compare(string(STRING)) == 0) {
      string* strings = new string[recordCount];

      bt = dds.get_factory()->NewStr(string(*it));
      ar->add_var(bt);
      ar->append_dim(recordCount, "record");
      
      index = 0;
      for(vector<string>::iterator it = ((vector<string>*)data)->begin(); 
	  it != ((vector<string>*)data)->end(); it++) {
	strings[index] = *it;
	index++;
      }
      
      ar->set_value(strings, recordCount);
      
    } else if(type.compare(string(INT16)) == 0) {
      short* int16 = new short[recordCount];
      bt = dds.get_factory()->NewInt16(*it);
      ar->add_var(bt);
      ar->append_dim(recordCount, "record");

      index = 0;
      for(vector<short>::iterator it = ((vector<short>*)data)->begin();
	  it != ((vector<short>*)data)->end(); it++) {
	int16[index] = *it;
	index++;
      }

      ar->set_value(int16, recordCount);

    } else if(type.compare(string(INT32)) == 0) {
      int* int32 = new int[recordCount];
      bt = dds.get_factory()->NewInt32(*it);
      ar->add_var(bt);
      ar->append_dim(recordCount, "record");

      index = 0;
      for(vector<int>::iterator it = ((vector<int>*)data)->begin();
	  it != ((vector<int>*)data)->end(); it++) {
	int32[index] = *it;
	index++;
      }

      ar->set_value((dods_int32*)int32, recordCount); //blah!

    } else if(type.compare(string(FLOAT32)) == 0) {
      float* floats = new float[recordCount];
      bt = dds.get_factory()->NewFloat32(*it);
      ar->add_var(bt);
      ar->append_dim(recordCount, "record");

      index = 0;
      for(vector<float>::iterator it = ((vector<float>*)data)->begin(); 
	  it != ((vector<float>*)data)->end(); it++) {
	floats[index] = *it;
	index++;
      }

      ar->set_value(floats, recordCount);

    } else if(type.compare(string(FLOAT64)) == 0) {
      double* doubles = new double[recordCount];
      bt = dds.get_factory()->NewFloat64(*it);
      ar->add_var(bt);
      ar->append_dim(recordCount, "record");

      index = 0;
      for(vector<double>::iterator it = ((vector<double>*)data)->begin(); 
	  it != ((vector<double>*)data)->end(); it++) {
	doubles[index] = *it;
	index++;
      }

      ar->set_value(doubles, recordCount);
    } else {
	throw BESHandlerException( "Bad Things Man", __FILE__, __LINE__ ) ;
    }

    dds.add_var(ar);
    delete ar;
    delete bt;
  }

  delete csvObj;
}
