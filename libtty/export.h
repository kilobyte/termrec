#ifdef WIN32
# define public __declspec(dllexport)
#else
# define public __attribute__ ((visibility ("default")))
# pragma GCC visibility push(hidden)
#endif
