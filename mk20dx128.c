#include "mk20dx128.h"


extern unsigned long _stext;
extern unsigned long _etext;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;


extern volatile uint32_t systick_millis_count;


extern int main(void);


void fault_isr(void)
{
    while (1);
}


void unused_isr(void)
{
    fault_isr();
}


void systick_default_isr(void)
{
    ++systick_millis_count;
}


void reset_isr(void);
void nmi_isr(void)            __attribute__ ((weak, alias("unused_isr")));
void hardfault_isr(void)      __attribute__ ((weak, alias("unused_isr")));
void memmanagement_isr(void)  __attribute__ ((weak, alias("unused_isr")));
void busfault_isr(void)       __attribute__ ((weak, alias("unused_isr")));
void usagefault_isr(void)     __attribute__ ((weak, alias("unused_isr")));
void svc_isr(void)            __attribute__ ((weak, alias("unused_isr")));
void debugmon_isr(void)       __attribute__ ((weak, alias("unused_isr")));
void pendsv_isr(void)         __attribute__ ((weak, alias("unused_isr")));
void systick_isr(void)        __attribute__ ((weak, alias("systick_default_isr")));


__attribute__ ((section(".vectors"), used))
void (* const _isr_functions[])(void) = {
    (void *)&_estack,      /**< 0 ARM: Initial stack pointer */
    reset_isr,             /**< 1 ARM: Initial program counter */
    nmi_isr,               /**< 2 ARM: Non-Maskable Interrupt (NMI) */
    hardfault_isr,         /**< 3 ARM: Hard fault */
    memmanagement_isr,     /**< 4 ARM: Mem management fault */
    busfault_isr,          /**< 5 ARM: Bus fault */
    usagefault_isr,        /**< 6 ARM: Usage fault */
    fault_isr,             /**< 7 -- */
    fault_isr,             /**< 8 -- */
    fault_isr,             /**< 9 -- */
    fault_isr,             /**< 10 -- */
    svc_isr,               /**< 11 ARM: Supervisor call */
    debugmon_isr,          /**< 12 ARM: Debug monitor */
    fault_isr,             /**< 13 -- */
    pendsv_isr,            /**< 14 ARM: Pendable service request */
    systick_isr            /**< 15 ARM: System tick timer */
};


__attribute__ ((section(".flashconfig"), used))
const uint8_t flashconfig_bytes[16] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF
};


__attribute__ ((section(".startup")))
void reset_isr()
{
    uint32_t *src = &_etext;
    uint32_t *dest;
    unsigned int i;

    /* Initialize (disable?) the watchdog */
    WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
    WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
    WDOG_STCTRLH = WDOG_STCTRLH_ALLOWUPDATE;

    /* Clocks active to all GPIO */
    SIM_SCGC5 = 0x00043F82;
    SIM_SCGC6 = SIM_SCGC6_RTC
        | SIM_SCGC6_FTM0
        | SIM_SCGC6_FTM1
        | SIM_SCGC6_ADC0
        | SIM_SCGC6_FTFL;

    /* If the RTC oscillator isn't enabled, get it started early */
    if (!(RTC_CR & RTC_CR_OSCE)) {
	    RTC_SR = 0;
	    RTC_CR = RTC_CR_SC16P | RTC_CR_SC4P | RTC_CR_OSCE;
    }

    /* Release I/O pins hold, if we woke up from VLLS mode */
    if (PMC_REGSC & PMC_REGSC_ACKISO) PMC_REGSC |= PMC_REGSC_ACKISO;

    /* Load the text section into RAM */
    dest = &_sdata;
    while (dest < &_edata) *dest++ = *src++;

    /* Zero-initialize the bss section */
    dest = &_sbss;
    while (dest < &_ebss) *dest++ = 0;

    /* Use vector table in flash */
    SCB_VTOR = 0;

    // Default all interrupts to medium priority level
    for (i = 0; i < NVIC_NUM_INTERRUPTS; ++i) NVIC_SET_PRIORITY(i, 128);

    /* Start in FEI mode
     * Enable capacitors for crystal
     */
    OSC0_CR = OSC_SC8P | OSC_SC2P;
    
    /* Enable osc, 8-32 MHz range, low power mode */
    MCG_C2 = MCG_C2_RANGE0(2) | MCG_C2_EREFS;
    
    /* Switch to crystal as clock source, FLL input = 16 MHz / 512 */
    MCG_C1 =  MCG_C1_CLKS(2) | MCG_C1_FRDIV(4);
    
    /* Wait for crystal oscillator to begin */
    while ((MCG_S & MCG_S_OSCINIT0) == 0);
    
    /* Wait for FLL to use oscillator */
    while ((MCG_S & MCG_S_IREFST) != 0);
    
    /* Wait for MCGOUT to use oscillator */
    while ((MCG_S & MCG_S_CLKST_MASK) != MCG_S_CLKST(2));
    
    /* Now we're in FBE mode
     * Config PLL input for 16 MHz Crystal / 4 = 4 MHz
     */
    MCG_C5 = MCG_C5_PRDIV0(3);
    
    /* Config PLL for 96 MHz output */
    MCG_C6 = MCG_C6_PLLS | MCG_C6_VDIV0(0);
    
    /* Wait for PLL to start using xtal as its input */
    while (!(MCG_S & MCG_S_PLLST));
    
    /* Wait for PLL to lock */
    while (!(MCG_S & MCG_S_LOCK0));
    
    /* Now we're in PBE mode */
#if F_CPU == 96000000
    /* Config divisors: 96 MHz core, 48 MHz bus, 24 MHz flash */
    SIM_CLKDIV1 = SIM_CLKDIV1_OUTDIV1(0)
        | SIM_CLKDIV1_OUTDIV2(1)
        | SIM_CLKDIV1_OUTDIV4(3);
#elif F_CPU == 48000000
    /* Config divisors: 48 MHz core, 48 MHz bus, 24 MHz flash */
    SIM_CLKDIV1 = SIM_CLKDIV1_OUTDIV1(1)
        | SIM_CLKDIV1_OUTDIV2(1)
        | SIM_CLKDIV1_OUTDIV4(3);
#elif F_CPU == 24000000
    /* config divisors: 24 MHz core, 24 MHz bus, 24 MHz flash */
    SIM_CLKDIV1 = SIM_CLKDIV1_OUTDIV1(3)
        | SIM_CLKDIV1_OUTDIV2(3)
        | SIM_CLKDIV1_OUTDIV4(3);
#else
#error "Error, F_CPU must be 96000000, 48000000, or 24000000"
#endif
    
    /* Switch to PLL as clock source, FLL input = 16 MHz / 512 */
    MCG_C1 = MCG_C1_CLKS(0) | MCG_C1_FRDIV(4);
    
    /* Wait for PLL clock to be used */
    while ((MCG_S & MCG_S_CLKST_MASK) != MCG_S_CLKST(3));
    
    /* Now we're in PEE mode
     * Configure USB for 48 MHz clock
     */
    SIM_CLKDIV2 = SIM_CLKDIV2_USBDIV(1);  /* USB = 96 MHz PLL / 2 */
    
    /* USB uses PLL clock, trace is CPU clock, CLKOUT=OSCERCLK0 */
    SIM_SOPT2 = SIM_SOPT2_USBSRC
        | SIM_SOPT2_PLLFLLSEL
        | SIM_SOPT2_TRACECLKSEL
        | SIM_SOPT2_CLKOUTSEL(6);

    /* Initialize the SysTick counter */
    SYST_RVR = (F_CPU / 1000) - 1;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
    
    __enable_irq();
    
    main();
    while (1);
}
