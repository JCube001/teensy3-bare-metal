#include <stdint.h>

#include "mk20dx128.h"

#ifdef __cplusplus
extern "C" {
#endif


volatile uint32_t systick_millis_count = 0;


static inline uint32_t millis()
{
    volatile uint32_t ret = systick_millis_count;
    return ret;
}


void delay(uint32_t msec)
{
    uint32_t start = millis();
    while ((millis() - start) < msec);
}


int main(void)
{
    /* Setup
     * Set Teensy pin 13 as output
     */
    PORTC_PCR5 = (uint32_t)(1 << 8);
    GPIOC_PDDR |= (uint32_t)(1 << 5);
    GPIOC_PSOR |= (uint32_t)(1 << 5);
    
    /* Main loop */
    while (1)
    {
        GPIOC_PTOR |= (uint32_t)(1 << 5);
        delay(500);
    }
    
    return 0;
}


#ifdef __cplusplus
}
#endif
