#ifndef UDP_COMMONS_DEBUG_H_
#define UDP_COMMONS_DEBUG_H_


#if defined(__cplusplus)
extern "C"
#else
extern
#endif
void RaiseHardbreak(const char* file, int line, int counter);


#if defined(__cplusplus)
extern "C"
#else
extern
#endif
void RaiseSoftbreak();


#endif//UDP_COMMONS_DEBUG_H_
