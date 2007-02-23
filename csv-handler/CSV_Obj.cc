#include<iostream>
#include<sstream>
#include<iomanip>

#include"CSV_Obj.h"

CSV_Obj::CSV_Obj() {
  reader = new CSV_Reader();
  header = new CSV_Header();
  data = new vector<CSV_Data*>();
}

CSV_Obj::~CSV_Obj() {
  reader->close();
  delete reader;
  delete header;
  delete data;
}

bool CSV_Obj::open(const string& filepath) {
  return reader->open(filepath);
}

void CSV_Obj::printField(const string& field) {
  
  void* fieldData;
  string type;
  
  try {
    cout << "FIELD: " << field << endl;
    fieldData = getFieldData(field);
    type = header->getField(field)->getType();
    
    if(type.compare(STRING) == 0) {
      for(vector<string>::iterator it = ((vector<string>*)fieldData)->begin();
	  it != ((vector<string>*)fieldData)->end();
	  it++) {
	cout << *it << endl;
      }
    } else if(type.compare(FLOAT32) == 0) {
      for(vector<float>::iterator it = ((vector<float>*)fieldData)->begin();
	  it != ((vector<float>*)fieldData)->end();
	  it++) {
	cout << *it << endl;
      }
    } else if(type.compare(FLOAT64) == 0) {
      for(vector<double>::iterator it = ((vector<double>*)fieldData)->begin();
	  it != ((vector<double>*)fieldData)->end();
	  it++) {
	cout << *it << endl;
      }
    } else if(type.compare(INT16) == 0) {
      for(vector<short>::iterator it = ((vector<short>*)fieldData)->begin();
	  it != ((vector<short>*)fieldData)->end();
	  it++) {
	cout << *it << endl;
      }
    } else if(type.compare(INT32) == 0) {
      for(vector<int>::iterator it = ((vector<int>*)fieldData)->begin();
	  it != ((vector<int>*)fieldData)->end();
	  it++) {
	cout << *it << endl;
      }
    }
    cout << endl;
  } catch(string& error) { }
}

void CSV_Obj::load() {
  vector<string> txtLine;
  bool OnHeader = true;
  reader->reset();
  while(!reader->eof()) {
    txtLine = reader->get();
    if(OnHeader) {
      if(header->populate(&txtLine)) {
	for(unsigned int i = 0; i < txtLine.size(); i++) {
	  data->push_back(new CSV_Data());
	}
	OnHeader = false;
      }
    } else if(!txtLine.empty()) {
      int index = 0;
      for(vector<CSV_Data*>::iterator it = data->begin(); it != data->end(); it++) {
	try {
	  string token = txtLine.at(index);
	  slim(token);
	  (*it)->insert(header->getField(index), &token);
	} catch(...) { 
	  (*it)->insert(header->getField(index),new string("")); 
	}
	index++;
      }
      txtLine.clear();
    }
  }
}

vector<string> CSV_Obj::getFieldList() {
  return header->getFieldList();
}

string CSV_Obj::getFieldType(const string& fieldName) {
  return header->getFieldType(fieldName);
}

int CSV_Obj::getRecordCount() {
  CSV_Data* alphaField = data->at(0);
  string type = alphaField->getType();

  if(type.compare(string(STRING)) == 0) {
    return ((vector<string>*)alphaField->getData())->size();
  } else if(type.compare(string(FLOAT32)) == 0) {
    return ((vector<float>*)alphaField->getData())->size();
  } else if(type.compare(string(FLOAT64)) == 0) {
    return ((vector<double>*)alphaField->getData())->size();
  } else if(type.compare(string(INT16)) == 0) {
    return ((vector<short>*)alphaField->getData())->size();
  } else if(type.compare(string(INT32)) == 0) {
    return ((vector<int>*)alphaField->getData())->size();
  } else {
    return -1;
  }
}

void* CSV_Obj::getFieldData(const string& field) { 
  try {
    int index = header->getField(field)->getIndex();
    return data->at(index)->getData();
  } catch(string error) { 
    cerr << error << endl;
    return (void *)0; 
  }
}

vector<string> CSV_Obj::getRecord(const int rowNum) {
  vector<string> record;
  void* fieldData;
  string type;

  if(rowNum > getRecordCount())
    return record; //throw error?

  vector<string> fieldList = getFieldList();
  for(vector<string>::iterator it = fieldList.begin(); it != fieldList.end(); it++) {
    ostringstream oss;
    fieldData = getFieldData(*it);
    type = header->getField(*it)->getType();

    if(type.compare(string(STRING)) == 0) {
      record.push_back(((vector<string>*)fieldData)->at(rowNum));
    } else if(type.compare(string(FLOAT32)) == 0) {
      oss << ((vector<float>*)fieldData)->at(rowNum);
      record.push_back(oss.str());
    } else if(type.compare(string(FLOAT64)) == 0) {
      oss << ((vector<double>*)fieldData)->at(rowNum);
      record.push_back(oss.str());
    } else if(type.compare(string(INT16)) == 0) {
      oss << ((vector<short>*)fieldData)->at(rowNum);
      record.push_back(oss.str());
    } else if(type.compare(string(INT32)) == 0) {
      oss << ((vector<int>*)fieldData)->at(rowNum);
      record.push_back(oss.str());
    }
  }
  
  return record;
}
