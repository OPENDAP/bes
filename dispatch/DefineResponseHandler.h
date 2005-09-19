// DefineResponseHandler.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DefineResponseHandler_h
#define I_DefineResponseHandler_h 1

#include "DODSResponseHandler.h"

/** @brief response handler that creates a definition given container names
 * and optionally constraints and attributes for each of those containers.
 *
 * A request looks like:
 *
 * define &lt;def_name&gt; as &lt;container_list&gt;
 * <BR />
 * &nbsp;&nbsp;[where &lt;container_x&gt;.constraint="&lt;constraint&gt;"]
 * <BR />
 * &nbsp;&nbsp;[,&lt;container_x&gt;.attributes="&lt;attrs&gt;"]
 * <BR />
 * &nbsp;&nbsp;[aggregate by "&lt;aggregation_command&gt;"];
 *
 * where container_list is a list of containers representing points of data,
 * such as a file. For each container in the container_list the user can
 * specify a constraint and a list of attributes. You need not specify a
 * constraint for a given container or a list of attributes. If just
 * specifying a constraint then leave out the attributes. If just specifying a
 * list of attributes then leave out the constraint. For example:
 *
 * define d1 as container_1,container_2
 * <BR />
 * &nbsp;&nbsp;where container_1.constraint="constraint1"
 * <BR />
 * &nbsp;&nbsp;,container_2.constraint="constraint2"
 * <BR />
 * &nbsp;&nbsp;,container_2.attributes="attr1,attr2";
 *
 * It adds the new definition to the list. If a definition already exists
 * in the list with the given name then it is removed first and the new one is
 * added. The definition is currently available through the life of the server
 * process and is not persistent.
 *
 * @see DODSDefine
 * @see DODSDefineList
 * @see DODSContainer
 * @see DODSTransmitter
 * @see DODSTokenizer
 */
class DefineResponseHandler : public DODSResponseHandler
{
public:
				DefineResponseHandler( string name ) ;
    virtual			~DefineResponseHandler( void ) ;

    virtual void		execute( DODSDataHandlerInterface &dhi ) ;
    virtual void		transmit( DODSTransmitter *transmitter,
                                          DODSDataHandlerInterface &dhi ) ;

    static DODSResponseHandler *DefineResponseBuilder( string handler_name ) ;
};

#endif // I_DefineResponseHandler_h

// $Log: DefineResponseHandler.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
