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

/*  Basic interrupt driven serial port driver for uart1.
*/

/* Standard includes. */
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* Demo application includes. */
#include <machine/uart.h>
#include <machine/ic.h>
#include "serial.h"
/*-----------------------------------------------------------*/

#define comBLOCK_RETRY_TIME				10
/*-----------------------------------------------------------*/

/* The interrupt handlers are naked functions that call C handlers.  The C
handlers are marked as noinline to ensure they work correctly when the
optimiser is on. */
void interrupt5_handler( void ) __attribute__((naked));
static void prvTxHandler( void ) __attribute__((noinline));
void interrupt6_handler( void ) __attribute__((naked));
static void prvRxHandler( void ) __attribute__((noinline));

/*-----------------------------------------------------------*/

/* Queues used to hold received characters, and characters waiting to be
transmitted. */
static xQueueHandle xRxedChars;
static xQueueHandle xCharsForTx;
extern unsigned portBASE_TYPE *pxVectorTable;
/*-----------------------------------------------------------*/

xComPortHandle xSerialPortInitMinimal( unsigned long ulWantedBaud, unsigned portBASE_TYPE uxQueueLength )
{
	/* Create the queues used to hold Rx and Tx characters. */
	xRxedChars = xQueueCreate( uxQueueLength, ( unsigned portBASE_TYPE ) sizeof( signed char ) );
	xCharsForTx = xQueueCreate( uxQueueLength + 1, ( unsigned portBASE_TYPE ) sizeof( signed char ) );

	if( ( xRxedChars ) && ( xCharsForTx ) )
	{
		/* Set up interrupts */
		/* tx interrupt will be enabled when we need to send something */
		uart1->tx_mask = 0;
		uart1->rx_mask = 1;
		irq[IRQ_UART1_TX].ien = 1;
		irq[IRQ_UART1_TX].ipl = portSYSTEM_INTERRUPT_PRIORITY_LEVEL;
		irq[IRQ_UART1_RX].ien = 1;
		irq[IRQ_UART1_RX].ipl = portSYSTEM_INTERRUPT_PRIORITY_LEVEL;
	}

	return ( xComPortHandle ) 0;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed char *pcRxedChar, portTickType xBlockTime )
{
	/* The port handle is not required as this driver only supports uart1. */
	(void) pxPort;

	/* Get the next character from the buffer.	Return false if no characters
	   are available, or arrive before xBlockTime expires. */
	if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
	{
		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}
/*-----------------------------------------------------------*/

void vSerialPutString( xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength )
{
	int i;
	signed char *pChNext;

	/* Send each character in the string, one at a time. */
	pChNext = ( signed char * )pcString;
	for( i = 0; i < usStringLength; i++ )
	{
		/* Block until character has been transmitted. */
		while( xSerialPutChar( pxPort, *pChNext, comBLOCK_RETRY_TIME ) != pdTRUE ); pChNext++;
	}
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed char cOutChar, portTickType xBlockTime )
{
	( void ) pxPort;

	/* Place the character in the queue of characters to be transmitted. */
	if( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) != pdPASS )
	{
		return pdFAIL;
	}

	/* Turn on the Tx interrupt so the ISR will remove the character from the
	   queue and send it. This does not need to be in a critical section as
	   if the interrupt has already removed the character the next interrupt
	   will simply turn off the Tx interrupt again. */
	uart1->tx_mask = 1;

	return pdPASS;
}
/*-----------------------------------------------------------*/

void vSerialClose( xComPortHandle xPort )
{
	/* Not supported as not required by the demo application. */
	( void ) xPort;
}
/*-----------------------------------------------------------*/

/* UART Tx interrupt handler. */
void interrupt5_handler( void )
{
	/* This is a naked function. */
	portSAVE_CONTEXT();
	prvTxHandler();
	portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

static void prvTxHandler( void )
{
signed char cChar;
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* The interrupt was caused by the transmit fifo having space for at least one
	character. Are there any more characters to transmit? */
	if( xQueueReceiveFromISR( xCharsForTx, &cChar, &xHigherPriorityTaskWoken ) == pdTRUE )
	{
		/* A character was retrieved from the queue so can be sent to the uart now. */
		uart1->tx_data = cChar;
	}
	else
	{
		/* Queue empty, nothing to send so turn off the Tx interrupt. */
		uart1->tx_mask = 0;
	}

	/* If an event caused a task to unblock then we call "Yield from ISR" to
	ensure that the unblocked task is the task that executes when the interrupt
	completes if the unblocked task has a priority higher than the interrupted
	task. */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/

/* UART Rx interrupt. */
void interrupt6_handler( void )
{
	portSAVE_CONTEXT();
	prvRxHandler();
	portRESTORE_CONTEXT();
}
/*-----------------------------------------------------------*/

static void prvRxHandler( void )
{
signed char cChar;
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* The interrupt was caused by the receiver getting data. */
	cChar = uart1->rx_data;

	xQueueSendFromISR(xRxedChars, &cChar, &xHigherPriorityTaskWoken );

	/* If an event caused a task to unblock then we call "Yield from ISR" to
	ensure that the unblocked task is the task that executes when the interrupt
	completes if the unblocked task has a priority higher than the interrupted
	task. */
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
/*-----------------------------------------------------------*/


