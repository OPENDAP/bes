#ifndef _csv_obj_h
#define _csv_obj_h 1

#include<string>
#include<vector>
#include"CSV_Reader.h"
#include"CSV_Header.h"
#include"CSV_Data.h"

using namespace std;

class CSV_Obj {

 public:
  CSV_Obj();
  ~CSV_Obj();
  
  bool open(const string& filepath);

  void load();

  void printField(const string& field);

  vector<string> getFieldList();
  
  string getFieldType(const string& fieldName);
  
  int getRecordCount();

  void* getFieldData(const string& field);

  vector<string> getRecord(const int rowCount);

 private:
  CSV_Reader* reader;
  CSV_Header* header;

  vector<CSV_Data*>* data;
};

#endif _csv_obj_h
