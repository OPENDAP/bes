
// A simple model for the Coverity static checker
// Update this and load it into Coverity to prevent false
// positives due to code that's unusual - such as special
// macros or functions that free memory (so delete or free
// is not called when Coverity expects it to be).
// 
// See https://scan.coverity.com/models#overriding for more
// info about models

void ff_destroy_std_args(void* x) {
    __coverity_free__(x);
}

// I'm not sure why coverity doesn't pick this up, but make sure
// that the _throw5 function is recognized as throwing an exception.
// It may be that coverity does not handle template functions so
// well...
template < typename T, typename U, typename V, typename W, typename X >
void _throw5 (const char *fname, int line, int numarg, const T & a1, const U & a2, const V & a3, const W & a4,
    const X & a5)
{
    __coverity_panic__();
}

void  throw3 (const char *, const char *, int)
{
    __coverity_panic__();
}

namespace libdap {

class D4Attribute;

// This model was intended to indicate that 'x' will not be leaked
// memory because the D4Attributes dtor will clean it up. However,
// this model confuses the scanner into thinking the code deletes
// the pointer, so any code that uses 'x' after this call will generate
// a High Priority issue. jhrg 11/16/17
#if 0
class D4Attributes {
    void add_attribute_nocopy(D4Attribute *x)
    {
        __coverity_delete__(x);
    }
};
#endif

}
