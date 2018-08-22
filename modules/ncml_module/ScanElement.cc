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
#include "config.h"

#include "ScanElement.h"

#include <algorithm> // std::sort
#include <cstring>
#include <cerrno>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "AggregationElement.h"
#include "DirectoryUtil.h" // agg_util
#include "NCMLDebug.h"
#include "NCMLParser.h"
#include "NetcdfElement.h"
#include "RCObject.h"
#include "SimpleTimeParser.h"
#include "XMLHelpers.h"

#include "Error.h" // libdap

// ICU includes for the SimpleDateFormat used in this file only
#include <unicode/smpdtfmt.h> // class SimpleDateFormat
#include <unicode/timezone.h> // class TimeZone

using agg_util::FileInfo;
using agg_util::DirectoryUtil;

namespace ncml_module {
const string ScanElement::_sTypeName = "scan";
const vector<string> ScanElement::_sValidAttrs = getValidAttributes();

// The rep for the opaque pointer in the header.
struct ScanElement::DateFormatters {
    DateFormatters() :
        _pDateFormat(0), _pISO8601(0), _markPos(0), _sdfLen(0)
    {
    }
    ~DateFormatters()
    {
        SAFE_DELETE(_pDateFormat);
        SAFE_DELETE(_pISO8601);
    }

    // If we have a _dateFormatMark, we'll create a single
    // instance of the icu SimpleDateFormat in order
    // to process each file.
    icu::SimpleDateFormat* _pDateFormat;
    // We also will create a single instance of a format
    // for ISO 8601 times for output into the coordinate
    icu::SimpleDateFormat* _pISO8601;

    // The position of the # mark in the date format string
    // We match the preceding characters with the filename.
    size_t _markPos;

    // The length of the pattern we actually use for the
    // simple date format.  Thus the SDF pattern is
    // the portion of the string from _markPos+1 to _sdfLen.
    size_t _sdfLen;
};

ScanElement::ScanElement() :
    RCObjectInterface(), NCMLElement(0), _location(""), _suffix(""), _regExp(""), _subdirs(""), _olderThan(""), _dateFormatMark(
        ""), _enhance(""), _ncoords(""), _pParent(0), _pDateFormatters(0)
{
}

ScanElement::ScanElement(const ScanElement& proto) :
    RCObjectInterface(), NCMLElement(0), _location(proto._location), _suffix(proto._suffix), _regExp(proto._regExp), _subdirs(
        proto._subdirs), _olderThan(proto._olderThan), _dateFormatMark(proto._dateFormatMark), _enhance(proto._enhance), _ncoords(
        proto._ncoords), _pParent(proto._pParent) // weak ref so this is fair...
        , _pDateFormatters(0)
{
    if (!_dateFormatMark.empty()) {
        initSimpleDateFormats(_dateFormatMark);
    }
}

ScanElement::~ScanElement()
{
    deleteDateFormats();
    _pParent = 0;
}

AggregationElement*
ScanElement::getParent() const
{
    return _pParent;
}

void ScanElement::setParent(AggregationElement* pParent)
{
    _pParent = pParent;
}

const string&
ScanElement::getTypeName() const
{
    return _sTypeName;
}

ScanElement*
ScanElement::clone() const
{
    return new ScanElement(*this);
}

void ScanElement::setAttributes(const XMLAttributeMap& attrs)
{
    _location = attrs.getValueForLocalNameOrDefault("location", "");
    _suffix = attrs.getValueForLocalNameOrDefault("suffix", "");
    _regExp = attrs.getValueForLocalNameOrDefault("regExp", "");
    _subdirs = attrs.getValueForLocalNameOrDefault("subdirs", "true");
    _olderThan = attrs.getValueForLocalNameOrDefault("olderThan", "");
    _dateFormatMark = attrs.getValueForLocalNameOrDefault("dateFormatMark", "");
    _enhance = attrs.getValueForLocalNameOrDefault("enhance", "");
    _ncoords = attrs.getValueForLocalNameOrDefault("ncoords", "");

    // default is to print errors and throw which we want.
    validateAttributes(attrs, _sValidAttrs);

    // Until we implement them, we'll throw parse errors for those not yet implemented.
    throwOnUnhandledAttributes();

    // Create the SimpleDateFormat's if we have a _dateFormatMark
    if (!_dateFormatMark.empty()) {
        initSimpleDateFormats(_dateFormatMark);
    }
}

void ScanElement::handleBegin()
{
    if (!_parser->isScopeAggregation()) {
        THROW_NCML_PARSE_ERROR(line(), "ScanElement: " + toString() + " "
            "was not the direct child of an <aggregation> element as required!");
    }
}

void ScanElement::handleContent(const string& content)
{
    // shouldn't be any, use the super impl to throw if not whitespace.
    NCMLElement::handleContent(content);
}

void ScanElement::handleEnd()
{
    // Get to the our parent aggregation so we can add
    NetcdfElement* pCurrentDataset = _parser->getCurrentDataset();
    VALID_PTR(pCurrentDataset);
    AggregationElement* pParentAgg = pCurrentDataset->getChildAggregation();
    NCML_ASSERT_MSG(pParentAgg, "ScanElement::handleEnd(): Couldn't"
        " find the the child aggregation of the current dataset, which is "
        "supposed to be our parent!");
    pParentAgg->addScanElement(this);
}

string ScanElement::toString() const
{
    return "<" + _sTypeName + " " + "location=\"" + _location + "\" "
        + // always print this one even in empty.
        printAttributeIfNotEmpty("suffix", _suffix) + printAttributeIfNotEmpty("regExp", _regExp)
        + printAttributeIfNotEmpty("subdirs", _subdirs) + printAttributeIfNotEmpty("olderThan", _olderThan)
        + printAttributeIfNotEmpty("dateFormatMark", _dateFormatMark) + printAttributeIfNotEmpty("ncoords", _ncoords)
        + ">";
}

const string&
ScanElement::ncoords() const
{
    return _ncoords;
}

bool ScanElement::shouldScanSubdirs() const
{
    return (_subdirs == "true");
}

long ScanElement::getOlderThanAsSeconds() const
{
    if (_olderThan.empty()) {
        return 0L;
    }

    long secs = 0;
    bool success = agg_util::SimpleTimeParser::parseIntoSeconds(secs, _olderThan);
    if (!success) {
        THROW_NCML_PARSE_ERROR(line(), "Couldn't parse the olderThan attribute!  Expect a string of the form: "
            "\"%d %units\" where %d is a number and %units is a time unit string such as "
            " \"hours\" or \"s\".");
    }
    else {
        return secs;
    }
}

void ScanElement::getDatasetList(vector<NetcdfElement*>& datasets) const
{
    // Use BES root as our root
    DirectoryUtil scanner;
    scanner.setRootDir(scanner.getBESRootDir());

    BESDEBUG("ncml", "Scan will be relative to the BES root data path = " << scanner.getRootDir() << endl);

    setupFilters(scanner);

    vector<FileInfo> files;
    //vector<FileInfo> dirs;
    try // catch BES errors to give more context,,,,
    {
        // Call the right version depending on setting of subtree recursion.
        if (shouldScanSubdirs()) {
            scanner.getListingOfRegularFilesRecursive(_location, files);
        }
        else {
            scanner.getListingForPath(_location, &files, 0);
        }
    }
    catch (BESNotFoundError& ex) {
        ostringstream oss;
        oss << "In processing " << toString() << " we got a BESNotFoundError with msg=";
        ex.dump(oss);
        oss << " Perhaps a path is incorrect?" << endl;
        THROW_NCML_PARSE_ERROR(line(), oss.str());
    }

    // Let the other exceptions percolate up...  Internal errors
    // and Forbidden are pretty clear and likely not a typo
    // in the NCML like NotFound could be.

    BESDEBUG("ncml", "Scan " << toString() << " returned matching regular files: " << endl);
    if (files.empty()) {
        BESDEBUG("ncml", "WARNING: No matching files found!" << endl);
    }
    else {
        DirectoryUtil::printFileInfoList(files);
    }

    // Let the user know we're performing syntactic sugar with ncoords
    // We'll let the other context decide whether its proper to use it.
    if (!_ncoords.empty()) {
        BESDEBUG("ncml",
            "Scan has ncoords attribute specified: ncoords=" << _ncoords << "  Will be inherited by all matching datasets!" << endl);
    }

    // Adapt the file list into a temp vector of NetcdfElements
    // created from the parser's factory so they
    // get added to its memory pool
    // We use a temp vector since we need to sort the datasets
    // before appending them to the output dataset vector.
    XMLAttributeMap attrs;
    vector<NetcdfElement*> scannedDatasets;
    scannedDatasets.reserve(files.size());
    // Now add them...
    for (vector<FileInfo>::const_iterator it = files.begin(); it != files.end(); ++it) {
        // start fresh
        attrs.clear();

        // The path to the file, relative to the BES root as needed.
        attrs.addAttribute(XMLAttribute("location", it->getFullPath()));

        // If the user has specified the ncoords sugar,
        // pass it down into the netcdf element.
        if (!_ncoords.empty()) {
            attrs.addAttribute(XMLAttribute("ncoords", _ncoords));
        }

        // If there's a dateFormatMark, pull out the coordVal
        // and add it to the attrs map since we want to use that and
        // not the location for the new map vector.
        if (!_dateFormatMark.empty()) {
            string timeCoord = extractTimeFromFilename(it->basename());
            BESDEBUG("ncml", "Got an ISO 8601 time from dateFormatMark: " << timeCoord << endl);
            attrs.addAttribute(XMLAttribute("coordValue", timeCoord));
        }

        // Make the dataset using the parser so it's in the parser memory pool.
        RCPtr<NCMLElement> dataset = _parser->_elementFactory.makeElement("netcdf", attrs, *_parser);
        VALID_PTR(dataset.get());

        // Up the ref count (since it's in an RCPtr) and add to the result vector
        scannedDatasets.push_back(static_cast<NetcdfElement*>(dataset.refAndGet()));
    }

    // We have the scanned datasets in scannedDatasets vector now, so sort
    // on location() or coordValue() depending on whether we have a dateFormatMark...
    if (_dateFormatMark.empty()) // sort by location()
    {
        BESDEBUG("ncml", "Sorting scanned datasets by location()..." << endl);
        std::sort(scannedDatasets.begin(), scannedDatasets.end(), NetcdfElement::isLocationLexicographicallyLessThan);
    }
    else // sort by coordValue()
    {
        BESDEBUG("ncml",
            "Sorting scanned datasets by coordValue() since we got a dateFormatMark" " and the coordValue are ISO 8601 dates..." << endl);
        std::sort(scannedDatasets.begin(), scannedDatasets.end(), NetcdfElement::isCoordValueLexicographicallyLessThan);
    }

    // Also, if there's a dateFormatMark, we want to specify that a new
    // _CoordinateAxisType attribute be added with value "Time" (according to NcML Aggregations page)
    if (!_dateFormatMark.empty()) {
        VALID_PTR(getParent());
        getParent()->setAggregationVariableCoordinateAxisType("Time");
    }

    // Now we can append the sorted local vector of datasets to the output.
    // We need not worry about reference counts since the scannedDatasets
    // has them red'd already and won't deref when it goes out of scope.
    // We are merely transferring them to the ouput, so the
    // refcount is still correct as is.
    BESDEBUG("ncml", "Adding the sorted scanned datasets to the current aggregation list..." << endl);
    datasets.reserve(datasets.size() + scannedDatasets.size());
    datasets.insert(datasets.end(), scannedDatasets.begin(), scannedDatasets.end());
}

void ScanElement::setupFilters(agg_util::DirectoryUtil& scanner) const
{
    // If we have a suffix, set the filter.
    if (!_suffix.empty()) {
        BESDEBUG("ncml", "Scan will filter against suffix=\"" << _suffix << "\"" << endl);
        scanner.setFilterSuffix(_suffix);
    }

    if (!_regExp.empty()) {
        BESDEBUG("ncml", "Scan will filter against the regExp=\"" << _regExp << "\"" << endl);

        // If there's a problem compiling it, we'll know now.
        // So catch it and wrap it as a parse error, which tecnically it is.
        try {
            scanner.setFilterRegExp(_regExp);
        }
        catch (libdap::Error& err) {
            THROW_NCML_PARSE_ERROR(line(),
                "There was a problem compiling the regExp=\"" + _regExp + "\"  : " + err.get_error_message());
        }
    }

    if (!_olderThan.empty()) {
        long secs = getOlderThanAsSeconds();
        struct timeval tvNow;
        gettimeofday(&tvNow, 0);
        long cutoffTime = tvNow.tv_sec - secs;
        scanner.setFilterModTimeOlderThan(static_cast<time_t>(cutoffTime));
        BESDEBUG("ncml",
            "Setting scan filter modification time using duration: " << secs << " from the olderThan attribute=\"" << _olderThan << "\"" " The cutoff modification time based on now is: " << getTimeAsString(cutoffTime) << endl);
    }
}

// SimpleDateFormat to produce ISO 8601
static const string ISO_8601_FORMAT = "yyyy-MM-dd'T'HH:mm:ss'Z'";

/** File local helper to convert UnicodeString to std::string for ICU versions prior to 4.2
 * Fill in toString from the UnicodeString using UnicodeString::extract().
 * @return success  if true, toString is valid.
 */
static bool convertUnicodeStringToStdString(std::string& toString, const icu::UnicodeString& fromUniString)
{
    // This call exists in 4.2 but not 4.0 or 3.6
    // TODO use this call if we up our minimum ICU version
    // fromUniString.toUTF8String(toString);

    toString = ""; // empty it in case of error
    vector<char> buffer; // std::string element[0] isn't guaranteed contiguous like vectors, so we need a temp...
    buffer.resize(fromUniString.length() + 1); // +1 for NULL terminator
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t patternLen = fromUniString.extract(&buffer[0], buffer.size(), 0, errorCode);
    if (patternLen >= static_cast<int32_t>(buffer.size()) || U_FAILURE(errorCode)) {
        return false;
    }
    else {
        toString = std::string(&buffer[0]);
        return true;
    }
}

void ScanElement::initSimpleDateFormats(const std::string& dateFormatMark)
{
    // Make sure no accidental leaks
    deleteDateFormats();
    _pDateFormatters = new DateFormatters;
    VALID_PTR(_pDateFormatters);

    _pDateFormatters->_markPos = dateFormatMark.find_last_of("#");
    if (_pDateFormatters->_markPos == string::npos) {
        THROW_NCML_PARSE_ERROR(line(), "The scan@dateFormatMark attribute did not contain"
            " a marking # character before the date format!"
            " dateFormatMark=\"" + dateFormatMark + "\"");
    }

    // Get just the portion that is the SDF string
    string dateFormat = dateFormatMark.substr(_pDateFormatters->_markPos + 1, string::npos);
    BESDEBUG("ncml", "Using a date format of: " << dateFormat << endl);
    icu::UnicodeString usDateFormat(dateFormat.c_str());

    // Cache the length of the pattern for later substr calcs.
    _pDateFormatters->_sdfLen = dateFormat.size();

    // Try to make the formatter from the user given string
    UErrorCode success = U_ZERO_ERROR;
    _pDateFormatters->_pDateFormat = new icu::SimpleDateFormat(usDateFormat, success);
    if (U_FAILURE(success)) {
        THROW_NCML_PARSE_ERROR(line(), "Scan element failed to parse the SimpleDateFormat pattern: " + dateFormat);
    }
    VALID_PTR(_pDateFormatters->_pDateFormat);
    // Set it to the GMT timezone since we expect UTC times by default.
    _pDateFormatters->_pDateFormat->setTimeZone(*(icu::TimeZone::getGMT()));

    // Also create an ISO 8601 formatter for creating the coordValue's
    // from the parsed UDate's.
    _pDateFormatters->_pISO8601 = new icu::SimpleDateFormat(success);
    if (U_FAILURE(success)) {
        THROW_NCML_PARSE_ERROR(line(), "Scan element failed to create the ISO 8601 SimpleDateFormat"
            " using the pattern " + ISO_8601_FORMAT);
    }
    VALID_PTR(_pDateFormatters->_pISO8601);
    // We want to output UTC, so GMT as well.
    _pDateFormatters->_pISO8601->setTimeZone(*(icu::TimeZone::getGMT()));
    _pDateFormatters->_pISO8601->applyPattern(ISO_8601_FORMAT.c_str());
}

std::string ScanElement::extractTimeFromFilename(const std::string& filename) const
{
    VALID_PTR(_pDateFormatters);
    VALID_PTR(_pDateFormatters->_pDateFormat);
    VALID_PTR(_pDateFormatters->_pISO8601);

    // Skip the first set of chars before the # mark (we don't care that
    // they match, just the quantity).
    string sdfPortion = filename.substr(_pDateFormatters->_markPos, _pDateFormatters->_sdfLen);

    icu::UnicodeString usPattern;
    _pDateFormatters->_pDateFormat->toPattern(usPattern);
    string sdfPattern;
    bool conversionSuccess = convertUnicodeStringToStdString(sdfPattern, usPattern);
    NCML_ASSERT_MSG(conversionSuccess,
        "ScanElement::extractTimeFromFilename: couldn't convert the UnicodeString date pattern to a std::string!");

    BESDEBUG("ncml",
        "Scan is now matching the date portion of the filename " << sdfPortion << " to the SimpleDateFormat=" "\"" << sdfPattern << "\"" << endl);

    UErrorCode status = U_ZERO_ERROR;
    UDate theDate = _pDateFormatters->_pDateFormat->parse(sdfPortion.c_str(), status);
    if (U_FAILURE(status)) {
        THROW_NCML_PARSE_ERROR(line(), "SimpleDateFormat could not parse the pattern="
            "\"" + sdfPattern + "\""
            " on the filename portion=" + "\"" + sdfPortion + "\""
            " of the filename=" + "\"" + filename + "\""
            " Either the pattern was invalid or the filename did not match.");
    }

    icu::UnicodeString usISODate;
    _pDateFormatters->_pISO8601->format(theDate, usISODate);
    string result;
    conversionSuccess = convertUnicodeStringToStdString(result, usISODate);
    NCML_ASSERT_MSG(conversionSuccess,
        "ScanElement::extractTimeFromFilename: failed to convert the UnicodeString ISO date to a std::string!");
    // usISODate.toUTF8String(result);
    return result;
}

void ScanElement::deleteDateFormats() throw ()
{
    SAFE_DELETE(_pDateFormatters);
}

vector<string> ScanElement::getValidAttributes()
{
    vector<string> attrs;
    attrs.push_back("location");
    attrs.push_back("suffix");
    attrs.push_back("regExp");
    attrs.push_back("subdirs");
    attrs.push_back("olderThan");
    attrs.push_back("dateFormatMark");

    // it's in the schema, but we don't support it yet.
    // Will throw later.
    attrs.push_back("enhance");

    // OPeNDAP extension, syntactic sugar applied to all matches.
    attrs.push_back("ncoords");

    return attrs;
}

void ScanElement::throwOnUnhandledAttributes()
{
    if (!_enhance.empty()) {
        THROW_NCML_PARSE_ERROR(line(), "ScanElement: Sorry, enhance attribute is not yet supported.");
    }
}

std::string ScanElement::getTimeAsString(time_t theTime)
{
    struct tm* pTM = gmtime(&theTime);
    char buf[128];
    // this should be "Year-Month-Day Hour:Minute:Second"
    strftime(buf, 128, "%F %T", pTM);
    return string(buf);
}

}
