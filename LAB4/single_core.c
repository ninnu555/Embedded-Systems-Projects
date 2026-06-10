#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "shared.h"
#include "xil_io.h"

u32  t_start, t_end;

void vec_add_serial(u32 * restrict c,const u32 * restrict a,const u32 * restrict b,int n)
{
    for (int i = 0; i < n; ++i) {
        c[i] = a[i] + b[i];
    }

}

int main(void)
{
    // Disable caches to avoid coherency issues for this simple exercise.
 Xil_ICacheDisable();
    Xil_DCacheDisable();
    int i;

    while (Xil_In32(XPAR_AXI_GPIO_2_BASEADDR)==0);//wait for the start on button


    xil_printf("Core 0: Single-Core execution (fixed shared base)\r\n");

    // Initialize data in shared memory
    for (i = 0; i < ARRAY_SIZE; ++i) {
        SHARED->A[i] = i;
        SHARED->B[i] = 2 * i;
    }

    xil_printf("Core 0: Data initialized\r\n");
    t_start=Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);


    // process
    // for (i = 0; i < ARRAY_SIZE; ++i) {
    //    SHARED->C[i] = SHARED->A[i] + SHARED->B[i];
    // }
    vec_add_serial((u32 * restrict)SHARED->C, (const u32 * restrict)SHARED->A, (const u32 * restrict)SHARED->B, (int)ARRAY_SIZE);

    t_end=Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
    xil_printf("Core 0: Finished!\r\n");

    // Check correctness
    int errors = 0;
    for (i = 0; i < ARRAY_SIZE; ++i) {
        u32 expected = SHARED->A[i] + SHARED->B[i];
        if (SHARED->C[i] != expected) {
            errors++;
            if (errors < 10) {
                xil_printf("Mismatch at %d: got %lu, expected %lu\r\n",
                           i,
                           (unsigned long)SHARED->C[i],
                           (unsigned long)expected);
                int idx = i;
                xil_printf("Debug at %d: A=%lu, B=%lu, C=%lu, expected=%lu\r\n",
                           idx,
                           (unsigned long)SHARED->A[idx],
                           (unsigned long)SHARED->B[idx],
                           (unsigned long)SHARED->C[idx],
                           (unsigned long)(SHARED->A[idx] + SHARED->B[idx]));
            }
        }
    }


    xil_printf("Core 0: FATTO. Errors = %d, start = %x, end = %x, duration = %x\r\n",
               errors, t_start, t_end, t_end - t_start );

    while (1) {
        // Idle loop
    }

    return 0;
}
