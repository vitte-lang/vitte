#ifndef VITTE_SDK_SYS_SYS_H
#define VITTE_SDK_SYS_SYS_H




/* Platform layer stubs for SDK consumers. */
#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_SYS_WINDOWS 1
#else
  #define VITTE_SYS_WINDOWS 0
#endif

#if defined(__APPLE__)
  #define VITTE_SYS_DARWIN 1
#else
  #define VITTE_SYS_DARWIN 0
#endif

#if defined(__linux__)
  #define VITTE_SYS_LINUX 1
#else
  #define VITTE_SYS_LINUX 0
#endif

#if defined(__FreeBSD__)
  #define VITTE_SYS_FREEBSD 1
#else
  #define VITTE_SYS_FREEBSD 0
#endif


#endif /* VITTE_SDK_SYS_SYS_H */
