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


/* 
  BASIC INTERRUPT DRIVEN DRIVER FOR USB. 

  This file contains all the usb components that must be compiled
  to ARM mode.  The components that can be compiled to either ARM or THUMB
  mode are contained in USB-CDC.c.

*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Demo application includes. */
#include "Board.h"
#include "usb.h"
#include "USB-CDC.h"

#define usbINT_CLEAR_MASK	(AT91C_UDP_TXCOMP | AT91C_UDP_STALLSENT | AT91C_UDP_RXSETUP | AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1 )
/*-----------------------------------------------------------*/

/* Messages and queue used to communicate between the ISR and the USB task. */
static xISRStatus xISRMessages[ usbQUEUE_LENGTH + 1 ];
extern xQueueHandle xUSBInterruptQueue;
/*-----------------------------------------------------------*/

/* The ISR can cause a context switch so is declared naked. */
void vUSB_ISR_Wrapper( void ) __attribute__ ((naked));

/* The function that actually performs the ISR work.  This must be separate
from the wrapper function to ensure the correct stack frame gets set up. */
void vUSB_ISR_Handler( void );
/*-----------------------------------------------------------*/

void vUSB_ISR_Handler( void )
{
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
static volatile unsigned long ulNextMessage = 0;
xISRStatus *pxMessage;
unsigned long ulRxBytes;
unsigned char ucFifoIndex;

    /* Use the next message from the array. */
	pxMessage = &( xISRMessages[ ( ulNextMessage & usbQUEUE_LENGTH ) ] );
	ulNextMessage++;

    /* Save UDP ISR state for task-level processing. */
	pxMessage->ulISR = AT91C_BASE_UDP->UDP_ISR;
	pxMessage->ulCSR0 = AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ];

    /* Clear interrupts from ICR. */
	AT91C_BASE_UDP->UDP_ICR = AT91C_BASE_UDP->UDP_IMR | AT91C_UDP_ENDBUSRES;
	
    
	/* Process incoming FIFO data.  Must set DIR (if needed) and clear RXSETUP 
	before exit. */

    /* Read CSR and get incoming byte count. */
	ulRxBytes = ( pxMessage->ulCSR0 >> 16 ) & usbRX_COUNT_MASK;
	
	/* Receive control transfers on endpoint 0. */
	if( pxMessage->ulCSR0 & ( AT91C_UDP_RXSETUP | AT91C_UDP_RX_DATA_BK0 ) )
	{
		/* Save FIFO data buffer for either a SETUP or DATA stage */
		for( ucFifoIndex = 0; ucFifoIndex < ulRxBytes; ucFifoIndex++ )
		{
			pxMessage->ucFifoData[ ucFifoIndex ] = AT91C_BASE_UDP->UDP_FDR[ usbEND_POINT_0 ];
		}

		/* Set direction for data stage.  Must be done before RXSETUP is 
		cleared. */
		if( ( AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] & AT91C_UDP_RXSETUP ) )
		{
			if( ulRxBytes && ( pxMessage->ucFifoData[ usbREQUEST_TYPE_INDEX ] & 0x80 ) )
			{
				AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] |= AT91C_UDP_DIR;

				/* Might not be wise in an ISR! */
				while( !(AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] & AT91C_UDP_DIR) );
			}

			/* Clear RXSETUP */
			AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] &= ~AT91C_UDP_RXSETUP;

			/* Might not be wise in an ISR! */
			while ( AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] & AT91C_UDP_RXSETUP );
		}
		else
		{
		   /* Clear RX_DATA_BK0 */
		   AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] &= ~AT91C_UDP_RX_DATA_BK0;

		   /* Might not be wise in an ISR! */
		   while ( AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] & AT91C_UDP_RX_DATA_BK0 );
		}
	}
	
	/* If we received data on endpoint 1, disable its interrupts until it is 
	processed in the main loop */
	if( AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ] & ( AT91C_UDP_RX_DATA_BK0 | AT91C_UDP_RX_DATA_BK1 ) )
	{
		AT91C_BASE_UDP->UDP_IDR = AT91C_UDP_EPINT1;
	}
	
	AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_0 ] &= ~( AT91C_UDP_TXCOMP | AT91C_UDP_STALLSENT );
     
	/* Clear interrupts for the other endpoints, retain data flags for endpoint 
	1. */
	AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_1 ] &= ~( AT91C_UDP_TXCOMP | AT91C_UDP_STALLSENT | AT91C_UDP_RXSETUP );
	AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_2 ] &= ~usbINT_CLEAR_MASK;
	AT91C_BASE_UDP->UDP_CSR[ usbEND_POINT_3 ] &= ~usbINT_CLEAR_MASK;

	/* Post ISR data to queue for task-level processing */
	xQueueSendFromISR( xUSBInterruptQueue, &pxMessage, &xHigherPriorityTaskWoken );

	/* Clear AIC to complete ISR processing */
	AT91C_BASE_AIC->AIC_EOICR = 0;

	/* Do a task switch if needed */
	if( xHigherPriorityTaskWoken )
	{
		/* This call will ensure that the unblocked task will be executed
		immediately upon completion of the ISR if it has a priority higher
		than the interrupted task. */
		portYIELD_FROM_ISR();
	}
}
/*-----------------------------------------------------------*/

void vUSB_ISR_Wrapper( void )
{
	/* Save the context of the interrupted task. */
	portSAVE_CONTEXT();

	/* Call the handler to do the work.  This must be a separate
	function to ensure the stack frame is set up correctly. */
	vUSB_ISR_Handler();

	/* Restore the context of whichever task will execute next. */
	portRESTORE_CONTEXT();
}


