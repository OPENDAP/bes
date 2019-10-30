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
#ifndef __NCML_MODULE__SCAN_ELEMENT_H__
#define __NCML_MODULE__SCAN_ELEMENT_H__

#include "NCMLElement.h"
#include "AggMemberDataset.h"

namespace agg_util {
class DirectoryUtil;
}

namespace ncml_module {
// FDecls
class NetcdfElement;
class AggregationElement;

/**
 * Implementation of the <scan> element used to scan directories
 * to create the set of files for an aggregation.
 */
class ScanElement: public NCMLElement {
public:
    // class vars
    // Name of the element
    static const std::string _sTypeName;

    // All possible attributes for this element.
    static const std::vector<std::string> _sValidAttrs;

private:
    ScanElement& operator=(const ScanElement& rhs); // disallow

public:
    ScanElement();
    ScanElement(const ScanElement& proto);
    virtual ~ScanElement();

    virtual const std::string& getTypeName() const;
    virtual ScanElement* clone() const; // override clone with more specific subclass
    virtual void setAttributes(const XMLAttributeMap& attrs);
    virtual void handleBegin();
    virtual void handleContent(const std::string& content);
    virtual void handleEnd();
    virtual std::string toString() const;

    ////// Accessors
    const std::string& ncoords() const;

    /** Get the aggregation of which I am a child */
    AggregationElement* getParent() const;

    /**
     * Set the parent of this element.
     * This should ONLY be called by the
     * AggregationElement when this is added
     * to it.
     * @param pParent  the weak ref to the parent.
     */
    void setParent(AggregationElement* pParent);

    /** is the subdirs attribute true? */
    bool shouldScanSubdirs() const;

    /** Get the olderThan attribute in seconds.
     * Returns 0 for the empty attribute
     * and -1 if there's an error parsing the attribute.
     * @return
     */
    long getOlderThanAsSeconds() const;

    /**
     * Actually perform the filesystem scan based
     * on the specified attributes (suffix, subdirs, etc).
     *
     * Fill in the vector with matching datasets, sorted
     * by the filename.
     *
     * NOTE: The members added to this vector will be ref()'d
     * so the caller needs to make sure to deref them!
     *
     * @param datasets The vector to add the datasets to.
     */
    void getDatasetList(std::vector<NetcdfElement*>& datasets) const;

private:
    // internal methods

    /** Set the filters on scanner from the attributes we have set. */
    void setupFilters(agg_util::DirectoryUtil& scanner) const;

    /** Create the SimpleDateFormat's _pDateFormat and _pISO8601
     * for subsequent use.
     * @param dateFormatMark the dateFormatMark to use to create _pDateFormat.
     */
    void initSimpleDateFormats(const std::string& dateFormatMark);

    /** delete _pDateFormat and _pISO8601 if needed. */
    void deleteDateFormats() throw ();

    /** Apply the dateFormatMark and DateFormatters to the given
     * filename to extract the time.  Return this time as an
     * ISO 8601 formatted string.
     * @param filename the basename (no path) of the file to which to apply
     *                  the dateFormatMark
     * @return and ISO 8601  formatted string with the extracted time.
     */
    std::string extractTimeFromFilename(const std::string& filename) const;

    static std::vector<std::string> getValidAttributes();

    /** throw a parse error for non-empty attributes we don't handle yet */
    void throwOnUnhandledAttributes();

    /** Get a humanreadable string expressing the given time theTime */
    static std::string getTimeAsString(time_t theTime);

private:
    // data rep
    std::string _location;
    std::string _suffix;
    std::string _regExp;
    std::string _subdirs;
    std::string _olderThan;
    std::string _dateFormatMark;
    std::string _enhance; // we're not implementing this one now.
    std::string _ncoords; // OPeNDAP EXTENSION to NcML.  Inherited by all matched datasets.

    // Back pointer to our parent
    AggregationElement* _pParent;

    // We use an opaque ptr (pimpl idiom) to push the
    // decl of the ICU classes to the .cc
    // to get config.h information as well as hide the icu headers.
    struct DateFormatters;
    DateFormatters* _pDateFormatters;
};

}

#endif /* __NCML_MODULE__SCAN_ELEMENT_H__ */
