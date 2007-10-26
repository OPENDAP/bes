// SampleSayCommand.h

#ifndef A_SampleSayCommand_h
#define A_SampleSayCommand_h 1

#include "BESCommand.h"

class SampleSayCommand : public BESCommand
{
private:
protected:
public:
    					SampleSayCommand( const string &cmd )
					    : BESCommand( cmd ) {}
    virtual				~SampleSayCommand() {}

    virtual BESResponseHandler *	parse_request( BESTokenizer &tokens,
					               BESDataHandlerInterface &dhi ) ;

    virtual void			dump( ostream &strm ) const ;
} ;

#endif // A_SampleSayCommand_h

