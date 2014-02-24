# Teensy 3.0 Bare Metal Example

An example showing how to blink the LED on the Teensy 3.0 board using only
plain C code.

## What does this code do?

This example project blinks the LED present on pin 13 of the Teensy 3.0 board.
Every half a second the light will cycle between the on and off states.

## Usage

First thing's first; make sure you have the
[arm-none-eabi](https://launchpad.net/gcc-arm-embedded) toolchain and it is
available on your PATH.

1. Run make.
2. Use the Teensy loader program which comes with
[Teensyduino](http://www.pjrc.com/teensy/td_download.html) to upload your code.
3. Reboot the Teensy board.

## Why does this code work?

In blink.c we start with the following.

```
volatile uint32_t systick_millis_count = 0;


static inline uint32_t millis()
{
    volatile uint32_t ret = systick_millis_count;
    return ret;
}
```

Notice that systick_millis_count is declared volatile. This is because the
content of systick_millis_count is modified by the systick interrupt of the
ARM processor. The function which handles this interrupt is located in
mk20dx128.c.

```
extern unsigned long _estack;


extern volatile uint32_t systick_millis_count;

...

void systick_default_isr(void)
{
    ++systick_millis_count;
}

...

void systick_isr(void)        __attribute__ ((weak, alias("systick_default_isr")));
```

That's great, we now know where the function is declared which executes upon a
systick interrupt, but how does the ARM processor know where to find this
function? That's accomplished by using a vector table.

```
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
```

The table above does not contain all the possible interrupts which the
MK20DX128 can fire. It only contains those we need to blink the LED. The
[ARM CMSIS](http://www.arm.com/products/processors/cortex-m/cortex-microcontroller-software-interface-standard.php)
defines these first 16 standard vectors. The reason they're called vectors is
because each one points to a function. The functions are always of type void
and take no arguments.

A special thing to note about the table above are the values of the first and
second elements of the array. The first element; the address of _estack sets
the initial value of the stack pointer (SP register) on the ARM processor.
The second element; reset_isr sets the initial value of the program counter
(PC register) on the ARM processor. In our case, the stack pointer is the end
of our stack because the stack grows from end to start and the program counter
is set to the location of our reset handler function.

Don't worry about the content of the reset_isr function for the time being.
You would need to have read the user guide for your microcontroller before you
would be able to go hacking at this function. I got most of the code for mine
from Teensyduino.

Finally there are the main and delay functions.

```
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
```

Main is always the last thing called in the reset handler (reset_isr in our
example). This is where you put your highest level application logic. A common
practice which Arduino users will be familiar with is subdividing main into
a setup code section and a forever loop section. Main should never be able to
return (it can happen though) and it should continue to execute as long as the
microcontroller has power.

The delay function is a simple blocking delay which shouldn't require any
special explanation.

The way the LED is controlled from main is by manipulating the values of the
registers which control the signal on the same pin the LED connected to. Note
that the following information can be found in the
[Kinetis K20 reference manual](http://cache.freescale.com/files/32bit/doc/ref_manual/K20P64M50SF0RM.pdf?fpsp=1&WT_TYPE=Reference%20Manuals&WT_VENDOR=FREESCALE&WT_FILE_FORMAT=pdf&WT_ASSET=Documentation)
and that the register macros (i.e. PORTC_PCR5) come from the Teensy 3.0 header
file mk20dx128.h.

The first step is to locate where pin 13 on the Teensy board is connected on
the K20 microcontroller. Pin 13 on the Teensy is port C, pin 5 on the
microcontroller. Looking at the PORT section of the documentation, each port
and pin combo has a control register associated with it. It also says that a
pin can be put into GPIO mode by setting just bit 8 to 1.

```
PORTC_PCR5 = (uint32_t)(1 << 8);
```

Great, the pin is in the right mode of operation now. Next we need to set the
direction data will flow in on this pin. In the GPIO section of the
documentation we see there is a register to set just for this purpose and that
a value of 0 on a bit means that pin is set for input while a value of 1 means
output. We want to set port C, pin 5 to output so we get the following.

```
GPIOC_PDDR |= (uint32_t)(1 << 5);
```

Next we need to explicitly allow the bits to be set in the data output register
for our port and pin. This is accomplished by setting the corresponding bit
to 1 in the set output register.

```
GPIOC_PSOR |= (uint32_t)(1 << 5);
```

Awesome, the correct port and pin combo is now set for output. In the main loop
we use a special toggle register to finally manipulate the output signal on
our LED's pin. We could have used an XOR operation to toggle the state of the
pin on the pin data output register (PDOR) but since the pin toggle output
register (PTOR) is a builtin feature of our microcontroller, it is wiser to
make good use of it.

```
GPIOC_PTOR |= (uint32_t)(1 << 5);
```

At last, the LED will now be blinking at the rate specified in the delay
function call.

That's all for now, hopefully I was able to teach you something useful about
Cortex ARM startup code in all of the text above.
