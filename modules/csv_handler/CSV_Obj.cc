// CSV_Obj.cc

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include <iostream>
#include <sstream>
#include <iomanip>

#include "CSV_Obj.h"
#include "CSV_Utils.h"

#include <BESInternalError.h>
#include <BESNotFoundError.h>

CSV_Obj::CSV_Obj()
{
	_reader = new CSV_Reader();
	_header = new CSV_Header();
	_data = new vector<CSV_Data*>();
}

CSV_Obj::~CSV_Obj()
{
	if (_reader) {
		_reader->close();
		delete _reader;
		_reader = 0;
	}
	if (_header) {
		delete _header;
		_header = 0;
	}
	if (_data) {
		CSV_Data *d = 0;
		vector<CSV_Data*>::iterator i = _data->begin();
		vector<CSV_Data*>::iterator e = _data->end();
		while (i != e) {
			d = (*i);
			delete d;
			_data->erase(i);
			i = _data->begin();
			e = _data->end();
		}
		delete _data;
		_data = 0;
	}
}

bool CSV_Obj::open(const string& filepath)
{
	return _reader->open(filepath);
}

void CSV_Obj::load()
{
	vector<string> txtLine;
	bool OnHeader = true;
	_reader->reset();
	while (!_reader->eof()) {
		_reader->get(txtLine);

		if (OnHeader) {
			if (_header->populate(&txtLine)) {
				for (unsigned int i = 0; i < txtLine.size(); i++) {
					_data->push_back(new CSV_Data());
				}
			}
			OnHeader = false;
		}
		else if (!txtLine.empty()) {
			int index = 0;
			vector<CSV_Data *>::iterator it = _data->begin();
			vector<CSV_Data *>::iterator et = _data->end();
			for (; it != et; it++) {
				CSV_Data *d = (*it);
				string token = txtLine.at(index);
				CSV_Utils::slim(token);
				CSV_Field *f = _header->getField(index);
				if (!f) {
					ostringstream err;
					err << " Attempting to add value " << token << " to field " << index << ", field does not exist";
					throw BESInternalError(err.str(), __FILE__, __LINE__);
				}
				d->insert(f, &token);
				index++;
			}
		}
		txtLine.clear();
	}
}

void CSV_Obj::getFieldList(vector<string> &list)
{
	_header->getFieldList(list);
}

string CSV_Obj::getFieldType(const string& fieldName)
{
	return _header->getFieldType(fieldName);
}

int CSV_Obj::getRecordCount()
{
	CSV_Data* alphaField = _data->at(0);
	string type = alphaField->getType();

	if (type.compare(string(STRING)) == 0) {
		return ((vector<string>*) alphaField->getData())->size();
	}
	else if (type.compare(string(FLOAT32)) == 0) {
		return ((vector<float>*) alphaField->getData())->size();
	}
	else if (type.compare(string(FLOAT64)) == 0) {
		return ((vector<double>*) alphaField->getData())->size();
	}
	else if (type.compare(string(INT16)) == 0) {
		return ((vector<short>*) alphaField->getData())->size();
	}
	else if (type.compare(string(INT32)) == 0) {
		return ((vector<int>*) alphaField->getData())->size();
	}
	else {
		return -1;
	}
}

void *
CSV_Obj::getFieldData(const string& field)
{
	void *ret = 0;
	CSV_Field *f = _header->getField(field);
	if (f) {
		int index = f->getIndex();
		CSV_Data *d = _data->at(index);
		if (d) {
			ret = d->getData();
		}
		else {
			string err = (string) "Unable to get data for field " + field;
			throw BESInternalError(err, __FILE__, __LINE__);
		}
	}
	else {
		string err = (string) "Unable to get data for field " + field + ", no such field exists";
		throw BESInternalError(err, __FILE__, __LINE__);
	}
	return ret;
}

vector<string> CSV_Obj::getRecord(const int rowNum)
{
	vector<string> record;
	void* fieldData;
	string type;

	int maxRows = getRecordCount();
	if (rowNum > maxRows) {
		ostringstream err;
		err << "Attempting to retrieve row " << rowNum << " of " << maxRows;
		throw BESInternalError(err.str(), __FILE__, __LINE__);
	}

	vector<string> fieldList;
	getFieldList(fieldList);
	vector<string>::iterator it = fieldList.begin();
	vector<string>::iterator et = fieldList.end();
	for (; it != et; it++) {
		string fieldName = (*it);
		ostringstream oss;
		fieldData = getFieldData(fieldName);
		CSV_Field *f = _header->getField(fieldName);
		if (!f) {
			ostringstream err;
			err << "Unable to retrieve data for field " << fieldName << " on row " << rowNum;
			throw BESInternalError(err.str(), __FILE__, __LINE__);
		}
		type = f->getType();

		if (type.compare(string(STRING)) == 0) {
			record.push_back(((vector<string>*) fieldData)->at(rowNum));
		}
		else if (type.compare(string(FLOAT32)) == 0) {
			oss << ((vector<float>*) fieldData)->at(rowNum);
			record.push_back(oss.str());
		}
		else if (type.compare(string(FLOAT64)) == 0) {
			oss << ((vector<double>*) fieldData)->at(rowNum);
			record.push_back(oss.str());
		}
		else if (type.compare(string(INT16)) == 0) {
			oss << ((vector<short>*) fieldData)->at(rowNum);
			record.push_back(oss.str());
		}
		else if (type.compare(string(INT32)) == 0) {
			oss << ((vector<int>*) fieldData)->at(rowNum);
			record.push_back(oss.str());
		}
	}

	return record;
}

void CSV_Obj::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "CSV_Obj::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	if (_reader) {
		strm << BESIndent::LMarg << "reader:" << endl;
		BESIndent::Indent();
		_reader->dump(strm);
		BESIndent::UnIndent();
	}
	if (_header) {
		strm << BESIndent::LMarg << "header:" << endl;
		BESIndent::Indent();
		_header->dump(strm);
		BESIndent::UnIndent();
	}
	if (_data) {
		strm << BESIndent::LMarg << "data:" << endl;
	}
	BESIndent::UnIndent();
}

