FILE* stream_open(char *name, char *mode);

#ifdef IS_WIN32
void reap_threads();
#endif
