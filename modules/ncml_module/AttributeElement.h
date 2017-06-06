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
#ifndef __NCML_MODULE__ATTRIBUTE_ELEMENT_H__
#define __NCML_MODULE__ATTRIBUTE_ELEMENT_H__

#include "NCMLElement.h"

namespace libdap {
class AttrTable;
}

namespace ncml_module {
class OtherXMLParser;
}

using namespace std;
namespace ncml_module {

/**
 * @brief Concrete class for NcML <attribute> element
 *
 * This element handles the addition, modification, and renaming of
 * attributes in the currently loaded DDX.
 *
 * If _type="Structure", then it refers to an attribute container.
 */
class AttributeElement: public NCMLElement {
private:
    AttributeElement& operator=(const AttributeElement& rhs); // disallow

public:
    // methods
    AttributeElement();
    AttributeElement(const AttributeElement& proto);
    virtual ~AttributeElement();
    virtual const string& getTypeName() const;
    virtual AttributeElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const string& content);
    virtual void handleEnd();
    virtual string toString() const;

public:
    // data rep
    static const string _sTypeName;
    static const vector<string> _sValidAttributes;
    static const string _default_global_container;

private:
    // method

    /**
     * @brief Top level dispatch call on start of <attribute> element.
     *
     * This call assumes the named attribute is to be added or modified at the current
     * parse scope.  There are two cases we handle:
     *
     * 1) Attribute Containers:   if _type=="Structure", the attribute element refers to an attribute container which
     *                            will have no value itself but contain other attributes or attribute containers.
     * 2) Atomic (leaf) Attributes:  these attributes can be of any of the other NcML types (not Structure) or DAP atomic types.
     *
     * If the attribute specified by \c _name (or alternatively, \c _orgName if non empty) is not found in the current
     * scope, it is created and added with the given type and value.  This applies to containers as well, which must have no value.
     * For the case of atomic attributes (scalar or vector), attribute@value may be empty and the value specified in the characters
     * content of the element:
     * i.e. <attribute name="foo" type="String" value="bar"/> == <attribute name="foo" type="String">bar</attribute>
     *
     * If the named attribute is found, we modify it to have the new type and value (essentially removing the old one and reading it).
     *
     * For vector-valued atomics, we assume whitespace is the separator for the values.  If attribute@separator is non-empty,
     * we use it to split the values.
     *
     * If \c _orgName is not empty, the attribute at this scope named \c _orgName is to be renamed to \c _name.
     *
     * Attributes are used as follows:
     *
     * _name the name of the attribute to add or modify.  Also, the new name if orgName != "".
     * _type can be any of the NcML types (in which case they're mapped to a DAP type), or a DAP type.
     *             If type == "Structure", the attribute is considered an attribute container and value must be "".
     * _value the untokenized value for the attribute as specified in the attribute@value.  Note this can be empty
     *              and the value specified on the characters content of the element for the case of atomic types.
     *              If type == "Structure", value.empty() must be true.  Note this can contain multiple tokens
     *              for the case of vectors of atomic types.  Default separator is whitespace, although attribute@separator
     *              can specify a new one.  This is handled at the top level dispatcher.
     * _separator  if non-empty, use the given characters as separators for vector data rather than whitespace.
     * _orgName If not empty(), this specifies that the attribute named orgName (which should exist) is to be renamed to
     *                 name.
     *
     * We use _internalType for most manipulations of values.
     *
     * @exception  An parse exception will be thrown if this call is made while already parsing a leaf attribute.
     */
    void processAttribute(NCMLParser& p);

    /**
     * Given an attribute with type==Structure at current scope p.getCurrentAttrTable(), either add a new container to if
     * one does not exist with /c _name, otherwise assume we're just specifying a new scope for a later child attribute.
     * In either case, push the scope of the container into the parser's scope for future calls.
     */
    void processAttributeContainerAtCurrentScope(NCMLParser& p);

    /**
     * @brief Handle addition of, renaming of, and changing values of atomic attributes in current scope,
     * depending on the values of the attributes.
     */
    void processAtomicAttributeAtCurrentScope(NCMLParser& p);

    /** Convert the NcML type _type into an internal (DAP or special) type */
    string getInternalType() const;

    /** Add this element as a new attribute into the current scope of the parser.
     * @param p parser to add this as a new attribute at p.getCurrentAttrTable().
     */
    void addNewAttribute(NCMLParser& p);

    /**
     * Change the existing attribute's value.
     * It is an error to call this if !p.attributeExistsAtCurrentScope().
     * If type.empty(), then just leave the type as it is now, otherwise use type.
     * @param p the parser to effect
     * @param name local scope name of the attribute
     * @param type type of the attribute, or "" to leave it the same.
     * @param value new value for the attribute, could be unsplit list of tokens.
     *              If so, they will be split using _separator and added as an array.
     */
    void mutateAttributeAtCurrentScope(NCMLParser& p, const string& name, const string& type, const string& value);

    /**
     * @brief Rename the existing atomic attribute named \c _orgName to \c _name
     * If !_value.empty() replace the old values, otherwise leave the current value intact.
     * _name the new name for the attribute.  _name must not already exist at current scope or an exception will be thrown.
     * _type the type for the attribute or empty() if leave the same.
     * _value new value(s) for the renamed entry to replace the old.  Old values are kept if _value.empty().
     * _orgName  the original name of the attribute to rename.  Must be atomic and exist at current scope.
     *
     * @exception Thrown if attribute _orgName doesn't exist in scope
     * @exception Thrown if attribute _name already exists in scope
     * @exception Thrown if attribute _orgName is a container and not atomic.
     */
    void renameAtomicAttribute(NCMLParser& p);

    /**
     * @brief Rename the existing attribute container at current scope with name _orgName to _name.
     * _name the new name for the container, must not exist in this scope.
     * _orgName the original name of the container at current scope, must exist and be a container.
     * @exception thrown if _orgName attribute container does not exist or is not an attribute container.
     * @return the attribute container whose name we changed.
     */
    libdap::AttrTable* renameAttributeContainer(NCMLParser& p);

    /** Pop this attribute's scope off the stack and if it's a container,
     * set the parser's current AttrTable to the containers parent container.
     */
    void processEndAttribute(NCMLParser& p);

    /**
     * If parsing type=="OtherXML", we create an OtherXMLParser proxy parser and
     * tell the NCMLParser to use it until this element is closed.
     */
    void startOtherXMLParse(NCMLParser& p);

    /** @return the list of the valid attributes for this element as a new vector.
     * Used to set _sValidAttributes */
    static vector<string> getValidAttributes();

private:
    string _name;
    string _type;
    string _value;
    string _separator;
    string _orgName;

    // Temp for tokenizing
    vector<string> _tokens;

    // If we're parsing an attr of type OtherXML,
    // We will create and pass this into the NCMLParser to collect
    // the OtherXML string.  We own the memory.
    OtherXMLParser* _pOtherXMLParser;
};

}

#endif /* __NCML_MODULE__ATTRIBUTE_ELEMENT_H__ */
