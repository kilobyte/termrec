#ifdef WIN32
# define export __declspec(dllexport)
#else
#ifdef GCC_VISIBILITY
# define export __attribute__ ((visibility ("default")))
# pragma GCC visibility push(hidden)
#else
# define export // we'll leak symbols...
#endif
#endif
