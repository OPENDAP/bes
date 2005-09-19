// DODSReturnManager.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSReturnManager_h
#define I_DODSReturnManager_h 1

#include <map>
#include <string>

using std::map ;
using std::string ;

#include "DODSDataHandlerInterface.h"
#include "DODSTransmitter.h"

class DODSTransmitter ;

/** @brief ReturnManager holds the list of response object transmitter that
 * knows how to transmit response objects in particular ways.
 *
 */
class DODSReturnManager {
private:
    static DODSReturnManager *	_instance ;

    map< string, DODSTransmitter * > _transmitter_list ;
protected:
				DODSReturnManager() ;
public:
    virtual			~DODSReturnManager() ;

    typedef map< string, DODSTransmitter * >::const_iterator Transmitter_citer ;
    typedef map< string, DODSTransmitter * >::iterator Transmitter_iter ;

    virtual bool		add_transmitter( const string &name,
						 DODSTransmitter *transmitter );
    virtual DODSTransmitter *	rem_transmitter( const string &name) ;
    virtual DODSTransmitter *	find_transmitter( const string &name ) ;

    static DODSReturnManager *	TheManager() ;
};

#endif // I_DODSReturnManager_h

// $Log: DODSReturnManager.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
