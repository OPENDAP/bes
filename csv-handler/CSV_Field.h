#ifndef _csv_field_h
#define _csv_field_h 1

#include<string>

using namespace std;

class CSV_Field {
 public:

  CSV_Field() { }
  ~CSV_Field() { }

  void insertName(const string& fieldName) { name = fieldName; }
  void insertType(const string& fieldType) { type = fieldType; }
  void insertIndex(const int& fieldIndex) { index = fieldIndex; }

  string getName() { return name; }
  string getType() { return type; }
  int getIndex() { return index; }

 private:
  string name;
  string type;
  int index;
};

#endif _csv_field_h
