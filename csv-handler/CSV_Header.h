#ifndef _csv_header_h
#define _csv_header_h 1

#include<string>
#include<map>
#include<vector>

#include"CSV_Field.h"

using namespace std;

class CSV_Header {
 public:
  CSV_Header();
  ~CSV_Header();
  
  const bool populate(vector<string>* foo);

  void print();

  vector<string> getFieldList();

  const string getFieldType(const string& fieldName);

  CSV_Field* getField(const int& index) throw(string);
  CSV_Field* getField(const string& fieldName) throw(string);
  
 private:
  map<string,CSV_Field*>* hdr;
  map<int,string>* index2field;
};

void slim(string& str);

#endif _csv_header_h
