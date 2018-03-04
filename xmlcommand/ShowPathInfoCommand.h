
#ifndef A_ShowPathInfoCommand_h
#define A_ShowPathInfoCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

#define SHOW_PATH_INFO_RESPONSE_STR "showPathInfo"


class ShowPathInfoCommand: public BESXMLCommand {
public:
    ShowPathInfoCommand(const BESDataHandlerInterface &base_dhi);
    virtual ~ShowPathInfoCommand()
    {
    }

    virtual void parse_request(xmlNode *node);

    virtual bool has_response()
    {
        return true;
    }

    virtual void dump(ostream &strm) const;

    static BESXMLCommand * CommandBuilder(const BESDataHandlerInterface &base_dhi);
};

#endif // A_ShowPathInfoCommand_h
