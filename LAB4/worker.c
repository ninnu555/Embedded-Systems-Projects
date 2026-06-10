#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "shared.h"
#include "xil_io.h"


void vec_add_serial(u32 * restrict c,const u32 * restrict a,const u32 * restrict b,int n)
{
    for (int i = 0; i < n; ++i) {
        c[i] = a[i] + b[i];
    }
}

int main(void)
{
    int i;

    // Disable caches for this exercise (same reason as core 0).
    Xil_ICacheDisable();
    Xil_DCacheDisable();

    xil_printf("Core 1: Waiting for start signal...\r\n");

    while (Xil_In32(XPAR_AXI_GPIO_2_BASEADDR)==0);//wait for the start on button



















    // Wait until core 0 sets start flag
    while (SHARED->start == 0) {
        // busy-wait
    }

//    xil_printf("Core 1: Starting second half computation\r\n");

    // Process second half of the array
    //for (i = ARRAY_SIZE / 2; i < ARRAY_SIZE; ++i) {
    //    SHARED->C[i] = SHARED->A[i] + SHARED->B[i];
    //}

    // SIMD processing
    vec_add_serial((u32 * restrict)SHARED->C+ARRAY_SIZE/2, (const u32 * restrict)SHARED->A+ARRAY_SIZE/2, (const u32 * restrict)SHARED->B+ARRAY_SIZE/2, (int)ARRAY_SIZE/2);

    SHARED->done1 = 1;

//    xil_printf("Core 1: Done.\r\n");

    while (1) {
        // Idle loop
    }

    return 0;
}
