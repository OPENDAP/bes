
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

#if 0
Structure *d4s;
D4Attribute *x = new D4Attribute("HDF5_HARDLINK",attr_str_c);
d4s->attributes()->add_attribute_nocopy(x);
//Should be modeled as __coverity_delete__(x)
#endif

libdap::D4Attributes::add_attribute_nocopy(x)
{
    __coverity_delete__(x);
}
// I'm not sure why coverity doesn't pick this up, but make sure
// that the _throw5 function is recognized as throwing an exception.
// It may be that coverity does not handle template functions so
// well...
template < typename T, typename U, typename V, typename W, typename X > static void
_throw5 (const char *fname, int line, int numarg,
         const T & a1, const U & a2, const V & a3, const W & a4, const X & a5)
{
__coverity_panic__;
}
