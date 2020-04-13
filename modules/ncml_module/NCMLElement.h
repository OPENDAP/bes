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
#ifndef __NCML_MODULE__NCMLELEMENT_H__
#define __NCML_MODULE__NCMLELEMENT_H__

#include <iostream>
#include "RCObject.h"
#include "XMLHelpers.h"
#include <string>
#include <vector>

using agg_util::RCPtr;
using agg_util::RCObject;
using agg_util::RCObjectPool;

namespace ncml_module {
// FDecls
class NCMLParser;

/**
 *  @brief Base class for NcML element concrete classes
 *
 *  Base class to define an NcML element for polymorphic dispatch
 *  in the NCMLParser.  These concrete elements classes will be
 *  made friends of the NCMLParser in order to split the monolithic
 *  parser into chunks with specific functionality.
 *
 *  New concrete subclasses MUST be entered in the factory or they cannot
 *  be created.
 *
 *  We subclass RCObject since we sometimes need to keep elements
 *  around longer than just the SAX parser stack and need to keep track of
 *  whether other objects need to hang onto strong references to them.
 */
class NCMLElement: public agg_util::RCObject {
public:

    /**
     *  Factory class for the NcML elements.
     *  Assumption: Concrete subclasses MUST
     *  define the following static methods:
     *  static const string& ConcreteClassName::getTypeName();
     *  static ConcreteClassName* ConcreteClassName::makeInstance(const AttrMap& attrs);
     * */
    class Factory {
    public:
        Factory();
        ~Factory();

        /**
         * Create an element of the proper type with the given AttrMap
         * for its defined attributes.
         * @return the new element or NULL if eltTypeName had to prototype.
         * @param eltTypeName element type name
         * @param attrs the map of the attributes defined for the element
         * @param parser  the parser which is creating the element.
         */
        RCPtr<NCMLElement> makeElement(const std::string& eltTypeName, const XMLAttributeMap& attrs,
            NCMLParser& parser);

    private:
        // Interface

        /** Add the initial prototypes to this so we are ready to rumble */
        void initialize();

    private:
        // Possible prototypes we can create from.  Uses getTypeName() for match.
        typedef std::vector<const NCMLElement*> ProtoList;

        /** Add the prototype subclass as an element we can factory up!
         * Used by createTheFactory to create the initial factory.
         * If a prototype with type name == proto->getTypeName() already
         * exists in the factory, proto will replace it.
         */
        void addPrototype(const NCMLElement* proto);

        /** Return the iterator for the prototype for elementTypeName, or _protos.end() if not found. */
        ProtoList::iterator findPrototype(const std::string& elementTypeName);

        ProtoList _protos;
    };

protected:
    // Abstract: Only subclasses can create these
    explicit NCMLElement(NCMLParser* p);

    NCMLElement(const NCMLElement& proto);

private:
    // Disallow assignment for now
    NCMLElement& operator=(const NCMLElement& rhs);

public:
    virtual ~NCMLElement();

    void setParser(NCMLParser* p);

    /** Return the current parse line number.  Shorthand */
    int line() const;

    /** Return the type of the element, which should be:
     * the same as ConcreteClassName::getTypeName() */
    virtual const std::string& getTypeName() const = 0;

    /** Make and return a copy of this.
     * Used by the factory from a prototype.
     */
    virtual NCMLElement* clone() const = 0;

    /** Set the attributes of this from the map.
     * @param attrs the attribute map to set this class to.
     */
    virtual void setAttributes(const XMLAttributeMap& attrs) = 0;

    /** Check that the given attributes are all in the valid set, otherwise
     * fill in *pInvalidAttrs with the problematic ones if it's not null.
     * If pInvalidAttrs && printInvalid is set, we print the problematic attributes to BESDEBUG "ncml" channel
     * If throwOnError is set, we throw a parse error instead of returning.
     * @return whether all attributes are in the valid set if not throw
     */
    virtual bool validateAttributes(const XMLAttributeMap& attrs, const std::vector<std::string>& validAttrs,
    		std::vector<std::string>* pInvalidAttrs = 0, bool printInvalid = true, bool throwOnError = true);

    /** Handle a begin on this element.
     * Called after creation and it is assumed the
     * attributes and _parser are already set.
     * */
    virtual void handleBegin() = 0;

    /** Handle the characters content for the element.
     * Default impl throws if the content is not all whitespace.
     * Subclasses that handle non-whitespace content should override.
     * @param content the string of characters in the element content.
     */
    virtual void handleContent(const std::string& content);

    /** Handle the closing of this element. */
    virtual void handleEnd() = 0;

    /** Return a string describing the element */
    virtual std::string toString() const = 0;

    /** Helper for subclasses implementing toString().
     * @return a string with attrName="attrValue" if !attrValue.empty(),
     *         otherwise return the empty string.
     */
    static std::string printAttributeIfNotEmpty(const std::string& attrName, const std::string& attrValue);

    /** @return whether the given attr is in the array validAttrs or not...  Helper for subclasses */
    static bool isValidAttribute(const std::vector<std::string>& validAttrs, const std::string& attr);

    /** @return whether all the attributes in attrMap are in validAttrs.
     * If pInvalidAttributes, fill it in with all the illegal attributes.
     */
    static bool areAllAttributesValid(const XMLAttributeMap& attrMap, const std::vector<std::string>& validAttrs,
        std::vector<std::string>* pInvalidAttributes = 0);

protected:
    // data rep
    NCMLParser* _parser;
};

}

/** Output obj.toString() to the stream */
inline std::ostream &
operator<<(std::ostream &strm, const ncml_module::NCMLElement &obj)
{
    strm << obj.toString();
    return strm;
}

#endif /* __NCML_MODULE__NCMLELEMENT_H__ */
