#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_PLATFORM_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_PLATFORM_H



    #if defined(_WIN32) || defined(_WIN64)
  #define VITTEC_PLATFORM_WINDOWS 1
#else
  #define VITTEC_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__)
  #define VITTEC_PLATFORM_DARWIN 1
#else
  #define VITTEC_PLATFORM_DARWIN 0
#endif

#if defined(__linux__)
  #define VITTEC_PLATFORM_LINUX 1
#else
  #define VITTEC_PLATFORM_LINUX 0
#endif

#if defined(__FreeBSD__)
  #define VITTEC_PLATFORM_FREEBSD 1
#else
  #define VITTEC_PLATFORM_FREEBSD 0
#endif

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_PLATFORM_H */
