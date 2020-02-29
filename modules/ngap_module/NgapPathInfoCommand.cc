
#include "NgapPathInfoCommand.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

NgapPathInfoCommand::NgapPathInfoCommand(const BESDataHandlerInterface &base_dhi) :
        BESXMLCommand(base_dhi)
{

}

/** @brief parse a show command. No properties or children elements
 *
 &lt;showCatalog node="containerName" /&gt;
 *
 * @param node xml2 element node pointer
 */
void NgapPathInfoCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != SHOW_Ngap_PATH_INFO_RESPONSE_STR) {
        string err = "The specified command " + name + " is not a gateway show path info command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    //  the action is to show the path info response
    d_xmlcmd_dhi.action = SHOW_Ngap_PATH_INFO_RESPONSE;
    d_xmlcmd_dhi.data[SHOW_Ngap_PATH_INFO_RESPONSE] = SHOW_Ngap_PATH_INFO_RESPONSE;
    d_cmd_log_info = "show gatewayPathInfo";

    // node is an optional property, so could be empty string
    d_xmlcmd_dhi.data[CONTAINER] = props["node"];
    if (!d_xmlcmd_dhi.data[CONTAINER].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[CONTAINER];
    }
    d_cmd_log_info += ";";

    BESDEBUG(SPI_DEBUG_KEY, "Built BES Command: '" << d_cmd_log_info << "'"<< endl );

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
void NgapPathInfoCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "NgapPathInfoCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
NgapPathInfoCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new NgapPathInfoCommand(base_dhi);
}

