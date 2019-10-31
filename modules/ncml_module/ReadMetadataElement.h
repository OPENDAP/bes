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
#ifndef __NCML_MODULE__READ_METADATA_ELEMENT_H__
#define __NCML_MODULE__READ_METADATA_ELEMENT_H__

#include "NCMLElement.h"

namespace ncml_module {

/**
 * @brief Concrete class for NcML <readMetadata> element
 *
 * This element is the default, so is basically a noop.  It
 * directs the parser to use the underlying metadata in the source
 * dataset.
 */
class ReadMetadataElement: public NCMLElement {
private:
    ReadMetadataElement& operator=(const ReadMetadataElement&); //disallow
public:
    ReadMetadataElement();
    ReadMetadataElement(const ReadMetadataElement& proto);
    virtual ~ReadMetadataElement();
    virtual const std::string& getTypeName() const;
    virtual ReadMetadataElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const std::string& content);
    virtual void handleEnd();
    virtual std::string toString() const;

    static const std::string _sTypeName;
    static const std::vector<std::string> _sValidAttributes; // will be empty
};

}

#endif /* __NCML_MODULE__READ_METADATA_ELEMENT_H__ */
