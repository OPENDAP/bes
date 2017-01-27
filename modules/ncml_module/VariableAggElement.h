//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2009 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////
#ifndef __NCML_MODULE__VARIABLE_AGG_ELEMENT_H__
#define __NCML_MODULE__VARIABLE_AGG_ELEMENT_H__

#include "NCMLElement.h"

namespace ncml_module {
class AggregationElement;

/**
 * @brief Element for the <variableAgg> element
 * child of an <aggregation>.
 */
class VariableAggElement: public NCMLElement {
private:
    VariableAggElement& operator=(const VariableAggElement& rhs); // disallow

public:
    static const string _sTypeName;
    static const vector<string> _sValidAttributes;

    VariableAggElement();
    VariableAggElement(const VariableAggElement& proto);
    virtual ~VariableAggElement();
    virtual const string& getTypeName() const;
    virtual VariableAggElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleEnd();
    virtual string toString() const;

    const string& name() const
    {
        return _name;
    }

    /** Get our parent aggregation off the parser stack */
    AggregationElement& getParentAggregation() const;

private:
    // methods

    static vector<string> getValidAttributes();

private:
    // data rep
    string _name;
};

}

#endif /* __NCML_MODULE__VARIABLE_AGG_ELEMENT_H__ */
