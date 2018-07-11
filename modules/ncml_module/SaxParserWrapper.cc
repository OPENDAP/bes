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

#include "SaxParserWrapper.h"

#include <exception>
#include <iostream>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <stdio.h> // for vsnprintf
#include <string>

#include "BESDebug.h"
#include "BESError.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "NCMLDebug.h"
#include "SaxParser.h"
#include "XMLHelpers.h"

// Toggle to tell the parser to use the Sax2 start/end element
// calls with namespace information.
// [ TODO We probably want to remove the non-namespace pathways at some point,
// but I will leave them here for now in case there's issues ]
#define NCML_PARSER_USE_SAX2_NAMESPACES 1

using namespace std;
using namespace ncml_module;

////////////////////////////////////////////////////////////////////////////////
// Helpers

#if NCML_PARSER_USE_SAX2_NAMESPACES
static const int SAX2_NAMESPACE_ATTRIBUTE_ARRAY_STRIDE = 5;
static int toXMLAttributeMapWithNamespaces(XMLAttributeMap& attrMap, const xmlChar** attributes, int num_attributes)
{
    attrMap.clear();
    for (int i = 0; i < num_attributes; ++i) {
        XMLAttribute attr;
        attr.fromSAX2NamespaceAttributes(attributes);
        attributes += SAX2_NAMESPACE_ATTRIBUTE_ARRAY_STRIDE; // jump to start of next record
        attrMap.addAttribute(attr);
    }
    return num_attributes;
}
#else
// Assumes the non-namespace calls, so attrs is stride 2 {name,value}
static int toXMLAttributeMapNoNamespaces(XMLAttributeMap& attrMap, const xmlChar** attrs)
{
    attrMap.clear();
    int count=0;
    while (attrs && *attrs != NULL)
    {
        XMLAttribute attr;
        attr.localname = XMLUtil::xmlCharToString(*attrs);
        attr.value = XMLUtil::xmlCharToString(*(attrs+1));
        attrMap.addAttribute(attr);
        attrs += 2;
        count++;
    }
    return count;
}
#endif // NCML_PARSER_USE_SAX2_NAMESPACES

/////////////////////////////////////////////////////////////////////
// Callback we will register that just pass on to our C++ engine
//
// NOTE WELL: New C handlers need to follow the given
//  other examples in order to avoid memory leaks
//  in libxml during an exception!

// To avoid cut & paste below, we use this macro to cast the void* into the wrapper and
// set up a proper error handling structure around the main call.
// The macro internally defines the symbol "parser" to the SaxParser contained in the wrapper.
// So for example, a safe handler call to SaxParser would look like:
// static void ncmlStartDocument(void* userData)
//{
//  BEGIN_SAFE_HANDLER_CALL(userData); // pass in the void*, which is a SaxParserWrapper*
//  parser.onStartDocument(); // call the dispatch on the wrapped parser using the autodefined name parser
//  END_SAFE_HANDLER_CALL; // end the error handling wrapper
//}

#define BEGIN_SAFE_PARSER_BLOCK(argName) { \
  SaxParserWrapper* _spw_ = static_cast<SaxParserWrapper*>(argName); \
    if (_spw_->isExceptionState()) \
    { \
      return; \
    } \
  else \
    { \
      try \
      { \
        SaxParser& parser = _spw_->getParser(); \
        parser.setParseLineNumber(_spw_->getCurrentParseLine());

// This is required after the end of the actual calls to the parser.
#define END_SAFE_PARSER_BLOCK } \
      catch (BESError& theErr) \
      { \
        BESDEBUG("ncml", "Caught BESError&, deferring..." << endl); \
        _spw_->deferException(theErr); \
      } \
      catch (std::exception& ex) \
      { \
        BESDEBUG("ncml", "Caught std::exception&, wrapping and deferring..." << endl); \
        BESInternalError _badness_("Wrapped std::exception.what()=" + string(ex.what()), __FILE__, __LINE__);\
        _spw_->deferException(_badness_); \
      } \
      catch (...)  \
      {   \
        BESDEBUG("ncml", "Caught unknown (...) exception: deferring default error." << endl); \
        BESInternalError _badness_("SaxParserWrapper:: Unknown Exception Type: ", __FILE__, __LINE__); \
        _spw_->deferException(_badness_);  \
      }  \
    } \
}

//////////////////////////////////////////////
// Our C SAX callbacks, wrapped carefully.

static void ncmlStartDocument(void* userData)
{
    BEGIN_SAFE_PARSER_BLOCK(userData)

    parser.onStartDocument();

    END_SAFE_PARSER_BLOCK;
}

static void ncmlEndDocument(void* userData)
{
    BEGIN_SAFE_PARSER_BLOCK(userData)
;    // BESDEBUG("ncml", "SaxParserWrapper::ncmlEndDocument() -  BEGIN"<< endl);

    parser.onEndDocument();

    // BESDEBUG("ncml", "SaxParserWrapper::ncmlEndDocument() -  END"<< endl);

    END_SAFE_PARSER_BLOCK;
}

#if !NCML_PARSER_USE_SAX2_NAMESPACES

static void ncmlStartElement(void * userData,
    const xmlChar * name,
    const xmlChar ** attrs)
{
    // BESDEBUG("ncml", "ncmlStartElement called for:<" << name << ">" << endl);
    BEGIN_SAFE_PARSER_BLOCK(userData);

    string nameS = XMLUtil::xmlCharToString(name);
    XMLAttributeMap map;
    toXMLAttributeMapNoNamespaces(map, attrs);

    // These args will be valid for the scope of the call.
    parser.onStartElement(nameS, map);

    END_SAFE_PARSER_BLOCK;
}

static void ncmlEndElement(void * userData,
    const xmlChar * name)
{
    BEGIN_SAFE_PARSER_BLOCK(userData);

    string nameS = XMLUtil::xmlCharToString(name);
    parser.onEndElement(nameS);

    END_SAFE_PARSER_BLOCK;
}
#endif //  !NCML_PARSER_USE_SAX2_NAMESPACES

#if NCML_PARSER_USE_SAX2_NAMESPACES
static
void ncmlSax2StartElementNs(void *userData, const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI,
    int nb_namespaces, const xmlChar **namespaces, int nb_attributes, int /* nb_defaulted */,
    const xmlChar **attributes)
{
    // BESDEBUG("ncml", "ncmlStartElement called for:<" << name << ">" << endl);
    BEGIN_SAFE_PARSER_BLOCK(userData)
;
    BESDEBUG("ncml", "SaxParserWrapper::ncmlSax2StartElementNs() - localname:" << localname << endl);

    XMLAttributeMap attrMap;
    toXMLAttributeMapWithNamespaces(attrMap, attributes, nb_attributes);

    XMLNamespaceMap nsMap;
    nsMap.fromSAX2Namespaces(namespaces, nb_namespaces);

    // These args will be valid for the scope of the call.
    string localnameString = XMLUtil::xmlCharToString(localname);
    string prefixString = XMLUtil::xmlCharToString(prefix);
    string uriString = XMLUtil::xmlCharToString(URI);

    parser.onStartElementWithNamespace(
        localnameString,
        prefixString,
        uriString,
        attrMap,
        nsMap);

    END_SAFE_PARSER_BLOCK;
}

static
void ncmlSax2EndElementNs(void *userData, const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI)
{
    BEGIN_SAFE_PARSER_BLOCK(userData);

    string localnameString = XMLUtil::xmlCharToString(localname);
    string prefixString = XMLUtil::xmlCharToString(prefix);
    string uriString = XMLUtil::xmlCharToString(URI);
    parser.onEndElementWithNamespace(localnameString, prefixString, uriString);

    END_SAFE_PARSER_BLOCK;
}
#endif // NCML_PARSER_USE_SAX2_NAMESPACES

static void ncmlCharacters(void* userData, const xmlChar* content, int len)
{
    BEGIN_SAFE_PARSER_BLOCK(userData);

    // len is since the content string might not be null terminated,
    // so we have to build out own and pass it up special....
    // TODO consider just using these xmlChar's upstairs to avoid copies, or make an adapter or something.
    string characters("");
    characters.reserve(len);
    const xmlChar* contentEnd = content+len;
    while(content != contentEnd)
    {
        characters += (const char)(*content++);
    }

    parser.onCharacters(characters);

    END_SAFE_PARSER_BLOCK;
}

static void ncmlWarning(void* userData, const char* msg, ...)
{
    BEGIN_SAFE_PARSER_BLOCK(userData);

    BESDEBUG("ncml", "SaxParserWrapper::ncmlWarning() - msg:" << msg << endl);

    char buffer[1024];
    va_list(args);
    va_start(args, msg);
    unsigned int len = sizeof(buffer);
    vsnprintf(buffer, len, msg, args);
    va_end(args);
    parser.onParseWarning(string(buffer));
    END_SAFE_PARSER_BLOCK;
}

static void ncmlFatalError(void* userData, const char* msg, ...)
{
    BEGIN_SAFE_PARSER_BLOCK(userData);

    BESDEBUG("ncml", "SaxParserWrapper::ncmlFatalError() - msg:" << msg << endl);

    char buffer[1024];
    va_list(args);
    va_start(args, msg);
    unsigned int len = sizeof(buffer);
    vsnprintf(buffer, len, msg, args);
    va_end(args);
    parser.onParseError(string(buffer));

    END_SAFE_PARSER_BLOCK;
}

////////////////////////////////////////////////////////////////////////////////
// class SaxParserWrapper impl

SaxParserWrapper::SaxParserWrapper(SaxParser& parser) :
    _parser(parser), _handler() // inits to all nulls.
    , _context(0), _state(NOT_PARSING), _errorMsg(""), _errorType(0), _errorFile(""), _errorLine(-1)
{
}

SaxParserWrapper::~SaxParserWrapper()
{
    // Really not much to do...  everything cleans itself up.
    _state = NOT_PARSING;
    cleanupParser();
}

bool SaxParserWrapper::parse(const string& ncmlFilename)
{
    bool success = true;

    // It's illegal to call this until it's done.
    if (_state == PARSING) {
        throw BESInternalError("Parse called again while already in parse.", __FILE__, __LINE__);
    }

    // OK, now we're parsing
    _state = PARSING;

    setupParser(ncmlFilename);

    // Old way where we have no context.
    //  int errNo = xmlSAXUserParseFile(&_handler, this, ncmlFilename.c_str());
    //  success = (errNo == 0);

    // Any BESError thrown in SaxParser callbacks will be deferred by the safe handler blocks
    // So that we safely pass this line.
    // Even if not, _context is cleared in dtor just in case.
    xmlParseDocument(_context);

    success = (_context->errNo == 0);

    cleanupParser();

    // If we deferred an exception during the libxml parse call, now's the time to rethrow it.
    if (isExceptionState()) {
        rethrowException();
    }

    // Otherwise, we're also done parsing.
    _state = NOT_PARSING;
    return success;
}

void SaxParserWrapper::deferException(BESError& theErr)
{
    _state = EXCEPTION;
    _errorType = theErr.get_bes_error_type();
    _errorMsg = theErr.get_message();
    _errorLine = theErr.get_line();
    _errorFile = theErr.get_file();
}

// HACK admittedly a little gross, but it's weird to have to copy an exception
// and this seemed the safest way rather than making dynamic storage, etc.
void SaxParserWrapper::rethrowException()
{
    // Clear our state out so we can parse again though.
    _state = NOT_PARSING;

    switch (_errorType) {
    case BES_INTERNAL_ERROR:
        throw BESInternalError(_errorMsg, _errorFile, _errorLine);
        break;

    case BES_INTERNAL_FATAL_ERROR:
        throw BESInternalFatalError(_errorMsg, _errorFile, _errorLine);
        break;

    case BES_SYNTAX_USER_ERROR:
        throw BESSyntaxUserError(_errorMsg, _errorFile, _errorLine);
        break;

    case BES_FORBIDDEN_ERROR:
        throw BESForbiddenError(_errorMsg, _errorFile, _errorLine);
        break;

    case BES_NOT_FOUND_ERROR:
        throw BESNotFoundError(_errorMsg, _errorFile, _errorLine);
        break;

    default:
        throw BESInternalError("Unknown exception type.", __FILE__, __LINE__);
        break;
    }
}

int SaxParserWrapper::getCurrentParseLine() const
{
    if (_context) {
        return xmlSAX2GetLineNumber(_context);
    }
    else {
        return -1;
    }
}

static void setAllHandlerCBToNulls(xmlSAXHandler& h)
{
    h.internalSubset = 0;
    h.isStandalone = 0;
    h.hasInternalSubset = 0;
    h.hasExternalSubset = 0;
    h.resolveEntity = 0;
    h.getEntity = 0;
    h.entityDecl = 0;
    h.notationDecl = 0;
    h.attributeDecl = 0;
    h.elementDecl = 0;
    h.unparsedEntityDecl = 0;
    h.setDocumentLocator = 0;
    h.startDocument = 0;
    h.endDocument = 0;
    h.startElement = 0;
    h.endElement = 0;
    h.reference = 0;
    h.characters = 0;
    h.ignorableWhitespace = 0;
    h.processingInstruction = 0;
    h.comment = 0;
    h.warning = 0;
    h.error = 0;
    h.fatalError = 0;
    h.getParameterEntity = 0;
    h.cdataBlock = 0;
    h.externalSubset = 0;

    // unsigned int initialized; magic number the init should fill in
    /* The following fields are extensions available only on version 2 */
    // void *_private; //i'd assume i don't set this either...
    h.startElementNs = 0;
    h.endElementNs = 0;
    h.serror = 0;
}

void SaxParserWrapper::setupParser(const string& filename)
{
    // setup the handler for version 2,
    // which sets an internal version magic number
    // into _handler.initialized
    // but which doesn't clear the handlers to 0.
    xmlSAXVersion(&_handler, 2);

    // Initialize all handlers to 0 by hand to start
    // so we don't blow those internal magic numbers.
    setAllHandlerCBToNulls(_handler);

    // Put our static functions into the handler
    _handler.startDocument = ncmlStartDocument;
    _handler.endDocument = ncmlEndDocument;
    _handler.warning = ncmlWarning;
    _handler.error = ncmlFatalError;
    _handler.fatalError = ncmlFatalError;
    _handler.characters = ncmlCharacters;

    // We'll use one or the other until we're sure it works.
#if NCML_PARSER_USE_SAX2_NAMESPACES
    _handler.startElement = 0;
    _handler.endElement = 0;
    _handler.startElementNs = ncmlSax2StartElementNs;
    _handler.endElementNs = ncmlSax2EndElementNs;
#else
    _handler.startElement = ncmlStartElement;
    _handler.endElement = ncmlEndElement;
    _handler.startElementNs = 0;
    _handler.endElementNs = 0;
#endif // NCML_PARSER_USE_SAX2_NAMESPACES

    // Create the non-validating parser context for the file
    // using this as the userData for making exception-safe
    // C++ calls.
    _context = xmlCreateFileParserCtxt(filename.c_str());
    if (!_context) {
        THROW_NCML_PARSE_ERROR(-1, "Cannot parse: Unable to create a libxml parse context for " + filename);
    }
    _context->sax = &_handler;
    _context->userData = this;
    _context->validate = false;
}

void SaxParserWrapper::cleanupParser() throw ()
{
    if (_context) {
        // Remove our handler from it.
        _context->sax = NULL;

        // Free it up.
        xmlFreeParserCtxt(_context);
        _context = 0;
    }
}

