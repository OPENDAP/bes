// DODSReturnManager.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSReturnManager.h"

DODSReturnManager *DODSReturnManager::_instance = 0 ;

DODSReturnManager::DODSReturnManager()
{
}

DODSReturnManager::~DODSReturnManager()
{
    DODSReturnManager::Transmitter_iter i ;
    DODSTransmitter *t = 0 ;
    for( i = _transmitter_list.begin(); i != _transmitter_list.end(); i++ )
    {
	t = (*i).second ;
	delete t ;
    }
}

bool
DODSReturnManager::add_transmitter( const string &name,
				    DODSTransmitter *transmitter )
{
    if( find_transmitter( name ) == 0 )
    {
	_transmitter_list[name] = transmitter ;
	return true ;
    }
    return false ;
}

DODSTransmitter *
DODSReturnManager::rem_transmitter( const string &name )
{
    DODSTransmitter *ret = 0 ;
    DODSReturnManager::Transmitter_iter i ;
    i = _transmitter_list.find( name ) ;
    if( i != _transmitter_list.end() )
    {
	ret = (*i).second;
	_transmitter_list.erase( i ) ;
    }
    return ret ;
}

DODSTransmitter *
DODSReturnManager::find_transmitter( const string &name )
{
    DODSReturnManager::Transmitter_citer i ;
    i = _transmitter_list.find( name ) ;
    if( i != _transmitter_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

DODSReturnManager *
DODSReturnManager::TheManager()
{
    if( _instance == 0 )
    {
	_instance = new DODSReturnManager ;
    }
    return _instance ;
}

