
#pragma once

#if defined(_MSC_VER)
#include <stdint.h>
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define or ||
#define and &&
#define not !
#endif

#define DEBUG 0
#define HAVE_HSIEH_HASH 1
