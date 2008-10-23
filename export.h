#ifdef WIN32
# define export __declspec(dllexport)
#else
# define export __attribute__ ((visibility ("default")))
# pragma GCC visibility push(hidden)
#endif
