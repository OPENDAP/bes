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
