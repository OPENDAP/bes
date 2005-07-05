// DODSReturnManager.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef I_DODSReturnManager_h
#define I_DODSReturnManager_h 1

#include <map>
#include <string>

using std::map ;
using std::less ;
using std::allocator ;
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
    map< string, DODSTransmitter *, less< string >, allocator< string > > _transmitter_list ;
public:
				DODSReturnManager() ;
    virtual			~DODSReturnManager() ;

    typedef map< string, DODSTransmitter *, less< string >, allocator< string > >::const_iterator Transmitter_citer ;
    typedef map< string, DODSTransmitter *, less< string >, allocator< string > >::iterator Transmitter_iter ;

    virtual bool		add_transmitter( const string &name,
						 DODSTransmitter *transmitter );
    virtual DODSTransmitter *	rem_transmitter( const string &name) ;
    virtual DODSTransmitter *	find_transmitter( const string &name ) ;
};

#endif // I_DODSReturnManager_h

// $Log: DODSReturnManager.h,v $
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
