// GatewayRequestHandler.cc

#include "config.h"

#include <InternalErr.h>

#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESTextInfo.h>
#include "BESDapNames.h"
#include "BESDataDDSResponse.h"
#include "BESDDSResponse.h"
#include "BESDASResponse.h"
#include <BESConstraintFuncs.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>

#include "GatewayRequestHandler.h"
#include "GatewayResponseNames.h"

using namespace libdap;

GatewayRequestHandler::GatewayRequestHandler(const string &name) :
        BESRequestHandler(name)
{
    add_handler(VERS_RESPONSE, GatewayRequestHandler::gateway_build_vers);
    add_handler(HELP_RESPONSE, GatewayRequestHandler::gateway_build_help);
}

GatewayRequestHandler::~GatewayRequestHandler()
{
}

bool GatewayRequestHandler::gateway_build_vers(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESVersionInfo *info = dynamic_cast<BESVersionInfo *>(dhi.response_handler->get_response_object());
    if (!info)
        throw InternalErr(__FILE__, __LINE__, "Expected a BESVersionInfo instance");
#if 0
    info->add_module(PACKAGE_NAME, PACKAGE_VERSION);
#endif
    info->add_module(MODULE_NAME, MODULE_VERSION);
    return ret;
}

bool GatewayRequestHandler::gateway_build_help(BESDataHandlerInterface &dhi)
{
    bool ret = true;
    BESInfo *info = dynamic_cast<BESInfo *>(dhi.response_handler->get_response_object());
    if (!info)
        throw InternalErr(__FILE__, __LINE__, "Expected a BESInfo instance");

    // This is an example. If you had a help file you could load it like
    // this and if your handler handled the following responses.
    map < string, string > attrs;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
#if 0
    attrs["name"] = PACKAGE_NAME;
    attrs["version"] = PACKAGE_VERSION;
#endif
    list < string > services;
    BESServiceRegistry::TheRegistry()->services_handled(Gateway_NAME, services);
    if (services.size() > 0) {
        string handles = BESUtil::implode(services, ',');
        attrs["handles"] = handles;
    }
    info->begin_tag("module", &attrs);
    //info->add_data_from_file( "Gateway.Help", "Gateway Help" ) ;
    info->end_tag("module");

    return ret;
}

void GatewayRequestHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "GatewayRequestHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESRequestHandler::dump(strm);
    BESIndent::UnIndent();
}

