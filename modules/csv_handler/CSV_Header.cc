// CSV_Header.cc

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

#include "CSV_Header.h"
#include "CSV_Utils.h"

#include <BESInternalError.h>

CSV_Header::CSV_Header()
{
	_hdr = new map<string, CSV_Field*>;
	_index2field = new map<int, string>;
}

CSV_Header::~CSV_Header()
{
	if (_hdr) {
		map<string, CSV_Field*>::iterator i = _hdr->begin();
		map<string, CSV_Field*>::iterator e = _hdr->end();
		for (; i != e; i++) {
			CSV_Field *f = (*i).second;
			delete f;
			(*i).second = 0;
		}
		delete _hdr;
		_hdr = 0;
	}
	if (_index2field) {
		delete _index2field;
		_index2field = 0;
	}
}

bool CSV_Header::populate(vector<string> *headerinfo) const
{
	string::size_type lastPos;

	string fieldName;
	string fieldType;
	int fieldIndex = 0;

	vector<string>::iterator it = headerinfo->begin();
	vector<string>::iterator et = headerinfo->end();
	for (; it != et; it++) {
		string headerinfo_s = (*it);
		CSV_Utils::slim(headerinfo_s);
		string::size_type headerinfo_l = headerinfo_s.length();

		// lastPos = headerinfo_s.find_first_of( "<" ) ; not used. jg 3/25/11
		lastPos = headerinfo_s.find_first_of("<", 0);
		if (lastPos == string::npos) {
			ostringstream err;
			err << "malformed header information in column " << fieldIndex << ", missing type in " << headerinfo_s;
			throw BESInternalError(err.str(), __FILE__, __LINE__);
		}
		if (*(--headerinfo_s.end()) != '>') {
			ostringstream err;
			err << "malformed header information in column " << fieldIndex << ", missing type in " << headerinfo_s;
			throw BESInternalError(err.str(), __FILE__, __LINE__);
		}
		fieldName = headerinfo_s.substr(0, lastPos);
		fieldType = headerinfo_s.substr(lastPos + 1, headerinfo_l - lastPos - 2);

		CSV_Field* field = new CSV_Field();
		field->insertName(fieldName);
		field->insertType(fieldType);
		field->insertIndex(fieldIndex);

		_hdr->insert(make_pair(fieldName, field));
		_index2field->insert(make_pair(fieldIndex, fieldName));

		fieldIndex++;
	}

	return true;
}

CSV_Field *
CSV_Header::getField(const int& index)
{
	CSV_Field *f = 0;
	if (_index2field->find(index) != _index2field->end()) {
		string fieldName = _index2field->find(index)->second;
		f = _hdr->find(fieldName)->second;
	}
	else {
		ostringstream err;
		err << "Could not find field in column " << index;
		throw BESInternalError(err.str(), __FILE__, __LINE__);
	}
	return f;
}

CSV_Field *
CSV_Header::getField(const string& fieldName)
{
	CSV_Field *f = 0;
	if (_hdr->find(fieldName) != _hdr->end()) {
		f = _hdr->find(fieldName)->second;
	}
	else {
		ostringstream err;
		err << "Could not find field \"" << fieldName;
		throw BESInternalError(err.str(), __FILE__, __LINE__);
	}
	return f;
}

const string CSV_Header::getFieldType(const string& fieldName)
{
	string type;
	map<string, CSV_Field*>::iterator it = _hdr->find(fieldName);

	if (it != _hdr->end()) {
		type = (it->second)->getType();
	}
	return type;
}

void CSV_Header::getFieldList(vector<string> &list)
{
	for (unsigned int index = 0; index < _index2field->size(); index++) {
		list.push_back(_index2field->find(index)->second);
	}
}

void CSV_Header::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "CSV_Header::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	map<int, string>::const_iterator ii = _index2field->begin();
	map<int, string>::const_iterator ie = _index2field->end();
	for (; ii != ie; ii++) {
		strm << BESIndent::LMarg << (*ii).first << ": " << (*ii).second << endl;
	}
	map<string, CSV_Field*>::const_iterator fi = _hdr->begin();
	map<string, CSV_Field*>::const_iterator fe = _hdr->end();
	for (; fi != fe; fi++) {
		strm << BESIndent::LMarg << (*fi).first << ": " << endl;
		BESIndent::Indent();
		(*fi).second->dump(strm);
		BESIndent::UnIndent();
	}
	BESIndent::UnIndent();
}

