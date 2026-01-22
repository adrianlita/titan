#include <titan.h>
#include <stdint.h>
#include "bsp.h"
#include <periph/rng.h>
#include <utils/rand.h>


void titan_setup(void) {
/*
    Note1:
    - this functions runs *before* the main()
    - whenever this function *returns*, main() will start
    - this function runs in the same CONTEXT as main task
    


    What can (should) you do here ?
    - firmware update, if pending
    - initialize random seed, if rand is used
    - setup global policies
    - setup GPIO, and drivers needed before running the actual main()
    - setup RTC and timing
    - setup subsystems:
        - filesystem: setup and initialize external memories and sync external data
        - networking: if networking is used, this is the best place to set it up
    - create and launch other tasks

    Useful functions here:
        - titan_get_boot_reason()
*/

    uint32_t new_seed[4];
    rng_init();
    rng_random(new_seed, 4);
    rng_random(new_seed, 4);
    rng_random(new_seed, 4);   //get some randomness
    rng_deinit();
    srand(new_seed);    //initialize rand() seed
}
