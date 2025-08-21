\
    .syntax unified
    .cpu cortex-m4
    .thumb

    .section .isr_vector, "a", %progbits
    .word  _estack
    .word  Reset_Handler
    .word  0 /* NMI */
    .word  0 /* HardFault */
    .word  0,0,0,0,0,0,0 /* reserved */
    .word  0 /* SVC */
    .word  0 /* DebugMon */
    .word  0 /* PendSV */
    .word  0 /* SysTick */

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
    Reset_Handler:
        bl  main
    loop:
        b   loop

    .size Reset_Handler, . - Reset_Handler
    .section .stack
    .word 0x20020000 /* _estack — ajuste selon la RAM */
