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
#ifndef __NCML_MODULE__REMOVE_ELEMENT_H__
#define __NCML_MODULE__REMOVE_ELEMENT_H__

#include "NCMLElement.h"
#include <Array.h>

namespace ncml_module {
// FDecls
class NCMLParser;

/** Concrete subclass for the <remove> NcML element */
class RemoveElement: public NCMLElement {
private:
    RemoveElement& operator=(const RemoveElement&); // disallow

public:
    RemoveElement();
    RemoveElement(const RemoveElement& proto);
    virtual ~RemoveElement();

    virtual const std::string& getTypeName() const;
    virtual RemoveElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const std::string& content);
    virtual void handleEnd();
    virtual std::string toString() const;

    // Type of the element
    static const std::string _sTypeName;
    static const vector<string> _sValidAttributes;

private:

    /** @brief Remove the object at current parse scope with \c name.
     *
     * We can only remove attributes now (this includes containers, recursively).
     *
     *  @throw BESSyntaxUserError if _type != "attribute" or _type != ""
     *  @throw BESSyntaxUserError if name is not found at the current scope.
     *
     *  @param name local scope name of the object to remove
     *  @param type  the type of the object to remove: we only handle "attribute" now!  (this includes containers).
     */
    void processRemove(NCMLParser& p);

    /** @brief Helper for processRemove() called when _type == "attribute"
     * to remove the named attribute at the current parser scope of p.
     * */
    void processRemoveAttribute(NCMLParser& p);

    /** @brief Helper for processRemove() called when _type == "variable"
     * to remove the named variables at the current parser scope of p.
     * */
    void processRemoveVariable(NCMLParser& p);

    /** @brief Helper for processRemove() called when _type == "dimension"
     * to remove the named dimensions at the current parser scope of p.
     * */
    void processRemoveDimension(NCMLParser& p);

    static vector<string> getValidAttributes();

    // removes dimension with name from arr
    void removeDimension(libdap::Array* arr, string name);

private:
    // data rep
    std::string _name;
    std::string _type;
};

}

#endif /* __NCML_MODULE__REMOVE_ELEMENT_H__ */
