    #include <stdio.h>
    #include "platform.h"
    #include "xil_printf.h"


    ////////////////////////////////////////////////////////////////////////////////
    ////  Base Addresses
    ////////////////////////////////////////////////////////////////////////////////

    #define GPIO0_BASE_ADDR     0x40000000  // LEDs
    #define GPIO1_BASE_ADDR     0x40010000  // Switches
    #define GPIO2_BASE_ADDR     0x40020000  // Buttons
    #define INTC_BASE_ADDR      0x41200000  // Interrupt Controller

    ////////////////////////////////////////////////////////////////////////////////
    ////  Registers' offests
    ////////////////////////////////////////////////////////////////////////////////
    // GPIO Register's Offsets
    #define GPIO_DATA_OFFSET    0x000
    #define GPIO_TRI_OFFSET     0x004
    #define GPIO_GIER_OFFSET    0x11C
    #define GPIO_IER_OFFSET     0x128
    #define GPIO_ISR_OFFSET     0x120

    // Interrupt Controller Register's Offsets
    #define INTC_ISR_OFFSET     0x00
    #define INTC_IER_OFFSET     0x08
    #define INTC_IAR_OFFSET     0x0C
    #define INTC_MER_OFFSET     0x1C

    // --- Pointers to registers ---
    // GPIO0 (LEDs) REGISTERS
    volatile int * GPIO0_DATA   = (volatile int *) (GPIO0_BASE_ADDR + GPIO_DATA_OFFSET); // Data register
    volatile int * GPIO0_TRI    = (volatile int *) (GPIO0_BASE_ADDR + GPIO_TRI_OFFSET ); // Tri-state (direction) register
    volatile int * GPIO0_GIER   = (volatile int *) (GPIO0_BASE_ADDR + GPIO_GIER_OFFSET); // Global Interrupt Enable Register
    volatile int * GPIO0_IER    = (volatile int *) (GPIO0_BASE_ADDR + GPIO_IER_OFFSET ); // Interrupt Enable Register
    volatile int * GPIO0_ISR    = (volatile int *) (GPIO0_BASE_ADDR + GPIO_ISR_OFFSET ); // Interrupt Status Register
    // GPIO1 (SWITCHES) REGISTERS
    volatile int * GPIO1_DATA   = (volatile int *) (GPIO1_BASE_ADDR + GPIO_DATA_OFFSET);
    volatile int * GPIO1_TRI    = (volatile int *) (GPIO1_BASE_ADDR + GPIO_TRI_OFFSET );
    volatile int * GPIO1_GIER   = (volatile int *) (GPIO1_BASE_ADDR + GPIO_GIER_OFFSET);
    volatile int * GPIO1_IER    = (volatile int *) (GPIO1_BASE_ADDR + GPIO_IER_OFFSET );
    volatile int * GPIO1_ISR    = (volatile int *) (GPIO1_BASE_ADDR + GPIO_ISR_OFFSET );
    // GPIO2 (BUTTONS) REGISTERS
    volatile int * GPIO2_DATA   = (volatile int *) (GPIO2_BASE_ADDR + GPIO_DATA_OFFSET);
    volatile int * GPIO2_TRI    = (volatile int *) (GPIO2_BASE_ADDR + GPIO_TRI_OFFSET );
    volatile int * GPIO2_GIER   = (volatile int *) (GPIO2_BASE_ADDR + GPIO_GIER_OFFSET);
    volatile int * GPIO2_IER    = (volatile int *) (GPIO2_BASE_ADDR + GPIO_IER_OFFSET );
    volatile int * GPIO2_ISR    = (volatile int *) (GPIO2_BASE_ADDR + GPIO_ISR_OFFSET );
    // INTERRUPT CONTROLLER REGISTERS
    volatile int * INTC_ISR     = (volatile int *) (INTC_BASE_ADDR + INTC_ISR_OFFSET); // Interrupt Status Register
    volatile int * INTC_IER     = (volatile int *) (INTC_BASE_ADDR + INTC_IER_OFFSET); // Interrupt Enable Register
    volatile int * INTC_IAR     = (volatile int *) (INTC_BASE_ADDR + INTC_IAR_OFFSET); // Interrupt Acknowledge Register
    volatile int * INTC_MER     = (volatile int *) (INTC_BASE_ADDR + INTC_MER_OFFSET); // Master Enable Register

    ////////////////////////////////////////////////////////////////////////////////
    ////  Function Prototype
    ////////////////////////////////////////////////////////////////////////////////
    void my_ISR (void) __attribute__ ((interrupt_handler));	// once an interrupt is detected, uB goes to the ISR (named myISR)

    int neg_flag = 0;
    int matricola_last_figure = 0;  // 70/83/65490


    int main(void) {
        init_platform();
        
        // Point (b): Interrupt version - driving LEDs via switches and buttons
        initialize_gpio_intc_interrupts();

        while (1) {

        }
        
        cleanup_platform();
        return 0;
    }

    void initialize_gpio_intc_interrupts (void) {

        // clearing stale pendings
        // *INTC_IER = 0;
        // *INTC_MER = 0;
        // *GPIO1_ISR = 0xFFFFFFFF;
        // *GPIO2_ISR = 0xFFFFFFFF;
        // *INTC_IAR = 0xFFFFFFFF;
        // THESE ABOVE TO BE CHECKED IF NEEDED

        // setting peripherals as inputs/outputs
        // *GPIO0_TRI = 0x0;		        // LEDs set as output
        *GPIO1_TRI = 0xFFFFFFFF;	    // switches set as input
        *GPIO2_TRI = 0xFFFFFFFF;	    // buttons set as input

        // globally enabling GPIOs' interrupts
        *GPIO1_GIER = (1 << 31);
        *GPIO2_GIER = (1 << 31);

        // enabling GPIO's channels
        *GPIO1_IER 	= 0b1; 	            // enabling GPIO1's channel 1
        *GPIO2_IER 	= 0b1; 	            // enabling GPIO2's channel 1

        // enabling Interrupt Controller's interrupts
        *INTC_IER = 0b11;				// enabling Interrupt Enable Register (ch2 (for GPIO2's interrupt) and ch1 (for GPIO1's interupt))
        *INTC_MER = 0b11;				// enabling Master IRQ Enable and Hardware Interrupt Enable

        microblaze_enable_interrupts(); // enabling interrupts on the microblaze processor
    }

void my_ISR (void){
    // This function is called when an interrupt is detected.
    // One pression of the button will negate the state.
    // The consequent release doesn't do anything.
    // Another pression will negate again the state (that means restoring the normal state).
    // The consequent release doesn't do anything.

    if (*INTC_ISR & 0x1) {              // Switches interrupt (GPIO1) - sensitive to any change
        // Switches interrupt detected
        print("switch cliccato!\n\r");
        *GPIO1_ISR  = 0x1;              // reset GPIO1 interrupt flag in GPIO1
        *INTC_IAR   = 0x1;	            // reset INTC interrupt flag in INTC with IAR (Toggle on write)
        
        if(neg_flag == 0) {
            // Normal mode: shows the binary value of the switches
            *GPIO0_DATA = get_msb_bin_index_switches(GPIO1_DATA);
            print("switch NON invertiti!\n\r");
        }   
        else {
            // Neg mode: shows the inverted binary value of the switches + matricola last figure
            *GPIO0_DATA = ~(matricola_last_figure + get_msb_bin_index_switches(GPIO1_DATA));
            print("switch invertiti con matricola aggiunta!\n\r");
        }
    }

    if ((*INTC_ISR & 0x2) && (*GPIO2_DATA & 0x1)) {         // Button interrupt (GPIO2) - only pression sensitive
        // Button pression detected!
        int DELAY_COUNT = 1000000;
        for (long long i = 0; i < DELAY_COUNT; i++) {
            // busy-wait in order to debounce the button pression
        }
        print("bottone premuto!\n\r");
        *INTC_IAR   = 0x2;
        *GPIO2_ISR  = 0x1;
        neg_flag = !neg_flag;   // every button pression will change the state of this variable 
        if (neg_flag == 0) {
            // Activating Normal mode
            *GPIO0_DATA = get_msb_bin_index_switches(GPIO1_DATA);
            print("modalità normale!\n\r");
        }
        else {
            // Activating Neg mode
            *GPIO0_DATA = ~(matricola_last_figure + get_msb_bin_index_switches(GPIO1_DATA));
            print("inversione bit con matricola aggiunta!\n\r");
        }

    }

    else if (*INTC_ISR & 0x2) {         // Button interrupt (GPIO2) interrupt - release case
        // Button release - no action on release, just clear interrupt flags
        *INTC_IAR   = 0x2;
        *GPIO2_ISR  = 0x1;
    }
}

    int get_msb_bin_index_switches (int *switches_state) {
        // Gets the index of the most significant bit set in the switches' state
        int sw_state = *switches_state;
        for(int i=3; i>=0; i--) {
            if((sw_state & 0b1000) == 0b1000) {
                return i;
            }
            else {
                sw_state <<= 1;
            }
            }
        return 0;   // necessary to prevent ruturning undesired huge values

        }

