/*
    FreeRTOS V7.2.0 - Copyright (C) 2012 Real Time Engineers Ltd.


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!
    
    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?                                      *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    
    http://www.FreeRTOS.org - Documentation, training, latest information, 
    license and contact details.
    
    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool.

    Real Time Engineers ltd license FreeRTOS to High Integrity Systems, who sell 
    the code with commercial support, indemnification, and middleware, under 
    the OpenRTOS brand: http://www.OpenRTOS.com.  High Integrity Systems also
    provide a safety engineered and independently SIL3 certified version under 
    the SafeRTOS brand: http://www.SafeRTOS.com.
*/

	RSEG    CODE:CODE(2)
	thumb

	EXTERN ulRegTest1LoopCounter
	EXTERN ulRegTest2LoopCounter

	PUBLIC vRegTest1Task
	PUBLIC vRegTest2Task

/*-----------------------------------------------------------*/
vRegTest1Task

	/* Fill the core registers with known values.  This is only done once. */
	movs r1, #101
	movs r2, #102
	movs r3, #103
	movs r4, #104
	movs r5, #105
	movs r6, #106
	movs r7, #107
	movs r0, #108
	mov	 r8, r0
	movs r0, #109
	mov  r9, r0
	movs r0, #110
	mov	 r10, r0
	movs r0, #111
	mov	 r11, r0
	movs r0, #112
	mov  r12, r0
	movs r0, #100

reg1_loop
	/* Repeatedly check that each register still contains the value written to
	it when the task started. */
	cmp	r0, #100
	bne	reg1_error_loop
	cmp	r1, #101
	bne	reg1_error_loop
	cmp	r2, #102
	bne	reg1_error_loop
	cmp r3, #103
	bne	reg1_error_loop
	cmp	r4, #104
	bne	reg1_error_loop
	cmp	r5, #105
	bne	reg1_error_loop
	cmp	r6, #106
	bne	reg1_error_loop
	cmp	r7, #107
	bne	reg1_error_loop
	movs r0, #108
	cmp	r8, r0
	bne	reg1_error_loop
	movs r0, #109
	cmp	r9, r0
	bne	reg1_error_loop
	movs r0, #110
	cmp	r10, r0
	bne	reg1_error_loop
	movs r0, #111
	cmp	r11, r0
	bne	reg1_error_loop
	movs r0, #112
	cmp	r12, r0
	bne	reg1_error_loop

	/* Everything passed, increment the loop counter. */
	push { r1 }
	ldr	r0, =ulRegTest1LoopCounter
	ldr r1, [r0]
	adds r1, r1, #1
	str r1, [r0]
	pop { r1 }

	/* Start again. */
	movs r0, #100
	b reg1_loop

reg1_error_loop
	/* If this line is hit then there was an error in a core register value.
	The loop ensures the loop counter stops incrementing. */
	b reg1_error_loop
	nop



vRegTest2Task

	/* Fill the core registers with known values.  This is only done once. */
	movs r1, #1
	movs r2, #2
	movs r3, #3
	movs r4, #4
	movs r5, #5
	movs r6, #6
	movs r7, #7
	movs r0, #8
	mov	r8, r0
	movs r0, #9
	mov r9, r0
	movs r0, #10
	mov	r10, r0
	movs r0, #11
	mov	r11, r0
	movs r0, #12
	mov r12, r0
	movs r0, #10

reg2_loop
	/* Repeatedly check that each register still contains the value written to
	it when the task started. */
	cmp	r0, #10
	bne	reg2_error_loop
	cmp	r1, #1
	bne	reg2_error_loop
	cmp	r2, #2
	bne	reg2_error_loop
	cmp r3, #3
	bne	reg2_error_loop
	cmp	r4, #4
	bne	reg2_error_loop
	cmp	r5, #5
	bne	reg2_error_loop
	cmp	r6, #6
	bne	reg2_error_loop
	cmp	r7, #7
	bne	reg2_error_loop
	movs r0, #8
	cmp	r8, r0
	bne	reg2_error_loop
	movs r0, #9
	cmp	r9, r0
	bne	reg2_error_loop
	movs r0, #10
	cmp	r10, r0
	bne	reg2_error_loop
	movs r0, #11
	cmp	r11, r0
	bne	reg2_error_loop
	movs r0, #12
	cmp	r12, r0
	bne	reg2_error_loop

	/* Everything passed, increment the loop counter. */
	push { r1 }
	ldr	r0, =ulRegTest2LoopCounter
	ldr r1, [r0]
	adds r1, r1, #1
	str r1, [r0]
	pop { r1 }

	/* Start again. */
	movs r0, #10
	b reg2_loop

reg2_error_loop
	/* If this line is hit then there was an error in a core register value.
	The loop ensures the loop counter stops incrementing. */
	b reg2_error_loop
	nop

	END
