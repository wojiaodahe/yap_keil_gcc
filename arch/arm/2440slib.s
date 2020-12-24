
#define ALIGN        .align 4,0x90
#define CONFIG_AEABI

.global __aeabi_idiv
.global __aeabi_idivmod
.global __aeabi_uidiv
.global __aeabi_uidivmod

.macro ARM_DIV_BODY dividend, divisor, result, curbit

    @ Initially shift the divisor left 3 bits if possible,
    @ set curbit accordingly.  This allows for curbit to be located
    @ at the left end of each 4 bit nibbles in the division loop
    @ to save one loop in most cases.
    tst    \divisor, #0xe0000000
    moveq    \divisor, \divisor, lsl #3
    moveq    \curbit, #8
    movne    \curbit, #1

    @ Unless the divisor is very big, shift it up in multiples of
    @ four bits, since this is the amount of unwinding in the main
    @ division loop.  Continue shifting until the divisor is 
    @ larger than the dividend.
1:    cmp    \divisor, #0x10000000
    cmplo    \divisor, \dividend
    movlo    \divisor, \divisor, lsl #4
    movlo    \curbit, \curbit, lsl #4
    blo    1b

    @ For very big divisors, we must shift it a bit at a time, or
    @ we will be in danger of overflowing.
1:    cmp    \divisor, #0x80000000
    cmplo    \divisor, \dividend
    movlo    \divisor, \divisor, lsl #1
    movlo    \curbit, \curbit, lsl #1
    blo    1b

    mov    \result, #0


    @ Division loop
1:    cmp    \dividend, \divisor
    subhs    \dividend, \dividend, \divisor
    orrhs    \result,   \result,   \curbit
    cmp    \dividend, \divisor,  lsr #1
    subhs    \dividend, \dividend, \divisor, lsr #1
    orrhs    \result,   \result,   \curbit,  lsr #1
    cmp    \dividend, \divisor,  lsr #2
    subhs    \dividend, \dividend, \divisor, lsr #2
    orrhs    \result,   \result,   \curbit,  lsr #2
    cmp    \dividend, \divisor,  lsr #3
    subhs    \dividend, \dividend, \divisor, lsr #3
    orrhs    \result,   \result,   \curbit,  lsr #3
    cmp    \dividend, #0            @ Early termination?
    movnes    \curbit,   \curbit,  lsr #4    @ No, any more bits to do?
    movne    \divisor,  \divisor, lsr #4
    bne    1b

.endm


.macro ARM_DIV2_ORDER divisor, order

    cmp    \divisor, #(1 << 16)
    movhs    \divisor, \divisor, lsr #16
    movhs    \order, #16
    movlo    \order, #0

    cmp    \divisor, #(1 << 8)
    movhs    \divisor, \divisor, lsr #8
    addhs    \order, \order, #8

    cmp    \divisor, #(1 << 4)
    movhs    \divisor, \divisor, lsr #4
    addhs    \order, \order, #4

    cmp    \divisor, #(1 << 2)
    addhi    \order, \order, #3
    addls    \order, \order, \divisor, lsr #1

.endm


.macro ARM_MOD_BODY dividend, divisor, order, spare

    mov    \order, #0

    @ Unless the divisor is very big, shift it up in multiples of
    @ four bits, since this is the amount of unwinding in the main
    @ division loop.  Continue shifting until the divisor is 
    @ larger than the dividend.
1:    cmp    \divisor, #0x10000000
    cmplo    \divisor, \dividend
    movlo    \divisor, \divisor, lsl #4
    addlo    \order, \order, #4
    blo    1b

    @ For very big divisors, we must shift it a bit at a time, or
    @ we will be in danger of overflowing.
1:    cmp    \divisor, #0x80000000
    cmplo    \divisor, \dividend
    movlo    \divisor, \divisor, lsl #1
    addlo    \order, \order, #1
    blo    1b


    @ Perform all needed substractions to keep only the reminder.
    @ Do comparisons in batch of 4 first.
    subs    \order, \order, #3        @ yes, 3 is intended here
    blt    2f

1:    cmp    \dividend, \divisor
    subhs    \dividend, \dividend, \divisor
    cmp    \dividend, \divisor,  lsr #1
    subhs    \dividend, \dividend, \divisor, lsr #1
    cmp    \dividend, \divisor,  lsr #2
    subhs    \dividend, \dividend, \divisor, lsr #2
    cmp    \dividend, \divisor,  lsr #3
    subhs    \dividend, \dividend, \divisor, lsr #3
    cmp    \dividend, #1
    mov    \divisor, \divisor, lsr #4
    subges    \order, \order, #4
    bge    1b

    tst    \order, #3
    teqne    \dividend, #0
    beq    5f

    @ Either 1, 2 or 3 comparison/substractions are left.
2:    cmn    \order, #2
    blt    4f
    beq    3f
    cmp    \dividend, \divisor
    subhs    \dividend, \dividend, \divisor
    mov    \divisor,  \divisor,  lsr #1
3:    cmp    \dividend, \divisor
    subhs    \dividend, \dividend, \divisor
    mov    \divisor,  \divisor,  lsr #1
4:    cmp    \dividend, \divisor
    subhs    \dividend, \dividend, \divisor
5:
.endm


__udivsi3:
__aeabi_uidiv:

    subs    r2, r1, #1
    moveq    pc, lr
    bcc    Ldiv0
    cmp    r0, r1
    bls    11f
    tst    r1, r2
    beq    12f

    ARM_DIV_BODY r0, r1, r2, r3

    mov    r0, r2
    mov    pc, lr

11:    moveq    r0, #1
    movne    r0, #0
    mov    pc, lr

12:    ARM_DIV2_ORDER r1, r2

    mov    r0, r0, lsr r2
    mov    pc, lr


__umodsi3:

    subs    r2, r1, #1            @ compare divisor with 1
    bcc    Ldiv0
    cmpne    r0, r1                @ compare dividend with divisor
    moveq   r0, #0
    tsthi    r1, r2                @ see if divisor is power of 2
    andeq    r0, r0, r2
    movls    pc, lr

    ARM_MOD_BODY r0, r1, r2, r3

    mov    pc, lr


__divsi3:
__aeabi_idiv:

    cmp    r1, #0
    eor    ip, r0, r1            @ save the sign of the result.
    beq    Ldiv0
    rsbmi    r1, r1, #0            @ loops below use unsigned.
    subs    r2, r1, #1            @ division by 1 or -1 ?
    beq    10f
    movs    r3, r0
    rsbmi    r3, r0, #0            @ positive dividend value
    cmp    r3, r1
    bls    11f
    tst    r1, r2                @ divisor is power of 2 ?
    beq    12f

    ARM_DIV_BODY r3, r1, r0, r2

    cmp    ip, #0
    rsbmi    r0, r0, #0
    mov    pc, lr

10:    teq    ip, r0                @ same sign ?
    rsbmi    r0, r0, #0
    mov    pc, lr

11:    movlo    r0, #0
    moveq    r0, ip, asr #31
    orreq    r0, r0, #1
    mov    pc, lr

12:    ARM_DIV2_ORDER r1, r2

    cmp    ip, #0
    mov    r0, r3, lsr r2
    rsbmi    r0, r0, #0
    mov    pc, lr


__modsi3:

    cmp    r1, #0
    beq    Ldiv0
    rsbmi    r1, r1, #0            @ loops below use unsigned.
    movs    ip, r0                @ preserve sign of dividend
    rsbmi    r0, r0, #0            @ if negative make positive
    subs    r2, r1, #1            @ compare divisor with 1
    cmpne    r0, r1                @ compare dividend with divisor
    moveq    r0, #0
    tsthi    r1, r2                @ see if divisor is power of 2
    andeq    r0, r0, r2
    bls    10f

    ARM_MOD_BODY r0, r1, r2, r3

10:    cmp    ip, #0
    rsbmi    r0, r0, #0
    mov    pc, lr

#ifdef CONFIG_AEABI

__aeabi_uidivmod:

    stmfd    sp!, {r0, r1, ip, lr}
    bl    __aeabi_uidiv
    ldmfd    sp!, {r1, r2, ip, lr}
    mul    r3, r0, r2
    sub    r1, r1, r3
    mov    pc, lr

__aeabi_idivmod:

    stmfd    sp!, {r0, r1, ip, lr}
    bl    __aeabi_idiv
    ldmfd    sp!, {r1, r2, ip, lr}
    mul    r3, r0, r2
    sub    r1, r1, r3
    mov    pc, lr

#endif

Ldiv0:

    str    lr, [sp, #-8]!
    @ bl    __div0
    mov    r0, #0            @ About as wrong as it could be.
    ldr    pc, [sp], #8

