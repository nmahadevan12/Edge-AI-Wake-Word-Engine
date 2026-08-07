.syntax unified
.cpu cortex-m4
.thumb

.global g_pfnVectors
.global Reset_Handler
.global Default_Handler

.extern main
.extern _estack
.extern SysTick_Handler

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
    bl main
1:  b 1b
.size Reset_Handler, .-Reset_Handler

.section .text.Default_Handler,"ax",%progbits
.thumb_func
.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, .-Default_Handler
