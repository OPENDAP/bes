
#ifndef A_W10NShowPathInfoCommand_h
#define A_W10NShowPathInfoCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

#define W10N_SHOW_PATH_INFO_REQUEST "showW10nPathInfo"
#define W10N_SHOW_PATH_INFO_REQUEST_HANDLER_KEY "show.w10nPathInfo"

class W10nShowPathInfoCommand: public BESXMLCommand {
public:
    W10nShowPathInfoCommand(const BESDataHandlerInterface &base_dhi);
    virtual ~W10nShowPathInfoCommand()
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

#endif // A_W10NShowPathInfoCommand_h
