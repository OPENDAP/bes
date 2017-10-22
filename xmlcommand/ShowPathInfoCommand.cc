
#include "ShowPathInfoCommand.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"

#define SPI_DEBUG_KEY "show_path_info"

#define SHOW_PATH_INFO_RESPONSE "show.pathInfo"
#define SHOW_PATH_INFO_RESPONSE_STR "showPathInfo"


ShowPathInfoCommand::ShowPathInfoCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a show command. No properties or children elements
 *
 &lt;showCatalog node="containerName" /&gt;
 *
 * @param node xml2 element node pointer
 */
void ShowPathInfoCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != SHOW_PATH_INFO_RESPONSE_STR) {
        string err = "The specified command " + name + " is not a show w10n command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // the the action is to show the w10n info response
    _dhi.action = SHOW_PATH_INFO_RESPONSE;
    _dhi.data[SHOW_PATH_INFO_RESPONSE] = SHOW_PATH_INFO_RESPONSE;
    _str_cmd = "show pathInfo";

    // node is an optional property, so could be empty string
    _dhi.data[CONTAINER] = props["node"];
    if (!_dhi.data[CONTAINER].empty()) {
        _str_cmd += " for " + _dhi.data[CONTAINER];
    }
    _str_cmd += ";";

    BESDEBUG(SPI_DEBUG_KEY, "Built BES Command: '" << _str_cmd << "'"<< endl );

    // now that we've set the action, go get the response handler for the
    // action by calling set_response() in our parent class
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void ShowPathInfoCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ShowPathInfoCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
ShowPathInfoCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new ShowPathInfoCommand(base_dhi);
}

