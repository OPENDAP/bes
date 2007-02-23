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
