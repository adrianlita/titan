#include <assert.h>
#include <stdint.h>


#if (TITAN_ASSERT_LEVEL == 1)
__WEAK void titan_assert() {
    while(1) { }
}
#elif (TITAN_ASSERT_LEVEL == 2)
__WEAK void titan_assert(uint32_t location) {
    (void)location;
    
    while(1) { }
}
#elif (TITAN_ASSERT_LEVEL >= 3)
__WEAK void titan_assert(char const * const file, uint32_t location) {
    (void)file;
    (void)location;
    
    while(1) { }
}
#endif
