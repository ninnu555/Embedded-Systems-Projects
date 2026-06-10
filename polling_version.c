#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"


////////////////////////////////////////////////////////////////////////////////
////  Base Peripheral Addresses
////////////////////////////////////////////////////////////////////////////////

#define GPIO0_BASE_ADDR     0x40000000  // LEDs
#define GPIO1_BASE_ADDR     0x40010000  // Switches

////////////////////////////////////////////////////////////////////////////////
////  Peripheral Register offsets
////////////////////////////////////////////////////////////////////////////////
// GPIO Register Data Offsets
#define GPIO_DATA_OFFSET    0x000

// --- Pointers to registers ---
// GPIO0 (LEDs) DATA REGISTER
volatile int * GPIO0_DATA   = (volatile int *) (GPIO0_BASE_ADDR + GPIO_DATA_OFFSET); // Data register

// GPIO1 (SWITCHES) DATA REGISTER
volatile int * GPIO1_DATA   = (volatile int *) (GPIO1_BASE_ADDR + GPIO_DATA_OFFSET);


int main(void) {
    init_platform();

    // Point (a): Polling version - driving LEDs via switches 
    run_polling_basic_version();

    cleanup_platform();
    return 0;
}

void run_polling_basic_version (void) {
    while (*GPIO0_DATA == 0){
    // if LEDs are off, enter here     
        while (*GPIO1_DATA != 0) {
        // if switches are not all off, enter here
            *GPIO0_DATA = *GPIO1_DATA;
        }
    *GPIO0_DATA = 0;    // getting infinite pollingloop
}
}