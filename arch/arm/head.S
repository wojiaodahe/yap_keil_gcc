.global kernel_main
.global init_system
.global current
.global next_run
.global common_irq_handler
.global sys_call_schedule
.global enable_irq
.global disable_irq
.global OSIntNesting

.global __int_schedule
.global __soft_schedule
.global OS_Start
.global syscall
.global _start

.global  IRQ_MODE_STACK
.global  FIQ_MODE_STACK
.global  SVC_MODE_STACK
.global  SYS_MODE_STACK



.equ DISABLE_IRQ, 	0x80
.equ DISABLE_FIQ,  	0x40
.equ SYS_MOD,		0x1f
.equ IRQ_MOD,		0x12
.equ FIQ_MOD,		0x11
.equ SVC_MOD,	  	0x13
.equ ABT_MOD,	  	0x17
.equ UND_MOD ,	  	0x1b
.equ MOD_MASK,	 	0x1f

.text
.code 32

_start:
	b   Reset
HandleUndef:
    b   HandleUndef 		@0x04: ??????????????????????????????
Handle_swi:
    b   HandleSWI			@0x08: ????????????????????????????SWI????????????
HandlePrefetchAbort:
    b   HandlePrefetchAbort			@ 0x0c: ?????????????????????????????????
HandleDataAbort:
    b  	HandleDataAbort			@ 0x10: ?????????????????????????????????????
HandleNotUsed:
    b   HandleNotUsed		@ 0x14: ??????
Handle_irq:
    b   HandleIRQ			@ 0x18: ???????????????????
HandleFIQ:
    b   HandleFIQ			@ 0x1c: ??????????????????????

Reset:

    msr cpsr_cxsf, #0xd2    @ ????????????
	ldr		r0, =IRQ_MODE_STACK
	ldr		r0, [r0]
	mov		sp, r0

	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | SVC_MOD)
	ldr		r0, =SVC_MODE_STACK
	ldr		r0, [r0]
	mov		sp, r0

	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | FIQ_MOD)
	ldr		r0, =FIQ_MODE_STACK
	ldr		r0, [r0]
	mov		sp, r0
    
	msr cpsr_cxsf, #0xdf	@????
	ldr		r0, =SYS_MODE_STACK
	ldr		r0, [r0]
	mov		sp, r0

	mrc    p15, 0, r0, c1, c0, 0    
	bic    r0, r0, #0x0001          @??MMU
	mcr    p15, 0, r0, c1, c0, 0    @tq2440 ??ok???????jz2440?????
                                    @??: ??jlink???????????,?????????????????
                                    @??????(????????????)
                                    @????:jz440 ? tq2440??(??????????)?jtag??????(reset??) 
                                    @?????p15??????,mmu?????,??jz2440???????????
                                    @?????p15???,??mmu??????,???????bss????????
                                    @???????mmu???,????????????
                                    @!!!!!!?????????,??mmu?????????bss?,?????????????????!!!!!!!
	@bl copy_proc_beg

	bl init_system

_clear_bss:
	ldr r1, _bss_start_
	ldr r3, _bss_end_
	mov r2, #0x0
1:
	cmp r1, r3
	beq _main
	str r2, [r1], #0x4
	b	1b

_main:
    ldr lr, =halt_loop      @ ????????????
    ldr pc, =kernel_main           @ ??????main??????

/*
copy_proc_beg:
	adr	r0, _start
	ldr	r2, BaseOfROM
	cmp	r0, r2
	ldreq	r0, TopOfROM
	beq	InitRam	
	ldr r3, TopOfROM
loop1:
	ldmia	r0!, {r4-r7}
	stmia	r2!, {r4-r7}
	cmp	r2, r3
	bcc	loop1
	
	sub	r2, r2, r3
	sub	r0, r0, r2				
		
InitRam:
	ldr	r2, BaseOfBSS
	ldr	r3, BaseOfZero	
loop2:
	cmp	r2, r3
	ldrcc	r1, [r0], #4
	strcc	r1, [r2], #4
	bcc loop2

	mov	r0,	#0
	ldr	r3,	EndOfBSS
loop3:
	cmp	r2,	r3
	strcc	r0, [r2], #4
	bcc loop3
	
	mov pc, lr
*/
halt_loop:
    b   halt_loop

HandleIRQ:
	STMFD   SP!, {R1-R3}			@ We will use R1-R3 as temporary registers
@----------------------------------------------------------------------------
@   R1--SP
@	R2--PC 
@   R3--SPSR
@------------------------------------------------------------------------
	MOV     R1, SP
	ADD     SP, SP, #12             @Adjust IRQ stack pointer
	SUB     R2, LR, #4              @Adjust PC for return address to task

	MRS     R3, SPSR				@ Copy SPSR (Task CPSR)
	

	@CHANGE_TO_SYS       @??????? SYS ??????????????????????????????
    msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | SYS_MOD)

	STMFD   SP!, {R2}				@ Push task''s PC 
	STMFD	sp!, {R3}				@ Push Task''s cpsr
	STMFD   SP!, {R4-R12, LR}		@ Push task''s LR,R12-R4
	
	LDMFD   R1!, {R4-R6}			@ Load Task''s R1-R3 from IRQ stack 
	STMFD   SP!, {R4-R6}			@ Push Task''s R1-R3 to SYS stack
	STMFD   SP!, {R0}			    @ Push Task''s R0 to SYS stack
 	
	ldr r0, =current@
	ldr r0, [r0]
	str sp, [r0]
	
    bl common_irq_handler

	bl disable_irq	
	
	ldmfd	sp!, {r0 - r12}
	ldmfd	sp!, {r14}
	stmfd	sp!, {r0}
	mov 	r0,	 sp
	add		sp,  sp, #12
	
	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | FIQ_MOD)

	mov 	sp, r0
	ldmfd	sp!, {r0}
	ldmfd	sp!, {r14}
	
   	MSR		SPSR_cxsf, R14
	LDMFD 	SP!, {PC}^


__int_schedule:               @???????????????????r0 - r12 ???cpsr??????????
	LDR		R0, =current
	LDR		R0, [R0]
	LDR		SP, [R0]

	ldmfd	sp!, {r0 - r12}
	ldmfd	sp!, {r14}
	stmfd	sp!, {r0}
	mov 	r0,	 sp
	add		sp,  sp, #12
	
	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | FIQ_MOD)

	mov 	sp, r0
	ldmfd	sp!, {r0}
	ldmfd	sp!, {r14}
	
   	MSR		SPSR_cxsf, R14
	LDMFD 	SP!, {PC}^

__soft_schedule:
	STMFD	SP!, {LR}           @PC
	MRS		R14,  CPSR       	@Push CPSR
	STMFD	SP!, {R14}	
	STMFD	SP!, {R0-R12, LR}   @R0-R12 LR
		
	LDR		R0, =current
	LDR		R0, [R0]
	STR		SP, [R0]
	
	LDR		R0, =next_run
	LDR		R1, =current
	LDR		R0, [R0]
	STR		R0, [R1]

	LDR		R0, =next_run
	LDR		R0, [R0]
	LDR		SP, [R0]
	
  	ldmfd	sp!, {r0 - r12}
	ldmfd	sp!, {r14}
	stmfd	sp!, {r0}
	mov 	r0,	 sp
	add		sp,  sp, #12
	
	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | FIQ_MOD)

	mov 	sp, r0
	ldmfd	sp!, {r0}
	ldmfd	sp!, {r14}
	
   	MSR		SPSR_cxsf, R14
	LDMFD 	SP!, {PC}^

OS_Start:
	LDR		R0, =current
	LDR		R0, [R0]
	LDR		SP, [R0]

	ldmfd	sp!, {r0 - r12}
	ldmfd	sp!, {r14}
	stmfd	sp!, {r0}
	mov 	r0,	 sp
	add		sp,  sp, #12
	
	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | FIQ_MOD)

	mov 	sp, r0
	ldmfd	sp!, {r0}
	ldmfd	sp!, {r14}
	
   	MSR		SPSR_cxsf, R14
	LDMFD 	SP!, {PC}^

HandleSWI:
	STMFD   SP!, {R1-R3}			@ We will use R1-R3 as temporary registers
@----------------------------------------------------------------------------
@   R1--SP
@	R2--PC 
@   R3--SPSR
@------------------------------------------------------------------------
	MOV     R1, SP
	ADD     SP, SP, #12             @Adjust SWI stack pointer
	MOV     R2, LR		            @Adjust PC for return address to task

	MRS     R3, SPSR				@ Copy SPSR (Task CPSR)
	

	@CHANGE_TO_SYS       @??????? SYS ??????????????????????????????
    msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | SYS_MOD)

	STMFD   SP!, {R2}				@ Push task''s PC 
	STMFD	sp!, {R3}				@ Push Task''s cpsr
	STMFD   SP!, {R4-R12, LR}		@ Push task''s LR,R12-R4
	
	LDMFD   R1!, {R4-R6}			@ Load Task''s R1-R3 from IRQ stack 
	STMFD   SP!, {R4-R6}			@ Push Task''s R1-R3 to SYS stack
	STMFD   SP!, {R0}			    @ Push Task''s R0 to SYS stack
	
	
    bl enable_irq
	ldr		r1, [r13, #72]
	ldr 	r2, [r13, #68]
	ldr	 	r3, [r13, #60]
	ldr		r0, [r3, #-4]
	bic		r0, r0, #0xff000000
    bl sys_call_schedule

	str		r0, [r13, #64]
	bl disable_irq
    
	ldmfd	sp!, {r0 - r12}
	ldmfd	sp!, {r14}
	stmfd	sp!, {r0}
	mov 	r0,	 sp
	add		sp,  sp, #12
	
	msr     CPSR_cxsf, #(DISABLE_FIQ | DISABLE_IRQ | FIQ_MOD)

	mov 	sp, r0
	ldmfd	sp!, {r0}
	ldmfd	sp!, {r14}
	
   	MSR		SPSR_cxsf, R14
	LDMFD 	SP!, {PC}^

syscall:
	stmfd	r13!, {r0}
	stmfd	r13!, {r1}
	sub		r13, r13, #4
	swi	  	4
	ldmfd	r13!, {r0}
	add		r13, r13, #8
	mov pc, lr


current_task_sp:
	mov r0, sp
	mov pc, lr

data_abort:
	
	ldr sp, =0x31000000
	mrs		r14, spsr
	MSR		cPSR_cxsf, R14
	b .


_bss_start_:	.word   __bss_start__
_bss_end_:		.word   __bss_end__
