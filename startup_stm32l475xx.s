.syntax unified
.cpu cortex-m4
.thumb

.global g_pfnVectors
.global Reset_Handler
.global Default_Handler

.extern main
.extern _estack
.extern SysTick_Handler
.extern _sbss
.extern _ebss
.extern _sdata
.extern _edata
.extern _sidata

.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word Default_Handler          /* NMI */
    .word Default_Handler          /* HardFault */
    .word Default_Handler          /* MemManage */
    .word Default_Handler          /* BusFault */
    .word Default_Handler          /* UsageFault */
    .word 0
    .word 0
    .word 0
    .word 0
    .word Default_Handler          /* SVCall */
    .word Default_Handler          /* Debug Monitor */
    .word 0
    .word Default_Handler          /* PendSV */
    .word SysTick_Handler          /* SysTick */
.size g_pfnVectors, .-g_pfnVectors

.section .text.Reset_Handler,"ax",%progbits
.thumb_func
.type Reset_Handler, %function
Reset_Handler:
    ldr sp, =_estack

    /* Copy .data from flash to RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
1:
    cmp r0, r1
    bcc 2f
    b 3f
2:
    ldr r3, [r2], #4
    str r3, [r0], #4
    b 1b

    /* Zero .bss — needed so number/timer = 0 after reset (SRAM survives NRST) */
3:
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
4:
    cmp r0, r1
    bcc 5f
    b 6f
5:
    str r2, [r0], #4
    b 4b

6:
    bl main
7:  b 7b
.size Reset_Handler, .-Reset_Handler

.section .text.Default_Handler,"ax",%progbits
.thumb_func
.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, .-Default_Handler
