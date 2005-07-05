// DODSAutoPtr.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODSAutoPtr_h_
#define DODSAutoPtr_h_ 1

template <class T>
class DODSAutoPtr
{
private:
    T* p;
    bool _is_vector;

    // disable copy constructor.
    template <class U> DODSAutoPtr(DODSAutoPtr<U> &){};

    // disable overloaded = operator.
    template <class U> DODSAutoPtr<T>& operator= (DODSAutoPtr<U> &){ return *this ; }

public:
    explicit DODSAutoPtr(T* pointed=0, bool v=false) ;
    ~DODSAutoPtr() ;

    T* set(T *pointed, bool v=false) ;
    T* get() const ;
    T* operator ->() const ;
    T& operator *() const ;
    T* release() ;
    void reset() ;
};

#endif // DODSAutoPtr_h_

// $Log: DODSAutoPtr.h,v $
// Revision 1.3  2005/02/09 19:43:47  pwest
// compiler warning removed
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
