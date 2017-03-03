
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define LIBMEMCACHED_WITH_SASL_SUPPORT 0
#define LIBMEMCACHED_VERSION_STRING "1.0.18"
#define LIBMEMCACHED_VERSION_HEX 0x01000018
#define BUILDING_LIBMEMCACHED

#ifdef __cplusplus
}
#endif

#include <BaseTsd.h>

typedef int pid_t;
typedef SSIZE_T ssize_t;
#define or ||
#define and &&
#define not !

#  include <ws2tcpip.h>
#  include <errno.h>
#  ifndef EINPROGRESS
#    define EINPROGRESS WSAEINPROGRESS
#  endif
#  ifndef EALREADY
#    define EALREADY    WSAEALREADY
#  endif
#  ifndef EISCONN
#    define EISCONN     WSAEISCONN
#  endif
#  ifndef ENOBUFS
#    define ENOBUFS     WSAENOBUFS
#  endif
#  ifndef SHUT_RDWR
#    define SHUT_RDWR SD_BOTH
#  endif
#  ifndef SHUT_RD
#    define SHUT_RD SD_RECEIVE
#  endif
#  ifndef SHUT_WR
#    define SHUT_WR SD_SEND
#  endif
#  define srandom(x) srand(x)
#  define random() rand()
#  define index(s, c) strchr(s, c)
#  define __attribute__(x)
#  define inline
#  define strtoll _strtoi64
#  define strtoull _strtoui64
#  define snprintf _snprintf

#define YY_NO_UNISTD_H

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval * tp, struct timezone * tzp);

#ifdef __cplusplus
}
#endif
