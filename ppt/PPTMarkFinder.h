// PPTMarkFinder.h

class PPTMarkFinder
{
private:
    int				_markIndex ;
    int				_markLength ;
    unsigned char		_mark[64] ;

public:
    				PPTMarkFinder( unsigned char *mark,
				            int markLength ) ;

    int				getMarkIndex() { return _markIndex ; }

    bool			markCheck( unsigned char b ) ;
} ;

