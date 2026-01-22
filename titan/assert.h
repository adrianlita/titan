#pragma once

#include <stdint.h>

#ifndef TITAN_ASSERT_LEVEL
    #define TITAN_ASSERT_LEVEL      0xFF
#endif

#ifndef TITAN_SASSERT_ENABLE
    #define TITAN_SASSERT_ENABLE    0x01
#endif

#ifndef TITAN_KASSERT_ENABLE
    #define TITAN_KASSERT_ENABLE    0x01
#endif

#ifndef TITAN_PASSERT_ENABLE
    #define TITAN_PASSERT_ENABLE    0x01
#endif

#define STATIC_ASSERT(condition) ((void)sizeof(char[1 - 2*!!(condition)]))  //static assert https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/

#if (TITAN_ASSERT_LEVEL == 0)
    #define TITAN_DEBUG_FILE_MARK
    #define assert(condition)             ((void)0)
#elif (TITAN_ASSERT_LEVEL == 1)
    #define TITAN_DEBUG_FILE_MARK
    #define assert(condition) ((condition) ? (void)0 : titan_assert())
void titan_assert(void);
#elif (TITAN_ASSERT_LEVEL == 2)
    #define TITAN_DEBUG_FILE_MARK
    #define assert(condition) ((condition) ? (void)0 : titan_assert((uint32_t)__LINE__))
void titan_assert(uint32_t location);
#elif (TITAN_ASSERT_LEVEL >= 3)
    #define TITAN_DEBUG_FILE_MARK static char const __titan_assert_file_[] = __FILE__
    #define assert(condition) ((condition) ? (void)0 : titan_assert(__titan_assert_file_, (uint32_t)__LINE__))
void titan_assert(char const * const file, uint32_t location);
#endif

#if (TITAN_SASSERT_ENABLE == 0)
    #define TITAN_SDEBUG_FILE_MARK
    #define sassert(condition)             ((void)0)
#else
    #define TITAN_SDEBUG_FILE_MARK         TITAN_DEBUG_FILE_MARK
    #define sassert(condition)             assert(condition)
#endif

#if (TITAN_KASSERT_ENABLE == 0)
    #define TITAN_KDEBUG_FILE_MARK
    #define kassert(condition)             ((void)0)
#else
    #define TITAN_KDEBUG_FILE_MARK         TITAN_DEBUG_FILE_MARK
    #define kassert(condition)             assert(condition)
#endif

#if ((TITAN_SASSERT_ENABLE == 0) && (TITAN_KASSERT_ENABLE == 0))
    #define TITAN_SKDEBUG_FILE_MARK
#else
    #define TITAN_SKDEBUG_FILE_MARK        TITAN_DEBUG_FILE_MARK
#endif

#if (TITAN_PASSERT_ENABLE == 0)
    #define TITAN_PDEBUG_FILE_MARK
    #define passert(condition)             ((void)0)
#else
    #define TITAN_PDEBUG_FILE_MARK         TITAN_DEBUG_FILE_MARK
    #define passert(condition)             assert(condition)
#endif
