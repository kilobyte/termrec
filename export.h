#ifdef WIN32
# define export __declspec(dllexport)
# define VISIBILITY_ENABLE
# define VISIBILITY_DISABLE
#else
#ifdef GCC_VISIBILITY
# define export __attribute__ ((visibility ("default")))
# pragma GCC visibility push(hidden)
# define VISIBILITY_ENABLE  _Pragma("GCC visibility push(default)")
# define VISIBILITY_DISABLE _Pragma("GCC visibility push(hidden)")
#else
# define export // we'll leak symbols...
# define VISIBILITY_ENABLE
# define VISIBILITY_DISABLE
#endif
#endif
