/*
 *
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/cputype.h>
#include <asm/asm-offsets.h>

/*
 * Realview/Versatile Express specific entry point for secondary CPUs.
 * This provides a "holding pen" into which all secondary cores are held
 * until we're ready for them to initialise.
 */

	.macro	switch_el, xreg, el3_label, el2_label, el1_label
	mrs	\xreg, CurrentEL
	cmp	\xreg, 0xc
	b.eq	\el3_label
	cmp	\xreg, 0x8
	b.eq	\el2_label
	cmp	\xreg, 0x4
	b.eq	\el1_label
	.endm

ENTRY(armv8_switch_to_el2)
	switch_el x0, 1f, 0f, 0f
0:	ret
1:
	mov	x0, #0x5b1	/* Non-secure EL0/EL1 | HVC | 64bit EL2 */
	msr	scr_el3, x0
	msr	cptr_el3, xzr	/* Disable coprocessor traps to EL3 */
	mov	x0, #0x33ff
	msr	cptr_el2, x0	/* Disable coprocessor traps to EL2 */

	/* Initialize SCTLR_EL2 */
	msr	sctlr_el2, xzr

	/* Return to the EL2_SP2 mode from EL3 */
	mov	x0, sp
	msr	sp_el2, x0	/* Migrate SP */
	mrs	x0, vbar_el3
	msr	vbar_el2, x0	/* Migrate VBAR */
	mov	x0, #0x3c9
	msr	spsr_el3, x0	/* EL2_SP2 | D | A | I | F */
	msr	elr_el3, x30
	eret
ENDPROC(armv8_switch_to_el2)

ENTRY(armv8_switch_to_el1)
	switch_el x0, 0f, 1f, 0f
0:	ret
1:
	/* Initialize Generic Timers */
	mrs	x0, cnthctl_el2
	orr	x0, x0, #0x3		/* Enable EL1 access to timers */
	msr	cnthctl_el2, x0
	msr	cntvoff_el2, xzr
	mrs	x0, cntkctl_el1
	orr	x0, x0, #0x3		/* Enable EL0 access to timers */
	msr	cntkctl_el1, x0

	/* Initialize MPID/MPIDR registers */
	mrs	x0, midr_el1
	mrs	x1, mpidr_el1
	msr	vpidr_el2, x0
	msr	vmpidr_el2, x1

	/* Disable coprocessor traps */
	mov	x0, #0x33ff
	msr	cptr_el2, x0		/* Disable coprocessor traps to EL2 */
	msr	hstr_el2, xzr		/* Disable coprocessor traps to EL2 */
	mov	x0, #3 << 20
	msr	cpacr_el1, x0		/* Enable FP/SIMD at EL1 */

	/* Initialize HCR_EL2 */
	mov	x0, #(1 << 31)		/* 64bit EL1 */
	orr	x0, x0, #(1 << 29)	/* Disable HVC */
	msr	hcr_el2, x0

	/* SCTLR_EL1 initialization */
	mov	x0, #0x0800
	movk	x0, #0x30d0, lsl #16
	msr	sctlr_el1, x0

	/* Return to the EL1_SP1 mode from EL2 */
	mov	x0, sp
	msr	sp_el1, x0		/* Migrate SP */
	mrs	x0, vbar_el2
	msr	vbar_el1, x0		/* Migrate VBAR */
	mov	x0, #0x3c5
	msr	spsr_el2, x0		/* EL1_SP1 | D | A | I | F */
	msr	elr_el2, x30
	eret
ENDPROC(armv8_switch_to_el1)

ENTRY(secondary_start_head)
	bl  armv8_switch_to_el2
	bl  armv8_switch_to_el1
	bl	el2_setup		/* Drop to EL1, w20=cpu_boot_mode */
	bl	set_cpu_boot_mode_flag

	mrs x0, mpidr_el1 	// Get our cpu id
	and x1, x0, #0x3 	// cpu id
	lsr x0, x0, #8
	and x0, x0, #0x0F 	// check processor affinity
	orr x0, x1, x0 		// mark cpu group 0 or 1

	adr	x1, 1f
	ldp	x2, x3, [x1]
	sub	x1, x1, x2
	add	x3, x3, x1
pen:	ldr	x4, [x3]
	cmp x4, x0
	b.eq 2f
	b pen
2:
	b	secondary_startup

ENDPROC(secondary_start_head)

    .align  3
1:  .quad   .
    .quad   secondary_pen_release
