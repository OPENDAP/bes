
#ifndef A_GatewayPathInfoCommand_h
#define A_GatewayPathInfoCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

#define SPI_DEBUG_KEY "gateway-path-info"
#define SHOW_GATEWAY_PATH_INFO_RESPONSE "show.gatewayPathInfo"
#define SHOW_GATEWAY_PATH_INFO_RESPONSE_STR "showGatewayPathInfo"

class GatewayPathInfoCommand: public BESXMLCommand {
public:
    GatewayPathInfoCommand(const BESDataHandlerInterface &base_dhi);
    virtual ~GatewayPathInfoCommand()
    {
    }

    virtual void parse_request(xmlNode *node);

    virtual bool has_response()
    {
        return true;
    }

    void dump(std::ostream &strm) const override;

    static BESXMLCommand * CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_GatewayPathInfoCommand_h
