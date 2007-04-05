// PPTMarkFinder.h

#ifndef PPTMarkFinder_h_
#define PPTMarkFinder_h_ 1

#define PPTMarkFinder_Buffer_Size 64

class PPTMarkFinder
{
private:
    int				_markIndex ;
    int				_markLength ;
    unsigned char		_mark[PPTMarkFinder_Buffer_Size] ;

public:
    				PPTMarkFinder( unsigned char *mark,
				            int markLength ) ;

    int				getMarkIndex() { return _markIndex ; }

    bool			markCheck( unsigned char b ) ;
} ;

#endif // PPTMarkFinder_h_
