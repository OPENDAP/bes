// DODSEncode.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <unistd.h>
#include <iostream>
#include <string>

using std::string ;
using std::cerr ;
using std::cout ;
using std::endl ;

#include "DODSEncode.h"

char
DODSEncode::sap_bit( unsigned char val, int pos )
{
    int result ;
    if( ( pos > 7 ) ||( pos < 0 ) ) 
    {
	cerr << "pos not in [0,7]" << endl ;
	exit( 1 ) ;
    }
    switch( pos )
    {
	case 0:
	    result = ( val&1 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 1 ) return '\1' ;
	    break ;
	case 1:
	    result = ( val&2 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 2 ) return '\1' ;
	    break ;
	case 2:
	    result = ( val&4 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 4 ) return '\1' ;
	    break ;
	case 3:
	    result = ( val&8 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 8 ) return '\1' ;
	    break ;
	case 4:
	    result = ( val&16 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 16 ) return '\1' ;
	    break ;
	case 5:
	    result = ( val&32 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 32 ) return '\1' ;
	    break ;
	case 6:
	    result = ( val&64 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 64 ) return '\1' ;
	    break ;
	case 7:
	    result = ( val&128 ) ;
	    if( result == 0 ) return '\0' ;
	    else if( result == 128 ) return '\1' ;
	    break ;
    }
    return 0 ;
}

void
DODSEncode::my_encode( const char * text, const char *key, char *encoded_text )
{
    char bit_control[64] ;
    char key_control[64] ;
    int j = 0 ;
    for( int i = 0 ; i<64 ; i++ )
    {
	bit_control[i] = DODSEncode::sap_bit( text[i/8], j ) ;
	key_control[i] = DODSEncode::sap_bit( key [i/8], j ) ;
	if( ++j == 8 ) j = 0 ;
    }
    setkey( key_control ) ;
    encrypt( bit_control, 0 ) ;
    for( int j = 0 ; j<64 ; j++ )
    {
	if( bit_control[j] == '\0' ) encoded_text[j] = '0' ;
	else if( bit_control[j] == '\1' ) encoded_text[j] = '1' ;
    }
}

void
DODSEncode::my_decode( const char * encoded_text, const char *key,
                       char *decoded_text )
{
    char bit_control[64] ;
    char key_control[64] ;
    int j = 0 ;
    for( int i = 0 ; i<64 ; i++ )
    {
	key_control[i] = DODSEncode::sap_bit( key[i/8], j ) ;
	if( ++j == 8 ) j = 0 ;
	if( encoded_text[i] == '0' ) bit_control[i] = '\0' ;
	else if( encoded_text[i] == '1' ) bit_control[i] = '\1' ;
    }
    setkey( key_control ) ;
    encrypt( bit_control, 1 ) ;
    int pos0, pos1, pos2, pos3, pos4, pos5, pos6, pos7 ;
    pos0 = pos1 = pos2 = pos3 = pos4 = pos5 = pos6 = pos7 = 0 ;
    int k = 0 ;
    for( int j = 0 ; j<64 ; j+= 8 )
    {
	if( bit_control[j+0] == '\0' ) pos0 = 0 ;
	else if( bit_control[j+0] == '\1' ) pos0 = 1 ;
	if( bit_control[j+1] == '\0' ) pos1 = 0 ;
	else if( bit_control[j+1] == '\1' ) pos1 = 1 ;
	if( bit_control[j+2] == '\0' ) pos2 = 0 ;
	else if( bit_control[j+2] == '\1' ) pos2 = 1 ;
	if( bit_control[j+3] == '\0' ) pos3 = 0 ;
	else if( bit_control[j+3] == '\1' ) pos3 = 1 ;
	if( bit_control[j+4] == '\0' ) pos4 = 0 ;
	else if( bit_control[j+4] == '\1' ) pos4 = 1 ;
	if( bit_control[j+5] == '\0' ) pos5 = 0 ;
	else if( bit_control[j+5] == '\1' ) pos5 = 1 ;
	if( bit_control[j+6] == '\0' ) pos6 = 0 ;
	else if( bit_control[j+6] == '\1' ) pos6 = 1 ;
	if( bit_control[j+7] == '\0' ) pos7 = 0 ;
	else if( bit_control[j+7] == '\1' ) pos7 = 1 ;
	int total = ( 128*pos7 ) + ( 64*pos6 ) + ( 32*pos5 ) +
	            ( 16*pos4 ) + ( 8*pos3 ) + ( 4*pos2 ) +
		    ( 2*pos1 ) + ( 1*pos0 ) ;
	decoded_text[k++] = total ;
    }
}

void
DODSEncode::encode( const char *text, const char *key, char *encoded_text )
{
    // take the text and break it up into bits of 8
    // if size is less than 8 then pad with spaces
    if( strlen( text ) < 8 )
    {
	char new_text[9] ;
	int i = 0 ;
	for( ; i < 8; i++ )
	{
	    if( text[i] == '\0' )
	    {
		break ;
	    }
	    new_text[i] = text[i] ;
	}
	for( ; i < 8; i++ )
	{
	    new_text[i] = ' ' ;
	}
	new_text[i] = '\0' ;
	DODSEncode::my_encode( new_text, key, encoded_text ) ;
	encoded_text[64] = '\0' ;
    }
    else if( strlen( text ) > 8 )
    {
	DODSEncode::encode( text+8, key, encoded_text+64 ) ;
	char new_t[8] ;
	strncpy( new_t, text, 8 ) ;
	new_t[8] = '\0' ;
	DODSEncode::my_encode( new_t, key, encoded_text ) ;
    }
    else
    {
	DODSEncode::my_encode( text, key, encoded_text ) ;
	encoded_text[64] = '\0' ;
    }
}

void
DODSEncode::decode( const char * encoded_text, const char *key,
		    char *decoded_text )
{
    // take encoded_text. take 64 chars at a time and call my_decode with
    // it, contact results together
    string ets = encoded_text ;
    if( strlen( encoded_text ) > 64 )
    {
	DODSEncode::decode( encoded_text+64, key, decoded_text+8 ) ;
	char new_et[64] ;
	strncpy( new_et, encoded_text, 64 ) ;
	new_et[64] = '\0' ;
	DODSEncode::my_decode( new_et, key, decoded_text ) ;
    }
    else if( strlen( encoded_text ) < 64 )
    {
	decoded_text = 0 ;
	return ;
    }
    else
    {
	DODSEncode::my_decode( encoded_text, key, decoded_text ) ;
	decoded_text[8] = '\0' ;
	int i = 7 ;
	while( decoded_text[i] == ' ' )
	{
	    decoded_text[i] = '\0' ;
	    i-- ;
	}
    }
}

string
DODSEncode::encode( const string &text, const string &key )
{
    char b1[2048] ;
    DODSEncode::encode( text.c_str(), key.c_str(), b1 ) ;
    return b1 ;
}

string
DODSEncode::decode( const string &encoded_text, const string &key )
{
    char b2[2048] ;
    DODSEncode::decode( encoded_text.c_str(), key.c_str(), b2 ) ;
    return b2 ;
}

// $Log: DODSEncode.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
