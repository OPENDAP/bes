// -*- mode: c++; c-basic-offset:4 -*-
//
// FoW10nJsonTransform.cc
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef W10NJSONTRANSFORM_H_
#define W10NJSONTRANSFORM_H_

#include <string>
#include <vector>
#include <map>


#include <libdap/BaseType.h>
#include <libdap/DDS.h>
#include <libdap/Array.h>


#include <BESObj.h>
#include <BESDataHandlerInterface.h>

/**
 * Used to transform a DDS into a w10n JSON metadata or w10n JSON data document.
 * The output is written to a local file whose name is passed as a parameter
 * to the constructor.
 */
class W10nJsonTransform: public BESObj {
private:
	libdap::DDS *_dds;
	std::string _localfile;
	std::string _returnAs;
	std::string _indent_increment;

	std::ostream *_ostrm;
	bool          _usingTempFile;

	//void writeNodeMetadata(std::ostream *strm, libdap::BaseType *bt, std::string indent);
	void writeVariableMetadata(std::ostream *strm, libdap::BaseType *bt, std::string indent);
	void writeDatasetMetadata(std::ostream *strm, libdap::DDS *dds, std::string indent);

	// void transformAtomic(std::ostream *strm, libdap::BaseType *bt, std::string indent);


	//void transform(std::ostream *strm, libdap::DDS *dds, std::string indent, bool sendData);
	//void transform(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);

    //void transform(std::ostream *strm, Structure *s,string indent );
    //void transform(std::ostream *strm, Grid *g, string indent);
    //void transform(std::ostream *strm, Sequence *s, string indent);
	//void transform(std::ostream *strm, libdap::Constructor *cnstrctr, std::string indent, bool sendData);
	//void transform_node_worker(std::ostream *strm, std::vector<libdap::BaseType *> leaves, std::vector<libdap::BaseType *> nodes, std::string indent, bool sendData);


    //void transform(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);
    void writeAttributes(std::ostream *strm, libdap::AttrTable &attr_table, std::string  indent);

    template<typename T>
    void json_simple_type_array(std::ostream *strm, libdap::Array *a, std::string indent);
    void json_string_array(std::ostream *strm, libdap::Array *a, std::string indent);

    void json_array_starter(ostream *strm, libdap::Array *a, string indent);
    template<typename T> void json_simple_type_array_sender(ostream *strm, libdap::Array *a);
    void json_string_array_sender(ostream *strm, libdap::Array *a);
    void json_array_ender(ostream *strm, string indent);

    template<typename T>
    unsigned  int json_simple_type_array_worker(
    		std::ostream *strm,
    		T *values,
    		unsigned int indx,
    		std::vector<unsigned int> *shape,
    		unsigned int currentDim,
			bool flatten
    		);

    void sendW10nMetaForDDS(ostream *strm, libdap::DDS *dds, string indent);
    void sendW10nMetaForVariable(ostream *strm, libdap::BaseType *bt, string indent, bool traverse);
    std::ostream *getOutputStream();
    void releaseOutputStream();
    void sendW10nDataForVariable(ostream *strm, libdap::BaseType *bt, string indent);
    void sendW10nData(ostream *strm, libdap::BaseType *b, string indent);
    void sendW10nData(ostream *strm, libdap::Array *b, string indent);

public:

    W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, const std::string &localfile);
    W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, std::ostream *ostrm);
	virtual ~W10nJsonTransform();

	//virtual void transform(bool sendData);
	//virtual void sendMetadata();
	//virtual void sendData();
	virtual void sendW10nMetaForDDS();
	virtual void sendW10nMetaForVariable(string &vName, bool isTop);
	virtual void sendW10nDataForVariable(string &vName);

	void dump(std::ostream &strm) const override;

};

#endif /* W10NJSONTRANSFORM_H_ */
