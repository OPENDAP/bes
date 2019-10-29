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

#ifndef __NCML_MODULE__SAX_PARSER_WRAPPER_H__
#define __NCML_MODULE__SAX_PARSER_WRAPPER_H__

#include <string>
#include <libxml/parserInternals.h>
#include "BESError.h"

using namespace std;

namespace ncml_module {
class SaxParser;
}

namespace ncml_module {
/**
 * @brief Wrapper for libxml SAX parser C callbacks into C++.
 *
 * On a parse(const string& ncmlFilename) call, the filename is parsed using the libxml C SAX parser
 * and the C callbacks are passed onto our C++ parser via the SaxParser interface class.
 *
 * Since the underlying libxml is C and uses its internal static memory pools
 * which will be used by other parts of the BES, we have to be careful with exceptions.
 * Any BESError thrown in a SaxParser callback is caught and deferred (stored in this).
 * We enter an error state and ignore all further callbacks until the libxml parser exits cleanly.
 * Then we recreate and rethrow the deferred exception.
 *
 * Any other exception types (...) are also caught and a local BESInternalError is thrown at parse end,
 * so be careful with which exceptions are thrown in SaxParser callbacks.
 *
 * @author mjohnson <m.johnson@opendap.org>
 */
class SaxParserWrapper {
private:
    // Inner Classes

    /** Describes the internal parser state.
     *  NOT_PARSING is when we have not started a parse yet, or have finished one.
     *  PARSING is when we have called the libxml parse function and are dispatching to SaxParser
     *  EXCEPTION is when a SaxParser has thrown BESError and we need to clean up and rethrow it.
     *  NUM_STATES is the number of valid states for error checking.
     */
    enum ParserState {
        NOT_PARSING = 0, PARSING, EXCEPTION, NUM_STATES
    };

private:
    // Data Rep

    /** The SaxParser we are wrapping */
    SaxParser& _parser;

    /** Struct with all the callback functions in it used by parse.
     We add them in the constructor.  They are all static functions
     in the impl file since they're callbacks from C which call through
     to the SaxParser interface.
     */
    xmlSAXHandler _handler;

#if 1
    /** the xml parser context (internals) so we can get access to line numbers
     *  in the parse and pass them along for better debug output on exception.
     */
    xmlParserCtxtPtr _context;
#endif


    /** Current state of the parser.  If EXCEPTION, _error will be the deferred exception. */
    ParserState _state;

    /** If _state==EXCEPTION, these will be a copy of the
     * deferred BESError's data to rethrow after the parser cleans up
     * */
    string _errorMsg;
    int _errorType;
    string _errorFile;
    int _errorLine;

private:
    SaxParserWrapper(const SaxParserWrapper&); // illegal
    SaxParserWrapper& operator=(const SaxParserWrapper&); // illegal

public:
    /**
     * @brief Create a wrapper for the given parser.
     *
     * @param parser Must exist for the duration of the life of the wrapper.
     */
    explicit SaxParserWrapper(SaxParser& parser);
    virtual ~SaxParserWrapper();

    /** @brief Do a SAX parse of the ncmlFilename
     * and pass the calls to wrapper parser.
     *
     * @param path to the file to parse.
     *
     * @throws Can throw BESError via SaxParser
     *
     * @return successful parse
     */
    bool parse(const string& ncmlFilename);

    SaxParser& getParser() const
    {
        return _parser;
    }

    ////////////////////////////
    /// The remaining calls are for the internals of the parser, but need to be public

    /** If we get a BESError exception thrown in a SaxParser call,
     * defer it by entering the EXCEPTION state and copying the exception.
     * In EXCEPTION state, we don't pass on any callbacks to SaxParser.
     * When the underlying C parser completes and cleans up its storage,
     * then we recreate and throw the exception.
     * NOTE: We can't store theErr itself since it will be destroyed by the exception system.
     * @see rethrowException
     */
    void deferException(BESError& theErr);

    /** Used by the callbacks to know whether we have a deferred
     * exception.
     */
    bool isExceptionState() const
    {
        return _state == EXCEPTION;
    }

    /**
     * If there's a deferred exception, this will throw the right subclass type
     * from the preserved state at deferral time.
     */
    void rethrowException();

    /** Return the current line of the parse we're on, assuming we're not in an exception state
     * and that we are parsing.
     */
    int getCurrentParseLine() const;

private:

    /**
     * Prepare the parser by setting the handler callback functions.
     * @todo We could move this to a static initializer or constructor
     * This function used to build a parser context 'object', bound to a filename. For each
     * different file parsed, a new context was made. Once we swithced back to the
     * xmlSAXUserParseFile() function, the context was not needed. jhrg 10/8/19 */
    void setupParser();

#if 1
    // Leak fix. jhrg 6/21/19
    /** Clean the _context and any other state */
    void cleanupParser() throw ();
#endif


};
// class SaxParserWrapper

}// namespace ncml_module

#endif /*__NCML_MODULE__SAX_PARSER_WRAPPER_H__ */
