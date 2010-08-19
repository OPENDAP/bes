#include "HE2CFNcML.h"

#include <libxml/xmlstring.h>
#include <sys/stat.h>
#include <sstream>

using namespace std;

// BEGIN: XML parser call back functions.
static void ncml_startDocument(void* userData)
{
    DBG(cout << "Got a start document" << endl);
}

static void ncml_endDocument(void* userData)
{
    DBG(cout << "Got an end document" << endl);    
}


static void ncml_startElementNs(void* userData,
                                const xmlChar *localname,
                                const xmlChar *prefix,
                                const xmlChar *URI,
                                int nb_namespaces,
                                const xmlChar **namespaces,
                                int nb_attributes,
                                int /* nb_defaulted */,
                                const xmlChar **attributes)
{

    HE2CFNcML *ncml = static_cast<HE2CFNcML*>(userData);
    // Save the last element.
    // ncml->element_stack = ncml->element;
    ncml->element = ncml->get_string_from_xmlchar(localname);
    if(nb_attributes > 0){
        DBG(cout << "Got "
            << nb_attributes
            << " attributes."
            << endl);        
        ncml->read_attribute(nb_attributes, attributes);
    }
    DBG(cout
        << "Got a start element:"
        << ncml->element
        << endl);
    
}

static void ncml_endElementNs(void* userData,
                              const xmlChar *localname,
                              const xmlChar *prefix,
                              const xmlChar *URI)
                              
{
    HE2CFNcML *ncml = static_cast<HE2CFNcML*>(userData);
    ncml->read_content(ncml->get_string_from_xmlchar(localname));
    DBG(cout << "Got an end element:"
        << localname
        << " from file:"
        << ncml->get_filename()
        << endl);
}



static void ncmlCharacters(void* userData, const xmlChar* content, int len)
{
    
    HE2CFNcML *ncml = static_cast<HE2CFNcML*>(userData);

    ncml->content = "";
    ncml->content.reserve(len);
    DBG(cout << "Got content length: " << len << endl);

    const xmlChar* contentEnd = content+len;
    while(content != contentEnd)
        {
            ncml->content += (const char)(*content++);
        }
    DBG(cout << "Got content: " << ncml->content << endl);

  
}

static void ncmlWarning(void* userData, const char* msg, ...)
{
    HE2CFNcML *ncml = static_cast<HE2CFNcML*>(userData);
    
    char buffer[1024];
    va_list(args);
    va_start(args, msg);
    unsigned int len = sizeof(buffer);
    vsnprintf(buffer, len, msg, args);
    va_end(args);
    
    ostringstream error;
    error << "XML Parser Warning:"
          << buffer;
    ncml->write_error(error.str());
}

static void ncmlFatalError(void* userData, const char* msg, ...)
{
    HE2CFNcML *ncml = static_cast<HE2CFNcML*>(userData);
    
    char buffer[1024];
    va_list(args);
    va_start(args, msg);
    unsigned int len = sizeof(buffer);
    vsnprintf(buffer, len, msg, args);
    va_end(args);

    ostringstream error;
    error << "XML Parser Error:" 
          << buffer;
    ncml->write_error(error.str());

    
}

// END: XML parser call back functions.


// Constructor
HE2CFNcML::HE2CFNcML(): _handler(), _context(0)
{
    _check_name_clash = true;
    _check_multi_cvar = true;    
    _convention = "COARDS";     // default is "COARDS".
    _filename = "conf.xml";     // default is "conf.xml."
    _valid_char = '_';          // default is underscore(_) character.
    _short_name_size = -1;      // default is -1 which means short name option is disabled.
    _prefix = 'A';              // default is 'A' character.
    at = NULL;
    content = "";               
    element = "";               
    element_stack = "";

}

HE2CFNcML::~HE2CFNcML()
{
    _values.clear();
    _variables.clear();
    
}

// BEGIN: Private member functions
bool HE2CFNcML::exists()
{
    struct stat finfo;
    int s;
    // Attempt to get the file attributes
    s = stat(_filename.c_str(), &finfo);
    if(s == 0){
        return true;
    }
    else{
        return false;
    }
}


bool HE2CFNcML::process_attribute()
{
    at->append_attr(_attribute_name, _attribute_type, content);
    DBG(cout << "Got attribute value = " << content  << endl);
    return true;
}

bool HE2CFNcML::process_attribute_attr(string _name, string _value)
{
    if(_name == "name"){
        _attribute_name = _value;
    }
    else if (_name == "type"){
        _attribute_type = _value;
    }
    else {
        string error = element +  " has wrong attribute: " + _name;
        write_error(error);
        return false;
    }
    return true;
}

bool HE2CFNcML::process_check_multi_cvar()
{
    if(content == "0"){
        _check_multi_cvar = false;
        return true;
    }
    else{
        string error =  "<check_multi_cvar> input must be 0. The "
            + get_filename()
            + " has "
            + content
            + ".";
        write_error(error);
        return false;
    }
}

bool HE2CFNcML::process_check_name_clash()
{
    if(content == "0"){
        _check_name_clash = false;
        return true;
    }
    else{
        string error = "<check_multi_cvar> input must be 0. The "
            + get_filename()
            + " has "
            + content
            + ".";
        write_error(error);
        return false;
    }
}



bool HE2CFNcML::process_convention()
{
    if(content == "CF-1.4" || content == "COARDS"){
        _convention = content;
        return true;
    }
    else {
        string error = "Unsupported convention " + content;
        write_error(error);
        return false;
    }
}

bool HE2CFNcML::process_prefix()
{
    
    if(content.size() == 1){
        char c = content.at(0);
        if(isalpha(c)){
            _prefix = content.at(0);
            return true;
        }
        else{
            string error = "<prefix> input must be an alphabetic character.";
            write_error(error);
            return false;
        }
    }
    else{
        ostringstream error;
        error << "<prefix> input must be a single character. The "
              << get_filename()
              << " has "
              << content.size()
              << " characters.";
        write_error(error.str());
        return false;
    }

}

bool HE2CFNcML::process_shortname()
{
    // Check the validity of input
    int i = 0;
    for(i=0; i < content.size(); i++){
        char c = content.at(i);
        if(!isdigit(c)){
            ostringstream error;
            error << "<shortname> input must have numeric characters only.";
            write_error(error.str());
            return false;
        }
    }
    std::stringstream ss(content);
    
    if(!(ss >> _short_name_size)){
        ostringstream error;
        error << "<shortname> input has a too big number."
              << " Parser failed to convert "
              << content << " to integer.";
        write_error(error.str());
        return false;
    }
    
    return true;
}


bool HE2CFNcML::process_suffix()
{
    // Check the validity of input

    int i = 0;
    
    if(content.size() > 3){
        string error =
            "<suffix> input size must be less than or equal to 3:"
            + content;
        write_error(error);
        return false;        
    }
       
    for(i=0; i < content.size(); i++){
        char c = content.at(i);
        if(!(isalnum(c) || c == '_')){
            string error = "<suffix> input must have alphanumeric characters only.";
            write_error(error);
            return false;
        }
    }
    _suffix = content;
    return true;
}


bool HE2CFNcML::process_validchar()
{
    
    if(content.size() == 1){
        char c = content.at(0);
        if(isalnum(c)){
            _valid_char = content.at(0);
            return true;
        }
        else{
            string error = "<validchar> input must be an alpha numeric character.";
            write_error(error);
            return false;
        }
    }
    else{
        ostringstream error;
        error << "<validchar> input must be a single character. The "
              << get_filename()
              << " has "
              << content.size()
              << " characters.";
        write_error(error.str());
        return false;
    }

}

bool HE2CFNcML::process_values(string delimeters)
{

    if(element_stack != "z_dim") {
        string error = "<values> tag must be defined inside <z_dim> tag.";
        write_error(error);
        return false;
    }

    // Tokenize the content.
    // Copied from NCMLUtil::tokenize()
    // Skip delimiters at beginning.
    string::size_type lastPos = content.find_first_not_of(delimeters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = content.find_first_of(delimeters, lastPos);

    int count=0; // how many we added.
    
    while (string::npos != pos || string::npos != lastPos)
        {

            // Found a token, add it to the vector.
            std::stringstream ss(content.substr(lastPos, pos - lastPos));
            int val;
            if(!(ss >> val)){
                ostringstream error;
                error << "<values> input has a wrong value."
                      << " Parser failed to convert "
                      << ss << " to integer.";
                write_error(error.str());
                return false;                
            }
            DBG(cout << "adding value:" << val << endl);
            _values.push_back(val);
            count++;
            // Skip delimiters.  Note the "not_of"
            lastPos = content.find_first_not_of(",", pos);
            // Find next "non-delimiter"
            pos = content.find_first_of(",", lastPos);
        }

    if(count != _z_dim_size){
        string error = "Count mismatch!";
        write_error(error);
        return false;
    }
    
    return true;

}

bool HE2CFNcML::process_variable()
{
    // commit writing the newly built attribute table into DAS table.
    if(element != "attribute"){
        string error = "<variable> tag needs at least one <attribute> tag inside.";
        element_stack = "";     // Reset the stack
        write_error(error);        
        return false;
    }
    
    if(element_stack == "variable"){
        element_stack = "";     // Reset the stack
        // Remember the processed variable name so that
        // we can thrown error for any non-existing variables later.
        _variables.push_back(_variable); 
        return true;        
    }
    else {
        string error = "Malformed <variable> tag.";
        write_error(error);
        return false;
    }

}

bool HE2CFNcML::process_variable_attr(string name, string value)
{
    if(name == "name"){
        // Check if the value is CF compliant.        
        _variable = value;
        element_stack = element;
        at = _das->add_table(value, new AttrTable);
        return true;
    }
    else{
        return false;
    }

}
bool HE2CFNcML::process_z_dim()
{
    if(element_stack == "z_dim"){
        element_stack = "";     // reset stack
    }
    DBG(cout << _z_dim_name << " has " << _z_dim_size << " values" << endl);        
    return true;
}

bool HE2CFNcML::process_z_dim_attr(string _name, string _value)
{
    if(_name == "name"){
        _z_dim_name = _value;
        element_stack = element;
    }
    else if (_name == "length"){
        std::stringstream ss(_value);
    
        if(!(ss >> _z_dim_size)){
            ostringstream error;
            error << "<"
                  << element
                  << ">'s "
                  << _name
                  << "  attribute has a too big number."
                  << " Parser failed to convert "
                  << content << " to integer.";
            write_error(error.str());
            return false;
        }
    }
    else {
        ostringstream error;
        error << element
              << " has wrong attribute: "
              << _name;
        write_error(error.str());
        return false;
    }
    return true;
}

bool HE2CFNcML::process_z_var()
{
    DBG(cout << _z_var_gname << "/" << _z_var_fname << endl);
    return true;
}


bool HE2CFNcML::process_z_var_attr(string _name, string _value)
{
    if(_name == "gname"){
        _z_var_gname = _value;
    }
    else if (_name == "fname"){
        _z_var_fname = _value;        
    }
    else {
        ostringstream error;
        error
            << "<"
            << element
            << "/>"
            << " has wrong attribute: "
            << _name;
        write_error(error.str());
        return false;
    }
    return true;
}

// Same as setAllHandlerCBToNulls(xmlSAXHandler& h) in SaxParserWrapper.cc
void HE2CFNcML::set_callbacks(xmlSAXHandler& h)
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
    h.startElementNs = 0;
    h.endElementNs = 0;
    h.serror = 0;
}

void HE2CFNcML::write_error(string _error)
{
    throw InternalErr(__FILE__, __LINE__,
                        _error);        
    
}
// END: Private member functions
    
// BEGIN: Public member functions
bool HE2CFNcML::get_check_multi_cvar()
{
    return _check_multi_cvar;
}

bool HE2CFNcML::get_check_name_clash()
{
    return _check_name_clash;
}

string HE2CFNcML::get_convention()
{
    return _convention;
}

string HE2CFNcML::get_current_working_directory(string hdf_file_name)
{
    int pos = hdf_file_name.find_last_of('/', hdf_file_name.length() - 1);
    return hdf_file_name.substr(0, pos+1);
}

string HE2CFNcML::get_filename()
{
    return _filename;
}

char HE2CFNcML::get_prefix()
{
    return _prefix;
}


int HE2CFNcML::get_short_name_size()
{
    return _short_name_size;
}


// This function is copied from XMLHelpers.cc.
string HE2CFNcML::get_string_from_xmlchar(const xmlChar* theCharsOrNull)
{
    const char* asChars = reinterpret_cast<const char*>(theCharsOrNull);
    return ( (asChars)?(string(asChars)):(string("")) );
}

string HE2CFNcML::get_suffix()
{
    return _suffix;
}

char HE2CFNcML::get_valid_char()
{
    return _valid_char;
}

// This is one way of adding attributes.
bool HE2CFNcML::read(DAS& das)
{
    _das  = &das;
    read();
}

bool HE2CFNcML::read()
{
    DBG(cout << "Reading " << _filename << endl);
    if(!exists())
        return false;
    
    bool success = true;
    
    // See SaxParserWrapper::setupParser() in ncml_module.
    xmlSAXVersion(&_handler, 2);
    set_callbacks(_handler);
    
    _handler.startDocument = ncml_startDocument;
    _handler.endDocument = ncml_endDocument;
    _handler.startElement = 0;  // version 1
    _handler.endElement = 0;    // versoin 1
    _handler.startElementNs = ncml_startElementNs; // version 2
    _handler.endElementNs = ncml_endElementNs; // version 2
    _handler.characters = ncmlCharacters; // handle the content
    _handler.error = ncmlFatalError;
    _handler.fatalError = ncmlFatalError;    
    _handler.warning = ncmlWarning;
    
    _context = xmlCreateFileParserCtxt(_filename.c_str());
    if(!_context){
        ostringstream error;
        error << "Cannot parse: Unable to create a libxml parser context for "
            + _filename;
        write_error(error.str());
        return false;
    }
    _context->sax = &_handler;
    _context->userData = this;

    
    // See SaxParserWrapper::parse().
    xmlParseDocument(_context);

    success = (_context->errNo == 0);

    // See SaxParserWrapper::cleanupParser().
    if(_context){
        _context->sax = NULL;
        xmlFreeParserCtxt(_context);
        _context = 0;
    }
    return success;
}    


/// sets the internal variables depending on the input element attributes.
bool HE2CFNcML::read_attribute(int nb_attributes, const xmlChar **attributes)
{

    if(element == "z_dim"){
        
        if(nb_attributes != 2){
            ostringstream error;
            error << "<z_dim> tag must have two attributes. It has "
                  << nb_attributes << ".";
            write_error(error.str());
            return false;
        }
        
        for(int i=0; i < nb_attributes; ++i){
            string attr_name = get_string_from_xmlchar(*attributes);
            // copied from XMLUtil::xmlCharToStringFromIterators() in
            // XMLHelpers.cc.
            string attr_value = string(reinterpret_cast<const char*>
                                       (*(attributes+3)),
                                       reinterpret_cast<const char*>
                                       (*(attributes+4)));
            
            DBG(cout << "Got an attribute:"
                << attr_name
                << "="
                << attr_value
                << endl);
            
            attributes += 5;    // libxml2 way
            process_z_dim_attr(attr_name, attr_value);            
        }
        return true;
    }
    else if(element == "z_var"){
        
        if(nb_attributes != 2){
            ostringstream error;
            error << "<z_var> tag must have two attributes. It has "
                  << nb_attributes << ".";
            write_error(error.str());
            return false;
        }
        
        for(int i=0; i < nb_attributes; ++i){
            string attr_name = get_string_from_xmlchar(*attributes);
            // copied from XMLUtil::xmlCharToStringFromIterators() in XMLHelpers.cc.
            string attr_value = string(reinterpret_cast<const char*>
                                       (*(attributes+3)),
                                       reinterpret_cast<const char*>
                                       (*(attributes+4)));
            DBG(cout << "Got an attribute:"
                << attr_name
                << "="
                << attr_value
                << endl);
            attributes += 5;    // libxml2 way
            process_z_var_attr(attr_name, attr_value);            
        }
        return true;        
        
    }
    else if(element == "variable"){
        if(nb_attributes != 1){
            ostringstream error;
            error << "<variable> tag must have one attribute. It has "
                  << nb_attributes << ".";
            write_error(error.str());
            return false;
        }

        string attr_name = get_string_from_xmlchar(*attributes);
        // copied from XMLUtil::xmlCharToStringFromIterators() in XMLHelpers.cc.
        string attr_value = string(reinterpret_cast<const char*>(*(attributes+3)),
                                   reinterpret_cast<const char*>(*(attributes+4)));
        DBG(cout << "Got an attribute:"
            << attr_name
            << "="
            << attr_value
            << endl);
        attributes += 5;    // libxml2 way
        process_variable_attr(attr_name, attr_value);            

        return true;        
        
    }
    else if(element == "attribute"){
        if(nb_attributes != 2){
            ostringstream error;
            error << "<attribute> tag must have two attributes. It has "
                  <<  nb_attributes << ".";
            write_error(error.str());
            return false;
        }
        if(element_stack != "variable"){
            ostringstream error;
            error << "<attribute> must be defined inside <variable> tag.";
            write_error(error.str());
            return false;
        }
        for(int i=0; i < nb_attributes; ++i){
            string attr_name = get_string_from_xmlchar(*attributes);
            // copied from XMLUtil::xmlCharToStringFromIterators() in XMLHelpers.cc.
            string attr_value = string(reinterpret_cast<const char*>(*(attributes+3)),
                                       reinterpret_cast<const char*>(*(attributes+4)));
            DBG(cout << "Got an attribute:"
                << attr_name
                << "="
                << attr_value
                << endl);
            attributes += 5;    // libxml2 way
            process_attribute_attr(attr_name, attr_value);            
        }
        return true;        
        
    }
    
    else{
        ostringstream error;
        error << element
              << " element doesn't allow any attributes.";
        write_error(error.str());
        return false;
    }
}


/// sets the internal variables depending on the input element content.
bool HE2CFNcML::read_content(string _element)
{
    // Ideally, Factory method is better than this but
    // let's make it simple for the moment.
    if(_element == "validchar" && element == "validchar"){
        return process_validchar();
    }
    else if(_element == "shortname" && element == "shortname"){
        return process_shortname();
    }
    else if(_element == "prefix" && element == "prefix"){
        return process_prefix();
    }
    else if(_element == "suffix" && element == "suffix"){
        return process_suffix();
    }
    else if(_element == "values" && element == "values"){
        return process_values(",");
    }
    else if(_element == "z_dim" && (element == "z_dim" || element == "values")){
        // Insert the assembled information into a z_dim vector.
        return process_z_dim();
    }
    else if(_element == "z_var" && element == "z_var"){
        // Insert the assembled information into a z_var vector.
        return process_z_var();
    }
    else if(_element == "convention" && element == "convention"){
        return process_convention();
    }
    else if(_element == "check_name_clash" && element == "check_name_clash"){
        return process_check_name_clash();
    }
    else if(_element == "check_multi_cvar" &&
            element == "check_multi_cvar"){
        return process_check_multi_cvar();
    }
    else if(_element == "variable"){
        return process_variable();
    }
    else if(_element == "attribute" &&
            element == "attribute"){
        return process_attribute();
    }
    else if(_element == "hdf4_handler"){
        return true;
    }
    else{
        ostringstream error;
        error << "Invalid element </" << _element << ">";
        write_error(error.str());
        return false;
    }
}

void HE2CFNcML::set_filename(string s)
{
    _filename = s;
}    

bool HE2CFNcML::set_variable_clear(string s)
{

    for(unsigned int i=0; i < _variables.size(); i++){
        if(_variables.at(i) == s){
            _variables.erase(_variables.begin()+i);
            return true;
        }
    }
    return false;
}    


// END: Public member functions
