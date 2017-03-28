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
#ifndef __NCML_MODULE__DIMENSION_ELEMENT_H__
#define __NCML_MODULE__DIMENSION_ELEMENT_H__

#include "Dimension.h"
#include "NCMLElement.h"

namespace ncml_module {

class NCMLParser;

/**
 * @brief Class for parsing and representing <dimension> elements.
 *
 * In the initial version of the module, we will not be supporting
 * many of the attributes.  In particular, we currently support:
 *
 * dimension@name:    the name of the dimension, which must be unique in
 *                    the current containing netcdf element (we don't
 *                    handle group namespaces yet).
 *
 * dimension@length:  the size of this dimension as an unsigned int.
 *
 * The other attributes, namely one of { orgName, isUnlimited,
 * isVariableLength, isShared } will not be supported in this version
 * of the module.
 */
class DimensionElement: public NCMLElement {
private:
    DimensionElement& operator=(const DimensionElement& rhs); // disallow

public:
    static const string _sTypeName;
    static const vector<string> _sValidAttributes;

    DimensionElement();
    DimensionElement(const DimensionElement& proto);
    DimensionElement(const agg_util::Dimension& dim);
    virtual ~DimensionElement();
    virtual const string& getTypeName() const;
    virtual DimensionElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const string& content);
    virtual void handleEnd();
    virtual string toString() const;

    /**
     * @return whether name() and getSize() are the same.
     * This function doesn't care about the other attributes (yet?)
     */
    bool checkDimensionsMatch(const DimensionElement& rhs) const;

    const string& name() const;
    const string& length() const
    {
        return _length;
    }

    /** Parsed version of length() */
    unsigned int getLengthNumeric() const;
    unsigned int getSize() const;

    const agg_util::Dimension& getDimension() const
    {
        return _dim;
    }

private:

    /** Fill in _dim from our string _length and other attrs.
     * @exception Throws BESSyntaxUserError if the token in _length
     *            cannot be successfully parsed as an unsigned int.
     */
    void parseAndCacheDimension();

    /** Make sure they didn't set any attributes we can't handle.
     * @exception Throw BESSyntaxUserError if there's an attribute we can't handle.
     * */
    void validateOrThrow();

    /** @return the list of valid attributes as a new vector
     * Used to set the static.
     */
    static vector<string> getValidAttributes();

private:
    // string _name; // within _dim
    string _length; // unparsed size
    string _orgName; // unused
    string _isUnlimited; // unused
    string _isShared; // unused
    string _isVariableLength; // unused

    // the actual parsed values from above...
    agg_util::Dimension _dim;
};

}

#endif /* __NCML_MODULE__DIMENSION_ELEMENT_H__ */
