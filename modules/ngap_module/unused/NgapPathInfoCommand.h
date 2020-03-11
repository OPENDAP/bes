
#ifndef A_NgapPathInfoCommand_h
#define A_NgapPathInfoCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

#define SPI_DEBUG_KEY "ngap-path-info"
#define SHOW_NGAP_PATH_INFO_RESPONSE "show.ngapPathInfo"
#define SHOW_NGAP_PATH_INFO_RESPONSE_STR "showNgapPathInfo"

class NgapPathInfoCommand: public BESXMLCommand {
public:
    NgapPathInfoCommand(const BESDataHandlerInterface &base_dhi);
    virtual ~NgapPathInfoCommand()
    {
    }

    virtual void parse_request(xmlNode *node);

    virtual bool has_response()
    {
        return true;
    }

    virtual void dump(std::ostream &strm) const;

    static BESXMLCommand * CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_NgapPathInfoCommand_h
